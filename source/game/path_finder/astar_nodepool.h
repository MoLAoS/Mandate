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
// File: astar_nodepool.h
//

#ifndef _GLEST_GAME_PATHFINDER_ASTAR_NODEPOOL_H_
#define _GLEST_GAME_PATHFINDER_ASTAR_NODEPOOL_H_

#define LOW_LEVEL_SEARCH_ADMISSABLE_HEURISTIC

#include "vec.h"
#include "timer.h"
#include "unit_stats_base.h"

using Shared::Platform::Chrono;

namespace Glest { namespace Game { 

class Unit;
class Map;

namespace Search {

#ifdef PATHFINDER_TIMING
struct PathFinderStats {      
	int64 num_searches;
	double search_avg;
	int64 num_searches_last_interval;
	double search_avg_last_interval;
	int64 num_searches_this_interval;
	double search_avg_this_interval;

	int64 worst_search;
	int64 calls_rejected;

	PathFinderStats ( char * name );
	void resetCounters ();
	void AddEntry ( int64 ticks );
	void IncReject () { calls_rejected++; }
	char* GetTotalStats ();
	char* GetStats ();
	char buffer[512];
	char prefix[32];
};
#endif

// =====================================================
// struct AStarNode
//
// A node structure for A* and friends
// =====================================================
struct AStarNode {
	Vec2i pos;
	AStarNode *next;
	AStarNode *prev;
	float heuristic;
	float distToHere;
	bool exploredCell;
	float est () const { return distToHere + heuristic; }
}; // 25 bytes == 28 in mem ?

// Comparison function for the heap
class AStarComp {
public:
	bool operator () ( const AStarNode *one, const AStarNode *two ) const;
};

// ========================================================
// class AStarNodePool
//
// A Node Manager for A* like algorithms
// ========================================================
class AStarNodePool {
	// =====================================================
	// struct DoubleMarkerArray
	//
	// A Marker Array supporting two mark types, open and closed.
	// =====================================================
	struct DoubleMarkerArray {
		int stride;
		unsigned int counter;
		unsigned int *marker;

		DoubleMarkerArray () {counter=1;marker=NULL;};
		~DoubleMarkerArray () {if (marker) delete marker; }

		void init ( int w, int h ) { stride = w; marker = new unsigned int[w*h]; 
		memset ( marker, 0, w * h * sizeof(unsigned int) ); }
		inline void newSearch () { counter += 2; }

		inline void setNeither ( const Vec2i &pos ) { marker[pos.y * stride + pos.x] = 0; }
		inline void setOpen ( const Vec2i &pos ) { marker[pos.y * stride + pos.x] = counter; }
		inline void setClosed ( const Vec2i &pos ) { marker[pos.y * stride + pos.x] = counter + 1; }

		inline bool isOpen ( const Vec2i &pos ) { return marker[pos.y * stride + pos.x] == counter; }
		inline bool isClosed ( const Vec2i &pos ) { return marker[pos.y * stride + pos.x] == counter + 1; }
		inline bool isListed ( const Vec2i &pos ) { return marker[pos.y * stride + pos.x] >= counter; } 
	};

	// =====================================================
	// struct PointerArray
	//
	// An array of pointers
	// =====================================================
	struct PointerArray {
		int stride;
		void **pArray;

		void init ( int w, int h ) { stride = w; pArray = new void*[w*h]; 
		memset ( pArray, NULL, w * h * sizeof(void*) ); }

		inline void  set ( const Vec2i &pos, void *ptr ) { pArray[pos.y * stride + pos.x] = ptr; }
		inline void* get ( const Vec2i &pos ) { return pArray[pos.y * stride + pos.x]; }
	};

public:
	AStarNodePool ();
	virtual ~AStarNodePool ();

	void init ( Map *map );

	// sets a temporary maximum number of nodes to use (50 <= max <= pathFindMaxNodes)
	void setMaxNodes ( const int max );

	// reset the node pool for a new search (resets tmpMaxNodes too)
	void reset ();

	// create and add a new node to the open list
	bool addToOpen ( AStarNode* prev, const Vec2i &pos, float h, float d, bool exp = true );
	// add a node to the open list
	void addOpenNode ( AStarNode *node );

	// returns the node with the lowest heuristic (from open and closed)
	AStarNode* getBestHNode () { return leastH; }

	// test if a position is open
	bool isOpen ( const Vec2i &pos ) { return markerArray.isOpen ( pos ); }

	// conditionally update a node (known to be open)
	void updateOpenNode ( const Vec2i &pos, AStarNode *neighbour, float cost );

	// returns the 'best' node from open (and removes it from open, placing it in closed)
	AStarNode* getBestCandidate ();

	// test if a position is closed
	bool isClosed ( const Vec2i &pos ) { return markerArray.isClosed ( pos ); } 

	// needed for canPathOut()
	bool isListed ( const Vec2i &pos ) { return markerArray.isListed ( pos ); }
#ifndef LOW_LEVEL_SEARCH_ADMISSABLE_HEURISTIC
	// conditionally update a node (known to be closed) and re-open if updated
	void updateClosedNode ( const Vec2i &pos, AStarNode *neighbour, float cost );
#endif
#ifdef PATHFINDER_TIMING
	int64 startTime;
	void startTimer () { startTime = Chrono::getCurMicros (); }
	int64 stopTimer () { return Chrono::getCurMicros () - startTime; }
#endif
#ifdef _GAE_DEBUG_EDITION_
	virtual list<Vec2i>* getOpenNodes ();
	virtual list<Vec2i>* getClosedNodes ();
	list<Vec2i> listedNodes;
#endif

private:
	AStarNode *leastH;
	AStarNode *stock;
	int numNodes;
	int tmpMaxNodes;

	DoubleMarkerArray markerArray;
	PointerArray pointerArray;
	vector<AStarNode*> openHeap;
};

}}}

#endif