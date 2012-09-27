// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2009-2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include <algorithm>
#include <list>
#include "game_constants.h"
#include "route_planner.h"
#include "cartographer.h"
#include "search_engine.h"
#include "node_pool.h"
#include "node_map.h"
#include "config.h"
#include "map.h"
#include "game.h"
#include "unit.h"
#include "unit_type.h"
#include "sim_interface.h"
#include "debug_stats.h"
#include "leak_dumper.h"
#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif
#define _PROFILE_PATHFINDER() _PROFILE_FUNCTION()
using namespace Shared::Graphics;
using namespace Shared::Util;
using std::cout;
using std::endl;

namespace Glest { namespace Pathing {
#if _GAE_DEBUG_EDITION_
	template <typename NodeStorage>
	void collectOpenClosed(NodeStorage *ns) {
		list<Vec2i> *nodes = ns->getOpenNodes();
		list<Vec2i>::iterator nit = nodes->begin();
		for ( ; nit != nodes->end(); ++nit )
			g_debugRenderer.getPFCallback().openSet.insert(*nit);
		delete nodes;
		nodes = ns->getClosedNodes();
		for ( nit = nodes->begin(); nit != nodes->end(); ++nit )
			g_debugRenderer.getPFCallback().closedSet.insert(*nit);
		delete nodes;
	}
	void collectPath(const Unit *unit) {
		const UnitPath &path = *unit->getPath();
		for (UnitPath::const_iterator pit = path.begin(); pit != path.end(); ++pit)
			g_debugRenderer.getPFCallback().pathSet.insert(*pit);
	}
	void collectWaypointPath(const Unit *unit) {
		const WaypointPath &path = *unit->getWaypointPath();
		g_debugRenderer.clearWaypoints();
		WaypointPath::const_iterator it = path.begin();
		for ( ; it != path.end(); ++it) {
			Vec3f vert = g_world.getMap()->getVertexData()->get(Map::toTileCoords(*it)).vert();
			vert.x += it->x % GameConstants::cellScale + 0.5f;
			vert.z += it->y % GameConstants::cellScale + 0.5f;
			vert.y += 0.15f;
			g_debugRenderer.addWaypoint(vert);
		}
	}
	void clearOpenClosed(const Vec2i &start, const Vec2i &target) {
		g_debugRenderer.getPFCallback().pathStart = start;
		g_debugRenderer.getPFCallback().pathDest = target;
		g_debugRenderer.getPFCallback().pathSet.clear();
		g_debugRenderer.getPFCallback().openSet.clear();
		g_debugRenderer.getPFCallback().closedSet.clear();
	}
#endif
// =====================================================
// 	class RoutePlanner
// =====================================================

RoutePlanner::RoutePlanner(World *world)
		: world(world)
		, nsgSearchEngine(NULL)
		, nodeStore(NULL)
		, tSearchEngine(NULL)
		, tNodeStore(NULL) {
	const int &w = world->getMap()->getW();
	const int &h = world->getMap()->getH();
	nodeStore = new NodePool(w, h);
	GridNeighbours gNeighbours(w, h);
	nsgSearchEngine = new SearchEngine<NodePool>(gNeighbours, nodeStore, true);
	nsgSearchEngine->setInvalidKey(Vec2i(-1));
	nsgSearchEngine->getNeighbourFunc().setSearchSpace(SearchSpace::CELLMAP);
	int numNodes = w * h / 4096 * 250;	tNodeStore = new TransitionNodeStore(numNodes);
	TransitionNeighbours tNeighbours;
	tSearchEngine = new TransitionSearchEngine(tNeighbours, tNodeStore, true);
	tSearchEngine->setInvalidKey(NULL);
}

RoutePlanner::~RoutePlanner() {
	delete nsgSearchEngine;
	delete tSearchEngine;
}

bool RoutePlanner::isLegalMove(Unit *unit, const Vec2i &pos2) const {
	assert(world->getMap()->isInside(pos2));
	assert(unit->getPos().dist(pos2) < 1.5);

	if (unit->getPos().dist(pos2) > 1.5) {
		unit->clearPath();
		return false;
	}
	const Vec2i &pos1 = unit->getPos();
	const int &size = unit->getSize();
	const Field &field = unit->getCurrField();
	Zone zone;
    if (field == Field::AIR) {
    zone = Zone::AIR;
    } else if (field == Field::LAND) {
    zone = Zone::LAND;
    } else if (field == Field::WALL || field == Field::STAIR) {
    zone = Zone::WALL;
    }
	AnnotatedMap *annotatedMap = world->getCartographer()->getMasterMap();
	if (!annotatedMap->canOccupy(pos2, size, field)) {
		return false;
	}
	if ( pos1.x != pos2.x && pos1.y != pos2.y ) {
		Vec2i diag1, diag2;
		getDiags(pos1, pos2, size, diag1, diag2);
		if (!annotatedMap->canOccupy(diag1, 1, field) || !annotatedMap->canOccupy(diag2, 1, field)
		|| !world->getMap()->getCell(diag1)->isFree(zone)
		|| !world->getMap()->getCell(diag2)->isFree(zone)) {
			return false;
		}
	}
	for (int i = pos2.x; i < unit->getSize() + pos2.x; ++i) {
		for (int j = pos2.y; j < unit->getSize() + pos2.y; ++j) {
			if (world->getMap()->getCell(i,j)->getUnit(zone) != unit
			&& !world->getMap()->isFreeCell(Vec2i(i, j), field)) {
				return false;
			}
		}
	}
	return true;
}

float RoutePlanner::quickSearch(Field field, int size, const Vec2i &start, const Vec2i &dest) {
	MoveCost moveCost(field, size, world->getCartographer()->getMasterMap());
	DiagonalDistance heuristic(dest);
	nsgSearchEngine->setStart(start, heuristic(start));
	PosGoal goal(dest);
	AStarResult r = nsgSearchEngine->aStar(goal, moveCost, heuristic);
	if (r == AStarResult::COMPLETE && nsgSearchEngine->getGoalPos() == dest) {
		return nsgSearchEngine->getCostTo(dest);
	}
	return numeric_limits<float>::infinity();
}

HAAStarResult RoutePlanner::setupHierarchicalOpenList(Unit *unit, const Vec2i &target) {
	Transitions transitions;
	Vec2i startCluster = ClusterMap::cellToCluster(unit->getPos());
	ClusterMap *clusterMap = world->getCartographer()->getClusterMap();
	clusterMap->getTransitions(startCluster, unit->getCurrField(), transitions);
	DiagonalDistance dd(target);
	nsgSearchEngine->getNeighbourFunc().setSearchCluster(startCluster);
	bool startTrap = true;
	AnnotatedMap *aMap = world->getCartographer()->getMasterMap();
	aMap->annotateLocal(unit);
	for (Transitions::iterator it = transitions.begin(); it != transitions.end(); ++it) {
		float cost = quickSearch(unit->getCurrField(), unit->getSize(), unit->getPos(), (*it)->nwPos);
		if (cost != numeric_limits<float>::infinity()) {
			tSearchEngine->setOpen(*it, dd((*it)->nwPos), cost);
			startTrap = false;
		}
	}
	aMap->clearLocalAnnotations(unit);
	if (startTrap) {
		bool locked = true;
		for (Transitions::iterator it = transitions.begin(); it != transitions.end(); ++it) {
			float cost = quickSearch(unit->getCurrField(), unit->getSize(), unit->getPos(), (*it)->nwPos);
			if (cost != numeric_limits<float>::infinity()) {
				tSearchEngine->setOpen(*it, dd((*it)->nwPos), cost);
				locked = false;
			}
		}
		if (locked) {
			return HAAStarResult::FAILURE;
		}
	}
	if (startTrap) {
		return HAAStarResult::START_TRAP;
	}
	return HAAStarResult::COMPLETE;
}

HAAStarResult RoutePlanner::setupHierarchicalSearch(Unit *unit, const Vec2i &dest, TransitionGoal &goalFunc) {
	HAAStarResult res = setupHierarchicalOpenList(unit, dest);
	if (res == HAAStarResult::FAILURE) {
		return HAAStarResult::FAILURE;
	}
	bool startTrap = res == HAAStarResult::START_TRAP;
	Transitions transitions;
	Vec2i cluster = ClusterMap::cellToCluster(dest);
	g_cartographer.getClusterMap()->getTransitions(cluster, unit->getCurrField(), transitions);
	nsgSearchEngine->getNeighbourFunc().setSearchCluster(cluster);
	bool goalTrap = true;
	for (Transitions::iterator it = transitions.begin(); it != transitions.end(); ++it) {
		float cost = quickSearch(unit->getCurrField(), unit->getSize(), dest, (*it)->nwPos);
		if (cost != numeric_limits<float>::infinity()) {
			goalFunc.goalTransitions().insert(*it);
			goalTrap = false;
		}
	}
	return startTrap ? HAAStarResult::START_TRAP
		  : goalTrap ? HAAStarResult::GOAL_TRAP
					 : HAAStarResult::COMPLETE;
}

HAAStarResult RoutePlanner::findWaypointPath(Unit *unit, const Vec2i &dest, WaypointPath &waypoints) {
	SECTION_TIMER(PATHFINDER_HIERARCHICAL);
	TIME_FUNCTION();
	_PROFILE_PATHFINDER();
	TransitionGoal goal;
	HAAStarResult setupResult = setupHierarchicalSearch(unit, dest, goal);
	nsgSearchEngine->getNeighbourFunc().setSearchSpace(SearchSpace::CELLMAP);
	if (setupResult == HAAStarResult::FAILURE) {
		return HAAStarResult::FAILURE;
	}
	TransitionCost cost(unit->getCurrField(), unit->getSize());
	TransitionHeuristic heuristic(dest);
	AStarResult res = tSearchEngine->aStar(goal,cost,heuristic);
	if (res == AStarResult::COMPLETE) {
		WaypointPath &wpPath = *unit->getWaypointPath();
		wpPath.clear();
		waypoints.push(dest);
		const Transition *t = tSearchEngine->getGoalPos();
		while (t) {
			waypoints.push(t->nwPos);
			t = tSearchEngine->getPreviousPos(t);
		}
		return setupResult;
	}
	return HAAStarResult::FAILURE;
}

class UnexploredGoal {
private:
	set<const Transition*> potentialGoals;
	int team;
public:
	UnexploredGoal(int teamIndex) : team(teamIndex) {}
	bool operator()(const Transition *t, const float d) const {
		Edges::const_iterator it =  t->edges.begin();
		for ( ; it != t->edges.end(); ++it) {
			if (!g_map.getTile(Map::toTileCoords((*it)->transition()->nwPos))->isExplored(team)) {
				const_cast<UnexploredGoal*>(this)->potentialGoals.insert(t);
				return false;
			}
		}
		return false;
	}
	const Transition* getBestSeen(const Vec2i &currPos, const Vec2i &target) {
		const Transition *best = 0;
		float distToBest = numeric_limits<float>::infinity();
		foreach (set<const Transition*>, it, potentialGoals) {
			float myDist = (*it)->nwPos.dist(target) + (*it)->nwPos.dist(currPos);
			if (myDist < distToBest) {
				best = *it;
				distToBest = myDist;
			}
		}
		return best;
	}
};

class UnexploredCost {
	Field field;
	int size;
	int team;
public:
	UnexploredCost(Field f, int s, int team) : field(f), size(s), team(team) {}
	float operator()(const Transition *t, const Transition *t2) const {
		Edges::const_iterator it =  t->edges.begin();
		for ( ; it != t->edges.end(); ++it) {
			if ((*it)->transition() == t2) {
				break;
			}
		}
		if (it == t->edges.end()) {
			throw runtime_error("bad connection in ClusterMap.");
		}
		if ((*it)->maxClear() >= size
		&& g_map.getTile(Map::toTileCoords((*it)->transition()->nwPos))->isExplored(team)) {
			return (*it)->cost(size);
		}
		return numeric_limits<float>::infinity();
	}
};

HAAStarResult RoutePlanner::findWaypointPathUnExplored(Unit *unit, const Vec2i &dest, WaypointPath &waypoints) {
	SECTION_TIMER(PATHFINDER_HIERARCHICAL);
	TIME_FUNCTION();
	_PROFILE_PATHFINDER();
	HAAStarResult res = setupHierarchicalOpenList(unit, dest);
	nsgSearchEngine->getNeighbourFunc().setSearchSpace(SearchSpace::CELLMAP);
	if (res == HAAStarResult::FAILURE) {
		return HAAStarResult::FAILURE;
	}
	UnexploredGoal goal(unit->getTeam());
	UnexploredCost cost(unit->getCurrField(), unit->getSize(), unit->getTeam());
	TransitionHeuristic heuristic(dest);
	tSearchEngine->aStar(goal, cost, heuristic);
	const Transition *t = goal.getBestSeen(unit->getPos(), dest);
	if (!t) {
		return HAAStarResult::FAILURE;
	}
	WaypointPath &wpPath = *unit->getWaypointPath();
	wpPath.clear();
	while (t) {
		waypoints.push(t->nwPos);
		t = tSearchEngine->getPreviousPos(t);
	}
	return res;
}

bool RoutePlanner::refinePath(Unit *unit) {
	SECTION_TIMER(PATHFINDER_LOWLEVEL);
	_PROFILE_PATHFINDER();
	WaypointPath &wpPath = *unit->getWaypointPath();
	UnitPath &path = *unit->getPath();
	assert(!wpPath.empty());
	const Vec2i &startPos = path.empty() ? unit->getPos() : path.back();
	const Vec2i &destPos = wpPath.front();
	AnnotatedMap *aMap = world->getCartographer()->getAnnotatedMap(unit);
	MoveCost cost(unit, aMap);
	DiagonalDistance dd(destPos);
	PosGoal posGoal(destPos);
	nsgSearchEngine->setStart(startPos, dd(startPos));
	AStarResult res = nsgSearchEngine->aStar(posGoal, cost, dd);
	if (res != AStarResult::COMPLETE) {
		return false;
	}
	IF_DEBUG_EDITION( collectOpenClosed<NodePool>(nsgSearchEngine->getStorage()); )
	Vec2i pos = nsgSearchEngine->getGoalPos();
	assert(pos == destPos);
	list<Vec2i>::iterator it = path.end();
	while (pos.x != -1) {
		it = path.insert(it, pos);
		pos = nsgSearchEngine->getPreviousPos(pos);
	}
	it = path.erase(it);
	wpPath.pop();
	return true;
}

#undef max

void RoutePlanner::smoothPath(Unit *unit) {
	SECTION_TIMER(PATHFINDER_LOWLEVEL);
	_PROFILE_PATHFINDER();
	if (unit->getPath()->size() < 3) {
		return;
	}
	AnnotatedMap* const &aMap = world->getCartographer()->getMasterMap();
	int min_x = numeric_limits<int>::max(),
		max_x = -1,
		min_y = numeric_limits<int>::max(),
		max_y = -1;
	set<Vec2i> onPath;
	UnitPath::iterator it = unit->getPath()->begin();
	for ( ; it  != unit->getPath()->end(); ++it) {
		if (it->x < min_x) min_x = it->x;
		if (it->x > max_x) max_x = it->x;
		if (it->y < min_y) min_y = it->y;
		if (it->y > max_y) max_y = it->y;
		onPath.insert(*it);
	}
	Rect2i bounds(min_x, min_y, max_x + 1, max_y + 1);
	it = unit->getPath()->begin();
	UnitPath::iterator nit = it;
	++nit;
	while (nit != unit->getPath()->end()) {
		onPath.erase(*it);
		Vec2i sp = *it;
		for (OrdinalDir d(0); d < OrdinalDir::COUNT; ++d) {
			Vec2i p = *it + OrdinalOffsets[d];
			if (p == *nit) continue;
			Vec2i intersect(-1);
			while (bounds.isInside(p)) {
				if (!aMap->canOccupy(p, unit->getSize(), unit->getCurrField())) {
					break;
				}
				if (d % 2 == 1) {
					Vec2i off1, off2;
					getDiags(p - OrdinalOffsets[d], p, unit->getSize(), off1, off2);
					if (!aMap->canOccupy(off1, 1, unit->getCurrField())
					|| !aMap->canOccupy(off2, 1, unit->getCurrField())) {
						break;
					}
				}
				if (onPath.find(p) != onPath.end()) {
					intersect = p;
					break;
				}
				p += OrdinalOffsets[d];
			}
			if (intersect != Vec2i(-1)) {
				UnitPath::iterator eit = nit;
				while (*eit != intersect) {
					onPath.erase(*eit++);
				}
				nit = unit->getPath()->erase(nit, eit);
				sp += OrdinalOffsets[d];
				while (sp != intersect) {
					unit->getPath()->insert(nit, sp);
					onPath.insert(sp);
					sp += OrdinalOffsets[d];
				}
				break;
			}
		}
		nit = ++it;
		++nit;
	}
}

const int minPathRefinement = int(GameConstants::clusterSize * 1.5f);

TravelState RoutePlanner::doRouteCache(Unit *unit) {
	UnitPath &path = *unit->getPath();
	WaypointPath &wpPath = *unit->getWaypointPath();
	float step = unit->getPos().dist(path.front());
	if (step > 1.5f || step < 0.9f) {
		PF_LOG( "Invalid route cache." );
		PF_PATH_LOG( unit );
		return TravelState::BLOCKED; // invalid
	}
	if (attemptMove(unit)) {
		if (!wpPath.empty() && path.size() < 12) {
			IF_DEBUG_EDITION( clearOpenClosed(unit->getPos(), wpPath.back()); )
			while (!wpPath.empty() && path.size() < minPathRefinement) {
				if (!refinePath(unit)) {
					wpPath.clear();
					PF_LOG( "refinePath() failed. [route cache], clearing waypoint-path" );
					break;
				}
			}
			PF_LOG( "doRouteCache() path refined." );
			smoothPath(unit);
			IF_DEBUG_EDITION( collectPath(unit); )
		}
		PF_LOG( "moving from " << unit->getPos() << " to " << unit->getNextPos() );
		PF_PATH_LOG( unit );
		return TravelState::MOVING;
	}
	path.incBlockCount();
	if (!path.isBlocked()) {
		PF_LOG( "doRouteCache() BLOCKED" );
		return TravelState::BLOCKED;
	}
	IF_DEBUG_EDITION( clearOpenClosed(unit->getPos(), wpPath.empty() ? path.back() : wpPath.front()); )
	if (repairPath(unit) && attemptMove(unit)) {
		PF_UNIT_LOG( unit, "doRouteCache() Blocked path repaired, moving from " << unit->getPos() << " to " << unit->getNextPos() );
		PF_PATH_LOG( unit );
		IF_DEBUG_EDITION( collectPath(unit); )
		path.resetBlockCount();
		return TravelState::MOVING;
	}
	PF_LOG( "doRouteCache() BLOCKED, block count exceeded, clearing paths" );
	unit->clearPath();
	return TravelState::BLOCKED;
}

TravelState RoutePlanner::doQuickPathSearch(Unit *unit, const Vec2i &target) {
	SECTION_TIMER(PATHFINDER_LOWLEVEL);
	_PROFILE_PATHFINDER();
	AnnotatedMap *aMap = world->getCartographer()->getAnnotatedMap(unit);
	UnitPath &path = *unit->getPath();
	IF_DEBUG_EDITION( clearOpenClosed(unit->getPos(), target); )
	aMap->annotateLocal(unit);
	float cost = quickSearch(unit->getCurrField(), unit->getSize(), unit->getPos(), target);
	aMap->clearLocalAnnotations(unit);
	IF_DEBUG_EDITION( collectOpenClosed<NodePool>(nodeStore); )
	if (cost != numeric_limits<float>::infinity()) {
		Vec2i pos = nsgSearchEngine->getGoalPos();
		while (pos.x != -1) {
			path.push_front(pos);
			pos = nsgSearchEngine->getPreviousPos(pos);
		}
		if (path.size() > 1) {
			path.pop();
			if (attemptMove(unit)) {
				IF_DEBUG_EDITION( collectPath(unit); )
				PF_LOG( "doQuickPathSearch() ok. moving from " << unit->getPos() << " to " << unit->getNextPos() );
				PF_PATH_LOG( unit );
				return TravelState::MOVING;
			}
		}
		PF_LOG( "doQuickPathSearch() search success, but path invalid. clearing path." );
		unit->clearPath();
	}
	PF_LOG( "doQuickPathSearch() failed." );
	return TravelState::BLOCKED;
}

TravelState RoutePlanner::findAerialPath(Unit *unit, const Vec2i &targetPos) {
	SECTION_TIMER(PATHFINDER_LOWLEVEL);
	_PROFILE_PATHFINDER();
	AnnotatedMap *aMap = world->getCartographer()->getMasterMap();
	UnitPath &path = *unit->getPath();
	PosGoal goal(targetPos);
	MoveCost cost(unit, aMap);
	DiagonalDistance dd(targetPos);
	nsgSearchEngine->setNodeLimit(256);
	nsgSearchEngine->setStart(unit->getPos(), dd(unit->getPos()));
	aMap->annotateLocal(unit);
	AStarResult res = nsgSearchEngine->aStar(goal, cost, dd);
	aMap->clearLocalAnnotations(unit);
	if (res == AStarResult::COMPLETE || res == AStarResult::NODE_LIMIT) {
		Vec2i pos = nsgSearchEngine->getGoalPos();
		while (pos.x != -1) {
			path.push_front(pos);
			pos = nsgSearchEngine->getPreviousPos(pos);
		}
		if (path.size() > 1) {
			path.pop();
			if (attemptMove(unit)) {
				PF_LOG( "findAerialPath() Ok." );
				PF_PATH_LOG( unit );
				return TravelState::MOVING;
			}
		} else {
			unit->clearPath();
		}
	}
	PF_LOG( "findAerialPath() failed, incrementing blockcount." );
	path.incBlockCount();
	return TravelState::BLOCKED;
}

TravelState RoutePlanner::findPathToLocation(Unit *unit, const Vec2i &finalPos) {
	SECTION_TIMER(PATHFINDER_TOTAL);
	PF_UNIT_LOG( unit, "findPathToLocation() current pos = " << unit->getPos() << " target pos = " << finalPos );
	PF_LOG( "Command class = " << CmdClassNames[g_simInterface.processingCommandClass()] );
	if (!world->getMap()->isInside(finalPos)) {
		stringstream ss;
		ss << __FUNCTION__ << "() passed bad arg, pos = " << finalPos
			<< "\nWhile processing " << CmdClassNames[g_simInterface.processingCommandClass()]
			<< " command.";
		g_logger.logError(ss.str());
	}
	UnitPath &path = *unit->getPath();
	WaypointPath &wpPath = *unit->getWaypointPath();
	if (finalPos == unit->getPos()) {
		unit->stop();
		PF_LOG( "ARRIVED, at pos." );
		PF_PATH_LOG( unit );
		return TravelState::ARRIVED;
	}
	if (!path.empty()) {
		if (doRouteCache(unit) == TravelState::MOVING) {
			return TravelState::MOVING;
		}
		if (!path.isBlocked()) {
			return TravelState::BLOCKED;
		}
	} else {
		PF_LOG( "path is empty." );
	}
	const Vec2i &target = computeNearestFreePos(unit, finalPos);
	if (target == unit->getPos()) {
		unit->stop();
		PF_LOG( "ARRIVED, as close as possible." );
		PF_PATH_LOG( unit );
		return TravelState::ARRIVED;
	}
	if (unit->getCurrField() == Field::AIR) {
		return findAerialPath(unit, target);
	}
	Vec2i startCluster = ClusterMap::cellToCluster(unit->getPos());
	Vec2i destCluster  = ClusterMap::cellToCluster(target);
	if (startCluster.dist(destCluster) < 3.f) {
		if (doQuickPathSearch(unit, target) == TravelState::MOVING) {
			return TravelState::MOVING;
		}
	}
	PF_LOG( "Performing hierarchical search." );
	tSearchEngine->reset();
	HAAStarResult res;
	tSearchEngine->reset();
	RUNTIME_CHECK(world->getMap()->isInside(target));
	if (unit->getTeam() == -1 || g_map.getTile(Map::toTileCoords(target))->isExplored(unit->getTeam())) {
		res = findWaypointPath(unit, target, wpPath);
	} else {
		res = findWaypointPathUnExplored(unit, target, wpPath);
	}
	if (res == HAAStarResult::FAILURE) {
		if (unit->getFaction()->isThisFaction()) {
			g_console.addLine(g_lang.get("DestinationUnreachable"));
		}
		PF_LOG( "Route not possible." );
		return TravelState::IMPOSSIBLE;
	} else if (res == HAAStarResult::START_TRAP) {
		PF_UNIT_LOG( unit, "START_TRAP." );
		if (wpPath.size() < 2) {
			PF_LOG( "Only one waypoint, blocked." );
			PF_PATH_LOG( unit );
			return TravelState::BLOCKED;
		}
	}
	IF_DEBUG_EDITION( collectWaypointPath(unit); )
	RUNTIME_CHECK(!wpPath.empty());
	IF_DEBUG_EDITION( clearOpenClosed(unit->getPos(), target); )
	AnnotatedMap *aMap = world->getCartographer()->getMasterMap();
	aMap->annotateLocal(unit);
	wpPath.condense();
	while (!wpPath.empty() && path.size() < minPathRefinement) {
		if (!refinePath(unit)) {
			PF_LOG( "refinePath failed." );
			PF_PATH_LOG( unit );
			aMap->clearLocalAnnotations(unit);
			path.incBlockCount();
			return TravelState::BLOCKED;
		}
	}
	smoothPath(unit);
	aMap->clearLocalAnnotations(unit);
	IF_DEBUG_EDITION( collectPath(unit); )
	if (path.empty()) {
		PF_LOG( "post hierarchical search failure, path empty." );
		PF_PATH_LOG( unit );
		return TravelState::BLOCKED;
	}
	if (attemptMove(unit)) {
		PF_LOG( "moving from " << unit->getPos() << " to " << unit->getNextPos() );
		PF_PATH_LOG( unit );
		return TravelState::MOVING;
	}
	PF_LOG( "post hierarchical search failure, path invalid?" );
	PF_PATH_LOG( unit );
	unit->stop();
	path.incBlockCount();
	return TravelState::BLOCKED;
}

TravelState RoutePlanner::customGoalSearch(PMap1Goal &goal, Unit *unit, const Vec2i &target) {
	SECTION_TIMER(PATHFINDER_LOWLEVEL);
	_PROFILE_PATHFINDER();
	UnitPath &path = *unit->getPath();
	const Vec2i &start = unit->getPos();
	MoveCost moveCost(unit->getCurrField(), unit->getSize(), world->getCartographer()->getMasterMap());
	DiagonalDistance heuristic(target);
	nsgSearchEngine->setNodeLimit(512);
	nsgSearchEngine->setStart(start, heuristic(start));
	AStarResult r;
	AnnotatedMap *aMap = world->getCartographer()->getMasterMap();
	aMap->annotateLocal(unit);
	r = nsgSearchEngine->aStar(goal, moveCost, heuristic);
	aMap->clearLocalAnnotations(unit);
	if (r == AStarResult::COMPLETE) {
		Vec2i pos = nsgSearchEngine->getGoalPos();
		IF_DEBUG_EDITION( clearOpenClosed(unit->getPos(), pos); )
		IF_DEBUG_EDITION( collectOpenClosed<NodePool>(nsgSearchEngine->getStorage()); )
		while (pos.x != -1) {
			path.push_front(pos);
			pos = nsgSearchEngine->getPreviousPos(pos);
		}
		if (!path.empty()) path.pop();
		IF_DEBUG_EDITION( collectPath(unit); )
		if (attemptMove(unit)) {
			PF_LOG( "customGoalSearch() ok. moving from " << unit->getPos() << " to " << unit->getNextPos() );
			PF_PATH_LOG( unit );
			return TravelState::MOVING;
		}
		PF_LOG( "customGoalSearch() search success, but path invalid." );
		unit->clearPath();
	}
	PF_LOG( "customGoalSearch() search failed." );
	return TravelState::BLOCKED;
}

TravelState RoutePlanner::findPathToGoal(Unit *unit, PMap1Goal &goal, const Vec2i &target) {
	SECTION_TIMER(PATHFINDER_TOTAL);
	PF_UNIT_LOG( unit, "findPathToGoal() current pos = " << unit->getPos() << " target pos = " << target );
	PF_LOG( "Command class = " << CmdClassNames[g_simInterface.processingCommandClass()] );
	UnitPath &path = *unit->getPath();
	WaypointPath &wpPath = *unit->getWaypointPath();
	if (goal(unit->getPos(), 0.f)) {
		unit->stop();
		PF_LOG( "ARRIVED, at goal." );
		PF_PATH_LOG( unit );
		return TravelState::ARRIVED;
	}
	if (!path.empty()) {
		if (doRouteCache(unit) == TravelState::MOVING) {
			return TravelState::MOVING;
		}
		path.incBlockCount();
		if (!path.isBlocked()) {
			PF_LOG( "BLOCKED" );
			return TravelState::BLOCKED;
		}
		unit->clearPath();
	}
	if (unit->getPos().dist(target) < 50.f) {
		if (customGoalSearch(goal, unit, target) == TravelState::MOVING) {
			return TravelState::MOVING;
		} else {
			return TravelState::BLOCKED;
		}
	}
	PF_LOG( "Performing hierarchical search." );
	tSearchEngine->reset();
	if (g_map.getTile(Map::toTileCoords(target))->isExplored(unit->getTeam())) {
		if (!findWaypointPath(unit, target, wpPath)) {
			PF_LOG( "Route not possible (normal search)." );
			return TravelState::IMPOSSIBLE;
		}
	} else {
		if (!findWaypointPathUnExplored(unit, target, wpPath)) {
			PF_LOG( "Route not possible (un-explored goal)." );
			return TravelState::IMPOSSIBLE;
		}
	}
	IF_DEBUG_EDITION( collectWaypointPath(unit); )
	RUNTIME_CHECK(wpPath.size() > 1);
	wpPath.pop();
	IF_DEBUG_EDITION( clearOpenClosed(unit->getPos(), target); )\
	while (wpPath.size() > 1 && wpPath.back().dist(target) < 32.f) {
		wpPath.pop_back();
	}
	AnnotatedMap *aMap = world->getCartographer()->getMasterMap();
	aMap->annotateLocal(unit);
	while (!wpPath.empty() && path.size() < minPathRefinement) {
		if (!refinePath(unit)) {
			PF_LOG( "BLOCKED, refinePath failed!!" );
			PF_PATH_LOG( unit );
			aMap->clearLocalAnnotations(unit);
			return TravelState::BLOCKED;
		}
	}
	smoothPath(unit);
	aMap->clearLocalAnnotations(unit);
	IF_DEBUG_EDITION( collectPath(unit); )
	if (attemptMove(unit)) {
		PF_LOG( "moving from " << unit->getPos() << " to " << unit->getNextPos() );
		PF_PATH_LOG( unit );
		return TravelState::MOVING;
	}
	unit->stop();
	PF_LOG( "post hierarchical search failure, path invalid?" );
	PF_PATH_LOG( unit );
	return TravelState::BLOCKED;
}

bool RoutePlanner::repairPath(Unit *unit) {
	SECTION_TIMER(PATHFINDER_LOWLEVEL);
	UnitPath &path = *unit->getPath();
	WaypointPath &wpPath = *unit->getWaypointPath();

	Vec2i dest;
	if (path.size() < 10 && !wpPath.empty()) {
		dest = wpPath.front();
	} else {
		dest = path.back();
	}
	unit->clearPath();
	AnnotatedMap *aMap = world->getCartographer()->getAnnotatedMap(unit);
	aMap->annotateLocal(unit);
	if (quickSearch(unit->getCurrField(), unit->getSize(), unit->getPos(), dest)
	!= numeric_limits<float>::infinity()) {
		Vec2i pos = nsgSearchEngine->getGoalPos();
		while (pos.x != -1) {
			path.push_front(pos);
			pos = nsgSearchEngine->getPreviousPos(pos);
		}
		if (path.size() > 2) {
			path.pop();
			if (!wpPath.empty() && wpPath.front() == path.back()) {
				wpPath.pop();
			}
		} else {
			unit->clearPath();
		}
	}
	aMap->clearLocalAnnotations(unit);
	if (!path.empty()) {
		IF_DEBUG_EDITION (
			collectOpenClosed<NodePool>(nsgSearchEngine->getStorage());
			collectPath(unit);
		)
		return true;
	}
	return false;
}

#if _GAE_DEBUG_EDITION_

TravelState RoutePlanner::doFullLowLevelAStar(Unit *unit, const Vec2i &dest) {
	UnitPath &path = *unit->getPath();
	WaypointPath &wpPath = *unit->getWaypointPath();
	const Vec2i &target = computeNearestFreePos(unit, dest);
	if (target == unit->getPos()) {
		unit->stop();
		return TravelState::ARRIVED;
	}
	unit->clearPath();
	AnnotatedMap *aMap = g_world.getCartographer()->getAnnotatedMap(unit);
	SearchEngine<NodeMap> *se = g_world.getCartographer()->getSearchEngine();
	DiagonalDistance dd(target);
	MoveCost cost(unit, aMap);
	PosGoal goal(target);
	se->setNodeLimit(-1);
	se->setStart(unit->getPos(), dd(unit->getPos()));
	AStarResult res = se->aStar(goal,cost,dd);
	list<Vec2i>::iterator it;
	IF_DEBUG_EDITION (
		list<Vec2i> *nodes = NULL;
		NodeMap* nm = se->getStorage();
	)
	Vec2i goalPos, pos;
	switch (res) {
		case AStarResult::COMPLETE:
			goalPos = se->getGoalPos();
			pos = goalPos;
			while (pos.x != -1) {
				path.push(pos);
				pos = se->getPreviousPos(pos);
			}
			if (!path.empty()) path.pop();
			IF_DEBUG_EDITION (
				collectOpenClosed<NodeMap>(se->getStorage());
				collectPath(unit);
			)
			break;
		case AStarResult::FAILURE:
			return TravelState::IMPOSSIBLE;
		default:
			throw runtime_error("Something that shouldn't have happened, did happen :(");
	}
	if (path.empty()) {
		unit->stop();
		return TravelState::ARRIVED;
	}
	if (attemptMove(unit)) return TravelState::MOVING;
	unit->stop();
	path.incBlockCount();
	return TravelState::BLOCKED;
}

#endif // _GAE_DEBUG_EDITION_

Vec2i RoutePlanner::computeNearestFreePos(const Unit *unit, const Vec2i &finalPos) {
	Vec2i pos = finalPos;
	RUNTIME_CHECK(world->getMap()->isInside(pos));
	Vec2i unitPos = unit->getPos();
	int size = unit->getType()->getSize();
	Field field = unit->getCurrField();
	int teamIndex = unit->getTeam();
	if (world->getMap()->areAproxFreeCells(pos, size, field, teamIndex)) {
		return pos;
	}
	Vec2i nearestPos = unitPos;
	float nearestDist = unitPos.dist(pos);
	for (int i = -maxFreeSearchRadius; i <= maxFreeSearchRadius; ++i) {
		for (int j = -maxFreeSearchRadius; j <= maxFreeSearchRadius; ++j) {
			Vec2i currPos = pos + Vec2i(i, j);
			if (world->getMap()->areAproxFreeCells(currPos, size, field, teamIndex)) {
				float dist = currPos.dist(pos);
				if (dist < nearestDist) {
					nearestPos = currPos;
					nearestDist = dist;
				} else if (dist == nearestDist) {
					if (currPos.dist(unitPos) < nearestPos.dist(unitPos)) {
						nearestPos = currPos;
					}
				}
			}
		}
	}
	return nearestPos;
}
}}
