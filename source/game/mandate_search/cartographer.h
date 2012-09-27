// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_CARTOGRAPHER_H_
#define _GLEST_GAME_CARTOGRAPHER_H_

#include "game_constants.h"

#include "influence_map.h"
#include "annotated_map.h"

#include "world.h"
#include "config.h"

#include "search_engine.h"
#include "sigslot.h"

using std::make_pair;

namespace Glest { namespace Search {

/** A map containing a visility counter and explored flag for every map tile. 
  * WIP, not in use, exploration state is currently maintained in the tile map */
class ExplorationMap {
#	pragma pack(push, 2)
		struct ExplorationState {	/**< The exploration state of one tile for one team */			
			uint16 visCounter : 15;	/**< Visibility counter, the number of team's units that can see this tile */ 
			uint16 explored	  :  1;	/**< Explored flag */
		}; // max 32768 units per _team_
#	pragma pack(pop)
	ExplorationState *state; /**< The Data */
	Map *cellMap;
public:
	ExplorationMap(Map *cMap) : cellMap(cMap) { /**< Construct ExplorationMap, sets everything to zero */
		state = new ExplorationState[cellMap->getTileW() * cellMap->getTileH()];
		memset(state, 0, sizeof(ExplorationState) * cellMap->getTileW() * cellMap->getTileH());
	}
	~ExplorationMap(){ delete[] state; }
	/** @param pos cell coordinates @return number of units that can see this tile */
	int  getVisCounter(const Vec2i &pos) const	{ return state[pos.y * cellMap->getTileH() + pos.x].visCounter; }
	/** @param pos tile coordinates to increase visibilty on */
	void incVisCounter(const Vec2i &pos) const	{ ++state[pos.y * cellMap->getTileH() + pos.x].visCounter;		}
	/** @param pos tile coordinates to decrease visibilty on */
	void decVisCounter(const Vec2i &pos) const	{ --state[pos.y * cellMap->getTileH() + pos.x].visCounter;		}
	/** @param pos tile coordinates @return true if explored. */
	bool isExplored(const Vec2i &pos)	 const	{ return state[pos.y * cellMap->getTileH() + pos.x].explored;	}
	/** @param pos coordinates of tile to set as explored */
	void setExplored(const Vec2i &pos)	 const	{ state[pos.y * cellMap->getTileH() + pos.x].explored = 1;		}

};


struct ResourceMapKey {
	const ResourceType *resourceType;
	Field workerField;
	int workerSize;

	ResourceMapKey(const ResourceType *type, Field f, int s)
			: resourceType(type), workerField(f), workerSize(s) {}

	bool operator<(const ResourceMapKey &that) const {
		return (memcmp(this, &that, sizeof(ResourceMapKey)) < 0);
	}
};

struct StoreMapKey {
	const Unit *storeUnit;
	Field workerField;
	int workerSize;

	StoreMapKey(const Unit *store, Field f, int s)
			: storeUnit(store), workerField(f), workerSize(s) {}

	bool operator<(const StoreMapKey &that) const {
		return (memcmp(this, &that, sizeof(StoreMapKey)) < 0);
	}
};

struct BuildSiteMapKey {
	const UnitType *buildingType;
	Vec2i buildingPosition;
	CardinalDir buildingFacing;
	Field workerField;
	int workerSize;

	BuildSiteMapKey(const UnitType *type, const Vec2i &pos, CardinalDir facing, Field f, int s)
			: buildingType(type), buildingPosition(pos), buildingFacing(facing)
			, workerField(f), workerSize(s) {}

	bool operator<(const BuildSiteMapKey &that) const {
		return (memcmp(this, &that, sizeof(BuildSiteMapKey)) < 0);
	}
};

//
// Cartographer: 'Map' Manager
//
class Cartographer : public sigslot::has_slots {
private:
	typedef const ResourceType* rt_ptr;
	//typedef pair<Vec2i, Vec2i> PosPair;
	//typedef vector<PosPair> AreaList;
	typedef vector<Vec2i> V2iList;

	typedef map<ResourceMapKey, PatchMap<1>*> ResourceMaps;	// goal maps for harvester path searches to resourecs
	typedef map<StoreMapKey,	PatchMap<1>*> StoreMaps;	// goal maps for harvester path searches to store
	typedef map<BuildSiteMapKey,PatchMap<1>*> SiteMaps;		// goal maps for building sites.

	typedef list<pair<rt_ptr, Vec2i> >	ResourcePosList;    // list of resource type / position pairs
	typedef map<rt_ptr, V2iList>        ResourcePosMap;     // resource positions by type

	typedef TypeMap<int>                DetectorMap;
	typedef map<int, DetectorMap*>      DetectorMaps;
//	typedef vector<DetectorMap*>        DetectorMaps;
	typedef map<int, DetectorMaps>      TeamDetectorMaps;

	typedef map<int, AnnotatedMap*>     TeamAnnotatedMaps;
	typedef map<int, ExplorationMap*>   TeamExplorationMaps;

private:
	// Map abstractions for A* and HAA* search
	AnnotatedMap      *masterMap;     /**< Master annotated map, always correct */
//	TeamAnnotatedMaps  teamMaps;      /**< Team annotateded maps, 'foggy' */
	ClusterMap        *clusterMap;    /**< The ClusterMap (Hierarchical map abstraction) */

	// Resources
	ResourcePosMap resourceLocations; /**< The locations of each and every resource on the map */
	set<ResourceMapKey> resourceMapKeys;
	ResourcePosMap resDirtyAreas; /**< areas where resources have been depleted and updates are required */

	// Special search goal maps
	ResourceMaps   resourceMaps; /**< Goal Maps for each tech & tileset resource */
	StoreMaps      storeMaps;    /**< Goal maps for 'store' units */
	SiteMaps       siteMaps;     /**< Goal maps for building sites */

	// Exploration
	TeamExplorationMaps  m_explorationMaps; /**< Exploration maps for each team */
	TeamDetectorMaps     m_detectorMaps;    /**< Detector maps */

	// A* stuff
	NodeMap *nodeMap;
	SearchEngine<NodeMap,GridNeighbours> *nmSearchEngine;

	World *world;
	Map *cellMap;
	RoutePlanner *routePlanner;

private:
	void initResourceMap(ResourceMapKey key, PatchMap<1> *pMap);
	void fixupResourceMaps(const ResourceType *rt, const Vec2i &pos);

	PatchMap<1>* buildAdjacencyMap(const UnitType *uType, const Vec2i &pos, CardinalDir facing, Field f, int sz);
	PatchMap<1>* buildAdjacencyMap(const UnitType *uType, const Vec2i &pos) {
		return buildAdjacencyMap(uType, pos, CardinalDir::NORTH, Field::LAND, 1);
	}

	PatchMap<1>* buildStoreMap(StoreMapKey key) {
		const_cast<Unit*>(key.storeUnit)->Died.connect(this, &Cartographer::onStoreDestroyed);
		return (storeMaps[key] = buildAdjacencyMap(key.storeUnit->getType(), 
			key.storeUnit->getPos(), key.storeUnit->getModelFacing(), key.workerField, key.workerSize));
	}

	IF_DEBUG_EDITION( void debugAddBuildSiteMap(PatchMap<1>*); )

	PatchMap<1>* buildSiteMap(BuildSiteMapKey key) {
		PatchMap<1> *sMap = siteMaps[key] = buildAdjacencyMap(key.buildingType, key.buildingPosition,
			key.buildingFacing, key.workerField, key.workerSize);
		IF_DEBUG_EDITION( debugAddBuildSiteMap(sMap); )
		return sMap;
	}

	// slots
	void onResourceDepleted(Vec2i pos);
	void onStoreDestroyed(Unit *unit);

	//void onUnitBorn(Unit *unit);
	//void onUnitMoved(Unit *unit);
	//void onUnitMorphed(Unit *unit, const UnitType *type);
	//void onUnitDied(Unit *unit);

	void maintainUnitVisibility(Unit *unit, bool add);

	void saveResourceState(XmlNode *node);
	void loadResourceState(XmlNode *node);

public:
	Cartographer(World *world);
	virtual ~Cartographer();

	SearchEngine<NodeMap,GridNeighbours>* getSearchEngine() { return nmSearchEngine; }
	RoutePlanner* getRoutePlanner() { return routePlanner; }

	/** @return the number of units of team that can see a tile 
	  * @param team team index 
	  * @param pos the co-ordinates of the <b>tile</b> of interest. */
	int getTeamVisibility(int team, const Vec2i &pos) { return m_explorationMaps[team]->getVisCounter(pos); }
	/** Adds a unit's visibility to its team's exploration map */
	void applyUnitVisibility(Unit *unit)	{ maintainUnitVisibility(unit, true); }
	/** Removes a unit's visibility from its team's exploration map */
	void removeUnitVisibility(Unit *unit)	{ maintainUnitVisibility(unit, false); }

	void initTeamMaps();

	/** Update the annotated maps when an obstacle has been added or removed from the map.
	  * Unconditionally updates the master map, updates team maps if the team can see the cells,
	  * or mark as 'dirty' if they cannot currently see the change. @todo implement team maps
	  * @param pos position (north-west most cell) of obstacle
	  * @param size size of obstacle	*/
	void updateMapMetrics(const Vec2i &pos, const int size) { 
		masterMap->updateMapMetrics(pos, size);
		// who can see it ? update their maps too.
		// set cells as dirty for those that can't see it
	}

	void tick();

	void loadMapState(const XmlNode *node) {
		cellMap->loadExplorationState(node->getChild("explorationState"));
		loadResourceState(node->getChild("resourceState"));
	}

	void saveMapState(XmlNode *node) {
		cellMap->saveExplorationState(node->addChild("explorationState"));
		saveResourceState(node->addChild("resourceState"));
	}

	PatchMap<1>* getResourceMap(ResourceMapKey key) {
		return resourceMaps[key];
	}

	PatchMap<1>* getStoreMap(StoreMapKey key, bool build=true) {
		StoreMaps::iterator it = storeMaps.find(key);
		if (it != storeMaps.end()) {
			return it->second;
		}
		if (build) {
			return buildStoreMap(key);
		} else {
			return 0;
		}
	}

	PatchMap<1>* getStoreMap(const Unit *store, const Unit *worker) {
		StoreMapKey key(store, worker->getCurrField(), worker->getSize());
		return getStoreMap(key);
	}

	PatchMap<1>* getSiteMap(BuildSiteMapKey key) {
		SiteMaps::iterator it = siteMaps.find(key);
		if (it != siteMaps.end()) {
			return it->second;
		}
		return buildSiteMap(key);

	}

	PatchMap<1>* getSiteMap(const UnitType *ut, const Vec2i &pos, CardinalDir face, Unit *worker) {
		BuildSiteMapKey key(ut, pos, face, worker->getCurrField(), worker->getSize());
		return getSiteMap(key);
	}

	void adjustGlestimalMap(Field f, TypeMap<float> &iMap, const Vec2i &pos, float range);
	void buildGlestimalMap(Field f, V2iList &positions);

	Map*	getCellMap() { return cellMap; }

	void detectorActivated(Unit *unit);
	void detectorMoved(Unit *unit, Vec2i oldPos);
	void detectorDeactivated(Unit *unit);
	void detectorSightModified(Unit *unit, int oldSight);

	ClusterMap* getClusterMap() const { return clusterMap; }

	AnnotatedMap* getMasterMap()				const	{ return masterMap;	 }
	AnnotatedMap* getAnnotatedMap(int team )			{ return masterMap;/*teamMaps[team];*/ }
	AnnotatedMap* getAnnotatedMap(const Faction *faction) 	{ return getAnnotatedMap(faction->getTeam()); }
	AnnotatedMap* getAnnotatedMap(const Unit *unit)			{ return getAnnotatedMap(unit->getTeam()); }

	bool canDetect(int team, int cloakGroup, Vec2i tilePos) {
		return m_detectorMaps[team][cloakGroup]->getInfluence(tilePos);
	}
};

/** enumeration for build location type */
STRINGY_ENUM( LocationType, 
	DEAD_WEIGHT,		/**< For buildings with no further purpose (housing/energy source/etc) */
	DEFENSIVE_SITE,		/**< A site for defensive towers */
	RESOURCE_STORE,		/**< Location for a store */
	WARRIOR_PRODUCER,	/**< Location for a warrior producing building */
	RESEARCH_BUILDING	/**< Location for a research building */
)

/** AI Helper */
class Surveyor {
private:
	typedef map<const ResourceType *, Vec2i> ResourceMap;

private:
	Faction			*m_faction;
	Cartographer	*m_cartographer;

	PatchMap<2>		*m_baseMap;
	vector<Vec2i>	 m_enemyLocs;
	ResourceMap		 m_resourceMap;

	int m_w, m_h;
	Vec2i m_basePos;

private:
	void findResourceLocations();
	void buildBaseMap();

public:
	Surveyor(Faction *faction, Cartographer *cartographer);

	Vec2i findLocationForBuilding(const UnitType *buildingType, LocationType locType);
	Vec2i findLocationForExpansion();
	Vec2i findResourceLocation(const ResourceType *rt);
};

}}

#endif
