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

inline uint32 CellMetrics::get ( const Field field ) {
	switch ( field ) {
		case FieldWalkable: return field0;
		case FieldAir: return field1;
		case FieldAnyWater: return field2;
		case FieldDeepWater: return field3;
		case FieldAmphibious: return field4;
		default: throw runtime_error ( "Unknown Field passed to CellMetrics::get()" );
	}
	return 0;
}

inline void CellMetrics::set ( const Field field, uint32 val ) {
	assert ( val <= AnnotatedMap::maxClearanceValue );
	switch ( field ) {
		case FieldWalkable: field0 = val; return;
		case FieldAir: field1 = val; return;
		case FieldAnyWater: field2 = val; return;
		case FieldDeepWater: field3 = val; return;
		case FieldAmphibious: field4 = val; return;
		default: throw runtime_error ( "Unknown Field passed to CellMetrics::set()" );
	}

}

const int AnnotatedMap::maxClearanceValue = 3;

AnnotatedMap::AnnotatedMap ( Map *m ) {
	cMap = m;
	metrics.init ( m->getW(), m->getH() );
	initMapMetrics ( cMap );
}

AnnotatedMap::~AnnotatedMap () {
}

void AnnotatedMap::initMapMetrics ( Map *map ) {
	for ( int i=0; i < map->getH(); ++i ) {
		for ( int j=0; j < map->getW(); ++j ) {
			Vec2i pos( j, i );
			metrics[pos] = computeClearances ( pos );
		}
	}
}

// pos: location of object added/removed
// size: size of same object
// adding: true if the object has been added, false if it has been removed.
void AnnotatedMap::updateMapMetrics ( const Vec2i &pos, const int size, bool adding, Field field ) {
	assert ( cMap->isInside ( pos ) );
	assert ( cMap->isInside ( pos.x + size - 1, pos.y + size - 1 ) );

	// first, re-evaluate the cells occupied (or formerly occupied)
	for ( int i=0; i < size; ++i ) {
		for ( int j=0; j < size; ++j ) {
			Vec2i occPos = pos;
			occPos.x += i; occPos.y += j;
			metrics[occPos].set ( field, computeClearance ( occPos, field ) );
		}
	}

	// now, look to the left and above those cells, 
	// updating clearances where needed...
	list<Vec2i> leftList1, leftList2, aboveList1, aboveList2;
	list<Vec2i> *LeftList, *AboveList;
	LeftList = &leftList1;
	AboveList = &aboveList1;

	// the cell to the nothwest...
	Vec2i *corner = NULL;
	if ( pos.x-1 >= 0 && pos.y-1 >= 0 ) corner = &Vec2i(pos.x-1,pos.y-1);

	for ( int i = 0; i < size; ++i ) {
		// Check if positions are on map, (the '+i' components are 
		// along the sides of the building/object, so we assume 
		// they are ok). If so, list them
		if ( pos.x-1 >= 0 ) LeftList->push_back ( Vec2i (pos.x-1,pos.y+i) );
		if ( pos.y-1 >= 0 ) AboveList->push_back ( Vec2i (pos.x+i,pos.y-1) );
	}
	// This counts how far away from the new/old object we are, and is used 
	// to update the clearances without a costly call to ComputClearance() 
	// (if we're 'adding' that is)
	uint32 shell = 1;
	while ( !LeftList->empty() || !AboveList->empty() || corner ) {
		// the left and above lists for the next loop iteration
		list<Vec2i> *newLeftList, *newAboveList;
		newLeftList = LeftList == &leftList1 ? &leftList2 : &leftList1;
		newAboveList = AboveList == &aboveList1 ? &aboveList2 : &aboveList1;

		if ( !LeftList->empty() ) {
			for ( VLIt it = LeftList->begin (); it != LeftList->end (); ++it ) {
				// if we're adding and the current metric is bigger 
				// than shell, we need to update it
				if ( adding && metrics[*it].get(field) > shell ) {
					// FIXME: Does not handle cell maps properly ( shell + metric[y][x+shell].fieldX ??? )
					metrics[*it].set ( field, shell );
					if ( it->x - 1 >= 0 ) // if there is a cell to the left, add it to
						newLeftList->push_back ( Vec2i(it->x-1,it->y) ); // the new left list
				}
				// if we're removing and the metrics of the cell to the right has a 
				// larger metric than this cell, we _may_ need to update this cell.
				else if ( !adding ) {
					Vec2i rightPos ( it->x+1, it->y );
					if ( metrics[*it].get(field) < metrics[rightPos].get(field) ) {
						uint32 old = metrics[*it].get(field);
						metrics[*it].set ( field, computeClearance ( *it, field ) ); // re-compute
						// if we changed the metric and there is a cell to the left, add it to the 
						if ( metrics[*it].get(field) != old && it->x - 1 >= 0 ) // new left list
							newLeftList->push_back ( Vec2i(it->x-1,it->y) );
						// if we didn't change the metric, no need to keep looking left
					}
				}
				// else we didn't need to update this metric, and cells to the left
				// will therefore also still have correct metrics and we can stop
				// looking along this row (it->y).
			}
		}
		// Do the equivalent for the above list
		if ( !AboveList->empty() ) {
			for ( VLIt it = AboveList->begin (); it != AboveList->end (); ++it ) 			{
				if ( adding && metrics[*it].get(field) > shell ) {
					metrics[*it].set ( field,  shell );
					if ( it->y - 1 >= 0 )
						newAboveList->push_back ( Vec2i(it->x,it->y-1) );
				}
				else if ( !adding ) {
					Vec2i abovePos ( it->x, it->y-1 );
					if ( metrics[*it].get(field) < metrics[abovePos].get(field) ) {
						uint32 old = metrics[*it].get(field);
						metrics[*it].set ( field, computeClearance ( *it, field ) );
						if ( metrics[*it].get(field) != old && it->y - 1 >= 0 ) 
							newAboveList->push_back ( Vec2i(it->x,it->y-1) );
					}
				}
			}
		}
		if ( corner ) {
			// Deal with the corner...
			int x = corner->x, y  = corner->y;
			if ( adding && metrics[*corner].get(field) > shell ) {
				metrics[*corner].set ( field, shell ); // update
				// add cell left to the new left list, cell above to new above list, and
				// set corner to point at the next corner (x-1,y-1) (if they're on the map.)
				if ( x - 1 >= 0 ) {
					newLeftList->push_back ( Vec2i(x-1,y) );
					if ( y - 1 >= 0 ) *corner = Vec2i(x-1,y-1);
					else corner = NULL;
				}
				else corner = NULL;
				if ( y - 1 >= 0 ) newAboveList->push_back ( Vec2i(x,y-1) );
			}
			else if ( !adding  ) {
				Vec2i prevCorner ( x+1, y+1 );
				if ( metrics[*corner].get(field) < metrics[prevCorner].get(field) ) {
					uint32 old = metrics[*corner].get(field);
					metrics[*corner].set (field, computeClearance ( *corner, field ) );
					if ( metrics[*corner].get(field) != old ) { // did we update ?
						if ( x - 1 >= 0 ) {
							newLeftList->push_back ( Vec2i(x-1,y) );
							if ( y - 1 >= 0 ) *corner = Vec2i(x-1,y-1);
							else corner = NULL;
						}
						else corner = NULL;
						if ( y - 1 >= 0 ) newAboveList->push_back ( Vec2i(x,y-1) );
					}
					else {// no update, stop looking at corners
						corner = NULL;
					}
				}
				else { // no update
					corner = NULL;
				}
			}
			else { // no update
				corner = NULL;
			}
		}
		LeftList->clear (); 
		LeftList = newLeftList;
		AboveList->clear (); 
		AboveList = newAboveList;
		shell++;
	}// end while
}

//TODO
// this could be made much faster pretty easily... 
// make it so...
// if cell is an obstacle in field, clearance = 0. done.
// else...
//   let clearance = the south-east metric, 
//   if the east metric < clearance let clearance = east metric
//   if the south metric < clearance let clearance = south metric
//   clearance++. done.
CellMetrics AnnotatedMap::computeClearances ( const Vec2i &pos ) {
	assert ( cMap->isInside ( pos ) );
	CellMetrics clearances;

	Tile *container = cMap->getTile ( cMap->toTileCoords ( pos ) );

	if ( container->isFree () ) { // Tile is walkable?
		// Walkable
		while ( canClear ( pos, clearances.get ( FieldWalkable ) + 1, FieldWalkable ) )
			clearances.set ( FieldWalkable, clearances.get ( FieldWalkable ) + 1 );
		// Any Water
		while ( canClear ( pos, clearances.get ( FieldAnyWater ) + 1, FieldAnyWater ) )
			clearances.set ( FieldAnyWater, clearances.get ( FieldAnyWater ) + 1 );
		// Deep Water
		while ( canClear ( pos, clearances.get ( FieldDeepWater ) + 1, FieldDeepWater ) )
			clearances.set ( FieldDeepWater, clearances.get ( FieldDeepWater ) + 1 );
		// Amphibious
		while ( canClear ( pos, clearances.get ( FieldAmphibious ) + 1, FieldAmphibious ) )
			clearances.set ( FieldAmphibious, clearances.get ( FieldAmphibious ) + 1 );
	}

	// Air
	int clearAir = maxClearanceValue;
	if ( pos.x == cMap->getW() - 3 ) clearAir = 1;
	else if ( pos.x == cMap->getW() - 4 ) clearAir = 2;
	if ( pos.y == cMap->getH() - 3 ) clearAir = 1;
	else if ( pos.y == cMap->getH() - 4 && clearAir > 2 ) clearAir = 2;
	clearances.set ( FieldAir, clearAir );

	return clearances;
}
// as above, make faster...
uint32 AnnotatedMap::computeClearance ( const Vec2i &pos, Field field ) {
	assert ( cMap->isInside ( pos ) );
	uint32 clearance = 0;
	Tile *container = cMap->getTile ( cMap->toTileCoords ( pos ) );
	switch ( field ) {
		case FieldWalkable:
			if ( container->isFree () ) // surface cell is walkable?
				while ( canClear ( pos, clearance + 1, FieldWalkable ) )
					clearance++;
			return clearance;
		case FieldAir:
			return maxClearanceValue;
		case FieldAnyWater:
			if ( container->isFree () )
				while ( canClear ( pos, clearance + 1, FieldAnyWater ) )
					clearance++;
			return clearance;
		case FieldDeepWater:
			if ( container->isFree () )
				while ( canClear ( pos, clearance + 1, FieldDeepWater ) )
					clearance++;
			return clearance;
		case FieldAmphibious:
			if ( container->isFree () )
				while ( canClear ( pos, clearance + 1, FieldAmphibious ) )
					clearance++;
			return clearance;
		default:
			throw runtime_error ( "Illegal Field passed to AnnotatedMap::computeClearance()" );
			return 0;
	}
}

bool AnnotatedMap::canClear ( const Vec2i &pos, int clear, Field field ) {
	assert ( cMap->isInside ( pos ) );
	if ( clear > maxClearanceValue ) return false;

	// on map ?
	if ( pos.x + clear >= cMap->getW() - 2 || pos.y + clear >= cMap->getH() - 2 ) 
		return false;

	for ( int i=pos.y; i < pos.y + clear; ++i ) {
		for ( int j=pos.x; j < pos.x + clear; ++j ) {
			Vec2i checkPos ( j, i );
			Cell *cell = cMap->getCell ( checkPos );
			Tile *sc = cMap->getTile ( cMap->toTileCoords ( checkPos ) );
			switch ( field ) {
				case FieldWalkable:
					if ( ( cell->getUnit(ZoneSurface) && !cell->getUnit(ZoneSurface)->isMobile() )
					||   !sc->isFree () || cell->isDeepSubmerged () ) 
						return false;
					break;
				case FieldAnyWater:
					if ( ( cell->getUnit(ZoneSurface) && !cell->getUnit(ZoneSurface)->isMobile() )
					||   !sc->isFree () || !cell->isSubmerged () )
						return false;
					break;
				case FieldDeepWater:
					if ( ( cell->getUnit(ZoneSurface) && !cell->getUnit(ZoneSurface)->isMobile() )
					||   !sc->isFree () || !cell->isDeepSubmerged () ) 
						return false;
					break;
				case FieldAmphibious:
					if ( ( cell->getUnit(ZoneSurface) && !cell->getUnit(ZoneSurface)->isMobile() )
					||   !sc->isFree () )
						return false;
					break;
				default:
					throw runtime_error ( "Illegal Field passed to AnnotatedMap::canClear()" );
			}
		}// end for
	}// end for
	return true;
}


bool AnnotatedMap::canOccupy ( const Vec2i &pos, int size, Field field ) const {
	assert ( cMap->isInside ( pos ) );
	return metrics[pos].get ( field ) >= size ? true : false;
}

void AnnotatedMap::annotateLocal ( const Vec2i &pos, const int size, const Field field ) {
	assert ( cMap->isInside ( pos ) );
	assert ( cMap->isInside ( pos.x + size - 1, pos.y + size - 1 ) );
	const Vec2i *offsets1 = NULL, *offsets2 = NULL;
	int numOffsets1, numOffsets2;
	if ( size == 1 ) {
		offsets1 = OffsetsSize1Dist1;
		numOffsets1 = numOffsetsSize1Dist1;
		offsets2 = OffsetsSize1Dist2;
		numOffsets2 = numOffsetsSize1Dist2;
	}
	else if ( size == 2 ) {
		offsets1 = OffsetsSize2Dist1;
		numOffsets1 = numOffsetsSize2Dist1;
		offsets2 = OffsetsSize2Dist2;
		numOffsets2 = numOffsetsSize2Dist2;
	}
	else
		assert ( false );

	localAnnotateCells ( pos, size, field, offsets1, numOffsets1 );
	localAnnotateCells ( pos, size, field, offsets2, numOffsets2 );
}


void AnnotatedMap::localAnnotateCells ( const Vec2i &pos, const int size, const Field field, 
												const Vec2i *offsets, const int numOffsets ) {
	Zone zone = field == FieldAir ? ZoneAir : ZoneSurface;
	for ( int i = 0; i < numOffsets; ++i ) {
		Vec2i aPos = pos + offsets[i];
		if ( cMap->isInside (aPos) && metrics[aPos].get (field) 
		&&   ! cMap->getCell (aPos)->isFree (zone) ) {  
			// store the 'true' metric, if not already stored
			if ( localAnnt.find ( aPos ) == localAnnt.end () )
				localAnnt[aPos] = metrics[aPos].get ( field );
			metrics[aPos].set ( field, 0 );
			if ( size == 2 ) {  // we don't bother with this for size 1 units, the metrics may be
				// 'wrong', but they are 'right enough' (1, 2 & 3 are all the same to a size 1 unit).
				int shell = 1;
				for ( int i=0; i < 3; ++i ) {
					Vec2i check = aPos + AboveLeftDist1[i];
					if ( metrics[check].get ( field ) > shell ) {
						// check if we've allready annotated this cell...
						if ( localAnnt.find ( check ) == localAnnt.end () )
							localAnnt[check] = metrics[check].get ( field );
						metrics[check].set ( field, shell );
					}
				}
				shell ++;
				for ( int i=0; i < 5; ++i )
				{
					Vec2i check = aPos + AboveLeftDist2[i];
					if ( metrics[check].get ( field ) > shell ) {
						// check if we've allready annotated this cell...
						if ( localAnnt.find ( check ) == localAnnt.end () )
							localAnnt[check] = metrics[check].get ( field );
						metrics[check].set ( field, shell );
					}
				}
			} // end if ( size == 2 )
		} // end if [ on map && metric > 0 && cell has unit ]
	} // end for
}

void AnnotatedMap::clearLocalAnnotations ( Field field ) {
	for ( map<Vec2i,uint32>::iterator it = localAnnt.begin (); it != localAnnt.end (); ++ it ) {
		assert ( it->second <= maxClearanceValue );
		assert ( cMap->isInside ( it->first ) );
		metrics[it->first].set ( field, it->second );
	}
	localAnnt.clear ();
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
