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
// File: astar_nodepool.cpp
//
#include "pch.h"

#include "path_finder.h"
#include "astar_nodepool.h"
#include "map.h"
#include "unit.h"

namespace Glest { namespace Game { namespace Search {

bool AStarComp::operator () ( const AStarNode *one, const AStarNode *two ) const {
	if ( one->distToHere + one->heuristic < two->distToHere + two->heuristic ) 
		return false;
	if ( one->distToHere + one->heuristic > two->distToHere + two->heuristic ) 
		return true;
	// tie, prefer closer to goal...
	if ( one->heuristic < two->heuristic )
		return false;
	if ( one->heuristic > two->heuristic )
		return true;
	// still tied... prefer nodes 'in line' with goal ???

	// just distinguish them somehow...
	return one < two;
}

AStarNodePool::AStarNodePool () {
	stock = new AStarNode[pathFindNodesMax];
	numNodes = 0;
	tmpMaxNodes = pathFindNodesMax;
	leastH = NULL;
	openHeap.reserve ( pathFindNodesMax );
}

AStarNodePool::~AStarNodePool () {
	delete [] stock;
}

void AStarNodePool::init ( Map *map ) {
	markerArray.init ( map->getW(), map->getH() );
	pointerArray.init ( map->getW(), map->getH() );
}

void AStarNodePool::reset () {
	numNodes = 0;
	tmpMaxNodes = pathFindNodesMax;
	leastH = NULL;
	markerArray.newSearch ();
	openHeap.clear ();
#  ifdef _GAE_DEBUG_EDITION_
	listedNodes.clear ();
#  endif
}

void AStarNodePool::setMaxNodes ( const int max ) {
	assert ( max >= 50 && max <= pathFindNodesMax ); // reasonable number ?
	assert ( !numNodes ); // can't do this after we've started using it.
	tmpMaxNodes = max;
}

bool AStarNodePool::addToOpen ( AStarNode* prev, const Vec2i &pos, float h, float d, bool exp ) {
	if ( numNodes == tmpMaxNodes ) {
		return false;
	}
#  ifdef _GAE_DEBUG_EDITION_
	listedNodes.push_back ( pos );
#  endif
	stock[numNodes].next = NULL;
	stock[numNodes].prev = prev;
	stock[numNodes].pos = pos;
	stock[numNodes].distToHere = d;
	stock[numNodes].heuristic = h;
	stock[numNodes].exploredCell = exp;
	addOpenNode ( &stock[numNodes] );
	if ( numNodes ) {
		if ( h < leastH->heuristic ) {
			leastH = &stock[numNodes];
		}
	}
	else {
		leastH = &stock[numNodes];
	}

	numNodes++;
	return true;
}

void AStarNodePool::addOpenNode ( AStarNode *node ) {
	markerArray.setOpen ( node->pos );
	pointerArray.set ( node->pos, node );
	openHeap.push_back ( node );
	push_heap ( openHeap.begin(), openHeap.end(), AStarComp() );
}

void AStarNodePool::updateOpenNode ( const Vec2i &pos, AStarNode *neighbour, float cost ) {
	AStarNode *posNode = (AStarNode*)pointerArray.get ( pos );
	if ( neighbour->distToHere + cost < posNode->distToHere ) {
		posNode->distToHere = neighbour->distToHere + cost;
		posNode->prev = neighbour;

		// We could just push_heap from begin to 'posNode' (as we're only decreasing key)
		// but we need a quick method to get an iterator to posNode...
		//FIXME: We should just manage the heap oursleves...
		make_heap ( openHeap.begin(), openHeap.end(), AStarComp() );
	}
}

#ifndef LOW_LEVEL_SEARCH_ADMISSABLE_HEURISTIC

void AStarNodePool::updateClosedNode ( const Vec2i &pos, AStarNode *neighbour, float cost ) {
	AStarNode *posNode = closed[pos];
	if ( neighbour->distToHere + cost < posNode->distToHere ) {
		posNode->distToHere = neighbour->distToHere + cost;
		posNode->prev = neighbour;
		addOpenNode ( posNode );
		map<Vec2i,AStarNode*>::iterator it = closed.find ( posNode->pos );
		closed.erase ( it );
	}
}

#endif

AStarNode* AStarNodePool::getBestCandidate () {
	if ( openHeap.empty() ) return NULL;
	pop_heap ( openHeap.begin(), openHeap.end(), AStarComp() );
	AStarNode *ret = openHeap.back();
	openHeap.pop_back ();
	markerArray.setClosed ( ret->pos );
	return ret;
}

#ifdef _GAE_DEBUG_EDITION_

list<Vec2i>* AStarNodePool::getOpenNodes () {
	list<Vec2i> *ret = new list<Vec2i> ();
	list<Vec2i>::iterator it = listedNodes.begin();
	for ( ; it != listedNodes.end (); ++it ) {
		if ( isOpen ( *it ) ) ret->push_back ( *it );
	}
	return ret;
}

list<Vec2i>* AStarNodePool::getClosedNodes () {
	list<Vec2i> *ret = new list<Vec2i> ();
	list<Vec2i>::iterator it = listedNodes.begin();
	for ( ; it != listedNodes.end (); ++it ) {
		if ( isClosed ( *it ) ) ret->push_back ( *it );
	}
	return ret;
}

#endif // defined ( _GAE_DEBUG_EDITION_ )

#ifdef PATHFINDER_TIMING

PathFinderStats::PathFinderStats ( char *name ) {
	assert ( strlen ( name ) < 31 );
	if ( name ) strcpy ( prefix, name );
	else strcpy ( prefix, "??? : " );
	num_searches = num_searches_last_interval = 
		worst_search = calls_rejected = num_searches_this_interval = 0;
	search_avg = search_avg_last_interval = search_avg_this_interval = 0.0;
}

void PathFinderStats::resetCounters () {
	num_searches_last_interval = num_searches_this_interval;
	search_avg_last_interval = search_avg_this_interval;
	num_searches_this_interval = 0;
	search_avg_this_interval = 0.0;
}
char * PathFinderStats::GetStats () {
	sprintf ( buffer, "%s Processed last interval: %d, Average (micro-seconds): %g", prefix,
		(int)num_searches_last_interval, search_avg_last_interval );
	return buffer;
}
char * PathFinderStats::GetTotalStats () {
	sprintf ( buffer, "%s Total Searches: %d, Search Average (micro-seconds): %g, Worst: %d.", prefix,
		(int)num_searches, search_avg, (int)worst_search );
	return buffer;
}
void PathFinderStats::AddEntry ( int64 ticks ) {
	if ( num_searches_this_interval ) {
		search_avg_this_interval = ( search_avg_this_interval * (double)num_searches_this_interval 
			+ (double)ticks ) / (double)( num_searches_this_interval + 1 );
	}
	else {
		search_avg_this_interval = (double)ticks;
	}
	num_searches_this_interval++;
	if ( num_searches ) {
		search_avg  = ( (double)num_searches * search_avg + (double)ticks ) / (double)( num_searches + 1 );
	}
	else {
		search_avg = (double)ticks;
	}
	num_searches++;

	if ( ticks > worst_search ) worst_search = ticks;
}
#endif // defined ( PATHFINDER_TIMING )


}}}