// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Jaagup Repän <jrepan@gmail.com>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//				  2009 James McCulloch <silnarm@gmail.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "map.h"
#include "earthquake_type.h"

#include <cassert>

#include "world.h"
#include "tileset.h"
#include "unit.h"
#include "resource.h"
#include "logger.h"
#include "tech_tree.h"
#include "config.h"
#include "socket.h"
#include "selection.h"
#include "program.h"
#include "script_manager.h"
#include "FSFactory.hpp"

#include "leak_dumper.h"
#include "fixed.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Math;

namespace Glest{ namespace Game{

using Search::Cartographer;

// =====================================================
// 	class Map
// =====================================================

// ===================== PUBLIC ========================

Map::Map() 
		: title()
		, waterLevel(0.f)
		, heightFactor(0.f)
		, avgHeight(0.f)
		, w(0)
		, h(0)
		, tileW(0)
		, tileH(0)
		, maxPlayers(0)
		, cells(NULL)
		, tiles(NULL)
		, startLocations(NULL)
		//, surfaceHeights(NULL)
		, earthquakes() {

	// If this is expanded, maintain Tile::read() and write()
	assert(Tileset::objCount < 256);

	// same as above
	assert(GameConstants::maxPlayers == 4);
}

Map::~Map() {
	Logger::getInstance().add("Cells", !Program::getInstance()->isTerminating());

	if(cells)			{delete[] cells;}
	if(tiles)			{delete[] tiles;}
	if(startLocations)	{delete[] startLocations;}
//	if (surfaceHeights)	{delete[] surfaceHeights;}
}

char encodeExplorationState(Tile *tile) {
	char result = 0;
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (tile->isExplored(i)) {
			result |= (1 << i);
		}
	}
	result += 48;
	return result;
}

void decodeExplorationState(Tile *tile, char state) {
	state -= 48;
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		tile->setExplored(i, state & (1 << i));
	}
}

void Map::saveExplorationState(XmlNode *node) const {
	XmlNode *metrics = node->addChild("metrics");
	metrics->addAttribute("width", tileW);
	metrics->addAttribute("height", tileH);
	for (int y=0; y < tileH; ++y) {
		stringstream ss;
		for (int x=0; x < tileW; ++x) {
			ss << encodeExplorationState(getTile(x, y));
		}
		node->addChild(string("row") + intToStr(y))->addAttribute("value", ss.str());
	}
}

void Map::loadExplorationState(XmlNode *node) {
	assert(node->getChild("metrics")->getIntAttribute("width") == tileW);
	assert(node->getChild("metrics")->getIntAttribute("height") == tileH);
	for (int y=0; y < tileH; ++y) {
		stringstream ss(node->getChild(string("row") + intToStr(y))->getStringAttribute("value"));
		for (int x=0; x < tileW; ++x) {
			char state;
			ss >> state;
			decodeExplorationState(getTile(x, y), state);
		}
	}
}

void Map::load(const string &path, TechTree *techTree, Tileset *tileset) {
	FileOps *f = FSFactory::getInstance()->getFileOps();

	struct MapFileHeader {
		int32 version;
		int32 maxPlayers;
		int32 width;
		int32 height;
		int32 altFactor;
		int32 waterLevel;
		int8 title[128];
		int8 author[128];
		int8 description[256];
	};

	try {
		f->openRead(path.c_str());

		//read header
		MapFileHeader header;
		
		// FIXME: Better error handling starting here: report bad map instead of partially loading,
		// etc.
		f->read(&header, sizeof(MapFileHeader), 1);

		if (next2Power(header.width) != header.width) {
			throw runtime_error("Map width is not a power of 2");
		}

		if (next2Power(header.height) != header.height) {
			throw runtime_error("Map height is not a power of 2");
		}

		heightFactor = (float)header.altFactor;
		waterLevel = static_cast<float>((header.waterLevel - 0.01f) / heightFactor);
		title = header.title;
		maxPlayers = header.maxPlayers;
		tileW = header.width;
		tileH = header.height;
		w = tileW * GameConstants::cellScale;
		h = tileH * GameConstants::cellScale;


		//start locations
		startLocations = new Vec2i[maxPlayers];
		for (int i = 0; i < maxPlayers; ++i) {
			int x, y;
			f->read(&x, sizeof(int32), 1);
			f->read(&y, sizeof(int32), 1);
			startLocations[i] = Vec2i(x, y) * GameConstants::cellScale;
		}


		// Tiles & Cells
		cells = new Cell[w * h];
		tiles = new Tile[tileW * tileH];
		//surfaceHeights = new float[tileW * tileH]; // unused ???

		//float *surfaceHeight = surfaceHeights;

		const int &scale = GameConstants::mapScale;
		//read heightmap
		for (int y = 0; y < tileH; ++y) {
			for (int x = 0; x < tileW; ++x) {
				float32 alt;
				f->read(&alt, sizeof(float32), 1);
				Tile *sc = getTile(x, y);
				sc->setVertex(Vec3f(float(x * scale), alt / heightFactor, float(y * scale)));
			}
		}

		//read surfaces
		for (int y = 0; y < tileH; ++y) {
			for (int x = 0; x < tileW; ++x) {
				int8 surf;
				f->read(&surf, sizeof(int8), 1);
				getTile(x, y)->setTileType(surf - 1);
			}
		}

		//read objects and resources
		for (int y = 0; y < h; y += GameConstants::cellScale) {
			for (int x = 0; x < w; x += GameConstants::cellScale) {

				int8 objNumber;
				f->read(&objNumber, sizeof(int8), 1);
				Tile *sc = getTile(toTileCoords(Vec2i(x, y)));
				if (objNumber == 0) {
					sc->setObject(NULL);
				} else if (objNumber <= Tileset::objCount) {
					Object *o = new Object(tileset->getObjectType(objNumber - 1), sc->getVertex());
					sc->setObject(o);
					for (int i = 0; i < techTree->getResourceTypeCount(); ++i) {
						const ResourceType *rt = techTree->getResourceType(i);
						if (rt->getClass() == ResourceClass::TILESET && rt->getTilesetObject() == objNumber) {
							o->setResource(rt, Vec2i(x, y));
						}
					}
				} else {
					const ResourceType *rt = techTree->getTechResourceType(objNumber - Tileset::objCount) ;
					Object *o = new Object(NULL, sc->getVertex());
					o->setResource(rt, Vec2i(x, y));
					sc->setObject(o);
				}
			}
		}
		delete f;
	} catch (const exception &e) {
		delete f;
		delete [] cells;
		delete [] tiles;
		throw MapException(path, "Error loading map: " + path + "\n" + e.what());
	}
}

void Map::doChecksum(Checksum &checksum) {
	checksum.add(title);
	checksum.add<float>(waterLevel);
	checksum.add<float>(heightFactor);
	checksum.add<float>(avgHeight);
	checksum.add<int>(w);
	checksum.add<int>(h);
	checksum.add<int>(tileW);
	checksum.add<int>(tileH);
	checksum.add<int>(maxPlayers);
	for (int i=0; i < tileW * tileH; ++i) {
		checksum.add<Vec3f>(tiles[i].getVertex());
	}
	for (int i=0; i < maxPlayers; ++i) {
		checksum.add<Vec2i>(startLocations[i]);
	}
}

void Map::init() {
	Logger::getInstance().add("Heightmap computations", true);
	smoothSurface();
	computeNormals();
	computeInterpolatedHeights();
	computeNearSubmerged();
	computeCellColors();
	setCellTypes();
	calcAvgAltitude();
}

void Map::setCellTypes () {
	for ( int y = 0; y < getH(); ++y ) {
		for ( int x = 0; x < getW(); ++x ) {
			Cell *cell = getCell ( x, y );
			if ( cell->getHeight () < waterLevel - ( 1.5f / heightFactor ) )
				cell->setType ( SurfaceType::DEEP_WATER );
			else if ( cell->getHeight () < waterLevel )
				cell->setType ( SurfaceType::FORDABLE );
		}
	}
}
/*
void Map::setCellType ( Vec2i pos )
{
   Cell *cell = getCell ( pos );
   if ( cell->getHeight () < waterLevel - ( 1.5f / heightFactor ) )
      cell->setType ( SurfaceType::DEEP_WATER );
   else if ( cell->getHeight () < waterLevel )
      cell->setType ( SurfaceType::FORDABLE );
   else
      cell->setType ( SurfaceType::LAND );
}
*/
void Map::calcAvgAltitude() {
	double sum = 0.0;
	for (int y=0; y < getTileH(); ++y) {
		for (int x=0; x < getTileW(); ++x) {
			sum += getTile(x,y)->getHeight();
		}
	}
	avgHeight = (float)(sum / (getTileH() * getTileW()));
	//cout << "average map height = " << avgHeight << endl;
}

// ==================== is ====================

/**
 * @returns if there is a resource next to a unit, in "resourcePos" is stored the relative position
 * of the resource
 */
bool Map::isResourceNear(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos) const {
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			Vec2i cur = Vec2i(x, y) + pos;
			if (isInside(cur)) {
				Resource *r = getTile(toTileCoords(cur))->getResource();
				if (r && r->getType() == rt) {
					resourcePos = cur;
					return true;
				}
			}
		}
	}
	return false;
}


// ==================== free cells ====================
bool Map::fieldsCompatible(Cell *cell, Field mf) const {
	if (mf == Field::AIR || mf == Field::AMPHIBIOUS
	|| (mf == Field::LAND && ! cell->isDeepSubmerged()) 
	|| (mf == Field::ANY_WATER && cell->isSubmerged())
	|| (mf == Field::DEEP_WATER && cell->isDeepSubmerged())) {
		return true;
	}
	return false;
}

bool Map::isFreeCell(const Vec2i &pos, Field field) const {
	if ( !isInside(pos) || !getCell(pos)->isFree(field == Field::AIR ? Zone::AIR : Zone::LAND) )
		return false;
	if ( field != Field::AIR && !getTile(toTileCoords(pos))->isFree() )
		return false;
	// leave ^^^ that
	// replace all the rest with lookups into the map metrics...
	//return masterMap->canOccupy ( pos, 1, field );
	// placeUnit() needs to use this... and the metrics aren't constructed at that point...
	if ( field == Field::LAND && getCell(pos)->isDeepSubmerged() )
		return false;
	if ( field == Field::LAND && (pos.x > getW() - 4 || pos.y > getH() - 4) )
		return false;
	if ( field == Field::ANY_WATER && !getCell(pos)->isSubmerged() )
		return false;
	if ( field == Field::DEEP_WATER && !getCell(pos)->isDeepSubmerged() )
		return false;
	return true;
}

bool Map::isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const {
	if ( isInside(pos) ) {
		Cell *c = getCell(pos);
		if ( c->getUnit(unit->getCurrField()) == unit 
		&&   fieldsCompatible( c, field) ) 
			return true;
		else 
			return isFreeCell(pos, field);
	}
	return false;
}
// Is the Cell at 'pos' (for the given 'field') either free or does it contain any of the units in 'units'
bool Map::isFreeCellOrHaveUnits(const Vec2i &pos, Field field, const Selection::UnitContainer &units) const {
	if(isInside(pos)) {
		Unit *containedUnit = getCell(pos)->getUnit(field);
		if ( containedUnit && fieldsCompatible(getCell(pos), field) ) {
			Selection::UnitContainer::const_iterator i;
			for(i = units.begin(); i != units.end(); ++i) 
				if(containedUnit == *i)
					return true;
		}
		return isFreeCell(pos, field);
	}
	return false;
}

bool Map::canOccupy(const Vec2i &pos, Field field, const UnitType *ut) {
	if (ut->hasCellMap()) {
		for (int y=0; y < ut->getSize(); ++y) {
			for (int x=0; x < ut->getSize(); ++x) {
				if (ut->getCellMapCell(x, y)) {
					if (!isFreeCell(pos + Vec2i(x, y), field)) {
						return false;
					}
				}
			}
		}
		return true;
	} else {
		return areFreeCells(pos, ut->getSize(), field);
	}
}

// if the Cell is visible, true if free, false if occupied
// if the Cell is explored, true if Tile is free (no object), false otherwise
// if the Cell is unexplored, true
bool Map::isAproxFreeCell(const Vec2i &pos, Field field, int teamIndex) const {
	if (isInside(pos)) { // on map ?
		const Tile *sc = getTile(toTileCoords(pos));
		if (sc->isVisible(teamIndex)) {
			return sc->isFree() && isFreeCell(pos, field);
		} else if (sc->isExplored(teamIndex)) {
			return field == Field::LAND ? sc->isFree() && !getCell(pos)->isDeepSubmerged() : true;
		} else {
			return true;
		}
	}
	return false;
}

bool Map::areFreeCells(const Vec2i & pos, int size, Field field) const {
	for ( int i=pos.x; i<pos.x+size; ++i ) {
		for ( int j=pos.y; j<pos.y+size; ++j ) {
			if ( !isFreeCell(Vec2i(i,j), field) ) {
				return false;
			}
		}
	}
	return true;
}

bool Map::areFreeCells(const Vec2i &pos, int size, char *fieldMap) const {
	for ( int i = 0; i < size; ++i) {
		for ( int j = 0; j < size; ++j) {
			Field field;
			switch ( fieldMap[j*size+i] ) {
				case 'l': field = Field::LAND; break;
				case 'a': field = Field::AMPHIBIOUS; break;
				case 'w': field = Field::DEEP_WATER; break;
				case 'r': field = Field::LAND; break;
				default: throw runtime_error ( "Bad value in fieldMap" );
			}
			if ( !isFreeCell(Vec2i(pos.x+i, pos.y+j), field) ) {
				return false;
			}
		}
	}
	return true;
}

bool Map::areFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field, const Unit *unit) const {
	for(int i=pos.x; i<pos.x+size; ++i){
		for(int j=pos.y; j<pos.y+size; ++j){
			if(!isFreeCellOrHasUnit(Vec2i(i,j), field, unit)){
				return false;
			}
		}
	}
    return true;
}

bool Map::areFreeCellsOrHaveUnits(const Vec2i &pos, int size, Field field, const Selection::UnitContainer &units) const {
	for(int i=pos.x; i<pos.x+size; ++i){
		for(int j=pos.y; j<pos.y+size; ++j){
			if(!isFreeCellOrHaveUnits(Vec2i(i,j), field, units)){
				return false;
			}
		}
	}
    return true;
}

bool Map::areAproxFreeCells(const Vec2i &pos, int size, Field field, int teamIndex) const {
	for (int i = pos.x; i < pos.x + size; ++i) {
		for (int j = pos.y; j < pos.y + size; ++j) {
			if (!isAproxFreeCell(Vec2i(i, j), field, teamIndex)) {
				return false;
			}
		}
	}
	return true;
}

void Map::getOccupants(vector<Unit *> &results, const Vec2i &pos, int size, Zone field) const {
	results.clear();
	for (int i = pos.x; i < pos.x + size; ++i) {
		for (int j = pos.y; j < pos.y + size; ++j) {
			Vec2i currPos(i, j);
			Unit *occupant;
			if (isInside(currPos) && (occupant = getCell(currPos)->getUnit(field))) {
				vector<Unit *>::const_iterator i = find(results.begin(), results.end(), occupant);
				if (i == results.end()) {
					results.push_back(occupant);
				}
			}
		}
	}
}
/*
bool Map::isFieldMapCompatible ( const Vec2i &pos, const UnitType *building ) const {
	for (int i=0; i < building->getSize(); ++i) {
		for (int j=0; j < building->getSize(); ++j) {
			Field field;
			switch ( building->getFieldMapCell(i, j)) {
				case 'l': field = Field::LAND; break;
				case 'a': field = Field::AMPHIBIOUS; break;
				case 'w': field = Field::DEEP_WATER; break;
				default: throw runtime_error("Illegal value in FieldMap.");
			}
			if (!fieldsCompatible(getCell(pos), field)) {
				return false;
			}
		}
	}
	return true;
}
*/
// ==================== location calculations ====================

//Rect2(pos.x, pos.y, pos.x + size, pos.y + size)
/**
 * Find the nearest position from target to orig that is at least minRange from
 * the target, but no greater than maxRange.  If target is less than minRange,
 * the nearest location at least minRange is returned.  If orig and target are the
 * same and minRange is > 0, the return value will contain an undefined position.
 */
Vec2i Map::getNearestPos(const Vec2i &start, const Vec2i &target, int minRange, int maxRange, int targetSize) {
	assert(minRange >= 0);
	assert(minRange <= maxRange);
	assert(targetSize > 0);
	fixedVec2 ftarget;

	if(start == target) {
		return start;
	}

	if(targetSize > 1) {
		// use a circular distance around the parameter
		int offset = targetSize - 1;

		if (start.x > target.x + offset) {
			ftarget.x = target.x + offset;
		} else if(start.x > target.x) {
			ftarget.x = start.x;
		} else {
			ftarget.x = target.x;
		}

		if (start.y > target.y + offset) {
			ftarget.y = target.y + offset;
		} else if(start.y > target.y) {
			ftarget.y = start.y;
		} else {
			ftarget.y = target.y;
		}
	} else {
		ftarget.x = target.x;
		ftarget.y = target.y;
	}

	fixedVec2 fstart(start.x, start.y);
	//Vec2f ftarget(target.x, target.y);
	fixed len = fstart.dist(ftarget);

	if(len >= minRange && len <= maxRange) {
		return start;
	} else {
		// make sure we don't divide by zero, that's how this whole universe
		// thing got started in the first place (fyi, it wasn't a big bang).
		assert(minRange == 0 || start.x != target.x || start.y != target.y);
		
		fixed t = (len <= minRange ? minRange : maxRange) / len;
		fixedVec2 res(
			ftarget.x + (ftarget.x + fstart.x) * t,
			ftarget.y + (ftarget.y + fstart.y) * t);

 		return Vec2i(res.x.round(), res.y.round());
	}
}

bool Map::getNearestFreePos(Vec2i &result, const Unit *unit, const Vec2i &target, int minRange, int maxRange, int targetSize) const {
	Vec2i start = unit->getPos();
	int size = unit->getSize();
	Field field = unit->getCurrField();//==Field::AIR?Zone::AIR:Zone::LAND;
	Vec2i candidate;

	// first try quick and dirty approach (should hit most of the time)
	candidate = getNearestPos(start, target, minRange, maxRange, targetSize);
	if(areFreeCellsOrHasUnit(candidate, size, field, unit)) {
		result = candidate;
		return true;
	}

	// if not, take the long approach.
	// TODO: this code can be optimized by starting at the nearest position looking for a free cell
	// and moving away from the starting point.
	int sideSize = targetSize + size;
	fixed minDistance = fixed::max_int();

	for(int sideSize = targetSize + size + minRange - 1; sideSize < targetSize + size + maxRange; ++sideSize) {
		int topY = target.y - size;
		int bottomY = target.y + targetSize;
		int leftX = target.x - size;
		int rightX = target.x + targetSize;

		for(int i = 0; i < sideSize; ++i) {
			findNearestFree(result, start, size, field, Vec2i(leftX + i, topY), minDistance, unit);		// above
			findNearestFree(result, start, size, field, Vec2i(rightX, topY + i), minDistance, unit);	// right
			findNearestFree(result, start, size, field, Vec2i(rightX - i, bottomY), minDistance, unit);	// below
			findNearestFree(result, start, size, field, Vec2i(leftX, bottomY - i), minDistance, unit);	// left
		}
	}

	return minDistance <= 10000;

}

/**
 * Used for adjacent position calculations -- if candidate is closer to start than minDistance, then
 * minDistance is set to that distance and the value of candidate is copied into result.
 */
inline void Map::findNearest(Vec2i &result, const Vec2i &start, const Vec2i &candidate, fixed &minDistance) {
	fixed dist = fixedDist(candidate, start);
	if(dist < minDistance) {
		minDistance = dist;
		result = candidate;
	}
}

/**
 * Same as findNearest(Vec2i &, const Vec2i &, const Vec2i &, float &), except that all cells from
 * candidate to candidate + size - 1 must be free (i.e., a the candidate's size).
 */
inline void Map::findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, fixed &minDistance) const {
	if(areFreeCells(candidate, size, field)) {
		findNearest(result, start, candidate, minDistance);
	}
}

/**
 * Same as findNearestFree(Vec2i &, const Vec2i &, int, Field, const Vec2i &, float &), except that
 * candidate may be occupied by the specified unit.
 */
inline void Map::findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, fixed &minDistance, const Unit *unit) const {
	if(areFreeCellsOrHasUnit(candidate, size, field, unit)) {
		findNearest(result, start, candidate, minDistance);
	}
}

/**
 * Finds a location for a unit of the specified size to be adjacent to target of the specified size.
 */
Vec2i Map::getNearestAdjacentPos(const Vec2i &start, int size, const Vec2i &target, Field field, int targetSize) {
	int sideSize = targetSize + size;
	
	Vec2i result(0);
	
	fixed minDistance = fixed::max_int();

	int topY = target.y - size;
	int bottomY = target.y + targetSize;
	int leftX = target.x - size;
	int rightX = target.x + targetSize;

	for(int i = 0; i < sideSize; ++i) {
		findNearest(result, start, Vec2i(leftX + i, topY), minDistance);	// above
		findNearest(result, start, Vec2i(rightX, topY + i), minDistance);	// right
		findNearest(result, start, Vec2i(rightX - i, bottomY), minDistance);// below
		findNearest(result, start, Vec2i(leftX, bottomY - i), minDistance);	// left
	}
	return result;
}

/**
 * Finds a free location for a unit of the specified size to be adjacent to target of the specified
 * size.  If unit is non-null, then the cell(s) may be occupied by that unit (in essence can return
 * the units own location if he is already adjacent).
 */
bool Map::getNearestAdjacentFreePos(Vec2i &result, const Unit *unit, const Vec2i &start, int size,
		const Vec2i &target, Field field, int targetSize) const {
	int sideSize = targetSize + size;
	fixed noneFound = fixed::max_int();
	fixed minDistance = noneFound;

	int topY = target.y - size;
	int bottomY = target.y + targetSize;
	int leftX = target.x - size;
	int rightX = target.x + targetSize;

	if(unit) {
		for(int i = 0; i < sideSize; ++i) {
			findNearestFree(result, start, size, field, Vec2i(leftX + i, topY), minDistance, unit);		// above
			findNearestFree(result, start, size, field, Vec2i(rightX, topY + i), minDistance, unit);	// right
			findNearestFree(result, start, size, field, Vec2i(rightX - i, bottomY), minDistance, unit);	// below
			findNearestFree(result, start, size, field, Vec2i(leftX, bottomY - i), minDistance, unit);	// left
		}
	} else {
		for(int i = 0; i < sideSize; ++i) {
			findNearestFree(result, start, size, field, Vec2i(leftX + i, topY), minDistance);		// above
			findNearestFree(result, start, size, field, Vec2i(rightX, topY + i), minDistance);		// right
			findNearestFree(result, start, size, field, Vec2i(rightX - i, bottomY), minDistance);	// below
			findNearestFree(result, start, size, field, Vec2i(leftX, bottomY - i), minDistance);	// left
		}
	}

	return minDistance < noneFound;
}


void panic(Vec2i currPos, Unit *unit, Unit *other) {
	const UnitType *ut = unit->getType();
	int size = ut->getSize();
	Zone zone = unit->getCurrZone();
	Zone o_zone = other->getCurrZone();

	string str = string() + "Error: " + ut->getName() + "[id:" + intToStr(unit->getId())
		+ "] putting in cell ("  + intToStr(currPos.x) + ", " + intToStr(currPos.y) + ")"
		+ (zone == Zone::LAND ? "{LAND} " : zone == Zone::AIR ? "{AIR} " : "{OTHER} ")
		+ " cell is already occupied by a " + other->getType()->getName() + "[id:"
		+ intToStr(other->getId()) + "]"
		+ (o_zone == Zone::LAND ? "{LAND} " : o_zone == Zone::AIR ? "{AIR} " : "{OTHER} ");
	Logger::getErrorLog().add(str);
	throw runtime_error ( "Ooops... see glestadv-error.log" );
}

// ==================== unit placement ====================

//put a unit into the cells
void Map::putUnitCells(Unit *unit, const Vec2i &pos){
	assert(unit);
	const UnitType *ut = unit->getType();
	int size = ut->getSize();
	Zone zone = unit->getCurrZone();

	for(int x = 0; x < size; ++x) {
		for(int y = 0; y < size; ++y) {
			Vec2i currPos = pos + Vec2i(x, y);
			assert(isInside(currPos));
			if (!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
				if (getCell(currPos)->getUnit(zone) != NULL) {
					panic(currPos, unit, getCell(currPos)->getUnit(zone));
				}
				assert(getCell(currPos)->getUnit(zone) == NULL);
				getCell(currPos)->setUnit(zone, unit);
			}
		}
	}
	unit->setPos(pos);
	ScriptManager::unitMoved(unit);
}

//removes a unit from cells
void Map::clearUnitCells(Unit *unit, const Vec2i &pos){

	assert(unit);
	const UnitType *ut = unit->getType();
	int size = ut->getSize();
	Zone zone = unit->getCurrZone ();

	for(int x = 0; x < size; ++x) {
		for(int y = 0; y < size; ++y) {
			Vec2i currPos = pos + Vec2i(x, y);
			assert(isInside(currPos));

			if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
				assert(getCell(currPos)->getUnit(zone) == unit);
				getCell(currPos)->setUnit(zone, NULL);
			}
		}
	}
}

/**
 * Evicts current inhabitants of cells and adds them to the supplied vector<Unit *>
 */
void Map::evict(Unit *unit, const Vec2i &pos, vector<Unit *> &evicted) {
	assert(unit);
	const UnitType *ut = unit->getType();
	int size = ut->getSize();
	Zone field = unit->getCurrField()==Field::AIR?Zone::AIR:Zone::LAND;

	for(int x = 0; x < size; ++x) {
		for(int y = 0; y < size; ++y) {
			Vec2i currPos(pos.x + x, pos.y + y);
			assert(isInside(currPos));

			if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
				Unit *evictedUnit = getCell(currPos)->getUnit(field);
				if(evictedUnit) {
					clearUnitCells(evictedUnit, evictedUnit->getPos());
					evicted.push_back(evictedUnit);
				}
			}
		}
	}
}


// ==================== misc ====================

//returnis if unit is next to pos
bool Map::isNextTo(const Vec2i &pos, const Unit *unit) const{
	for(int i=-1; i<=1; ++i){
		for(int j=-1; j<=1; ++j){
			if(isInside(pos.x+i, pos.y+j)) {
				if(getCell(pos.x+i, pos.y+j)->getUnit(Zone::LAND)==unit){
					return true;
				}
			}
		}
	}
    return false;
}
void Map::clampPos(Vec2i &pos) const{
	if(pos.x<0) pos.x=0;
	if(pos.y<0) pos.y=0;
	if(pos.x>=w) pos.x=w-1;
	if(pos.y>=h) pos.y=h-1;
}

void Map::prepareTerrain(const Unit *unit) {
	flatternTerrain(unit);
	computeNormals();
	computeInterpolatedHeights();
}

void Map::update(float slice) {
	for(Earthquakes::iterator i = earthquakes.begin(); i != earthquakes.end(); ++i) {
		(*i)->resetSurface();
	}

	for(Earthquakes::iterator i = earthquakes.begin(); i != earthquakes.end();) {
		(*i)->update(slice);
		if((*i)->isDone()) {
			delete (*i);
			i = earthquakes.erase(i);
		} else {
			++i;
		}
	}
}

// ==================== PRIVATE ====================

// ==================== compute ====================

void Map::flatternTerrain(const Unit *unit){
	float refHeight= getTile(toTileCoords(unit->getCenteredPos()))->getHeight();
	for(int i=-1; i<=unit->getType()->getSize(); ++i){
		for(int j=-1; j<=unit->getType()->getSize(); ++j){
			Vec2i pos= unit->getPos()+Vec2i(i, j);
			Cell *c= getCell(pos);
			Tile *sc= getTile(toTileCoords(pos));
			//we change height if pos is inside world, if its free or ocupied by the currenty building
			if(isInside(pos) && sc->getObject()==NULL && (c->getUnit(Zone::LAND)==NULL || c->getUnit(Zone::LAND)==unit)){
				sc->setHeight(refHeight);
			}
		}
	}
}

//compute normals
void Map::computeNormals(){  
	//compute center normals
	for(int i=1; i<tileW-1; ++i){
		for(int j=1; j<tileH-1; ++j){
			getTile(i, j)->setNormal(
				getTile(i, j)->getVertex().normal(getTile(i, j-1)->getVertex(), 
					getTile(i+1, j)->getVertex(), 
					getTile(i, j+1)->getVertex(), 
					getTile(i-1, j)->getVertex()));
		}
	}
}

void Map::computeInterpolatedHeights() {
	for ( int i = 0; i < w; ++i ) {
		for ( int j = 0; j < h; ++j ) {
			getCell(i, j)->setHeight(getTile(toTileCoords(Vec2i(i, j)))->getHeight());
		}
	}
	for ( int i = 1; i < tileW - 1; ++i ) {
		for ( int j = 1; j < tileH - 1; ++j ) {
			for ( int k = 0; k < GameConstants::cellScale; ++k ) {
				for ( int l = 0; l < GameConstants::cellScale; ++l) {
					if ( k == 0 && l == 0 ) {
						getCell(i * GameConstants::cellScale, j * GameConstants::cellScale)->setHeight(getTile(i, j)->getHeight());
					} else if ( k != 0 && l == 0 ) {
						getCell(i * GameConstants::cellScale + k, j * GameConstants::cellScale)->setHeight((
							getTile(    i, j)->getHeight() + 
							getTile(i + 1, j)->getHeight() ) / 2.f);
					} else if ( l != 0 && k == 0 ) {
						getCell(i * GameConstants::cellScale, j * GameConstants::cellScale + l)->setHeight((
							getTile(i,     j)->getHeight() + 
							getTile(i, j + 1)->getHeight() ) / 2.f);
					} else {
						getCell(i * GameConstants::cellScale + k, j * GameConstants::cellScale + l)->setHeight((
							getTile(    i,     j)->getHeight() +
							getTile(    i, j + 1)->getHeight() +
							getTile(i + 1,     j)->getHeight() +
							getTile(i + 1, j + 1)->getHeight() ) / 4.f);
					}
				}
			}
		}
	}
}


//================================================================================================
// Earthquake friendly versions, these are broken for maps with either dimension <= 32
//================================================================================================
//compute normals
void Map::computeNormals(Rect2i range) {
	if (range == Rect2i(0, 0, 0, 0)) {
		range = Rect2i(0, 0, h, w);
	}

	//compute center normals
	for (int i = range.p[0].x / GameConstants::cellScale + 1; i < range.p[1].x / GameConstants::cellScale - 1; ++i) {
		for (int j = range.p[0].y / GameConstants::cellScale + 1; j < range.p[1].y / GameConstants::cellScale - 1; ++j) {
			getTile(i, j)->setNormal(
				getTile(i, j)->getVertex().normal(getTile(i, j - 1)->getVertex(),
						getTile(i + 1, j)->getVertex(),
						getTile(i, j + 1)->getVertex(),
						getTile(i - 1, j)->getVertex()));
		}
	}
}

void Map::computeInterpolatedHeights(Rect2i range){
	if(range == Rect2i(0, 0, 0, 0)) {
		range = Rect2i(0, 0, h, w);
	}

	for (int i = range.p[0].x; i < range.p[1].x; ++i) {
		for (int j = range.p[0].y; j < range.p[1].y; ++j) {
			getCell(i, j)->setHeight(getTile(toTileCoords(Vec2i(i, j)))->getHeight());
		}
	}

	for (int i = range.p[0].x / GameConstants::cellScale + 1; i < range.p[1].x / GameConstants::cellScale - 1; ++i) {
		for (int j = range.p[0].y / GameConstants::cellScale + 1; j < range.p[1].y / GameConstants::cellScale - 1; ++j) {
			for(int k=0; k<GameConstants::cellScale; ++k){
				for(int l=0; l<GameConstants::cellScale; ++l){
					if(k==0 && l==0){
						getCell(i*GameConstants::cellScale, j*GameConstants::cellScale)->setHeight(getTile(i, j)->getHeight());
					}
					else if(k!=0 && l==0){
						getCell(i*GameConstants::cellScale+k, j*GameConstants::cellScale)->setHeight((
							getTile(i, j)->getHeight()+
							getTile(i+1, j)->getHeight())/2.f);
					}
					else if(l!=0 && k==0){
						getCell(i*GameConstants::cellScale, j*GameConstants::cellScale+l)->setHeight((
							getTile(i, j)->getHeight()+
							getTile(i, j+1)->getHeight())/2.f);
					}
					else{
						getCell(i*GameConstants::cellScale+k, j*GameConstants::cellScale+l)->setHeight((
							getTile(i, j)->getHeight()+
							getTile(i, j+1)->getHeight()+
							getTile(i+1, j)->getHeight()+
							getTile(i+1, j+1)->getHeight())/4.f);
					}
				}
			}
		}
	}
}
//================================================================================================

void Map::smoothSurface() {
	float *oldHeights = new float[tileW * tileH];

	for(int i = 0; i < tileW*tileH; ++i) {
		oldHeights[i] = tiles[i].getHeight();
	}

	for(int i = 1; i < tileW - 1; ++i) {
		for(int j = 1; j < tileH - 1; ++j) {

			float height = 0.f;

			for(int k = -1; k <= 1; ++k) {
				for(int l = -1; l <= 1; ++l) {
					height += oldHeights[(j + k) * tileW + (i + l)];
				}
			}

			height /= 9.f;
			Tile *sc = getTile(i, j);
			sc->setHeight(height);
			sc->updateObjectVertex();
		}
	}

	delete [] oldHeights;
}

void Map::computeNearSubmerged(){
	for(int x = 0; x < tileW; ++x){
		for(int y = 0; y < tileH; ++y) {
			bool anySubmerged = false;
			for(int xoff = -1; xoff <= 2 && !anySubmerged; ++xoff) {
				for(int yoff = -1; yoff <= 2 && !anySubmerged; ++yoff) {
					Vec2i pos(x + xoff, y + yoff);
					if(isInsideTile(pos) && getSubmerged(getTile(pos))) {
						anySubmerged = true;
					}
				}
			}
			getTile(x, y)->setNearSubmerged(anySubmerged);
		}
	}
}

void Map::computeCellColors(){
	for(int i=0; i<tileW; ++i){
		for(int j=0; j<tileH; ++j){
			Tile *sc= getTile(i, j);
			if(getDeepSubmerged(sc)){
				float factor= clamp(waterLevel-sc->getHeight()*1.5f, 1.f, 1.5f);
				sc->setColor(Vec3f(1.0f, 1.0f, 1.0f)/factor);
			}
			else{
				sc->setColor(Vec3f(1.0f, 1.0f, 1.0f));
			}
		}
	}
}

#ifdef DEBUG
//makes sure a unit is in cells if alive or not if not alive
void Map::assertUnitCells(const Unit * unit) {
	assert(unit);
	// make sure alive/dead is sane
	assert((unit->getHp() == 0 && unit->isDead()) || (unit->getHp() > 0 && unit->isAlive()));

	const UnitType *ut = unit->getType();
	int size = ut->getSize();
	Field field = unit->getCurrField();

	for(int x = 0; x < size; ++x) {
		for(int y = 0; y < size; ++y) {
			Vec2i currPos = unit->getPos() + Vec2i(x, y);
			assert(isInside(currPos));

			if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
				if(unit->getCurrSkill()->getClass() != SkillClass::DIE) {
					assert(getCell(currPos)->getUnit(field) == unit);
				} else {
					assert(getCell(currPos)->getUnit(field) != unit);
				}
			}
		}
	}
}
#endif


}}//end namespace


