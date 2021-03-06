// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//				  2009-2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_SINGLE_UNIT_PATH_H_
#define _GLEST_GAME_SINGLE_UNIT_PATH_H_

#include "game_constants.h"
#include "influence_map.h"
#include "annotated_map.h"
#include "cluster_map.h"
#include "config.h"
#include "profiler.h"

#include "search_engine.h"
#include "cartographer.h"

#include "world.h"

#include <set>
#include <map>

using std::vector;
using std::list;
using std::map;
using Shared::Math::Vec2i;
using Shared::Platform::uint32;

namespace Glest { namespace Pathing {

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
__inline void getDiags(const Vec2i &s, const Vec2i &d, const int size, Vec2i &d1, Vec2i &d2) {
#	define _SEARCH_ENGINE_GET_DIAGS_DEFINED_
	assert(abs(s.x - d.x) == 1 && abs(s.y - d.y) == 1);
	if (size == 1) {
		d1.x = s.x; d1.y = d.y;
		d2.x = d.x; d2.y = s.y;
		return;
	}
	if (d.x > s.x) {
		if (d.y > s.y) {
			d1.x = d.x + size - 1; d1.y = s.y;
			d2.x = s.x; d2.y = d.y + size - 1;
		} else {
			d1.x = s.x; d1.y = d.y;
			d2.x = d.x + size - 1; d2.y = s.y + size - 1;
		}
	} else {
		if (d.y > s.y) {
			d1.x = d.x; d1.y = s.y;
			d2.x = s.x + size - 1; d2.y = d.y + size - 1;
		} else {
			d1.x = d.x; d1.y = s.y + size - 1;
			d2.x = s.x + size - 1; d2.y = d.y;
		}
	}
}
class MoveCost {
private:
	const int size;
	const Field field;
	const AnnotatedMap *aMap;
public:
	MoveCost(const Unit *unit, const AnnotatedMap *aMap)
			: size(unit->getSize()), field(unit->getCurrField()), aMap(aMap) {}
	MoveCost(const Field field, const int size, const AnnotatedMap *aMap )
			: size(size), field(field), aMap(aMap) {}
	float operator()(const Vec2i &p1, const Vec2i &p2) const {
		assert(p1.dist(p2) < 1.5 && p1 != p2);
		assert(g_map.isInside(p2));
		if (!aMap->canOccupy(p2, size, field)) {
			return numeric_limits<float>::infinity();
		}
		if (p1.x != p2.x && p1.y != p2.y) {
			Vec2i d1, d2;
			getDiags(p1, p2, size, d1, d2);
			assert(g_map.isInside(d1) && g_map.isInside(d2));
			if (!aMap->canOccupy(d1, 1, field) || !aMap->canOccupy(d2, 1, field) ) {
				return numeric_limits<float>::infinity();
			}
			return SQRT2;
		}
		return 1.0f;]
	}
};
// =====================================================
// 	class RoutePlanner
// =====================================================
class SingleUnitPathMaker {
public:
	RoutePlanner(World *world);
	~RoutePlanner();
	TravelState findPathToLocation(Unit *unit, const Vec2i &finalPos);
	TravelState findPath(Unit *unit, const Vec2i &finalPos) {
		return findPathToLocation(unit, finalPos);
	}
	TravelState findPathToResource(Unit *unit, const Vec2i &targetPos, const ResourceType *rt) {
		assert(rt->getClass() == ResourceClass::TECHTREE || rt->getClass() == ResourceClass::TILESET);
		ResourceMapKey mapKey(rt, unit->getCurrField(), unit->getSize());
		PMap1Goal goal(world->getCartographer()->getResourceMap(mapKey));
		return findPathToGoal(unit, goal, targetPos);
	}
	TravelState findPathToStore(Unit *unit, const Unit *store) {
		Vec2i target = store->getNearestOccupiedCell(unit->getPos());
		PMap1Goal goal(world->getCartographer()->getStoreMap(store, unit));
		return findPathToGoal(unit, goal, target);
	}
	TravelState findPathToBuildSite(Unit *unit, const UnitType *buildingType, const Vec2i &buildingPos, CardinalDir facing) {
		PMap1Goal goal(world->getCartographer()->getSiteMap(buildingType, buildingPos, facing, unit));
		return findPathToGoal(unit, goal, unit->getTargetPos());
	}
	bool isLegalMove(Unit *unit, const Vec2i &pos) const;
	SearchEngine<NodePool>* getSearchEngine() { return nsgSearchEngine; }
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
	HAAStarResult setupHierarchicalOpenList(Unit *unit, const Vec2i &target);
	HAAStarResult setupHierarchicalSearch(Unit *unit, const Vec2i &dest, TransitionGoal &goalFunc);
	HAAStarResult findWaypointPath(Unit *unit, const Vec2i &dest, WaypointPath &waypoints);
	HAAStarResult findWaypointPathUnExplored(Unit *unit, const Vec2i &dest, WaypointPath &waypoints);
	World *world;
	SearchEngine<NodePool>	 *nsgSearchEngine;
	NodePool *nodeStore;
	TransitionSearchEngine *tSearchEngine;
	TransitionNodeStore *tNodeStore;
	Vec2i computeNearestFreePos(const Unit *unit, const Vec2i &targetPos);
	bool attemptMove(Unit *unit) const {
		assert(!unit->getPath()->empty());
		Vec2i pos = unit->getPath()->peek();
		if (isLegalMove(unit, pos)) {
			unit->setNextPos(pos);
			unit->getPath()->pop();
			return true;
		}
		return false;
	}
	IF_DEBUG_EDITION(
		TravelState doFullLowLevelAStar(Unit *unit, const Vec2i &dest);
	)
};
class TransitionHeuristic {
private:
	DiagonalDistance dd;
public:
	TransitionHeuristic(const Vec2i &target) : dd(target) {}
	bool operator()(const Transition *t) const {
		return dd(t->nwPos);
	}
};

}}
#endif
