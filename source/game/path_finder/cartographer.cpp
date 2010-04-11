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

// Does not use pre-compiled header because it's optimized in debug build.

#include <algorithm>

#include "game_constants.h"
#include "cartographer.h"
#include "search_engine.h"
#include "cluster_map.h"

#include "pos_iterator.h"

#include "map.h"
#include "game.h"
#include "unit.h"
#include "unit_type.h"
#include "world.h"

#include "profiler.h"
#include "leak_dumper.h"

#if _GAE_DEBUG_EDITION_
#	include "renderer.h"
#endif

using namespace std;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Game { namespace Search {
//namespace Game { namespace Search {

/** Construct Cartographer object. Requires game settings, factions & cell map to have been loaded.
  */
Cartographer::Cartographer(World *world)
		: world(world), cellMap(0), routePlanner(0) {
	theLogger.add("Cartographer", true);
	_PROFILE_FUNCTION();

	cellMap = world->getMap();
	int w = cellMap->getW(), h = cellMap->getH();

	routePlanner = world->getRoutePlanner();

	nodeMap = new NodeMap(w, h);
	nmSearchEngine = new SearchEngine<NodeMap, GridNeighbours>(nodeMap, true);
	nmSearchEngine->setStorage(nodeMap);
	nmSearchEngine->setInvalidKey(Vec2i(-1));
	GridNeighbours::setSearchSpace(SearchSpace::CELLMAP);

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

	// Todo: more preprocessing. Discover all harvester units, 
	// check field, harvest reange and resource types...
	//
	// find and catalog all resources...
	for (int x=0; x < cellMap->getTileW() - 1; ++x) {
		for (int y=0; y < cellMap->getTileH() - 1; ++y) {
			const Resource * const r = cellMap->getTile(x,y)->getResource();
			if (r) {
				resourceLocations[r->getType()].push_back(Vec2i(x,y));
			}
		}
	}

	const TechTree *tt = world->getTechTree();
	for (int i=0; i < tt->getResourceTypeCount(); ++i) {
		const ResourceType *rt = tt->getResourceType(i);
		if (rt->getClass() == ResourceClass::TECHTREE || rt->getClass() == ResourceClass::TILESET) {
			Rectangle rect(0, 0, cellMap->getW() - 2, cellMap->getH() - 2);
			PatchMap<1> *pMap = new PatchMap<1>(rect, 0);
			initResourceMap(rt, pMap);
			resourceMaps[rt] = pMap;
		}
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

void Cartographer::initResourceMap(const ResourceType *rt, PatchMap<1> *pMap) {
/*
	static char buf[512];
	char *ptr = buf;
	ptr += sprintf(ptr, "Initialising resource map : %s.\n", rt->getName().c_str());
	int64 time_millis = Chrono::getCurMillis();
	int64 time_micros = Chrono::getCurMicros();
*/
	pMap->zeroMap();
	vector<Vec2i>::iterator it = resourceLocations[rt].begin();
	for ( ; it != resourceLocations[rt].end(); ++it) {
		Resource *r = world->getMap()->getTile(*it)->getResource();
		assert(r);

		r->Depleted.connect(this, &Cartographer::onResourceDepleted);

		Vec2i pos = *it * GameConstants::cellScale;
		Vec2i tl = pos + OrdinalOffsets[OrdinalDir::NORTH_WEST];
		Vec2i tr = pos + OrdinalOffsets[OrdinalDir::EAST] + OrdinalOffsets[OrdinalDir::NORTH_EAST];
		Vec2i bl = pos + OrdinalOffsets[OrdinalDir::SOUTH] + OrdinalOffsets[OrdinalDir::SOUTH_WEST];
		Vec2i br = pos + OrdinalOffsets[OrdinalDir::SOUTH_EAST] * 2;

		int n = tr.x - tl.x;
		for (int i=0; i <= n; ++i) {
			Vec2i top(tl.x + i, tl.y);
			Vec2i bot(tl.x + i, bl.y);
			if (world->getMap()->isInside(top) && masterMap->canOccupy(top, 1, Field::LAND)) {
				pMap->setInfluence(top, 1);
			}
			if (world->getMap()->isInside(bot) && masterMap->canOccupy(bot, 1, Field::LAND)) {
				pMap->setInfluence(bot, 1);
			}
		}
		for (int i=1; i <= n - 1; ++i) {
			Vec2i left(tl.x, tl.y + i);
			Vec2i right(tr.x, tl.y + i);
			if (world->getMap()->isInside(left) && masterMap->canOccupy(left, 1, Field::LAND)) {
				pMap->setInfluence(left, 1);
			}
			if (world->getMap()->isInside(right) && masterMap->canOccupy(right, 1, Field::LAND)) {
				pMap->setInfluence(right, 1);
			}
		}
	}
/*
	time_millis = Chrono::getCurMillis() - time_millis;
	time_micros = Chrono::getCurMicros() - time_micros;
	if (time_millis >= 3) {
		ptr += sprintf(ptr, "Took %dms\n", (int)time_millis);
	} else {
		ptr += sprintf(ptr, "Took %dms (%dus)\n", (int)time_millis, (int)time_micros);
	}
	theLogger.add(buf);
*/
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

void Cartographer::onResourceDepleted(Vec2i pos) {
	const ResourceType *rt = cellMap->getTile(pos / GameConstants::cellScale)->getResource()->getType();
	Vec2i tl = pos + OrdinalOffsets[OrdinalDir::NORTH_WEST];
	Vec2i br = pos + OrdinalOffsets[OrdinalDir::SOUTH_EAST] * 2;
	resDirtyAreas[rt].push_back(pair<Vec2i,Vec2i>(tl,br));
}

void Cartographer::fixupResourceMap(const ResourceType *rt, const Vec2i &tl, const Vec2i &br) {
	PatchMap<1> *pMap = resourceMaps[rt];
	assert(pMap);
	Vec2i junk;
	for (int y = tl.y; y <= br.y; ++y) {
		for (int x = tl.x; x <= br.x; ++x) {
			Vec2i pos(x,y);
			if (!world->getMap()->isInside(pos)) continue;
			if (masterMap->canOccupy(pos, 1, Field::LAND)
			&& cellMap->isResourceNear(pos, rt, junk)) {
				pMap->setInfluence(pos, 1);
			} else {
				pMap->setInfluence(pos, 0);
			}
		}
	}
}

void Cartographer::onStoreDestroyed(Unit *unit) {
	delete storeMaps[unit];
	storeMaps.erase(unit);
}

PatchMap<1>* Cartographer::buildAdjacencyMap(const UnitType *uType, const Vec2i &pos) {
	const Vec2i mapPos = pos + OrdinalOffsets[OrdinalDir::NORTH_WEST];
	const int sx = pos.x;
	const int sy = pos.y;

	Rectangle rect(mapPos.x, mapPos.y, uType->getSize() + 3, uType->getSize() + 3);
	PatchMap<1> *pMap = new PatchMap<1>(rect, 0);
	pMap->zeroMap();

	PatchMap<1> tmpMap(rect, 0);
	tmpMap.zeroMap();

	// mark cells occupied by unitType at pos (on tmpMap)
	Util::RectIterator iter(pos, pos + Vec2i(uType->getSize() - 1));
	while (iter.more()) {
		Vec2i gpos = iter.next();
		if (!uType->hasCellMap() || uType->getCellMapCell(gpos.x - sx, gpos.y - sy)) {
			tmpMap.setInfluence(gpos, 1);
		}
	}

	// mark goal cells on result map
	iter = Util::RectIterator(mapPos, mapPos + Vec2i(uType->getSize() + 1));
	while (iter.more()) {
		Vec2i gpos = iter.next();
		if (tmpMap.getInfluence(gpos) || !masterMap->canOccupy(gpos, 1, Field::LAND)) {
			continue; // building occupied cell or obstacle
		}
		foreach_enum (OrdinalDir, d) {
			if (tmpMap.getInfluence(gpos + OrdinalOffsets[d])) {
				// if next to a cell marked from first sweep, then this is a valid goal pos
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
					theDebugRenderer.addBuildSiteCell(pos);
				}
			}
		}
	}
)

void Cartographer::tick() {
	if (clusterMap->isDirty()) {
		clusterMap->update();
	}
	map<const ResourceType*, AreaList>::iterator it = resDirtyAreas.begin();
	for ( ; it != resDirtyAreas.end(); ++it) {
		if (!it->second.empty()) {
			AreaList::iterator alIt = it->second.begin();
			for ( ; alIt != it->second.end(); ++alIt) {
				fixupResourceMap(it->first, alIt->first, alIt->second);
			}
			it->second.clear();
		}
	}
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

}}}
