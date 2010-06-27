// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include <algorithm>

#include "game_constants.h"
#include "route_planner.h"
#include "node_map.h"

#include "pos_iterator.h"

#include "map.h"
#include "game.h"
#include "unit.h"
#include "unit_type.h"
#include "world.h"

#include "profiler.h"
#include "leak_dumper.h"

#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Search {
//namespace Game { namespace Search {

/** Construct Cartographer object. Requires game settings, factions & cell map to have been loaded.
  */
Cartographer::Cartographer(World *world)
		: world(world), cellMap(0), routePlanner(0) {
	g_logger.add("Cartographer", true);
	_PROFILE_FUNCTION();

	cellMap = world->getMap();
	int w = cellMap->getW(), h = cellMap->getH();

	routePlanner = world->getRoutePlanner();

	nodeMap = new NodeMap(w, h);
	GridNeighbours gNeighbours(w, h);
	nmSearchEngine = new SearchEngine<NodeMap, GridNeighbours>(gNeighbours, nodeMap, true);
	nmSearchEngine->setStorage(nodeMap);
	nmSearchEngine->setInvalidKey(Vec2i(-1));
	nmSearchEngine->getNeighbourFunc().setSearchSpace(SearchSpace::CELLMAP);

	masterMap = new AnnotatedMap(world);
	
	clusterMap = new ClusterMap(masterMap, this);

	// team search and visibility maps
	set<int> teams;
	for (int i=0; i < world->getFactionCount(); ++i) {
		const Faction *f = world->getFaction(i);
		int team = f->getTeam();
		if (teams.find(team) == teams.end()) {
			teams.insert(team);
			// AnnotatedMap needs to be 'bound' to explored status
			explorationMaps[team] = new ExplorationMap(cellMap);
			//teamMaps[team] = new AnnotatedMap(world, explorationMaps[team]);
		}
	}

	const TechTree *tt = world->getTechTree();

	vector<rt_ptr> harvestResourceTypes;
		
	for (int i = 0; i < tt->getResourceTypeCount(); ++i) {
		rt_ptr rt = tt->getResourceType(i);
		if (rt->getClass() == ResourceClass::TECHTREE
		|| rt->getClass() == ResourceClass::TILESET) {
			harvestResourceTypes.push_back(rt);
		}
	}
	for (int i = 0; i < tt->getFactionTypeCount(); ++i) {
		const FactionType *ft = tt->getFactionType(i);
		for (int j = 0; j < ft->getUnitTypeCount(); ++j) {
			const UnitType *ut = ft->getUnitType(j);
			for (int k=0; k < ut->getCommandTypeCount(); ++k) {
				const CommandType *ct = ut->getCommandType(k);
				if (ct->getClass() == CommandClass::HARVEST) {
					const HarvestCommandType *hct = static_cast<const HarvestCommandType *>(ct);
					foreach (vector<rt_ptr>, it, harvestResourceTypes) {
						if (hct->canHarvest(*it)) {
							ResourceMapKey key(*it, ut->getField(), ut->getSize());
							resourceMapKeys.insert(key);
						}
					}
				}
			}
		}
	}

	// find and catalog all resources...
	for (int x=0; x < cellMap->getTileW() - 1; ++x) {
		for (int y=0; y < cellMap->getTileH() - 1; ++y) {
			const Resource * const r = cellMap->getTile(x,y)->getResource();
			if (r) {
				resourceLocations[r->getType()].push_back(Vec2i(x,y));
			}
		}
	}

	Rectangle rect(0, 0, cellMap->getW() - 3, cellMap->getH() - 3);
	foreach (set<ResourceMapKey>, it, resourceMapKeys) {
		PatchMap<1> *pMap = new PatchMap<1>(rect, 0);
		initResourceMap(*it, pMap);
		resourceMaps[*it] = pMap;
	}
}

/** Destruct */
Cartographer::~Cartographer() {
	delete masterMap;
	delete clusterMap;
	delete nmSearchEngine;

	// Team Annotated Maps
	//deleteMapValues(teamMaps.begin(), teamMaps.end());
	//teamMaps.clear();

	// Exploration Maps
	deleteMapValues(explorationMaps.begin(), explorationMaps.end());
	explorationMaps.clear();
	
	// Goal Maps
	deleteMapValues(resourceMaps.begin(), resourceMaps.end());
	resourceMaps.clear();
	deleteMapValues(storeMaps.begin(), storeMaps.end());
	storeMaps.clear();
	deleteMapValues(siteMaps.begin(), siteMaps.end());
	siteMaps.clear();
}

void Cartographer::initResourceMap(ResourceMapKey key, PatchMap<1> *pMap) {
	const int &size = key.workerSize;
	const Field &field = key.workerField;
	const Map &map = *world->getMap();
	pMap->zeroMap();
	foreach (vector<Vec2i>, it, resourceLocations[key.resourceType]) {
		Resource *r = world->getMap()->getTile(*it)->getResource();
		assert(r);
		r->Depleted.connect(this, &Cartographer::onResourceDepleted);

		Vec2i tl = *it * GameConstants::cellScale + OrdinalOffsets[OrdinalDir::NORTH_WEST] * size;
		Vec2i br(tl.x + size + 2, tl.y + size + 2);

		Util::PerimeterIterator iter(tl, br);
		while (iter.more()) {
			Vec2i pos = iter.next();
			if (map.isInside(pos) && masterMap->canOccupy(pos, size, field)) {
				pMap->setInfluence(pos, 1);
			}
		}
	}
}


void Cartographer::onResourceDepleted(Vec2i pos) {
	const ResourceType *rt = cellMap->getTile(pos/GameConstants::cellScale)->getResource()->getType();
	resDirtyAreas[rt].push_back(pos);
//	Vec2i tl = pos + OrdinalOffsets[OrdinalDir::NORTH_WEST];
//	Vec2i br = pos + OrdinalOffsets[OrdinalDir::SOUTH_EAST] * 2;
//	resDirtyAreas[rt].push_back(pair<Vec2i,Vec2i>(tl,br));
}

void Cartographer::fixupResourceMaps(const ResourceType *rt, const Vec2i &pos) {
	const Map &map = *world->getMap();
	Vec2i junk;
	foreach (set<ResourceMapKey>, it, resourceMapKeys) {
		if (it->resourceType == rt) {
			PatchMap<1> *pMap = resourceMaps[*it];
			const int &size = it->workerSize;
			const Field &field = it->workerField;

			Vec2i tl = pos + OrdinalOffsets[OrdinalDir::NORTH_WEST] * size;
			Vec2i br(tl.x + size + 2, tl.y + size + 2);

			Util::RectIterator iter(tl, br);
			while (iter.more()) {
				Vec2i cur = iter.next();
				if (map.isInside(cur) && masterMap->canOccupy(cur, size, field)
				&& map.isResourceNear(cur, size, rt, junk)) {
					pMap->setInfluence(cur, 1);
				} else {
					pMap->setInfluence(cur, 0);
				}
			}
		}
	}
}

void Cartographer::onStoreDestroyed(Unit *unit) {
	///@todo fixme
//	delete storeMaps[unit];
//	storeMaps.erase(unit);
}

void Cartographer::saveResourceState(XmlNode *mapNode) {
	XmlNode *resourcesNode = mapNode->addChild("resources");
	foreach_const (ResourcePosMap, typeLocations, resourceLocations) {
		XmlNode *rNode = resourcesNode->addChild(typeLocations->first->getName());
		stringstream ss;
		foreach_const (vector<Vec2i>, it, typeLocations->second) {
			ss << *it << ":";
			Resource *r = cellMap->getTile(*it)->getResource();
			if (r) {
				ss << r->getAmount();
			} else {
				ss << 0;
			}
		}
		ss << "(-1,-1):-1";
		rNode->addAttribute("values", ss.str());
	}
}

void Cartographer::loadResourceState(XmlNode *node) {
	const TechTree *tt = world->getTechTree();
	XmlNode *resourcesNode = node->getChild("resources");
	Vec2i pos;
	char sep;
	int amount;

	for (int i=0; i < tt->getResourceTypeCount(); ++i) {
		rt_ptr rt = tt->getResourceType(i);
		if (rt->getClass() <= ResourceClass::TILESET) { // tech or tileset
			XmlNode *rNode = resourcesNode->getChild(rt->getName());
			stringstream ss(rNode->getAttribute("values")->getValue());
			ss >> pos >> sep >> amount;
			while (amount != -1) {
				Tile *tile = cellMap->getTile(pos);
				if (!tile->getResource()) {
					throw runtime_error("Error loading savegame, resource location data does not match map.");
				}
				if (amount) {
					tile->getResource()->setAmount(amount);
				} else {
					onResourceDepleted(Map::toUnitCoords(pos));
					tile->deleteResource();
					masterMap->updateMapMetrics(Map::toUnitCoords(pos), GameConstants::cellScale);
				}
				ss >> pos >> sep >> amount;
			}
		}
	}
}

PatchMap<1>* Cartographer::buildAdjacencyMap(const UnitType *uType, const Vec2i &pos, Field f, int size) {
	const Vec2i mapPos = pos + (OrdinalOffsets[OrdinalDir::NORTH_WEST] * size);
	const int sx = pos.x;
	const int sy = pos.y;

	Rectangle rect(mapPos.x, mapPos.y, uType->getSize() + 2 + size, uType->getSize() + 2 + size);
	PatchMap<1> *pMap = new PatchMap<1>(rect, 0);
	pMap->zeroMap();

	PatchMap<1> tmpMap(rect, 0);
	tmpMap.zeroMap();

	// mark cells occupied by unitType at pos (on tmpMap)
	Util::RectIterator rIter(pos, pos + Vec2i(uType->getSize() - 1));
	while (rIter.more()) {
		Vec2i gpos = rIter.next();
		if (!uType->hasCellMap() || uType->getCellMapCell(gpos.x - sx, gpos.y - sy)) {
			tmpMap.setInfluence(gpos, 1);
		}
	}

	// mark goal cells on result map
	rIter = Util::RectIterator(mapPos, pos + Vec2i(uType->getSize()));
	while (rIter.more()) {
		Vec2i gpos = rIter.next();
		if (tmpMap.getInfluence(gpos) || !masterMap->canOccupy(gpos, size, f)) {
			continue; // building or obstacle
		}
		Util::PerimeterIterator pIter(gpos - Vec2i(1), gpos + Vec2i(size));
		while (pIter.more()) {
			if (tmpMap.getInfluence(pIter.next())) {
				pMap->setInfluence(gpos, 1);
				break;
			}
		}
	}
	return pMap;
}

IF_DEBUG_EDITION(
	void Cartographer::debugAddBuildSiteMap(PatchMap<1> *siteMap) {
		Rectangle mapBounds = siteMap->getBounds();
		for (int ly = 0; ly < mapBounds.h; ++ly) {
			int y = mapBounds.y + ly;
			for (int lx = 0; lx < mapBounds.w; ++lx) {
				Vec2i pos(mapBounds.x + lx, y);
				if (siteMap->getInfluence(pos)) {
					g_debugRenderer.addBuildSiteCell(pos);
				}
			}
		}
	}
)

void Cartographer::tick() {
	if (clusterMap->isDirty()) {
		clusterMap->update();
	}
	foreach (ResourcePosMap, it, resDirtyAreas) {
		if (!it->second.empty()) {
			foreach (V2iList, posIt, it->second) {
				fixupResourceMaps(it->first, *posIt);
			}
			it->second.clear();
		}
	}
}

/** SearchEngine<>::aStar<>() goal function, used to build distance maps. */
class DistanceBuilderGoal {
private:
	TypeMap<float> *iMap; /**< inluence map to write distance data into.	  */
	float maxRange;

public:
	DistanceBuilderGoal(TypeMap<float> *iMap, float maxRange = numeric_limits<float>::infinity())
			: iMap(iMap), maxRange(maxRange) {}
	
	/** The goal function, writes costSoFar into the influence map.
	  * @param pos position to test @param costSoFar the cost of the shortest path to pos
	  * @return true if maxRange is exceeded. */
	bool operator()(const Vec2i &pos, const float costSoFar) const {
		if (costSoFar > maxRange) {
			return true;
		}
		iMap->setInfluence(pos, costSoFar);
		return false;
	}
};

void Cartographer::adjustGlestimalMap(Field f, TypeMap<float> &iMap, const Vec2i &pos, float range) {
	// Set pos open, write distance data to iMap from pos to 'range' in field f
	nmSearchEngine->setStart(pos, 0.f);
	MoveCost cost(f, 1, masterMap);
	DistanceBuilderGoal goal(&iMap, range);
	nmSearchEngine->aStar(goal, cost, ZeroHeuristic());
}

/** constructs an influence map using djkstra search from all tech resources, fills 'positions' with
  * a collection of Vec2i that are each at least 25 cells distant from a tech resource, and at 
  * least 15 cells distant from each other. */
void Cartographer::buildGlestimalMap(Field f, V2iList &positions) {
	Rectangle rect(0, 0, cellMap->getW() - 2, cellMap->getH() - 2);
	TypeMap<float> iMap(rect, 0.f);

	// 1. Setup search. Reset NodeMap and set positions of all Techtree resources open
	nmSearchEngine->reset();
	foreach (ResourcePosMap, it, resourceLocations) {
		if (it->first->getClass() == ResourceClass::TECHTREE) {
			foreach (V2iList, tPos, it->second) {
				Vec2i cPos = *tPos * 2;
				//FIXME: assumes Map::cellScale == 2, use a Util::RectIterator
				nmSearchEngine->setOpen(cPos, 0.f);
				nmSearchEngine->setOpen(cPos + OrdinalOffsets[OrdinalDir::EAST], 0.f);
				nmSearchEngine->setOpen(cPos + OrdinalOffsets[OrdinalDir::SOUTH], 0.f);
				nmSearchEngine->setOpen(cPos + OrdinalOffsets[OrdinalDir::SOUTH_EAST], 0.f);
			}
		}
	}
	// 2. Zap. Build distance map (distance at each cell in Field f to nearest tech resource)
	MoveCost cost(f, 1, masterMap);
	DistanceBuilderGoal goal(&iMap);
	nmSearchEngine->aStar(goal, cost, ZeroHeuristic());

	// 3. Find spawn points
	float big;
	do {
		// 3a. scan iMap, find largest value
		big = 0.f;
		Vec2i bigPos(-1);
		Util::RectIterator iter(cellMap->getBounds());
		while (iter.more()) {
			Vec2i pos = iter.next();
			float inf = iMap.getInfluence(pos);
			if (inf > big) {
				big = inf;
				bigPos = pos;
			}
		}
		// 3b. add cell with largest value to positioins, and adjust the influence map
		if (bigPos != Vec2i(-1)) {
			positions.push_back(bigPos);
			adjustGlestimalMap(f, iMap, bigPos, 15.f);
		}
	// while the best pos we found is at least 25 cells from a tech resource 
	// and at least 15 cells from another spawn point (travelling in Field f)
	} while (big > 25.f);
}

/** Initialise annotated maps for each team, call after initial units visibility is applied */
void Cartographer::initTeamMaps() {
	map<int, ExplorationMap*>::iterator it = explorationMaps.begin();
	for ( ; it != explorationMaps.end(); ++it ) {
		AnnotatedMap *aMap = getAnnotatedMap(it->first);
		ExplorationMap *eMap = it->second;

	}
}

/** Custom Goal function for maintaining the exploration maps */
class VisibilityMaintainerGoal {
private:
	float range;			/**< range of entity sight */
	ExplorationMap *eMap;	/**< exploration map to adjust */
	bool inc;				/**< true to increment, false to decrement */
public:
	/** Construct goal function object
	  * @param range the range of visibility
	  * @param eMap the ExplorationMap to adjust
	  * @param inc true to apply visibility, false to remove
	  */
	VisibilityMaintainerGoal(float range, ExplorationMap *eMap, bool inc)
		: range(range), eMap(eMap), inc(inc) {}

	/** The goal function 
	  * @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true when range is exceeded.
	  */
	bool operator()(const Vec2i &pos, const float costSoFar) const { 
		if ( costSoFar > range ) {
			return true;
		}
		if ( inc ) {
			eMap->incVisCounter(pos);
		} else {
			eMap->decVisCounter(pos);
		}
		return false; 
	}
};

/** Maintains visibility on a per team basis. Adds or removes a unit's visibility
  * to (or from) its team exploration map, using a Dijkstra search.
  * @param unit the unit to remove or add visibility for
  * @param add true to add this units visibility to its team map, false to remove
  */
void Cartographer::maintainUnitVisibility(Unit *unit, bool add) {
	
	/* This might be too expensive using Dijkstra...

	// set up goal function
	VisibilityMaintainerGoal goalFunc((float)unit->getSight(), explorationMaps[unit->getTeam()], add);
	// set up search engine
	GridNeighbours::setSearchSpace(SearchSpace::TILEMAP);
	nmSearchEngine->setNodeLimit(-1);
	nmSearchEngine->reset();
	nmSearchEngine->setOpen(Map::toTileCoords(unit->getCenteredPos()), 0.f);
	// zap
	nmSearchEngine->aStar<VisibilityMaintainerGoal,DistanceCost,ZeroHeuristic>
						 (goalFunc,DistanceCost(),ZeroHeuristic());
	// reset search space
	GridNeighbours::setSearchSpace(SearchSpace::CELLMAP);
	*/
}


}}
