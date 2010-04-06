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

namespace Glest { namespace Game { namespace Search {
//namespace Game { namespace Search {

class ClusterMap;
class RoutePlanner;

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
	/** @param pos cell coordinates @return number of units that can see this tile */
	int  getVisCounter(const Vec2i &pos) const	{ return state[pos.y * cellMap->getTileH() + pos.x].visCounter; }
	/** @param pos tile coordinates to increase visibilty on */
	void incVisCounter(const Vec2i &pos) const	{ state[pos.y * cellMap->getTileH() + pos.x].visCounter ++;		}
	/** @param pos tile coordinates to decrease visibilty on */
	void decVisCounter(const Vec2i &pos) const	{ state[pos.y * cellMap->getTileH() + pos.x].visCounter --;		}
	/** @param pos tile coordinates @return true if explored. */
	bool isExplored(const Vec2i &pos)	 const	{ return state[pos.y * cellMap->getTileH() + pos.x].explored;	}
	/** @param pos coordinates of tile to set as explored */
	void setExplored(const Vec2i &pos)	 const	{ state[pos.y * cellMap->getTileH() + pos.x].explored = 1;		}

};

//
// Cartographer: 'Map' Manager
//
class Cartographer : public sigslot::has_slots<> {
	/** Master annotated map, always correct */
	AnnotatedMap *masterMap;

	/*/* Team annotateded maps, 'foggy' */
	//map< int, AnnotatedMap* > teamMaps;

	/** The ClusterMap (Hierarchical map abstraction) */
	ClusterMap *clusterMap;

	typedef const ResourceType* rt_ptr;
	typedef pair<Vec2i, Vec2i> PosPair;
	typedef vector<PosPair> AreaList;

	typedef map<rt_ptr, PatchMap<1>*> ResourceMaps;		// goal maps for harvester path searches to resourecs
	typedef map<const Unit*, PatchMap<1>*> StoreMaps;	// goal maps for harvester path searches to store

	typedef pair<const UnitType*, Vec2i> SiteDesc;	// describes a building site.
	typedef map<SiteDesc, PatchMap<1>*> SiteMaps;	// goal maps for building sites.

	// Resources
	/** The locations of each and every resource on the map */
	map<rt_ptr, vector<Vec2i> > resourceLocations;

	/** areas where resources have been depleted and updates are required */
	map<rt_ptr, AreaList> resDirtyAreas;

	/** Goal Maps for each tech & tileset resource */
	ResourceMaps resourceMaps;

	StoreMaps storeMaps;
	SiteMaps siteMaps;

	// Exploration
	/** Exploration maps for each team */
	map< int, ExplorationMap* > explorationMaps;

	// A* stuff
	NodeMap *nodeMap;
	SearchEngine<NodeMap,GridNeighbours> *nmSearchEngine;

	World *world;
	Map *cellMap;
	RoutePlanner *routePlanner;

	void initResourceMap(const ResourceType *rt, PatchMap<1> *pMap);
	void fixupResourceMap(const ResourceType *rt, const Vec2i &tl, const Vec2i &br);

	PatchMap<1>* buildAdjacencyMap(const UnitType *uType, const Vec2i &pos);

	PatchMap<1>* buildStoreMap(Unit *unit) {
		unit->Died.connect(this, &Cartographer::onStoreDestroyed);
		return (storeMaps[unit] = buildAdjacencyMap(unit->getType(), unit->getPos()));
	}

	IF_DEBUG_EDITION( void debugAddBuildSiteMap(PatchMap<1>*); )

	PatchMap<1>* buildSiteMap(const UnitType *uType, const Vec2i &pos) {
		PatchMap<1> *smap = siteMaps[make_pair(uType, pos)] = buildAdjacencyMap(uType, pos);
		IF_DEBUG_EDITION( debugAddBuildSiteMap(smap); )
		return smap;
	}

	// slots
	void onResourceDepleted(Vec2i pos);
	void onStoreDestroyed(Unit *unit);

	void onUnitBorn(Unit *unit);
	void onUnitMoved(Unit *unit);
	void onUnitMorphed(Unit *unit, const UnitType *type);
	void onUnitDied(Unit *unit);

	void maintainUnitVisibility(Unit *unit, bool add);

public:
	Cartographer(World *world);
	virtual ~Cartographer();

	SearchEngine<NodeMap,GridNeighbours>* getSearchEngine() { return nmSearchEngine; }
	RoutePlanner* getRoutePlanner() { return routePlanner; }

	/** @return the number of units of team that can see a tile 
	  * @param team team index 
	  * @param pos the co-ordinates of the <b>tile</b> of interest. */
	int getTeamVisibility(int team, const Vec2i &pos) { return explorationMaps[team]->getVisCounter(pos); }
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

	PatchMap<1>* getResourceMap(const ResourceType* rt) {
		return resourceMaps[rt];
	}

	PatchMap<1>* getStoreMap(const Unit *unit) {
		StoreMaps::iterator it = storeMaps.find(unit);
		if (it != storeMaps.end()) {
			return it->second;
		}
		return buildStoreMap(const_cast<Unit*>(unit)); // connects signal, needs non-const...
	}

	PatchMap<1>* getSiteMap(const UnitType *ut, const Vec2i &pos) {
		SiteMaps::iterator it = siteMaps.find(make_pair(ut, pos));
		if (it != siteMaps.end()) {
			return it->second;
		}
		return buildSiteMap(ut, pos);
	}

	ClusterMap* getClusterMap() const { return clusterMap; }

	AnnotatedMap* getMasterMap()				const	{ return masterMap;							  }
	AnnotatedMap* getAnnotatedMap(int team )			{ return masterMap;/*teamMaps[team];*/					  }
	AnnotatedMap* getAnnotatedMap(const Faction *faction) 	{ return getAnnotatedMap(faction->getTeam()); }
	AnnotatedMap* getAnnotatedMap(const Unit *unit)			{ return getAnnotatedMap(unit->getTeam());	  }
};

//class Surveyor {
//};

}}}

#endif
