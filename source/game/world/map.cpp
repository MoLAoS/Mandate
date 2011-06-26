// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008      Jaagup Repän <jrepan@gmail.com>,
//				  2008      Daniel Santos <daniel.santos@pobox.com>
//				  2009-2010 James McCulloch <silnarm@gmail.com>
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
#include "cartographer.h"
#include "annotated_map.h"
#include "FSFactory.hpp"

#include "leak_dumper.h"
#include "fixed.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Math;

namespace Glest { namespace Sim {
using Main::Program;
using Search::Cartographer;
using Gui::Selection;

void Tile::deleteResource() {
	g_world.getMapObjectFactory().deleteInstance(object);
	object = 0;
}

// =====================================================
// 	class Map
// =====================================================

// ===================== PUBLIC ========================

Map::Map() 
		: title()
		, waterLevel(0.f)
		, heightFactor(0.f)
		, avgHeight(0.f)
		, m_cellSize(0)
		, m_tileSize(0)
		, maxPlayers(0)
		, cells(NULL)
		, tiles(NULL)
		, startLocations(NULL)
		, m_heightMap(0)
		, m_vertexData(0)
		/*, earthquakes()*/ {
	// If this is expanded, maintain Tile::read() and write()
	assert(Tileset::objCount < 256);
}

Map::~Map() {
	g_logger.logProgramEvent("~Cells", !Program::getInstance()->isTerminating());

	delete [] cells;
	delete [] tiles;
	delete [] startLocations;
	delete m_vertexData;
}

char encodeExplorationState(Tile *tile) {
	char result = 0;
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (tile->isExplored(i)) {
			result |= (1 << i);
		}
	}
	if (result < 10) {
		result += 48; // 0-9
	} else {
		result += 55; // A-F
	}
	return result;
}

void decodeExplorationState(Tile *tile, char state) {
	if (state < 58) {
		state -= 48;
	} else {
		state -= 55;
	}
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		tile->setExplored(i, state & (1 << i));
	}
}

void Map::saveExplorationState(XmlNode *node) const {
	XmlNode *metrics = node->addChild("metrics");
	metrics->addAttribute("width", m_tileSize.w);
	metrics->addAttribute("height", m_tileSize.h);
	for (int y=0; y < m_tileSize.h; ++y) {
		stringstream ss;
		for (int x=0; x < m_tileSize.w; ++x) {
			ss << encodeExplorationState(getTile(x, y));
		}
		node->addChild(string("row") + intToStr(y))->addAttribute("value", ss.str());
	}
}

void Map::loadExplorationState(XmlNode *node) {
	assert(node->getChild("metrics")->getIntAttribute("width") == m_tileSize.w);
	assert(node->getChild("metrics")->getIntAttribute("height") == m_tileSize.h);
	for (int y=0; y < m_tileSize.h; ++y) {
		stringstream ss(node->getChild(string("row") + intToStr(y))->getStringAttribute("value"));
		for (int x=0; x < m_tileSize.w; ++x) {
			char state;
			ss >> state;
			decodeExplorationState(getTile(x, y), state);
		}
	}
}

void Map::load(const string &path, TechTree *techTree, Tileset *tileset) {
	// supporting absolute paths with extension, e.g. showmap in map editor
	//HACK
	string path2 = path;
	bool old_phys=FSFactory::getInstance()->usePhysFS;
	if(path[0]=='/'){
		FSFactory::getInstance()->usePhysFS = false;
		path2 = cutLastExt(path.substr(1, path.length()-1));
	}

	FileOps *f = FSFactory::getInstance()->getFileOps();

	FSFactory::getInstance()->usePhysFS = old_phys;

	struct MapFileHeader {
		int32 version;
		int32 maxPlayers;
		int32 width;
		int32 height;
		int32 altFactor;
		int32 waterLevel;
		int8 title[128];
		int8 author[128];
		union {
			int8 description[256];
			struct {
				int8 short_desc[128];
				int32 magic; // 0x01020304 for meta
				int8 meta[124];
			};
		};
	};

	name = basename(path2);

	try {
		if (fileExists(path2 + ".mgm")) {
			f->openRead((path2 + ".mgm").c_str());
		} else {
			f->openRead((path2 + ".gbm").c_str());
		}

		//read header
		MapFileHeader header;
		
		// FIXME: Better error handling starting here: report bad map instead of partially loading,
		// etc.
		f->read(&header, sizeof(MapFileHeader), 1);

		if (!isPowerOfTwo(header.width)) {
			throw runtime_error("Map width is not a power of 2");
		}

		if (!isPowerOfTwo(header.height)) {
			throw runtime_error("Map height is not a power of 2");
		}

		heightFactor = (float)header.altFactor;
		waterLevel = static_cast<float>((header.waterLevel - 0.01f) / heightFactor);
		title = header.title;
		maxPlayers = header.maxPlayers;
		m_tileSize.w = header.width;
		m_tileSize.h = header.height;
		m_cellSize.w = m_tileSize.w * GameConstants::cellScale;
		m_cellSize.h = m_tileSize.h * GameConstants::cellScale;


		//start locations
		startLocations = new Vec2i[maxPlayers];
		for (int i = 0; i < maxPlayers; ++i) {
			int x, y;
			f->read(&x, sizeof(int32), 1);
			f->read(&y, sizeof(int32), 1);
			startLocations[i] = Vec2i(x, y) * GameConstants::cellScale;
		}

		// Tiles & Cells
		cells = new Cell[m_cellSize.w * m_cellSize.h];
		tiles = new Tile[m_tileSize.w * m_tileSize.h];

		m_heightMap = new float[m_tileSize.w * m_tileSize.h];

		//read heightmap
		for (int y = 0; y < m_tileSize.h; ++y) {
			for (int x = 0; x < m_tileSize.w; ++x) {
				float32 alt;
				f->read(&alt, sizeof(float32), 1);
				m_heightMap[y * m_tileSize.w + x] = alt / heightFactor;
			}
		}

		//read surfaces
		for (int y = 0; y < m_tileSize.h; ++y) {
			for (int x = 0; x < m_tileSize.w; ++x) {
				int8 surf;
				f->read(&surf, sizeof(int8), 1);
				getTile(x, y)->setTileType(surf - 1);
			}
		}

		//read objects and resources
		for (int y = 0; y < m_tileSize.h; ++y) {
			for (int x = 0; x < m_tileSize.w; ++x) {
				int8 objNumber;
				f->read(&objNumber, sizeof(int8), 1);
				Tile *tile = getTile(Vec2i(x, y));
				Vec3f vert(x * cellScale + cellScale / 2.f, 0.f, y * cellScale + cellScale / 2.f);

				if (objNumber == 0 || x == 0 || y == 0 || x >= m_tileSize.w - 2 || y >= m_tileSize.h - 2) {
					tile->setObject(NULL);
				} else if (objNumber <= Tileset::objCount) {
					MapObject *o = g_world.newMapObject(tileset->getObjectType(objNumber - 1), vert);
					tile->setObject(o);
					for (int i = 0; i < techTree->getResourceTypeCount(); ++i) {
						const ResourceType *rt = techTree->getResourceType(i);
						if (rt->getClass() == ResourceClass::TILESET && rt->getTilesetObject() == objNumber) {
							o->setResource(rt, Vec2i(x, y) * cellScale);
						}
					}
				} else {
					const ResourceType *rt = techTree->getTechResourceType(objNumber - Tileset::objCount) ;
					MapObject *o = g_world.newMapObject(NULL, vert);
					tile->setObject(o);
					o->setResource(rt, Vec2i(x, y) * cellScale);
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
//	checksum.add<float>(waterLevel);
//	checksum.add<float>(heightFactor);
//	checksum.add<float>(avgHeight);
	checksum.add<int>(m_cellSize.w);
	checksum.add<int>(m_cellSize.h);
	checksum.add<int>(m_tileSize.w);
	checksum.add<int>(m_tileSize.h);
	checksum.add<int>(maxPlayers);
//	for (int i=0; i < m_tileSize.w * m_tileSize.h; ++i) {
//		checksum.add<Vec3f>(tiles[i].getVertex());
//	}
	for (int i=0; i < maxPlayers; ++i) {
		checksum.add<Vec2i>(startLocations[i]);
	}
}

void Map::init() {
	g_logger.logProgramEvent("Heightmap computations", true);
	m_vertexData = new MapVertexData(m_tileSize);
	smoothSurface();
	computeNormals();
	computeInterpolatedHeights();
	computeNearSubmerged();
	computeTileColors();
	setCellTypes();
	calcAvgAltitude();
}

void Map::setCellTypes() {
	for (int y = 0; y < getH(); ++y) {
		for (int x = 0; x < getW(); ++x) {
			Cell *cell = getCell(x, y);
			if (cell->getHeight() < waterLevel - (1.5f / heightFactor)) {
				cell->setType ( SurfaceType::DEEP_WATER );
			} else if (cell->getHeight () < waterLevel) {
				cell->setType(SurfaceType::FORDABLE);
			}
		}
	}
}

void Map::calcAvgAltitude() {
	float sum = 0.0;
	for (int y=0; y < getTileH(); ++y) {
		for (int x=0; x < getTileW(); ++x) {
			sum += getTileHeight(x, y);
		}
	}
	avgHeight = (float)(sum / (getTileH() * getTileW()));
	//cout << "average map height = " << avgHeight << endl;
}

// ==================== is ====================

/** @returns if there is a resource next to a unit
  * @param pos unit position @param size unit size
  * @param rt Resource type of interest
  * @param resourcePos stores position of the resource if found */
bool Map::isResourceNear(const Vec2i &pos, int size, const ResourceType *rt, Vec2i &resourcePos) const {
	Vec2i p1 = pos + Vec2i(-1);
	Vec2i p2 = pos + Vec2i(size);
	Util::PerimeterIterator iter(p1, p2);
	while (iter.more()) {
		Vec2i cur = iter.next();
		if (isInside(cur)) {
			MapResource *r = getTile(toTileCoords(cur))->getResource();
			if (r && r->getType() == rt) {
				resourcePos = cur;
				return true;
			}
		}
	}
	return false;
}

// ==================== free cells ====================
bool Map::fieldsCompatible(Cell *cell, Field mf) const {
	if (mf == Field::AIR || mf == Field::AMPHIBIOUS
	|| (mf == Field::LAND && !cell->isDeepSubmerged()) 
	|| (mf == Field::ANY_WATER && cell->isSubmerged())
	|| (mf == Field::DEEP_WATER && cell->isDeepSubmerged())) {
		return true;
	}
	return false;
}

bool Map::isFreeCell(const Vec2i &pos, Field field) const {
	if (!isInside(pos) || !getCell(pos)->isFree(field == Field::AIR ? Zone::AIR : Zone::LAND)) {
		return false;
	}
	if (field != Field::AIR && !getTile(toTileCoords(pos))->isFree()) {
		return false;
	}
	return g_cartographer.getMasterMap()->canOccupy(pos, 1, field); 
}

bool Map::isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const {
	if (isInside(pos)) {
		Cell *c = getCell(pos);
		if (c->getUnit(field) == unit && fieldsCompatible(c, field)) {
			return true;
		} else {
			return isFreeCell(pos, field);
		}
	}
	return false;
}
// Is the Cell at 'pos' (for the given 'field') either free or does it contain any of the units in 'units'
bool Map::isFreeCellOrHaveUnits(const Vec2i &pos, Field field, const UnitVector &units) const {
	if (isInside(pos)) {
		Unit *containedUnit = getCell(pos)->getUnit(field);
		if (containedUnit && fieldsCompatible(getCell(pos), field)) {
			UnitVector::const_iterator i;
			for (i = units.begin(); i != units.end(); ++i) {
				if (containedUnit == *i) {
					return true;
				}
			}
		}
		return isFreeCell(pos, field);
	}
	return false;
}

bool Map::canOccupy(const Vec2i &pos, Field field, const UnitType *ut, CardinalDir facing) {
	if (ut->hasCellMap()) {
		for (int y=0; y < ut->getSize(); ++y) {
			for (int x=0; x < ut->getSize(); ++x) {
				if (ut->getCellMapCell(x, y, facing)) {
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
		if (teamIndex == -1 || sc->isVisible(teamIndex)) {
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
	for (int i=pos.x; i<pos.x+size; ++i) {
		for (int j=pos.y; j<pos.y+size; ++j) {
			if (!isFreeCell(Vec2i(i,j), field)) {
				return false;
			}
		}
	}
	return true;
}

bool Map::areFreeCells(const Vec2i &pos, int size, char *fieldMap) const {
	for (int i = 0; i < size; ++i) {
		for (int j = 0; j < size; ++j) {
			Field field;
			switch (fieldMap[j*size+i]) {
				case 'l': field = Field::LAND; break;
				case 'a': field = Field::AMPHIBIOUS; break;
				case 'w': field = Field::DEEP_WATER; break;
				case 'r': field = Field::LAND; break;
				default: throw runtime_error("Bad value in fieldMap");
			}
			if (!isFreeCell(Vec2i(pos.x+i, pos.y+j), field)) {
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

bool Map::areFreeCellsOrHaveUnits(const Vec2i &pos, int size, Field field, const UnitVector &units) const {
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
	fixed len = fstart.dist(ftarget);

	if (!len || (len >= minRange && len <= maxRange)) {
		return start;
	} else {
		assert(minRange == 0 || start.x != target.x || start.y != target.y);
		fixed t = (len <= minRange ? minRange : maxRange) / len;
		fixedVec2 res(
			ftarget.x + (ftarget.x - fstart.x) * t,
			ftarget.y + (ftarget.y - fstart.y) * t);
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
	///@todo this code can be optimized by starting at the nearest position looking for a free cell
	/// and moving away from the starting point.
	//int sideSize = targetSize + size;
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

	return minDistance < fixed::max_int();

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
	//int size = ut->getSize();
	Zone zone = unit->getCurrZone();
	Zone o_zone = other->getCurrZone();

	stringstream ss;
	ss << "Error: " << ut->getName() << " [id:" << unit->getId() << "] putting in cell ("
		<< currPos << "{" << ZoneNames[zone] << "}" << " cell is already occupied by a " 
		<< other->getType()->getName() << " [id:" << other->getId() << "] {"
		<< ZoneNames[o_zone] << "}";
	g_logger.logError(ss.str());
	throw runtime_error ( "Ooops... see " + g_fileFactory.getConfigDir() + "glestadv-error.log" );
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
			if (!ut->hasCellMap() || ut->getCellMapCell(x, y, unit->getModelFacing())) {
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

			if (!ut->hasCellMap() || ut->getCellMapCell(x, y, unit->getModelFacing())) {
				assert(getCell(currPos)->getUnit(zone) == unit);
				getCell(currPos)->setUnit(zone, NULL);
			}
		}
	}
}

// ==================== misc ====================

//returns if unit is next to pos
bool Map::isNextTo(const Vec2i &pos, const Unit *unit) const{
	Zone z = unit->getCurrZone();
	Util::RectIterator iter(pos - Vec2i(1), pos + Vec2i(1));
	while (iter.more()) {
		Vec2i cpos = iter.next();
		if (isInside(cpos)) {
			if (getCell(cpos)->getUnit(z) == unit) {
				return true;
			}
		}
	}
	return false;
}

void Map::clampPos(Vec2i &pos) const{
	pos.x = clamp(pos.x, 0, m_cellSize.w - 1);
	pos.y = clamp(pos.y, 0, m_cellSize.h - 1);
}

void Map::prepareTerrain(const Unit *unit) {
	flatternTerrain(unit);
	computeInterpolatedHeights();
}

#ifdef EARTHQUAKE_CODE
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
#endif
// ==================== PRIVATE ====================

// ==================== compute ====================

void Map::flatternTerrain(const Unit *unit) {
	float refHeight = getTileHeight(toTileCoords(unit->getCenteredPos()));
	Vec2i tile_tl = toTileCoords(unit->getPos());
	Vec2i tile_br = toTileCoords(unit->getPos() + Vec2i(unit->getSize()));
	Util::RectIterator iter(tile_tl, tile_br);
	while (iter.more()) {
		Vec2i tile_pos = iter.next();
		if (isInsideTile(tile_pos) && !getTile(tile_pos)->getObject()) {
			setTileHeight(tile_pos, refHeight);
		}
	}
	computeNormals();
}

//compute normals
void Map::computeNormals() {
	// set perimeter normals
	PerimeterIterator pIter(Vec2i(0), m_tileSize - Vec2i(1));
	while (pIter.more()) {
		Vec2i pos = pIter.next();
		m_vertexData->get(pos).norm() = Vec3f(0.f, 1.f, 0.f);
	}
	// compute center normals
	for (int i=1; i < m_tileSize.w - 1; ++i) {
		for (int j=1; j < m_tileSize.h - 1; ++j) {
			m_vertexData->get(i, j).norm() = m_vertexData->get(i, j).vert().normal(
					m_vertexData->get(i + 0, j - 1).vert(),
					m_vertexData->get(i + 1, j + 0).vert(),
					m_vertexData->get(i + 0, j + 1).vert(),
					m_vertexData->get(i - 1, j + 0).vert());
		}
	}
}

void Map::computeInterpolatedHeights() {
	for (int i = 0; i < m_cellSize.w; ++i) {
		for (int j = 0; j < m_cellSize.h; ++j) {
			getCell(i, j)->setHeight(getTileHeight(toTileCoords(i, j)));
		}
	}
	for (int i = 1; i < m_tileSize.w - 1; ++i) {
		for (int j = 1; j < m_tileSize.h - 1; ++j) {
			Vec2i tl = toUnitCoords(i, j);
			for (int k = 0; k < GameConstants::cellScale; ++k) {
				for (int l = 0; l < GameConstants::cellScale; ++l) {
					if (k == 0 && l == 0) {
						getCell(tl)->setHeight(getTileHeight(i, j));
					} else if (k != 0 && l == 0) {
						getCell(tl + Vec2i(k, 0))->setHeight(
							(getTileHeight(i, j) + getTileHeight(i + 1, j)) / 2.f);
					} else if (l != 0 && k == 0) {
						getCell(tl + Vec2i(0, l))->setHeight(
							(getTileHeight(i, j) + getTileHeight(i, j + 1)) / 2.f);
					} else {
						getCell(tl + Vec2i(k, l))->setHeight(
							(getTileHeight(i, j) + getTileHeight(i + 1, j) + 
							 getTileHeight(i, j + 1) + getTileHeight(i + 1, j + 1)) / 4.f);
					}
				}
			}
		}
	}
}

#ifdef EARTHQUAKE_CODE
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
#endif

void Map::smoothSurface() {
	Util::PerimeterIterator perimIter(Vec2i(0), m_tileSize - Vec2i(1));
	while (perimIter.more()) {
		Vec2i pos = perimIter.next();
		setTileHeight(pos, m_heightMap[pos.y * m_tileSize.w + pos.x]);
	}
	Util::RectIterator rectIter(Vec2i(1), m_tileSize - Vec2i(2));
	while (rectIter.more()) {
		Vec2i pos = rectIter.next();
		float height = m_heightMap[pos.y * m_tileSize.w + pos.x];
		perimIter = Util::PerimeterIterator(pos - Vec2i(1), pos + Vec2i(1));
		while (perimIter.more()) {
			Vec2i pos2 = perimIter.next();
			height += m_heightMap[pos2.y * m_tileSize.w + pos2.x];
		}
		height /= 9.f;
		setTileHeight(pos, height);

		Tile *tile = getTile(pos);
		MapObject *obj = tile->getObject();
		if (obj) {
			Vec3f pos = obj->getPos();
			pos.y = height;
			obj->setPos(pos);
		}
	}
	delete [] m_heightMap;
	m_heightMap = 0;
}

void Map::computeNearSubmerged() {
	for (int x = 0; x < m_tileSize.w; ++x) {
		for (int y = 0; y < m_tileSize.h; ++y) {
			bool anySubmerged = false;
			for (int xoff = -1; xoff <= 2 && !anySubmerged; ++xoff) {
				for (int yoff = -1; yoff <= 2 && !anySubmerged; ++yoff) {
					Vec2i pos(x + xoff, y + yoff);
					if (isInsideTile(pos) && isTileSubmerged(pos)) {
						anySubmerged = true;
					}
				}
			}
			getTile(x, y)->setNearSubmerged(anySubmerged);
		}
	}
}

void Map::computeTileColors() {
	for (int i=0; i < m_tileSize.w; ++i) {
		for (int j=0; j < m_tileSize.h; ++j) {
			Tile *tile = getTile(i, j);
			if (isTileDeepSubmerged(Vec2i(i, j))) {
				float factor = clamp(waterLevel - getTileHeight(i, j) * 1.5f, 1.f, 1.5f);
				tile->setColor(Vec3f(1.0f, 1.0f, 1.0f) / factor);
			} else {
				tile->setColor(Vec3f(1.0f, 1.0f, 1.0f));
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

	if (unit->getPos() != Vec2i(-1)) {
		for(int x = 0; x < size; ++x) {
			for(int y = 0; y < size; ++y) {
				Vec2i currPos = unit->getPos() + Vec2i(x, y);
				assert(isInside(currPos));
				if(!ut->hasCellMap() || ut->getCellMapCell(x, y, unit->getModelFacing())) {
					if(unit->isActive()) {
						Unit *testUnit = getCell(currPos)->getUnit(field);
						assert(testUnit == unit);
					} else {
						assert(getCell(currPos)->getUnit(field) != unit);
					}
				}
			}
		}
	} else {
		assert(unit->isCarried());
	}
}

#endif


}}//end namespace


