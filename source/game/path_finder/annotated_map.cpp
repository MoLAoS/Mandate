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
// File: annotated_map.cpp
//
// Annotated Map, for use in pathfinding.
//
#include "pch.h"

//#include "annotated_map.h"
//#include "graph_search.h"
#include "map.h"
#include "path_finder.h"

namespace Glest { namespace Game { namespace Search {

const int AnnotatedMap::maxClearanceValue = 5;

AnnotatedMap::AnnotatedMap ( Map *m ) {
	cMap = m;
	metrics.init ( m->getW(), m->getH() );
	initMapMetrics ( cMap );
}

AnnotatedMap::~AnnotatedMap () {
}

void AnnotatedMap::initMapMetrics ( Map *map ) {
	const int east = map->getW() - 1;
	int x = east;
	int y = map->getH() - 1;

	// set southern two rows to zero.
	for ( ; x >= 0; --x ) {
		metrics[Vec2i(x,y)].setAll ( 0 );
		metrics[Vec2i(x,y-1)].setAll ( 0 );
	}
	for ( y -= 2; y >= 0; -- y) {
		for ( x = east; x >= 0; --x ) {
			Vec2i pos ( x, y );
			if ( x > east - 2 ) { // far east tile, not valid
				metrics[pos].setAll ( 0 );
			}
			else {
				computeClearances ( pos );
			}
		}
	}
}

// pos: location of object added/removed
// size: size of same object
void AnnotatedMap::updateMapMetrics ( const Vec2i &pos, const int size ) {
	assert ( cMap->isInside ( pos ) );
	assert ( cMap->isInside ( pos.x + size - 1, pos.y + size - 1 ) );

	// first, re-evaluate the cells occupied (or formerly occupied)
	for ( int i = size - 1; i >= 0 ; --i ) {
		for ( int j = size - 1; j >= 0; --j ) {
			Vec2i occPos = pos;
			occPos.x += i; occPos.y += j;
			computeClearances ( occPos );
		}
	}
	cascadingUpdate ( pos, size );
}

void AnnotatedMap::cascadingUpdate ( const Vec2i &pos, const int size,  const Field field ) {
	list<Vec2i> *leftList, *aboveList, leftList1, leftList2, aboveList1, aboveList2;
	leftList = &leftList1;
	aboveList = &aboveList1;
	// both the left and above lists need to be sorted, bigger values first (right->left, bottom->top)
	for ( int i = size - 1; i >= 0; --i ) {
		// Check if positions are on map, (the '+i' components are along the sides of the building/object, 
		// so we assume they are ok). If so, list them
		if ( pos.x-1 >= 0 ) {
			leftList->push_back ( Vec2i (pos.x-1,pos.y+i) );
		}
		if ( pos.y-1 >= 0 ) {
			aboveList->push_back ( Vec2i (pos.x+i,pos.y-1) );
		}
	}
	// the cell to the nothwest...
	Vec2i *corner = NULL;
	Vec2i cornerHolder ( pos.x-1, pos.y-1 );
	if ( pos.x-1 >= 0 && pos.y-1 >= 0 ) {
		corner = &cornerHolder;
	}

	while ( !leftList->empty() || !aboveList->empty() || corner ) {
		// the left and above lists for the next loop iteration
		list<Vec2i> *newLeftList, *newAboveList;
		newLeftList = leftList == &leftList1 ? &leftList2 : &leftList1;
		newAboveList = aboveList == &aboveList1 ? &aboveList2 : &aboveList1;

		if ( !leftList->empty() ) {
			for ( VLIt it = leftList->begin (); it != leftList->end (); ++it ) {
				bool changed = false;
				if ( field == FieldCount ) { // permanent annotation, update all
					CellMetrics old = metrics[*it];
					computeClearances ( *it );
					if ( old != metrics[*it] ) {
						if ( it->x - 1 >= 0 ) { // if there is a cell to the left, add it to
							newLeftList->push_back ( Vec2i(it->x-1,it->y) ); // the new left list
						}
					}
				}
				else { // local annotation, only check field, store original clearances
					uint32 old = metrics[*it].get ( field );
					if ( old ) computeClearance ( *it, field );
					if ( old && old > metrics[*it].get ( field ) ) {
						if ( localAnnt.find (*it) == localAnnt.end() ) {
							localAnnt[*it] = old; // was original clearance
						}
						if ( it->x - 1 >= 0 ) { // if there is a cell to the left, add it to
							newLeftList->push_back ( Vec2i(it->x-1,it->y) ); // the new left list
						}
					}
				}
			}
		}
		if ( !aboveList->empty() ) {
			for ( VLIt it = aboveList->begin (); it != aboveList->end (); ++it ) {
				if ( field == FieldCount ) {
					CellMetrics old = metrics[*it];
					computeClearances ( *it );
					if ( old != metrics[*it] ) {
						if ( it->y - 1 >= 0 ) {
							newAboveList->push_back ( Vec2i(it->x,it->y-1) );
						}
					}
				}
				else {
					uint32 old = metrics[*it].get ( field );
					if ( old ) computeClearance ( *it, field );
					if ( old && old > metrics[*it].get ( field ) ) {
						if ( localAnnt.find (*it) == localAnnt.end() ) {
							localAnnt[*it] = old;
						}
						if ( it->y - 1 >= 0 )  {
							newAboveList->push_back ( Vec2i(it->x,it->y-1) );
						}
					}
				}
			}
		}
		if ( corner ) {
			// Deal with the corner...
			if ( field == FieldCount ) {
				CellMetrics old = metrics[*corner];
				computeClearances ( *corner );
				if ( old != metrics[*corner] ) {
					int x = corner->x, y  = corner->y;
					if ( x - 1 >= 0 ) {
						newLeftList->push_back ( Vec2i(x-1,y) );
						if ( y - 1 >= 0 ) {
							*corner = Vec2i(x-1,y-1);
						}
						else {
							corner = NULL;
						}
					}
					else {
						corner = NULL;
					}
					if ( y - 1 >= 0 ) { 
						newAboveList->push_back ( Vec2i(x,y-1) );
					}
				}
				else { // no update
					corner = NULL;
				}
			}
			else {
				uint32 old = metrics[*corner].get ( field );
				if ( old ) computeClearance ( *corner, field );
				if ( old && old > metrics[*corner].get ( field ) ) {
					if ( localAnnt.find (*corner) == localAnnt.end() ) {
						localAnnt[*corner] = old;
					}
					int x = corner->x, y  = corner->y;
					if ( x - 1 >= 0 ) {
						newLeftList->push_back ( Vec2i(x-1,y) );
						if ( y - 1 >= 0 ) {
							*corner = Vec2i(x-1,y-1);
						}
						else {
							corner = NULL;
						}
					}
					else {
						corner = NULL;
					}
					if ( y - 1 >= 0 ) {
						newAboveList->push_back ( Vec2i(x,y-1) );
					}
				}
				else {
					corner = NULL;
				}
			}
		}
		leftList->clear (); 
		leftList = newLeftList;
		aboveList->clear (); 
		aboveList = newAboveList;
	}// end while
}

void AnnotatedMap::annotateUnit ( const Unit *unit, const Field field ) {
	const int size = unit->getSize ();
	const Vec2i &pos = unit->getPos ();
	assert ( cMap->isInside ( pos ) );
	assert ( cMap->isInside ( pos.x + size - 1, pos.y + size - 1 ) );
	// first, re-evaluate the cells occupied
	for ( int i = size - 1; i >= 0 ; --i ) {
		for ( int j = size - 1; j >= 0; --j ) {
			Vec2i occPos = pos;
			occPos.x += i; occPos.y += j;
			if ( !unit->getType()->hasCellMap() || unit->getType()->getCellMapCell (i, j) ) {
				if ( localAnnt.find (occPos) == localAnnt.end() ) {
					localAnnt[occPos] = metrics[occPos].get (field);
				}
				metrics[occPos].set ( field, 0 );
			}
			else {
				uint32 old =  metrics[occPos].get (field);
				computeClearance (occPos, field);
				if ( old != metrics[occPos].get (field) && localAnnt.find (occPos) == localAnnt.end() ) {
					localAnnt[occPos] = old;
				}
			}
		}
	}
	// propegate changes to left and above
	cascadingUpdate ( pos, size, field );
}

// Set clearances (all fields) to 0 if cell blocked, or 'calculate' metrics
void AnnotatedMap::computeClearances ( const Vec2i &pos ) {
	assert ( cMap->isInside ( pos ) );
	assert ( pos.x <= cMap->getW() - 2 );
	assert ( pos.y <= cMap->getH() - 2 );

	Cell *cell = cMap->getCell ( pos );

	// is there a building here, or an object on the tile ??
	bool surfaceBlocked = ( cell->getUnit(ZoneSurface) && !cell->getUnit(ZoneSurface)->isMobile() )
							||   !cMap->getTile ( cMap->toTileCoords ( pos ) )->isFree ();
	// Walkable
	if ( surfaceBlocked || cell->isDeepSubmerged () )
		metrics[pos].set ( FieldWalkable, 0 );
	else
		computeClearance ( pos, FieldWalkable );
	// Any Water
	if ( surfaceBlocked || !cell->isSubmerged () )
		metrics[pos].set ( FieldAnyWater, 0 );
	else
		computeClearance ( pos, FieldAnyWater );
	// Deep Water
	if ( surfaceBlocked || !cell->isDeepSubmerged () )
		metrics[pos].set ( FieldDeepWater, 0 );
	else
		computeClearance ( pos, FieldDeepWater );
	// Amphibious:
	if ( surfaceBlocked )
		metrics[pos].set ( FieldAmphibious, 0 );
	else
		computeClearance ( pos, FieldAmphibious );
	// Air
	computeClearance ( pos, FieldAir );
}

// Computes clearance based on metrics to the sout and east, Does NOT check if this cell is an obstactle
// assumes metrics of cells to the south, south-east & east are correct
void AnnotatedMap::computeClearance ( const Vec2i &pos, Field f ) {
	uint32 clear = metrics[Vec2i(pos.x, pos.y + 1)].get ( f );
	if ( clear > metrics[Vec2i( pos.x + 1, pos.y + 1 )].get ( f ) )
		clear = metrics[Vec2i( pos.x + 1, pos.y + 1 )].get ( f );
	if ( clear > metrics[Vec2i( pos.x + 1, pos.y )].get ( f ) )
		clear = metrics[Vec2i( pos.x + 1, pos.y )].get ( f );
	clear ++;
	if ( clear > maxClearanceValue ) 
		clear = maxClearanceValue;
	metrics[pos].set ( f, clear );
}

void AnnotatedMap::annotateLocal ( const Unit *unit, const Field field ) {
	const Vec2i &pos = unit->getPos ();
	const int &size = unit->getSize ();
	assert ( cMap->isInside ( pos ) );
	assert ( cMap->isInside ( pos.x + size - 1, pos.y + size - 1 ) );
	const int dist = 3;
	set<Unit*> annotate;

	for ( int y = pos.y - dist; y < pos.y + size + dist; ++y ) {
		for ( int x = pos.x - dist; x < pos.x + size + dist; ++x ) {
			Vec2i cPos(x,y);
			if ( cMap->isInside(cPos) && metrics[cPos].get(field) ) {
				Unit *u = cMap->getCell(cPos)->getUnit(field);
				if ( u && u != unit ) {
					annotate.insert(u);
				}
			}
		}
	}
	for ( set<Unit*>::iterator it = annotate.begin(); it != annotate.end(); ++it ) {
		annotateUnit(*it, field);
	}
}

void AnnotatedMap::clearLocalAnnotations(Field field) {
	for ( map<Vec2i,uint32>::iterator it = localAnnt.begin(); it != localAnnt.end(); ++ it ) {
		assert(it->second <= maxClearanceValue);
		assert(cMap->isInside(it->first));
		metrics[it->first].set(field, it->second);
	}
	localAnnt.clear();
}

#ifdef _GAE_DEBUG_EDITION_

list<pair<Vec2i,uint32>>* AnnotatedMap::getLocalAnnotations () {
	list<pair<Vec2i,uint32>> *ret = new list<pair<Vec2i,uint32>> ();
	for ( map<Vec2i,uint32>::iterator it = localAnnt.begin (); it != localAnnt.end (); ++ it )
		ret->push_back ( pair<Vec2i,uint32> (it->first,metrics[it->first].get(FieldWalkable)) );
	return ret;
}

#endif

}}}
