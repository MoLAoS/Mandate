// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V2, see source/licence.txt
// ==============================================================

#ifndef _GAME_SEARCH_CONSTANTS_
#define _GAME_SEARCH_CONSTANTS_

#include "util.h"
using Shared::Util::EnumNames;

namespace Glest { namespace Search {

/** result set for path finding 
  * <ul><li><b>ARRIVED</b> Arrived at destination (or as close as unit can get to target)</li>
  *		<li><b>MOVING</b> On the way to destination</li>
  *		<li><b>BLOCKED</b> path is blocked</li></ul>
  */
REGULAR_ENUM( TravelState, 
	ARRIVED,
	MOVING,
	BLOCKED,
	IMPOSSIBLE
);

/** result set for A*
  * <ul><li><b>FAILURE</b> No path exists</li>
  *		<li><b>COMPLETE</b> complete path found</li>
  *		<li><b>NODE_LIMIT</b> node limit reached, partial path available</li>
  *		<li><b>TIME_LIMIT</b> search ongoing (time limit reached)</li></ul>
  */
REGULAR_ENUM( AStarResult, 
	FAILURE,
	COMPLETE,
	NODE_LIMIT,
	TIME_LIMIT
);

/** result set for HAA*
  * <ul><li><b>FAILURE</b> No path exists</li>
  *		<li><b>COMPLETE</b> path found</li>
  *		<li><b>START_TRAP</b> path found, but transitions in start cluster are blocked</li>
  *		<li><b>GOAL_TRAP</b> path found, but transitions in destination cluster are blocked</li></ul>
  */
REGULAR_ENUM( HAAStarResult,
	FAILURE,
	COMPLETE,
	START_TRAP,
	GOAL_TRAP
);

/** Specifies a 'space' to search 
  * <ul><li><b>CELLMAP</b> search on cell map</li>
  *		<li><b>TILEMAP</b> search on tile map</li></ul>
  */
REGULAR_ENUM( SearchSpace,
	CELLMAP,
	TILEMAP
);

/** The cardinal and ordinal directions enumerated for convenience */
REGULAR_ENUM( OrdinalDir,
	NORTH,
	NORTH_EAST,
	EAST,
	SOUTH_EAST,
	SOUTH,
	SOUTH_WEST,
	WEST,
	NORTH_WEST
);

REGULAR_ENUM( CardinalDir, 
	NORTH,
	EAST,
	SOUTH,
	WEST
);

}} // namespace Glest::Search

#endif
