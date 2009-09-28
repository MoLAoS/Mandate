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
// File: annotated_map.h
//
// Annotated Map, for use in pathfinding.
//

#ifndef _GLEST_GAME_ANNOTATED_MAP_H_
#define _GLEST_GAME_ANNOTATED_MAP_H_

#include "vec.h"
#include "unit_stats_base.h"
/*
#include <vector>
#include <list>
#include <set>
*/

namespace Glest { namespace Game {

class Map;

namespace Search {

const Vec2i AboveLeftDist1[3] =  {
	Vec2i ( -1,  0 ),
	Vec2i ( -1, -1 ),
	Vec2i (  0, -1 )
};

const Vec2i AboveLeftDist2[5] = {
	Vec2i ( -2,  0 ),
	Vec2i ( -2, -1 ),
	Vec2i ( -2, -2 ),
	Vec2i ( -1, -2 ),
	Vec2i (  0, -2 )
};

// Adding a new field?
// add the new Field (enum defined in units_stats_base.h)
// add the 'xml' name to the Fields::names array (in units_stats_base.cpp)
// add a comment below, claiming the first unused metric field.
// add a case to the switches in CellMetrics::get() & CellMetrics::set()
// add code to PathFinder::computeClearance(), PathFinder::computeClearances() 
// and PathFinder::canClear() (if not handled fully in computeClearance[s]).
// finaly, add a little bit of code to Map::fieldsCompatable().

// Allows for a maximum moveable unit size of 3. we can 
// (with modifications) path groups in formation using this, 
// maybe this is not enough.. perhaps give some fields more resolution? 
// Will Glest even do size > 2 moveable units without serious movement related issues ???
struct CellMetrics {
	CellMetrics () { memset ( this, 0, sizeof(*this) ); }
	// can't get a reference to a bit field, so we can't overload 
	// the [] operator, and we have to get by with these...
	inline uint32 get ( const Field );
	inline void   set ( const Field, uint32 val );

	bool isNearResource ( const Vec2i &pos );
	bool isNearStore ( const Vec2i &pos );

private:
	uint32 field0 : 2; // In Use: FieldWalkable = land + shallow water 
	uint32 field1 : 2; // In Use: FieldAir = air
	uint32 field2 : 2; // In Use: FieldAnyWater = shallow + deep water
	uint32 field3 : 2; // In Use: FieldDeepWater = deep water
	uint32 field4 : 2; // In Use: FieldAmphibious = land + shallow + deep water 
	uint32 field5 : 2; // Unused: ?
	uint32 field6 : 2; // Unused: ?
	uint32 field7 : 2; // Unused: ?
	uint32 field8 : 2; // Unused: ?
	uint32 field9 : 2; // Unused: ?
	uint32 fielda : 2; // Unused: ?
	uint32 fieldb : 2; // Unused: ?
	uint32 visTeam0 : 1; // map visibility... not used yet.
	uint32 visTeam1 : 1; //  will be used to remove calls to Tile::isExplored()
	uint32 visTeam2 : 1; //  from search algorithms, for better cache performance.
	uint32 visTeam3 : 1; // 
	uint32 adjResrc1 : 1; // to be used for semi co-operative resource gathering
	uint32 adjResrc2 : 1; // to be used for semi co-operative resource gathering
	uint32 adjStore1 : 1; // to be used for semi co-operative resource gathering
	uint32 adjStore2 : 1; // to be used for semi co-operative resource gathering
};

class MetricMap {
private:
	CellMetrics *metrics;
	int stride;

public:
	MetricMap () { stride = 0; metrics = NULL; }
	virtual ~MetricMap () { delete [] metrics; }

	void init ( int w, int h ) { 
		assert (w>0&&h>0); stride = w; metrics = new CellMetrics[w*h]; 
	}
	CellMetrics& operator [] ( const Vec2i &pos ) const { 
		return metrics[pos.y*stride+pos.x]; 
	}
};

class AnnotatedMap {
public:
	AnnotatedMap ( Map *map );
	virtual ~AnnotatedMap ();

	// Initialise the Map Metrics
	void initMapMetrics ( Map *map );

	// Start a 'cascading update' of the Map Metrics from a position and size
	void updateMapMetrics ( const Vec2i &pos, const int size, bool adding, Field field ); 

	// Interface to the clearance metrics, can a unit of size occupy a cell(s) ?
	bool canOccupy ( const Vec2i &pos, int size, Field field ) const;

	static const int maxClearanceValue;

	// Temporarily annotate the map for nearby units
	void annotateLocal ( const Vec2i &pos, const int size, const Field field );

	// Clear temporary annotations
	void clearLocalAnnotations ( Field field );

#  ifdef _GAE_DEBUG_EDITION_
	list<std::pair<Vec2i,uint32>>* getLocalAnnotations ();
#  endif

private:
	// for initMetrics () and updateMapMetrics ()
	CellMetrics computeClearances ( const Vec2i & );
	uint32 computeClearance ( const Vec2i &, Field );
	bool canClear ( const Vec2i &pos, int clear, Field field );

	// for annotateLocal ()
	void localAnnotateCells ( const Vec2i &pos, const int size, const Field field, 
		const Vec2i *offsets, const int numOffsets );

	int metricHeight;
	std::map<Vec2i,uint32> localAnnt;
	Map *cMap;
#ifdef _GAE_DEBUG_EDITION_
public:
#endif
	MetricMap metrics;
};

}}}

#endif
