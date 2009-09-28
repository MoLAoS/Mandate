// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//               2009      James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_PATHFINDER_H_
#define _GLEST_GAME_PATHFINDER_H_

#include "annotated_map.h"
#include "graph_search.h"
#include "config.h"

#include <set>
#include <map>

using std::vector;
using std::list;

using Shared::Graphics::Vec2i;
using Shared::Platform::uint32;
using Shared::Platform::int64;

namespace Glest{ namespace Game{

class Map;
class Unit;
class UnitPath;

namespace Search {

// Some 'globals' (oh no!!! run for cover...)

const int maxFreeSearchRadius = 10;
//const int pathFindRefresh = 10; // now unused
const int pathFindNodesMax = Config::getInstance ().getPathFinderMaxNodes ();

const int numOffsetsSize1Dist1 = 8;
const Vec2i OffsetsSize1Dist1 [numOffsetsSize1Dist1] = 
{
	Vec2i (  0, -1 ), // n
	Vec2i (  1, -1 ), // ne
	Vec2i (  0,  1 ), // e
	Vec2i (  1,  1 ), // se
	Vec2i (  1,  0 ), // s
	Vec2i ( -1,  1 ), // sw
	Vec2i ( -1,  0 ), // w
	Vec2i ( -1, -1 )  // nw
};

const int numOffsetsSize1Dist2 = 16;
const Vec2i OffsetsSize1Dist2 [numOffsetsSize1Dist2] =
{
	Vec2i (  0, -2 ), // n
	Vec2i (  1, -2 ), // nne
	Vec2i (  2, -2 ), // ne
	Vec2i (  2, -1 ), // ene
	Vec2i (  2,  0 ), // e
	Vec2i (  2,  1 ), // ese
	Vec2i (  2,  2 ), // se
	Vec2i (  1,  2 ), // sse
	Vec2i (  0,  2 ), // s
	Vec2i ( -1,  2 ), // ssw
	Vec2i ( -2,  2 ), // sw
	Vec2i ( -2,  1 ), // wsw
	Vec2i ( -2,  0 ), // w
	Vec2i ( -2, -1 ), // wnw
	Vec2i ( -2, -2 ), // nw
	Vec2i ( -1, -2 ), // nnw
};

const int numOffsetsSize2Dist1 = 12;
const Vec2i OffsetsSize2Dist1 [numOffsetsSize2Dist1] = 
{
	Vec2i (  0, -1 ), // n
	Vec2i (  1, -1 ), // n
	Vec2i (  2, -1 ), // ne
	Vec2i (  2,  0 ), // e
	Vec2i (  2,  1 ), // e
	Vec2i (  2,  2 ), // se
	Vec2i (  1,  2 ), // s
	Vec2i (  0,  2 ), // s
	Vec2i ( -1,  2 ), // sw
	Vec2i ( -1,  1 ), // w
	Vec2i ( -1,  0 ), // w
	Vec2i ( -1, -1 )  // nw
};
const int numOffsetsSize2Dist2 = 20;
const Vec2i OffsetsSize2Dist2 [numOffsetsSize2Dist2] = 
{
	Vec2i (  0, -2 ), // n
	Vec2i (  1, -2 ), // n
	Vec2i (  2, -2 ), // nne
	Vec2i (  3, -2 ), // ne
	Vec2i (  3, -1 ), // ene
	Vec2i (  3,  0 ), // e
	Vec2i (  3,  1 ), // e
	Vec2i (  3,  2 ), // ese
	Vec2i (  3,  3 ), // se
	Vec2i (  2,  3 ), // sse
	Vec2i (  1,  3 ), // s
	Vec2i (  0,  3 ), // s
	Vec2i ( -1,  3 ), // ssw
	Vec2i ( -2,  3 ), // sw
	Vec2i ( -2,  2 ), // wsw
	Vec2i ( -2,  1 ), // w
	Vec2i ( -2,  0 ), // w
	Vec2i ( -2, -1 ), // wnw
	Vec2i ( -2, -2 ), // nw
	Vec2i ( -1, -2 ), // nnw
};

enum TravelState { tsArrived, tsOnTheWay, tsBlocked };

// =====================================================
// 	class PathFinder
//
//	Finds paths for units using 
// 
// =====================================================
class PathFinder {
public:
	static PathFinder* getInstance ();
	~PathFinder();
	void init(Map *map);

	static void setResourceGoal ( const ResourceType *resType ) {resourceGoal = resType;}
	static bool resourceGoalFunc ( const Vec2i &pos );

	static void setStoreGoal ( const Unit *store ) { storeGoal = store; }
	static bool storeGoalFunc ( const Vec2i &pos );

	TravelState findPathToGoal ( Unit *unit, const Vec2i &targetPos, bool (*func)(const Vec2i&)=NULL );

	TravelState findPathToResource ( Unit *unit, const Vec2i &targetPos, const ResourceType *resType ) { 
		setResourceGoal ( resType ); 
		return findPathToGoal ( unit, targetPos, &resourceGoalFunc ); 
	}

	TravelState findPathToStore ( Unit *unit, const Vec2i &targetPos, const Unit *store ) { 
		setStoreGoal ( store ); 
		return findPathToGoal ( unit, targetPos, &storeGoalFunc ); 
	}

	TravelState findPath(Unit *unit, const Vec2i &finalPos) { 
		return findPathToGoal ( unit, finalPos ); 
	}

	// legal move ?
	bool isLegalMove ( Unit *unit, const Vec2i &pos ) const;

	// update the annotated map at pos 
	void updateMapMetrics ( const Vec2i &pos, const int size, bool adding, Field field )
	{ annotatedMap->updateMapMetrics ( pos, size, adding, field ); }

private:
	static const ResourceType *resourceGoal;
	static const Unit *storeGoal;
	static PathFinder *singleton;
	PathFinder();

	Vec2i computeNearestFreePos (const Unit *unit, const Vec2i &targetPos);
	void copyToPath ( const list<Vec2i> pathList, UnitPath *path );
	Map *map;
	static inline void getDiags ( const Vec2i &s, const Vec2i &d, const int size, Vec2i &d1, Vec2i &d2 );

public: // should be private ... debugging...
	AnnotatedMap *annotatedMap;
	GraphSearch *search;

#ifdef _GAE_DEBUG_EDITION_
	Vec2i PathStart, PathDest;
	std::set<Vec2i> OpenSet, ClosedSet, PathSet;
	std::map<Vec2i,uint32> LocalAnnotations;
	enum { ShowPathOnly, ShowOpenClosedSets, ShowLocalAnnotations } debug_texture_action;
#endif

}; // class PathFinder

}}}//end namespace

#endif
