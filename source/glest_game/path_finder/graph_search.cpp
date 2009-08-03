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
// File: graph_search.cpp
//
// Low Level Search Routines and additional support functions
//
#include "pch.h"

#include "graph_search.h"
#include "path_finder.h"
#include "map.h"
#include "vec.h"
#include "config.h"

namespace Glest { namespace Game { namespace Search {

SearchParams::SearchParams ( Unit *u ) {
	start = u->getPos(); 
	field = u->getCurrField ();
	size = u->getSize (); 
	team = u->getTeam ();
	goalFunc = NULL;
}

GraphSearch::GraphSearch () {
	aMap = NULL;
	cMap = NULL;
	bNodePool = NULL;
	aNodePool = NULL;

#	ifdef PATHFINDER_TIMING
		statsAStar = new PathFinderStats ( "A-Star Search : " );
		statsGreedy = new PathFinderStats ( "Greedy Search : " );
#	endif
}
GraphSearch::~GraphSearch () {
	delete bNodePool;
	delete aNodePool;
#	ifdef PATHFINDER_TIMING
		delete statsAStar;
		delete statsGreedy;
#	endif
}

void GraphSearch::init ( Map *cell_map, AnnotatedMap *annt_map, bool astar ) {
	cMap = cell_map;
	aMap = annt_map;	

	if ( astar ) {
		aNodePool = new AStarNodePool ();
		aNodePool->init ( cell_map );
	}
	else {
		bNodePool = new BFSNodePool ();
		bNodePool->init ( cell_map );
	}
}

bool GraphSearch::GreedySearch ( SearchParams &params, list<Vec2i> &path) {
#ifdef PATHFINDER_TIMING
	int64 startTime = Chrono::getCurMicros ();
#endif
	Zone zone = params.field == FieldAir ? ZoneAir : ZoneSurface;
	bNodePool->reset ();
	bNodePool->addToOpen ( NULL, params.start, heuristic ( params.start, params.dest ) );
	bool pathFound= true;
	bool nodeLimitReached= false;
	BFSNode *minNode= NULL;
	const Vec2i *Directions = OffsetsSize1Dist1;

	while( ! nodeLimitReached ) {
		minNode = bNodePool->getBestCandidate ();
		if ( ! minNode ) {  // open was empty? 
			pathFound = false;
			break; // failure
		}
		if ( minNode->pos == params.dest || ! minNode->exploredCell 
		||  ( params.goalFunc && params.goalFunc (minNode->pos) ) ) { 
			break; // success
		}
		for ( int i = 0; i < 8 && ! nodeLimitReached; ++ i )
		{	// for each neighbour of minNode
			Vec2i sucPos = minNode->pos + Directions[i];
			if ( !cMap->isInside ( sucPos ) 
			||	 !aMap->canOccupy ( sucPos, params.size, params.field )
			||	 bNodePool->isListed ( sucPos ) ) {
				continue; 
			}
			if ( minNode->pos.x != sucPos.x && minNode->pos.y != sucPos.y ) {
				// if diagonal move and either diag cell is not free...
				Vec2i diag1, diag2;
				getDiags ( minNode->pos, sucPos, params.size, diag1, diag2 );
				if ( !aMap->canOccupy (diag1, 1, params.field) 
				||   !aMap->canOccupy (diag2, 1, params.field) )
					continue; // not allowed
			}
			// else move is legal.
			bool exp = cMap->getTile (Map::toTileCoords (sucPos))->isExplored (params.team);
			if ( ! bNodePool->addToOpen ( minNode, sucPos, heuristic ( sucPos, params.dest ), exp ) )
				nodeLimitReached = true;
		} // end for ... inner loop
	} // end while ... outer loop
#	ifdef PATHFINDER_TIMING
		statsGreedy->AddEntry ( Chrono::getCurMicros () - startTime );
#	endif
	if ( !pathFound ) {
		path.clear ();
		return false;
	}
	BFSNode *lastNode= minNode;
	if ( nodeLimitReached ) {
		lastNode = bNodePool->getLowestH ();
	}

	// on the way
	// fill in next pointers
	BFSNode *currNode = lastNode;
	int steps = 0;
	while ( currNode->prev ) {
		currNode->prev->next = currNode;
		currNode = currNode->prev;
		steps++;
	}
	BFSNode *firstNode = currNode;

	//store path
	path.clear ();
	currNode = firstNode;
	while ( currNode ) {
		path.push_back ( currNode->pos );
		currNode = currNode->next;
	}
#	ifdef _GAE_DEBUG_EDITION_
	if ( Config::getInstance().getMiscDebugTextures() ) {
		PathFinder *pf = PathFinder::getInstance();
		pf->PathStart = path.front();
		pf->PathDest = path.back();
		pf->OpenSet.clear(); pf->ClosedSet.clear();
		pf->PathSet.clear(); pf->LocalAnnotations.clear ();
		if ( pf->debug_texture_action == PathFinder::ShowOpenClosedSets ) {
			list<Vec2i> *alist = bNodePool->getOpenNodes ();
			for ( VLIt it = alist->begin(); it != alist->end(); ++it )
				pf->OpenSet.insert ( *it );
			delete alist;
			alist = bNodePool->getClosedNodes ();
			for ( VLIt it = alist->begin(); it != alist->end(); ++it )
				pf->ClosedSet.insert ( *it );
			delete alist;
		}
		if ( pf->debug_texture_action == PathFinder::ShowOpenClosedSets 
		||   pf->debug_texture_action == PathFinder::ShowPathOnly )
			for ( VLIt it = path.begin(); it != path.end(); ++it )
				pf->PathSet.insert ( *it );
		if ( pf->debug_texture_action == PathFinder::ShowLocalAnnotations ) {
			pf->LocalAnnotations.clear();
			list<pair<Vec2i,uint32>> *annt = aMap->getLocalAnnotations ();
			for ( list<pair<Vec2i,uint32>>::iterator it = annt->begin(); it != annt->end(); ++it )
				pf->LocalAnnotations[it->first] = it->second;
			delete annt;
		}
	}
#	endif
	return true;
}

bool GraphSearch::AStarSearch ( SearchParams &params, list<Vec2i> &path ) {
#	ifdef PATHFINDER_TIMING
		aNodePool->startTimer ();
#	endif
	bool pathFound = false, nodeLimitReached = false;
	AStarNode *minNode = NULL;
	const Vec2i *Directions = OffsetsSize1Dist1;
	aNodePool->reset ();
	aNodePool->addToOpen ( NULL, params.start, heuristic ( params.start, params.dest ), 0 );
	while ( ! nodeLimitReached ) {
		minNode = aNodePool->getBestCandidate ();
		if ( ! minNode ) break; // done, failed
		if ( minNode->pos == params.dest || ! minNode->exploredCell 
		||  ( params.goalFunc && params.goalFunc (minNode->pos ) ) ) { // done, success
			pathFound = true; 
			break; 
		}
		for ( int i = 0; i < 8 && ! nodeLimitReached; ++i ) {  // for each neighbour
			Vec2i sucPos = minNode->pos + Directions[i];
			if ( ! cMap->isInside ( sucPos ) 
			||	 ! aMap->canOccupy (sucPos, params.size, params.field )) {
				continue;
			}
			bool diag = false;
			if ( minNode->pos.x != sucPos.x && minNode->pos.y != sucPos.y ) {
				// if diagonal move and either diag cell is not free...
				diag = true;
				Vec2i diag1, diag2;
				getDiags ( minNode->pos, sucPos, params.size, diag1, diag2 );
				if ( !aMap->canOccupy ( diag1, 1, params.field ) 
				||	 !aMap->canOccupy ( diag2, 1, params.field ) ) {
					continue; // not allowed
				}
			}
			// Assumes heuristic is admissable, or that you don't care if it isn't
			if ( aNodePool->isOpen ( sucPos ) ) {
				aNodePool->updateOpenNode ( sucPos, minNode, diag ? 1.4 : 1.0 );
			}
			else if ( ! aNodePool->isClosed ( sucPos ) ) {
				bool exp = cMap->getTile (Map::toTileCoords (sucPos))->isExplored (params.team);
				float h = heuristic ( sucPos, params.dest );
				float d = minNode->distToHere + (diag?1.4:1.0);
				if ( ! aNodePool->addToOpen ( minNode, sucPos, h, d, exp ) ) {
					nodeLimitReached = true;
				}
			}
		} // end for each neighbour of minNode
	} // end while ( ! nodeLimitReached )
#	ifdef PATHFINDER_TIMING
		statsAStar->AddEntry ( aNodePool->stopTimer () );
#	endif
	if ( ! pathFound && ! nodeLimitReached ) {
		return false;
	}
	if ( nodeLimitReached ) {
		// get node closest to goal
		minNode = aNodePool->getBestHNode ();
		path.clear ();
		while ( minNode ) {
			path.push_front ( minNode->pos );
			minNode = minNode->prev;
		}
		int backoff = path.size () / 10;
		// back up a bit, to avoid a possible cul-de-sac
		for ( int i=0; i < backoff ; ++i ) {
			path.pop_back ();
		}
	}
	else {  // fill in path
		path.clear ();
		while ( minNode ) {
			path.push_front ( minNode->pos );
			minNode = minNode->prev;
		}
	}
	if ( path.size () < 2 ) {
		path.clear ();
		return true; //tsArrived
	}

#	ifdef _GAE_DEBUG_EDITION_
		if ( Config::getInstance().getMiscDebugTextures() ) {
			PathFinder *pf = PathFinder::getInstance();
			pf->PathStart = path.front();
			pf->PathDest = path.back();
			pf->OpenSet.clear(); pf->ClosedSet.clear();
			pf->PathSet.clear(); pf->LocalAnnotations.clear ();
			if ( pf->debug_texture_action == PathFinder::ShowOpenClosedSets ) {
				list<Vec2i> *alist = aNodePool->getOpenNodes ();
				for ( VLIt it = alist->begin(); it != alist->end(); ++it )
				pf->OpenSet.insert ( *it );
				delete alist;
				alist = aNodePool->getClosedNodes ();
				for ( VLIt it = alist->begin(); it != alist->end(); ++it )
				pf->ClosedSet.insert ( *it );
				delete alist;
			}
			if ( pf->debug_texture_action == PathFinder::ShowOpenClosedSets 
			||   pf->debug_texture_action == PathFinder::ShowPathOnly )
				for ( VLIt it = path.begin(); it != path.end(); ++it )
					pf->PathSet.insert ( *it );
			if ( pf->debug_texture_action == PathFinder::ShowLocalAnnotations ) {
				pf->LocalAnnotations.clear();
				list<pair<Vec2i,uint32>> *annt = aMap->getLocalAnnotations ();
				for ( list<pair<Vec2i,uint32>>::iterator it = annt->begin(); it != annt->end(); ++it )
				pf->LocalAnnotations[it->first] = it->second;
				delete annt;
			}
		}
#	endif
	//assert ( assertValidPath ( path ) );
	return true;
}

bool GraphSearch::assertValidPath ( list<Vec2i> &path ) {
	if ( path.size () < 2 ) return true;
	VLIt it = path.begin();
	Vec2i prevPos = *it;
	for ( ++it; it != path.end(); ++it ) {
		if ( prevPos.dist(*it) < 0.99 || prevPos.dist(*it) > 1.42 )
			return false;
		prevPos = *it;
	}
	return true;
}

//KILLME Remove when Greedy search is removed...
bool GraphSearch::canPathOut_Greedy ( const Vec2i &pos, const int radius, Field field  ) {
	assert ( radius > 0 && radius <= 5 );
	bNodePool->reset ();
	bNodePool->addToOpen ( NULL, pos, 0 );
	bool pathFound= false;
	BFSNode *maxNode = NULL;
	const Vec2i *Directions = OffsetsSize1Dist1;

	while( ! pathFound ) {
		maxNode = bNodePool->getBestCandidate ();
		if ( ! maxNode ) break; // failure
		for ( int i = 0; i < 8 && ! pathFound; ++ i ) {
			Vec2i sucPos = maxNode->pos + Directions[i];
			if ( ! cMap->isInside ( sucPos ) 
			||   ! cMap->getCell( sucPos )->isFree( field == FieldAir ? ZoneAir: ZoneSurface ) )
				continue;
			//CanOccupy() will be cheapest, do it first...
			if ( aMap->canOccupy (sucPos, 1, field) && ! bNodePool->isListed (sucPos) ) {
				if ( maxNode->pos.x != sucPos.x && maxNode->pos.y != sucPos.y ) {
					// if diagonal move
					Vec2i diag1 ( maxNode->pos.x, sucPos.y );
					Vec2i diag2 ( sucPos.x, maxNode->pos.y );
					// and either diag cell is not free...
					if ( ! aMap->canOccupy ( diag1, 1, field ) 
					||   ! aMap->canOccupy ( diag2, 1, field )
					||   ! cMap->getCell( diag1 )->isFree( field == FieldAir ? ZoneAir: ZoneSurface ) 
					||   ! cMap->getCell( diag2 )->isFree( field == FieldAir ? ZoneAir: ZoneSurface ) )
						continue; // not allowed
				}
				// Move is legal.
				if ( -(maxNode->heuristic) + 1 >= radius ) 
					pathFound = true;
				else
					bNodePool->addToOpen ( maxNode, sucPos, maxNode->heuristic - 1.f );
			} // end if
		} // end for
	} // end while
	return pathFound;
}

// true if any path to at least radius cells away can be found
// Performs a modified A* Search where the f-value is substituted
// for zero minus the g-value, the algorithm thus tries to 'escape'
// (ie, get as far from start as quickly as possible).
bool GraphSearch::canPathOut ( const Vec2i &pos, const int radius, Field field  ) {
	assert ( radius > 0 && radius <= 5 );
	aNodePool->reset ();
	aNodePool->addToOpen ( NULL, pos, 0, 0 );
	bool pathFound= false;
	AStarNode *maxNode = NULL;
	const Vec2i *Directions = OffsetsSize1Dist1;

	while( ! pathFound ) {
		maxNode = aNodePool->getBestCandidate ();
		if ( ! maxNode ) break; // failure
		for ( int i = 0; i < 8 && ! pathFound; ++ i ) {
			Vec2i sucPos = maxNode->pos + Directions[i];
			if ( ! cMap->isInside ( sucPos ) 
			||   ! cMap->getCell( sucPos )->isFree( field == FieldAir ? ZoneAir: ZoneSurface ) )
				continue;
			//CanOccupy() will be cheapest, do it first...
			if ( aMap->canOccupy (sucPos, 1, field) && ! aNodePool->isListed (sucPos) ) {
				if ( maxNode->pos.x != sucPos.x && maxNode->pos.y != sucPos.y ) {
					// if diagonal move
					Vec2i diag1 ( maxNode->pos.x, sucPos.y );
					Vec2i diag2 ( sucPos.x, maxNode->pos.y );
					// and either diag cell is not free...
					if ( ! aMap->canOccupy ( diag1, 1, field ) || ! aMap->canOccupy ( diag2, 1, field )
					||   ! cMap->getCell( diag1 )->isFree( field == FieldAir ? ZoneAir: ZoneSurface ) 
					||   ! cMap->getCell( diag2 )->isFree( field == FieldAir ? ZoneAir: ZoneSurface ) )
						continue; // not allowed
				}
				// Move is legal.
				if ( -(maxNode->distToHere) + 1 >= radius ) 
					pathFound = true;
				else
					aNodePool->addToOpen ( maxNode, sucPos, 0, maxNode->distToHere - 1.f );
			} // end if
		} // end for
	} // end while
	return pathFound;
}

void GraphSearch::getDiags ( const Vec2i &s, const Vec2i &d, const int size, Vec2i &d1, Vec2i &d2 ) {
	assert ( s.x != d.x && s.y != d.y );
	if ( size == 1 ) {
		d1.x = s.x; d1.y = d.y;
		d2.x = d.x; d2.y = s.y;
		return;
	}
	if ( d.x > s.x ) {  // travelling east
		if ( d.y > s.y ) {  // se
			d1.x = d.x + size - 1; d1.y = s.y;
			d2.x = s.x; d2.y = d.y + size - 1;
		}
		else {  // ne
			d1.x = s.x; d1.y = d.y;
			d2.x = d.x + size - 1; d2.y = s.y - size + 1;
		}
	}
	else {  // travelling west
		if ( d.y > s.y ) {  // sw
			d1.x = d.x; d1.y = s.y;
			d2.x = s.x + size - 1; d2.y = d.y + size - 1;
		}
		else {  // nw
			d1.x = d.x; d1.y = s.y - size + 1;
			d2.x = s.x + size - 1; d2.y = d.y;
		}
	}
}

void GraphSearch::copyToPath ( const list<Vec2i> &pathList, list<Vec2i> &path ) {
	for ( VLConIt it = pathList.begin (); it != pathList.end(); ++it )
		path.push_back ( *it );
}

void GraphSearch::getCrossOverPoints ( const list<Vec2i> &forward, const list<Vec2i> &backward, list<Vec2i> &result ) {
	result.clear ();
	bNodePool->reset ();
	for ( VLConRevIt it1 = backward.rbegin(); it1 != backward.rend(); ++it1 )
		bNodePool->listPos ( *it1 );
	for ( VLConIt it2 = forward.begin(); it2 != forward.end(); ++it2 )
		if ( bNodePool->isListed ( *it2 ) )
			result.push_back ( *it2 );
}

bool GraphSearch::mergePath ( const list<Vec2i> &fwd, const list<Vec2i> &bwd, list<Vec2i> &co, list<Vec2i> &path ) {
	assert ( co.size () <= fwd.size () );

	if ( !path.empty () ) path.clear ();

	if ( fwd.size () == co.size () ) {  // paths never diverge
		copyToPath ( fwd, path );
		return true;
	}
	VLConIt fIt = fwd.begin ();
	VLConRevIt bIt = bwd.rbegin ();
	VLIt coIt = co.begin ();

	//the 'first' and 'last' nodes on fwd and bwd must be the first and last on co...
	assert ( *coIt == *fIt && *coIt == *bIt );
	assert ( co.back() == fwd.back() && co.back() == bwd.front() );
	if ( ! ( *coIt == *fIt && *coIt == *bIt ) ) {
		Logger::getInstance().add ( "LowLevelSearch::MergePath() was passed dodgey data..." );
		copyToPath ( fwd, path );
		return false;
	}
	// push start pos
	path.push_back ( *coIt );

	++fIt; ++bIt;
	// Should probably just iterate over co aswell...
	coIt = co.erase ( coIt );

	while ( coIt != co.end () ) {
		// coIt now points to a pos that is common to both paths, but isn't the start... 
		// skip any more duplicates, putting them onto the path
		while ( *coIt == *fIt ) {
			path.push_back ( *coIt );
			if ( *fIt != *bIt ) {
				Logger::getInstance().add ( "LowLevelSearch::MergePath() was passed a dodgey crossover list..." );
				if ( !path.empty () ) path.clear ();
				copyToPath ( fwd, path );
				return false;
			}
			coIt = co.erase ( coIt );
			if ( coIt == co.end() ) // that's it folks!
				return true;
			++fIt; ++bIt;
		}
		// coIt now points to the next common pos, 
		// fIt and bIt point to the positions where the path's have just diverged
		int fGap = 0, bGap = 0;

		VLConIt fStart = fIt; // save our spot
		VLConRevIt bStart = bIt; // ditto
		while ( *fIt != *coIt ) { fIt ++; fGap ++; }
		while ( *bIt != *coIt ) { bIt ++; bGap ++; }
		if ( bGap < fGap ) {  // copy section from bwd
			while ( *bStart != *coIt ) {
				path.push_back ( *bStart );
				bStart ++;
			}
		}
		else {  // copy section from fwd
			while ( *fStart != *coIt ) {
				path.push_back ( *fStart );
				fStart ++;
			}
		}
		// now *fIt == *bIt == *coIt... skip duplicates, etc etc...
	} // end while ( coIt != co.end () )
	Logger::getInstance().add ( "in LowLevelSearch::MergePath() your 'unreachable' code was reached... ?!?" );
	return true; // keep the compiler happy...
}

#ifdef PATHFINDER_TIMING
void GraphSearch::resetCounters () {
	statsAStar->resetCounters ();
	statsGreedy->resetCounters ();
}
PathFinderStats *GraphSearch::statsAStar = NULL;
PathFinderStats *GraphSearch::statsGreedy = NULL;
#endif


}}}

