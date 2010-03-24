// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2009      James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_PATHFINDER_H_
#define _GLEST_GAME_PATHFINDER_H_

#include "game_constants.h"
#include "influence_map.h"
#include "annotated_map.h"
#include "cluster_map.h"
#include "config.h"
#include "profiler.h"

#include "search_engine.h"
#include "cartographer.h"

#include <set>
#include <map>

using std::vector;
using std::list;
using std::map;

using Shared::Math::Vec2i;
using Shared::Platform::uint32;

namespace Glest { namespace Game { namespace Search {

/** maximum radius to search for a free position from a target position */
const int maxFreeSearchRadius = 10;
/** @deprecated not in use */
const int pathFindNodesMax = 2048;

typedef SearchEngine<TransitionNodeStore,TransitionNeighbours,const Transition*> TransitionSearchEngine;

class PMap1Goal {
protected:
	PatchMap<1> *pMap;

public:
	PMap1Goal(PatchMap<1> *pMap) : pMap(pMap) {}

	bool operator()(const Vec2i &pos, const float) const {
		if (pMap->getInfluence(pos)) {
			return true;
		}
		return false;
	}
};

// =====================================================
// 	class RoutePlanner
// =====================================================
/**	Finds paths for units using SearchEngine<>::aStar<>() */ 
class RoutePlanner {
public:
	RoutePlanner(World *world);
	~RoutePlanner();

	TravelState findPathToLocation(Unit *unit, const Vec2i &finalPos);

	/** @see findPathToLocation() */
	TravelState findPath(Unit *unit, const Vec2i &finalPos) { 
		return findPathToLocation(unit, finalPos); 
	}

	TravelState findPathToResource(Unit *unit, const Vec2i &targetPos, const ResourceType *rt) {
		assert(rt->getClass() == ResourceClass::TECHTREE || rt->getClass() == ResourceClass::TILESET);
		PMap1Goal goal(world->getCartographer()->getResourceMap(rt));
		return findPathToGoal(unit, goal, targetPos);
	}

	TravelState findPathToStore(Unit *unit, const Unit *store) {
		Vec2i target = store->getNearestOccupiedCell(unit->getPos());
		PMap1Goal goal(world->getCartographer()->getStoreMap(store));
		return findPathToGoal(unit, goal, target);
	}

	bool isLegalMove(Unit *unit, const Vec2i &pos) const;

	SearchEngine<NodeStore>* getSearchEngine() { return nsgSearchEngine; }

private:
	bool repairPath(Unit *unit);

	TravelState findAerialPath(Unit *unit, const Vec2i &targetPos);

	TravelState doRouteCache(Unit *unit);
	TravelState doQuickPathSearch(Unit *unit, const Vec2i &target);

	TravelState findPathToGoal(Unit *unit, PMap1Goal &goal, const Vec2i &targetPos);
	TravelState customGoalSearch(PMap1Goal &goal, Unit *unit, const Vec2i &target);

	float quickSearch(Field field, int Size, const Vec2i &start, const Vec2i &dest);
	bool refinePath(Unit *unit);
	void smoothPath(Unit *unit);

	HAAStarResult setupHierarchicalSearch(Unit *unit, const Vec2i &dest, TransitionGoal &goalFunc);
	HAAStarResult findWaypointPath(Unit *unit, const Vec2i &dest, WaypointPath &waypoints);

	World *world;
	SearchEngine<NodeStore>	 *nsgSearchEngine;
	NodeStore *nodeStore;
	TransitionSearchEngine *tSearchEngine;
	TransitionNodeStore *tNodeStore;

	Vec2i computeNearestFreePos(const Unit *unit, const Vec2i &targetPos);

	bool attemptMove(Unit *unit) const {
		Vec2i pos = unit->getPath()->peek(); 
		if (isLegalMove(unit, pos)) {
			unit->setNextPos(pos);
			unit->getPath()->pop();
			return true;
		}
		return false;
	}
#if _GAE_DEBUG_EDITION_
	TravelState doFullLowLevelAStar(Unit *unit, const Vec2i &dest);
#endif
#if DEBUG_SEARCH_TEXTURES
public:
	enum { SHOW_PATH_ONLY, SHOW_OPEN_CLOSED_SETS, SHOW_LOCAL_ANNOTATIONS } debug_texture_action;
#endif
}; // class RoutePlanner


//TODO: put these somewhere sensible
class TransitionHeuristic {
	DiagonalDistance dd;
public:
	TransitionHeuristic(const Vec2i &target) : dd(target) {}
	bool operator()(const Transition *t) const {
		return dd(t->nwPos);
	}
};

#if 0 == 1
//
// just use DiagonalDistance to waypoint ??
class AbstractAssistedHeuristic {
public:
	AbstractAssistedHeuristic(const Vec2i &target, const Vec2i &waypoint, float wpCost) 
			: target(target), waypoint(waypoint), wpCost(wpCost) {}
	/** search target */
	Vec2i target, waypoint;	
	float wpCost;
	/** The heuristic function.
	  * @param pos the position to calculate the heuristic for
	  * @return an estimate of the cost to target
	  */
	float operator()(const Vec2i &pos) const {
		float dx = (float)abs(pos.x - waypoint.x), 
			  dy = (float)abs(pos.y - waypoint.y);
		float diag = dx < dy ? dx : dy;
		float straight = dx + dy - 2 * diag;
		return  1.4 * diag + straight + wpCost;

	}
};
#endif

}}}//end namespace

#endif
