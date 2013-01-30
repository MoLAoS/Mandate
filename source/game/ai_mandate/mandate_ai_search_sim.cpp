// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "mandate_ai_search_sim.h"

#include "world.h"

using namespace Glest::Search;

namespace Glest { namespace Plan {
// ===============================
// 	class ExploredMap
// ===============================
Vec2i ExploredMap::findNearestUnexploredTile(Vec2i unitPos, Faction *faction, UnitDirection direction) {
    UnitDirection newDirection = direction;
    Vec2i returnType = Vec2i(0,0);
    int posX = unitPos.x;
    int posY = unitPos.y;
    int realMaxX = g_world.getMap()->getW() - 5;
    int realMaxY = g_world.getMap()->getH() - 5;
    Tile *tile;
    Cell *cell;
    int x = 0;
    int y = 0;
    if (newDirection == UnitDirection::NORTH || newDirection == UnitDirection::NORTHEAST || newDirection == UnitDirection::NORTHWEST) {
        if (posY == 5) {
            newDirection = UnitDirection::SOUTH;
        }
    }
    if (newDirection == UnitDirection::SOUTH || newDirection == UnitDirection::SOUTHEAST || newDirection == UnitDirection::SOUTHWEST) {
        if (posY == 122) {
            newDirection = UnitDirection::NORTH;
        }
    }
    if (newDirection == UnitDirection::EAST || newDirection == UnitDirection::NORTHEAST || newDirection == UnitDirection::SOUTHEAST) {
        if (posX == 122) {
            newDirection = UnitDirection::WEST;
        }
    }
    if (newDirection == UnitDirection::WEST || newDirection == UnitDirection::NORTHWEST || newDirection == UnitDirection::SOUTHWEST) {
        if (posX == 5) {
            newDirection = UnitDirection::EAST;
        }
    }
    switch (newDirection) {
        case UnitDirection::NORTH: x = 0, y = -1; break;
        case UnitDirection::SOUTH: x = 0, y = 1; break;
        case UnitDirection::EAST: x = 1, y = 0; break;
        case UnitDirection::WEST: x = -1, y = 0; break;
        case UnitDirection::NORTHWEST: x = -1, y = -1; break;
        case UnitDirection::NORTHEAST: x = 1, y = -1; break;
        case UnitDirection::SOUTHWEST: x = -1, y = 1; break;
        case UnitDirection::SOUTHEAST: x = 1, y = 1; break;
    }
    for (int j = 0; j < 5; ++j) {
        int xPos = posX + j * x;
        int yPos = posY + j * y;
        if (xPos < realMaxX && yPos < realMaxY && xPos > 4 && yPos > 4) {
            tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(xPos, yPos)));
            if(tile->isExplored(faction->getTeam())) {
                cell = g_world.getMap()->getCell(xPos, yPos);
                if (tile->isFree() && cell->isFree(Zone::LAND)) {
                    returnType = Vec2i(xPos, yPos);
                } else {
                    if (returnType != Vec2i(0,0)) {
                        return returnType;
                    }
                }
            }
        }
    }
    return returnType;
}

}}
