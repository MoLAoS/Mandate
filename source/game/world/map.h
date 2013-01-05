// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008      Jaagup Repän <jrepan@gmail.com>,
//				  2008      Daniel Santos <daniel.santos@pobox.com>
//				  2009-2010 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MAP_H_
#define _GLEST_GAME_MAP_H_

#include <cassert>
#include <map>
#include <set>

#include "vec.h"
#include "math_util.h"
#include "command_type.h"
#include "logger.h"
#include "object.h"
#include "object_type.h"
#include "game_constants.h"
#include "selection.h"
#include "exceptions.h"
#include "pos_iterator.h"
#include "fixed.h"

#include "unit.h"

using namespace Shared::Math;
using Shared::Graphics::Texture2D;
using namespace Glest::Util;
using Glest::Gui::Selection;

namespace Glest { namespace Sim {

inline Zone fieldZone(Field f) {
    Zone freeCell;
    if (f == Field::AIR) {
    freeCell = Zone::AIR;
    } else if (f == Field::LAND) {
    freeCell = Zone::LAND;
    } else if (f == Field::WALL || f == Field::STAIR) {
    freeCell = Zone::WALL;
    }
	return freeCell;
}

// =====================================================
// 	class Cell
//
///	A map cell that holds info about units present on it
// =====================================================

class Cell {
private:
	Unit *units[Zone::COUNT];	//units on this cell
	float height;
	SurfaceType surfaceType;

	Cell(Cell&);
	void operator=(Cell&);

public:
	Cell() {
		memset(units, 0, sizeof(units));
		height= 0;
		surfaceType = SurfaceType::LAND;
	}

	// get
	Unit *getUnit(Zone zone) const	{ return units[zone];			}
	Unit *getUnit(Field f) const	{ return getUnit(fieldZone(f));	}
	float getHeight() const			{ return height;				}
	SurfaceType getType() const 	{ return surfaceType;			}

	// is
	bool isSubmerged () const		{ return surfaceType != SurfaceType::LAND;			}
	bool isDeepSubmerged () const	{ return surfaceType == SurfaceType::DEEP_WATER;	}
	bool isFree(Zone zone) const	{ return !getUnit(zone) || getUnit(zone)->isPutrefacting(); }

	// set
	void setUnit(Zone zone, Unit *unit)	{ units[zone]= unit;	}
	void setHeight(float h)					{ height= h;			}
	void setType(SurfaceType type)			{ surfaceType = type;	}
};

// =====================================================
// 	class Tile
//
//	A heightmap tile, each Tile is composed by more than one Cell
// =====================================================

class Tile {
private:
	Vec3f color; // leave here, only needed by minimap

	// surface
	int tileType;
	int texId;
	//const Texture2D *tileTexture;

	// object & resource
	MapObject *object;

	// visibility
	bool visible[GameConstants::maxPlayers];
	bool explored[GameConstants::maxPlayers];

	// cache
	bool nearSubmerged;

public:
	Tile() : tileType(-1), texId(0), object(0), nearSubmerged(false) { }
	~Tile() { }

	// get/is
	const Vec3f &getColor() const               { return color;                              }
	int getTileType() const                     { return tileType;                           }
	int getTexId() const                        { return texId;                              }
	MapObject *getObject() const                { return object;                             }
	MapResource *getResource() const            { return object ? object->getResource() : 0; }
	bool getNearSubmerged() const               { return nearSubmerged;                      }
	bool isVisible(int teamIndex) const			{
		ASSERT_RANGE(teamIndex, GameConstants::maxPlayers);
		return visible[teamIndex];
	}
	bool isExplored(int teamIndex) const		{
		ASSERT_RANGE(teamIndex, GameConstants::maxPlayers);
		return explored[teamIndex];
	}

	//set
	void setColor(const Vec3f &color)               { this->color= color;        }
	void setTileType(int tileType)                  { this->tileType= tileType;  }
	void setTexId(int id)                           { this->texId = id;          }
	void setObject(MapObject *object)               { this->object= object;      }
	void setExplored(int teamIndex, bool explored)	{
		ASSERT_RANGE(teamIndex, GameConstants::maxPlayers);
		this->explored[teamIndex]= explored;
	}
	void setVisible(int teamIndex, bool visible)	{
		ASSERT_RANGE(teamIndex, GameConstants::maxPlayers);
		this->visible[teamIndex]= visible;
	}

	void setNearSubmerged(bool nearSubmerged)		{this->nearSubmerged= nearSubmerged;	}

	//misc
	bool isFree() const						{ return !object || object->getWalkable();	}
	bool isPlacement(string spot) const;
	bool isBonusObject(string name) const;
	void deleteResource();
};

/** Tile Vertex structure */
struct TileVertex {
private:
	Vec3f m_position;		// 12
	Vec3f m_normal;			// 12 // 24
	Vec2f m_tileTLTexCoord;	//  8 => 32 == great
	Vec2f m_fowTexCoord;	//  8 => 40 != good

	//Vec2f m_tileTRTexCoord;	//  8
	//Vec2f m_tileBRTexCoord;	//  8
	//Vec2f m_tileBLTexCoord;	//  8 => 64 == good.

public:
	TileVertex() : m_tileTLTexCoord(0.f) {}

	TileVertex& operator=(const TileVertex &that) {
		memcpy(this, &that, sizeof(TileVertex));
		return *this;
	}

	Vec3f& vert() { return m_position; }
	Vec3f& norm() { return m_normal; }
	Vec2f& tileTexCoord() { return m_tileTLTexCoord; }
	Vec2f& fowTexCoord() { return m_fowTexCoord; }
};

/** vertex data for the map, in video card friendly format */
class MapVertexData {
private:
	TileVertex *m_data;
	Vec2i       m_size;

public:
	MapVertexData(Vec2i size) : m_data(0), m_size(size) {
		m_data = new TileVertex[m_size.w * m_size.h];
		float scale = float(GameConstants::cellScale);
		for (int y = 0; y < m_size.h; ++y) {
			for (int x = 0; x < m_size.w; ++x) {
				m_data[y * m_size.w + x].vert() = Vec3f(x * scale, 0.f, y * scale);
			}
		}
	}
	~MapVertexData() { delete [] m_data; }

	TileVertex& get(Vec2i pos) {
		return m_data[pos.y * m_size.w + pos.x];
	}

	TileVertex& get(int x, int y) {
		return m_data[y * m_size.w + x];
	}

	TileVertex* data() {return m_data;}
};

// =====================================================
// 	class Map
//
///	Represents the game map (and loads it from a gbm file)
// =====================================================

class Map {
public:
//	typedef vector<Earthquake*> Earthquakes;

private:
	string name;
	string title;
	float waterLevel;
	float heightFactor;
	float avgHeight;
	Vec2i m_cellSize;
	Vec2i m_tileSize;
	int maxPlayers;
	Cell *cells;
	Tile *tiles;
	Vec2i *startLocations;

	float *m_heightMap;
	MapVertexData *m_vertexData;

//	Earthquakes earthquakes;

private:
	Map(Map&);
	void operator=(Map&);

public:
	Map();
	~Map();

	void init();
	void load(const string &path, TechTree *techTree, Tileset *tileset);
	void doChecksum(Checksum &checksum);

	void saveExplorationState(XmlNode *node) const;
	void loadExplorationState(XmlNode *node);

	//get
	string getName() const { return name; }
	Cell *getCell(int x, int y) const {
		assert(this->isInside(x,y) && "co-ordinates out of range");
		return &cells[y * m_cellSize.w + x];
	}
	Cell *getCell(const Vec2i &pos) const { return getCell(pos.x, pos.y); }

	Tile *getTile(int sx, int sy) const {
		assert(this->isInsideTile(sx,sy) && "co-ordinates out of range");
		return &tiles[sy * m_tileSize.w + sx];
	}
	Tile *getTile(const Vec2i &sPos) const { return getTile(sPos.x, sPos.y); }

	Tile *getTileFromCellPos(int x, int y) const {
		return getTile(x / GameConstants::cellScale, y / GameConstants::cellScale);
	}
	Tile *getTileFromCellPos(const Vec2i &pos) const { return getTileFromCellPos(pos.x, pos.y);	}

	bool isBonusObject(string name, const Vec2i &pos, Field field) const;
	bool nearUnitBonusObject(const UnitType *ut, const Vec2i &pos);

	int getW() const							{ return m_cellSize.w;  }
	int getH() const							{ return m_cellSize.h;   }
	int getTileW() const						{ return m_tileSize.w;    }
	int getTileH() const						{ return m_tileSize.h;	   }
	int getMaxPlayers() const					{ return maxPlayers;		}
	float getHeightFactor() const				{ return heightFactor;		 }
	float getAvgHeight() const					{ return avgHeight;			  }
	float getWaterLevel() const					{ return waterLevel;		   }
	Rect2i getBounds() const					{ return Rect2i(Vec2i(0), m_cellSize); }
	Vec2i getStartLocation(int locNdx) const	{
		assert(locNdx >= 0 && locNdx < GameConstants::maxPlayers && "invalid faction index");
		return startLocations[locNdx];
	}

	bool isTileSubmerged(const Vec2i &tilePos) const {
		return m_vertexData->get(tilePos).vert().y < waterLevel;
	}

	bool isTileDeepSubmerged(const Vec2i &tilePos) const {
		return m_vertexData->get(tilePos).vert().y < waterLevel - (1.5f / heightFactor);
	}

	float getTileHeight(const Vec2i &pos) const { return m_vertexData->get(pos).vert().y; }
	float getTileHeight(int x, int y) const		{ return m_vertexData->get(x,y).vert().y; }

	void setTileHeight(const Vec2i &pos, float h) { m_vertexData->get(pos).vert().y = h; }

	MapVertexData* getVertexData() { return m_vertexData; }
	//const Earthquakes &getEarthquakes() const			{return earthquakes;}

	//is
	bool isInside(int x, int y) const		{return x >= 0 && y >= 0 && x < m_cellSize.w && y < m_cellSize.h;}
	bool isInside(const Vec2i &pos) const	{return isInside(pos.x, pos.y);}
	bool isInsideTile(int sx, int sy) const	{return sx >= 0 && sy >= 0 && sx < m_tileSize.w && sy < m_tileSize.h;}
	bool isInsideTile(const Vec2i &sPos) const	{ return isInsideTile(sPos.x, sPos.y); }

	bool isResourceNear(const Vec2i &pos, int size, const ResourceType *rt, Vec2i &resourcePos) const;

	//free cells
	// This should just do a look up in the map metrics (currently maintained by the Cartographer object)
	// Is a cell of a given field 'free' to be occupied
	bool isFreeCell(const Vec2i &pos, Field field) const;
	bool isFoundation(string foundation, const Vec2i &pos, Field field) const;

	bool areFreeCells(const Vec2i &pos, int size, Field field) const;
	bool areFreeCells(const Vec2i &pos, int size, char *fieldMap) const;

	bool areFoundation(string foundation, const Vec2i &pos, int size, Field field) const;
	bool areFoundation(string foundation, const Vec2i &pos, int size, char *fieldMap) const;

	bool clearFoundation(string foundation, const Vec2i &pos, int size, Field field) const;

	bool isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const;
	bool areFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field, const Unit *unit) const;

	bool isFreeCellOrHaveUnits(const Vec2i &pos, Field field, const UnitVector &units) const;
	bool areFreeCellsOrHaveUnits(const Vec2i &pos, int size, Field field, const UnitVector &units) const;

	bool isAproxFreeCell(const Vec2i &pos, Field field, int teamIndex) const;
	bool areAproxFreeCells(const Vec2i &pos, int size, Field field, int teamIndex) const;

	void getOccupants(vector<Unit *> &results, const Vec2i &pos, int size, Zone field) const;

	bool canOccupy(const Vec2i &pos, Field field, const UnitType *ut, CardinalDir facing);

	bool fieldsCompatible ( Cell *cell, Field mf ) const;
	bool isFieldMapCompatible ( const Vec2i &pos, const UnitType *unitType ) const;

	// location calculations
	static Vec2i getNearestAdjacentPos(const Vec2i &start, int size, const Vec2i &target, Field field, int targetSize = 1);
	static Vec2i getNearestPos(const Vec2i &start, const Vec2i &target, int minRange, int maxRange, int targetSize = 1);
	static Vec2i getNearestPos(const Vec2i &start, const Unit *target, int minRange, int maxRange) {
		return getNearestPos(start, target->getPos(), minRange, maxRange, target->getType()->getSize());
	}

	bool getNearestAdjacentFreePos(Vec2i &result, const Unit *unit, const Vec2i &start, int size, const Vec2i &target, Field field, int targetSize = 1) const;
	bool getNearestAdjacentFreePos(Vec2i &result, const Unit *unit, const Vec2i &target, Field field, int targetSize = 1) const {
		return getNearestAdjacentFreePos(result, unit, unit->getPos(), unit->getType()->getSize(), target, field, targetSize);
	}

	bool getNearestFreePos(Vec2i &result, const Unit *unit, const Vec2i &target, int minRange, int maxRange, int targetSize = 1) const;
	bool getNearestFreePos(Vec2i &result, const Unit *unit, const Unit *target, int minRange, int maxRange) const {
		return getNearestFreePos(result, unit, target->getPos(), minRange, maxRange, target->getSize());
	}

	//unit placement
    void putUnitCells(Unit *unit, const Vec2i &pos);
	void clearUnitCells(Unit *unit, const Vec2i &pos);

	//misc
	bool isNextTo(const Vec2i &pos, const Unit *unit) const;
	void clampPos(Vec2i &pos) const;

	void prepareTerrain(const Unit *unit);
	void flatternTerrain(const Unit *unit);

	void computeNormals();
	void computeInterpolatedHeights();

#ifdef EARTHQUAKE_CODE
	void computeNormals(Rect2i range);
	void computeInterpolatedHeights(Rect2i range);

	void add(Earthquake *earthquake) 			{earthquakes.push_back(earthquake);}
	void update(float slice);
#endif

	//assertions
	#ifdef DEBUG
		void assertUnitCells(const Unit * unit);
	#else
		void assertUnitCells(const Unit * unit){}
	#endif

	//static
	static Vec2i toTileCoords(Vec2i cellPos)	{return cellPos / GameConstants::cellScale;}
	static Vec2i toTileCoords(int x, int y)		{return toTileCoords(Vec2i(x, y));}
	static Vec2i toUnitCoords(Vec2i tilePos)	{return tilePos * GameConstants::cellScale;}
	static Vec2i toUnitCoords(int x, int y)		{return toUnitCoords(Vec2i(x, y)); }
	static string getMapPath(const string &mapName)	{return "maps/"+mapName+".gbm";}

private:
	//compute
	void smoothSurface();
	void computeNearSubmerged();
	void computeTileColors();
	void setCellTypes();
	void calcAvgAltitude();

	static void findNearest(Vec2i &result, const Vec2i &start, const Vec2i &candidate, fixed &minDistance);
	void findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, fixed &minDistance) const;
	void findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, fixed &minDistance, const Unit *unit) const;
};

}} //end namespace

#endif
