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
// 	class ExplorationMap
// ===============================
DirectionalPosition ExploredMap::findNearestUnexploredTile(Vec2i unitPos, Faction *faction, Direction direction) {
    DirectionalPosition returnType;
    returnType.setPositionResult(Vec2i(NULL));
    returnType.setLastDirection(direction);
    bool initialDirection = false;
    if (direction == Direction::EAST || direction == Direction::SOUTH
        || direction == Direction::WEST || direction == Direction::NORTH ) {
        initialDirection = true;
    }
    int posX = unitPos.x;
    int posY = unitPos.y;
    int realMaxX = g_world.getMap()->getW() - 5;
    int realMaxY = g_world.getMap()->getH() - 5;
    Tile *tile;
    Cell *cell;

    if (direction == Direction::EAST && initialDirection == true) {
        for (int j = 0; j < 100; ++j) {
            int xPos = posX + j;
            int yPos = posY;
            if (xPos < realMaxX && yPos < realMaxY && xPos > 4 && yPos > 4) {
                tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(xPos, yPos)));
                if (tile->isExplored(faction->getTeam())) {
                    cell = g_world.getMap()->getCell(xPos, yPos);
                    if (tile->isFree() && cell->isFree(Zone::LAND)) {
                        returnType.setPositionResult(Vec2i(xPos, yPos));
                        returnType.setLastDirection(Direction::EAST);
                    } else {
                        if (returnType.getPositionResult() != Vec2i(NULL)) {
                            return returnType;
                        }
                    }
                }
            }
        }
    }
    if (direction == Direction::SOUTH && initialDirection == true) {
        for (int j = 0; j < 100; ++j) {
            int xPos = posX;
            int yPos = posY + j;
            if (xPos < realMaxX && yPos < realMaxY && xPos > 4 && yPos > 4) {
                tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(xPos, yPos)));
                if(tile->isExplored(faction->getTeam())) {
                    cell = g_world.getMap()->getCell(xPos, yPos);
                    if (tile->isFree() && cell->isFree(Zone::LAND)) {
                        returnType.setPositionResult(Vec2i(xPos, yPos));
                        returnType.setLastDirection(Direction::SOUTH);
                    } else {
                        if (returnType.getPositionResult() != Vec2i(NULL)) {
                            return returnType;
                        }
                    }

                }
            }
        }
    }
    if (direction == Direction::WEST && initialDirection == true) {
        for (int j = 0; j < 100; ++j) {
            int xPos = posX - j;
            int yPos = posY;
            if (xPos < realMaxX && yPos < realMaxY && xPos > 4 && yPos > 4) {
                tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(xPos, yPos)));
                if (tile->isExplored(faction->getTeam())) {
                    cell = g_world.getMap()->getCell(xPos, yPos);
                    if (tile->isFree() && cell->isFree(Zone::LAND)) {
                        returnType.setPositionResult(Vec2i(xPos, yPos));
                        returnType.setLastDirection(Direction::WEST);
                    } else {
                        if (returnType.getPositionResult() != Vec2i(NULL)) {
                            return returnType;
                        }
                    }
                }
            }
        }
    }
    if (direction == Direction::NORTH && initialDirection == true) {
        for (int j = 0; j < 100; ++j) {
            int xPos = posX;
            int yPos = posY - j;
            if (xPos < realMaxX && yPos < realMaxY && xPos > 4 && yPos > 4) {
                tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(xPos, yPos)));
                if(tile->isExplored(faction->getTeam())) {
                    cell = g_world.getMap()->getCell(xPos, yPos);
                    if (tile->isFree() && cell->isFree(Zone::LAND)) {
                        returnType.setPositionResult(Vec2i(xPos, yPos));
                        returnType.setLastDirection(Direction::NORTH);
                    } else {
                        if (returnType.getPositionResult() != Vec2i(NULL)) {
                            return returnType;
                        }
                    }
                }
            }
        }
    }
        for (int j = 0; j < 100; ++j) {
            int xPos = posX - j;
            int yPos = posY;
            if (xPos < realMaxX && yPos < realMaxY && xPos > 4 && yPos > 4) {
                tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(xPos, yPos)));
                if (tile->isExplored(faction->getTeam())) {
                    cell = g_world.getMap()->getCell(xPos, yPos);
                    if (tile->isFree() && cell->isFree(Zone::LAND)) {
                        returnType.setPositionResult(Vec2i(xPos, yPos));
                        returnType.setLastDirection(Direction::WEST);
                    } else {
                        if (returnType.getPositionResult() != Vec2i(NULL)) {
                            return returnType;
                        }
                    }
                }
            }
        }
        for (int j = 0; j < 100; ++j) {
            int xPos = posX + j;
            int yPos = posY;
            if (xPos < realMaxX && yPos < realMaxY && xPos > 4 && yPos > 4) {
                tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(xPos, yPos)));
                if (tile->isExplored(faction->getTeam())) {
                    cell = g_world.getMap()->getCell(xPos, yPos);
                    if (tile->isFree() && cell->isFree(Zone::LAND)) {
                        returnType.setPositionResult(Vec2i(xPos, yPos));
                        returnType.setLastDirection(Direction::EAST);
                    } else {
                        if (returnType.getPositionResult() != Vec2i(NULL)) {
                            return returnType;
                        }
                    }
                }
            }
        }
        for (int j = 0; j < 100; ++j) {
            int xPos = posX;
            int yPos = posY + j;
            if (xPos < realMaxX && yPos < realMaxY && xPos > 4 && yPos > 4) {
                tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(xPos, yPos)));
                if(tile->isExplored(faction->getTeam())) {
                    cell = g_world.getMap()->getCell(xPos, yPos);
                    if (tile->isFree() && cell->isFree(Zone::LAND)) {
                        returnType.setPositionResult(Vec2i(xPos, yPos));
                        returnType.setLastDirection(Direction::SOUTH);
                    } else {
                        if (returnType.getPositionResult() != Vec2i(NULL)) {
                            return returnType;
                        }
                    }

                }
            }
        }
        for (int j = 0; j < 100; ++j) {
            int xPos = posX;
            int yPos = posY - j;
            if (xPos < realMaxX && yPos < realMaxY && xPos > 4 && yPos > 4) {
                tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(xPos, yPos)));
                if(tile->isExplored(faction->getTeam())) {
                    cell = g_world.getMap()->getCell(xPos, yPos);
                    if (tile->isFree() && cell->isFree(Zone::LAND)) {
                        returnType.setPositionResult(Vec2i(xPos, yPos));
                        returnType.setLastDirection(Direction::NORTH);
                    } else {
                        if (returnType.getPositionResult() != Vec2i(NULL)) {
                            return returnType;
                        }
                    }
                }
            }
        }

        /*if (i % 2 == 0 && g_world.getMap()->isInside(Vec2i(posX + (i + 1), posY + (i + 1)))) {
            tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(posX + (i + 1), posY + (i + 1))));
            if(tile->isExplored(faction->getTeam()) && tile->isFree()) {
                returnPos = Vec2i(posX + (i + 1), posY + (i + 1));
                break;
            }
        }
        if (i % 2 == 0 && g_world.getMap()->isInside(Vec2i(posX + (i + 1), posY - (i + 1)))) {
            tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(posX + (i + 1), posY - (i + 1))));
            if(tile->isExplored(faction->getTeam()) && tile->isFree()) {
                returnPos = Vec2i(posX + (i + 1), posY - (i + 1));
                break;
            }
        }
        if (i % 2 == 0 && g_world.getMap()->isInside(Vec2i(posX - (i + 1), posY + (i + 1)))) {
            tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(posX - (i + 1), posY + (i + 1))));
            if(tile->isExplored(faction->getTeam()) && tile->isFree()) {
                returnPos = Vec2i(posX - (i + 1), posY + (i + 1));
                break;
            }
        }
        if (i % 2 == 0 && g_world.getMap()->isInside(Vec2i(posX - (i + 1), posY - (i + 1)))) {
            tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(posX - (i + 1), posY - (i + 1))));
            if(tile->isExplored(faction->getTeam()) && tile->isFree()) {
                returnPos = Vec2i(posX - (i + 1), posY - (i + 1));
                break;
            }
        }*/
    return returnType;
}

}}
