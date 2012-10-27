// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_MANDATE_AI_SEARCH_SIM_H_
#define _GLEST_MANDATE_AI_SEARCH_SIM_H_

#include "forward_decs.h"

using namespace Glest::Search;
using namespace Glest::Entities;

namespace Glest { namespace Plan {
// ===============================
// 	class ExploredMap
// ===============================
STRINGY_ENUM(Direction, NORTH, NORTHWEST, WEST, NORTHEAST, SOUTHWEST, EAST, SOUTHEAST, SOUTH)

class DirectionalPosition {
private:
    Vec2i positionResult;
    Direction lastDirection;

public:
    Vec2i getPositionResult() const {return positionResult;}
    Direction getLastDirection() const {return lastDirection;}

    void setPositionResult(Vec2i newPosition) {positionResult = newPosition;}
    void setLastDirection(Direction newDirection) {lastDirection = newDirection;}
};

// ===============================
// 	class ExploredMap
// ===============================
class ExploredMap {
private:
    Direction previousDirection;
public:
    Direction getPreviousDirection() const {return previousDirection;}
    DirectionalPosition findNearestUnexploredTile(Vec2i unitPos, Faction *faction, Direction direction);

};

}}

#endif
