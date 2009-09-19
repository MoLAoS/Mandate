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
// File: bestfirst_node_pool.h
//
// Low Level Search Routines and additional support structures
//

#ifndef _GLEST_GAME_BEST_FIRST_NODE_POOL_H_
#define _GLEST_GAME_BEST_FIRST_NODE_POOL_H_

#include "vec.h"
#include <list>

using Shared::Graphics::Vec2i;
using std::list;

namespace Glest { namespace Game { 

class Map;

namespace Search {

// =====================================================
// struct BFSNode
//
// A Greedy 'Best First' Search Node
// =====================================================
struct BFSNode {
	Vec2i pos;
	BFSNode *next;
	BFSNode *prev;
	float heuristic;
	bool exploredCell;
};

// ========================================================
// class BFSNodePool
//
// An interface to the open/closed lists.
//
// This NodePool supports the Greedy search algorithm, and 
// operates on BFSNode structures. Tests for 'listedness'
// report if a position is listed or not, there is no 
// mechanism to test whether a listed position is open or 
// closed.
// ========================================================
class BFSNodePool {
	// =====================================================
	// struct PosMarkerArray
	//
	// A Marker Array, using sizeof(unsigned int) bytes per 
	// cell, and a lazy clearing scheme.
	// =====================================================
	struct PosMarkerArray {
		int stride;
		unsigned int counter;
		unsigned int *marker;

		PosMarkerArray () {counter=0;marker=NULL;};
		~PosMarkerArray () {if (marker) delete marker; }

		void init ( int w, int h ) { stride = w; marker = new unsigned int[w*h]; 
		memset ( marker, 0, w * h * sizeof(unsigned int) ); }
		inline void newSearch () { ++counter; }
		inline void setMark ( const Vec2i &pos ) { marker[pos.y * stride + pos.x] = counter; }
		inline bool isMarked ( const Vec2i &pos ) { return marker[pos.y * stride + pos.x] == counter; }
	};

	BFSNode *stock;
	BFSNode **lists;
	int numOpen, numClosed, numTotal, numPos;
	int maxNodes, tmpMaxNodes;

	PosMarkerArray markerArray;
	BFSNode *leastH;

public:
	BFSNodePool ();
	~BFSNodePool ();
	void init ( Map *map );

	// reset everything, include maxNodes...
	void reset ();
	// will be == PathFinder::pathFindMaxNodes (returns 'normal' max, not temp)
	int getMaxNodes () const { return maxNodes; }
	// sets a temporary maximum number of nodes to use (50 <= max <= pathFindMaxNodes)
	void setMaxNodes ( const int max );
	// Is this pos already listed?
	bool isListed ( const Vec2i &pos ) { return markerArray.isMarked ( pos ); }

	void listPos ( const Vec2i &pos ) { markerArray.setMark ( pos ); }
	bool addToOpen ( BFSNode* prev, const Vec2i &pos, float h, bool exp = true );

	// moves 'best' node from open to closed, and returns it, or NULL if open is empty
	BFSNode* getBestCandidate ();

	// returns the lowest h-node of this search
	BFSNode* getLowestH () { return leastH; }

#ifdef _GAE_DEBUG_EDITION_
	list<Vec2i>* getOpenNodes ();
	list<Vec2i>* getClosedNodes ();
#endif

}; // class BFSNodePool

}}}; // namespace Glest::Game::LowLevelSearch

#endif
