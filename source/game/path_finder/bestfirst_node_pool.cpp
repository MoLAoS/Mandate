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
// File: bestfirst_node_pool.cpp
//
#include "pch.h"

#include "bestfirst_node_pool.h"
#include "map.h"
#include "path_finder.h"

namespace Glest { namespace Game { namespace Search {

BFSNodePool::BFSNodePool () {
	maxNodes = pathFindNodesMax;
	stock = new BFSNode[maxNodes];
	lists = new BFSNode*[maxNodes];
}

BFSNodePool::~BFSNodePool () {
	delete [] stock;
	delete [] lists;
}
void BFSNodePool::init ( Map *map ) {
	markerArray.init ( map->getW(), map->getH() );
	reset ();
}
// reset the node pool
void BFSNodePool::reset () {
	numOpen = numClosed = numTotal = numPos = 0;
	tmpMaxNodes = maxNodes;
	markerArray.newSearch ();
	leastH = NULL;
}

void BFSNodePool::setMaxNodes ( const int max ) {
	assert ( max >= 50 && max <= maxNodes ); // reasonable number ?
	assert ( !numTotal ); // can't do this after we've started using it.
	tmpMaxNodes = max;
}

bool BFSNodePool::addToOpen ( BFSNode* prev, const Vec2i &pos, float h, bool exp ) {
	if ( numTotal == tmpMaxNodes ) 
		return false;
	stock[numTotal].next = NULL;
	stock[numTotal].prev = prev;
	stock[numTotal].pos = pos;
	stock[numTotal].heuristic = h;
	stock[numTotal].exploredCell = exp;
	const int top = tmpMaxNodes - 1;
	if ( !numOpen ) {
		lists[top] = &stock[numTotal];
		leastH = &stock[numTotal];
	}
	else {  
		if ( leastH->heuristic > stock[numTotal].heuristic )
			leastH = &stock[numTotal];
		// find insert index
		// due to the nature of the greedy algorithm, new nodes are likely to have lower 
		// heuristics than the majority already in open, so we start checking from the low end.
		const int openStart = tmpMaxNodes - numOpen - 1;
		int offset = openStart;

		while ( offset < top && lists[offset+1]->heuristic < stock[numTotal].heuristic ) 
			offset ++;

		if ( offset > openStart ) { // shift lower nodes down...
			int moveNdx = openStart;
			while ( moveNdx <= offset ) {
				lists[moveNdx-1] = lists[moveNdx];
				moveNdx ++;
			}
		}
		// insert newbie in sorted pos.
		lists[offset] = &stock[numTotal];
	}
	listPos ( pos );
	numTotal ++;
	numOpen ++;
	return true;
}

// Moves the lowest heuristic node from open to closed and returns a 
// pointer to it, or NULL if there are no open nodes.
BFSNode* BFSNodePool::getBestCandidate () {
	if ( !numOpen ) return NULL;
	lists[numClosed] = lists[tmpMaxNodes - numOpen];
	numOpen --;
	numClosed ++;
	return lists[numClosed-1];
}

#ifdef _GAE_DEBUG_EDITION_

list<Vec2i>* BFSNodePool::getOpenNodes () {
	list<Vec2i> *ret = new list<Vec2i> ();
	const int openStart = tmpMaxNodes - numOpen - 1;
	for ( int i = openStart; i < tmpMaxNodes; ++i ) {
		ret->push_back ( lists[i]->pos );
	}
	return ret;
}

list<Vec2i>* BFSNodePool::getClosedNodes () {
	list<Vec2i> *ret = new list<Vec2i> ();
	for ( int i = 0; i < numClosed; ++i ) {
		ret->push_back ( lists[i]->pos );
	}
	return ret;
}

#endif

}}}