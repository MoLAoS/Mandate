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
//
// File: graph_search.h
//
// Low Level Search Routines and additional support functions
//

#ifndef _GLEST_GAME_PATHFINDER_GRAPH_SEARCH_H_
#define _GLEST_GAME_PATHFINDER_GRAPH_SEARCH_H_

#include "astar_nodepool.h"
#include "bestfirst_node_pool.h"
#include "vec.h"

// Vector list iterators...
typedef list<Vec2i>::iterator VLIt;
typedef list<Vec2i>::const_iterator VLConIt;
typedef list<Vec2i>::reverse_iterator VLRevIt;
typedef list<Vec2i>::const_reverse_iterator VLConRevIt;

namespace Glest { namespace Game { 

class Map;

namespace Search {

class AnnotatedMap;

// =====================================================
// class SearchParams
//
// Parameters for a single search
// =====================================================
struct SearchParams {
	Vec2i start, dest;
	Field field;
	int size, team;
	bool (*goalFunc)(const Vec2i&);
	SearchParams ( Unit *u );
};

#if ! ( defined WIN32 || defined WIN64 )
inline int min ( int a, int b ) { return a < b ? a : b; }
#endif

__inline float heuristic ( const Vec2i &p1, const Vec2i &p2 ) {
	int diagonal = min ( abs (p1.x-p2.x), abs (p1.y-p2.y) );
	int straight = abs (p1.x-p2.x) + abs (p1.y-p2.y) - 2 * diagonal;
	return 1.4 * diagonal + 1.0 * straight;
}

// class GraphSearch
//
// Encapsulates the search algorithms
class GraphSearch {
public:
	GraphSearch ();
	virtual ~GraphSearch ();

	void init ( Map *cm, AnnotatedMap *am, bool astar=true );
	//
	// Search Functions
	//

	// Greedy Best First Search
	bool GreedySearch ( SearchParams &params, list<Vec2i> &path );

	// GreedySearch() run from start to goal, then goal to start and
	// the two paths 'merged' to create a single higher quality path
	bool GreedyPingPong ( SearchParams &params, list<Vec2i> &path );

	// The Classic A* [Hart, Nilsson, & Raphael, 1968]
	bool AStarSearch ( SearchParams &params, list<Vec2i> &path );

	// fills d1 and d2 with the diagonal cells(coords) that need checking for a 
	// unit of size to move from s to d, for diagonal moves.
	// WARNING: ASSUMES ( s.x != d.x && s.y != d.y ) ie. the move IS diagonal
	// if this condition is not true, results are undefined
	static inline void getDiags ( const Vec2i &s, const Vec2i &d, const int size, Vec2i &d1, Vec2i &d2 );

	// Fringe Search ? (a bit of A* with a dash of IDA*) [Bj�rnsson, et al, 2005]
	//bool FringeSearch ( SearchParams params, list<Vec2i> &path );

	// true if any path to at least radius cells away can be found
	// Performs a modified Best First Search where the heuristic is substituted
	// for zero minus the distance to here, the algorithm thus tries to 'escape'
	// (ie, get as far from start as quickly as possible).
	bool canPathOut ( const Vec2i &pos, const int radius, Field field );
	bool canPathOut_Greedy ( const Vec2i &pos, const int radius, Field field );

private:
	void init (); 
	void getCrossOverPoints ( const list<Vec2i> &one, const list<Vec2i> &two, list<Vec2i> &result );
	bool mergePath ( const list<Vec2i> &fwd, const list<Vec2i> &bwd, list<Vec2i> &co, list<Vec2i> &path );
	void copyToPath ( const list<Vec2i> &pathList, list<Vec2i> &path );

	AnnotatedMap *aMap; // the annotated map to search on

	// REMOVE ME, replace with function pointer to a hasUnit() type function.
	// 'local annotations' takes care of this now, but we still need the explored
	// state from the map, we should keep a compact representation with the annotated
	// map for better cache performance in the search
	Map *cMap; // the cell map 

	BFSNodePool *bNodePool;
	AStarNodePool *aNodePool;

#ifdef PATHFINDER_TIMING
public:
	static PathFinderStats *statsAStar;
	static PathFinderStats *statsGreedy;
	static void resetCounters ();
#endif
	bool assertValidPath ( list<Vec2i> &path );
};

}}}

#endif
