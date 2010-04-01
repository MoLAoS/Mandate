// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Jaagup Repän <jrepan@gmail.com>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//				  2009 James McCulloch <silnarm at gmail>
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
#include "game_constants.h"
#include "selection.h"
#include "exceptions.h"
#include "pos_iterator.h"
#include "fixed.h"

namespace Glest{ namespace Game{

using namespace Shared::Math;
using Shared::Graphics::Texture2D;
using namespace Glest::Game::Util;

class Tileset;
class Unit;
class Resource;
class TechTree;
class World;
class UnitContainer;
class Earthquake;

inline Zone fieldZone(Field f) {
	return f == Field::AIR ? Zone::AIR : Zone::LAND;
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
	bool isFree(Zone field) const	{ return !getUnit(field) || getUnit(field)->isPutrefacting(); }

	// set
	void setUnit(Zone field, Unit *unit)	{ units[field]= unit;	}
	void setHeight(float h)					{ height= h;			}
	void setType(SurfaceType type)			{ surfaceType = type;	}
};

// =====================================================
// 	class Tile
//
//	A heightmap cell, each Tile is composed by more than one Cell
// =====================================================

class Tile{
private:
	//geometry
	Vec3f vertex;
	Vec3f normal;
	Vec3f color;
	Vec3f originalVertex;

	//tex coords
	Vec2f fowTexCoord;		//tex coords for TEXTURE1 when multitexturing and fogOfWar
	Vec2f surfTexCoord;		//tex coords for TEXTURE0

	//surface
	int tileType;
    const Texture2D *tileTexture;

	//object & resource
	Object *object;

	//visibility
	bool visible[GameConstants::maxPlayers];
    bool explored[GameConstants::maxPlayers];

	//cache
	bool nearSubmerged;

public:
	Tile() : vertex(0.f), normal(0.f, 1.f, 0.f), originalVertex(0.f), 
			tileType(-1), tileTexture(NULL), object(NULL) {}
	~Tile() { if(object) delete object;}

	//get
	const Vec3f &getVertex() const				{return vertex;		}
	float getHeight() const						{return vertex.y;	}
	const Vec3f &getColor() const				{return color;		}
	const Vec3f &getNormal() const				{return normal;		}
	int getTileType() const						{return tileType;	}
	const Texture2D *getTileTexture() const		{return tileTexture;}
	Object *getObject() const					{return object;		}
	Resource *getResource() const				{return object==NULL? NULL: object->getResource();}
	const Vec2f &getFowTexCoord() const			{return fowTexCoord;	}
	const Vec2f &getSurfTexCoord() const		{return surfTexCoord;	}
	bool getNearSubmerged() const				{return nearSubmerged;	}

	bool isVisible(int teamIndex) const			{return visible[teamIndex];}
	bool isExplored(int teamIndex) const		{return explored[teamIndex];}

	//set
	void setVertex(const Vec3f &vertex)				{originalVertex = this->vertex = vertex;}
	void setHeight(float height)					{originalVertex.y = vertex.y = height; }
	void setNormal(const Vec3f &normal)				{this->normal= normal;				  }
	void setColor(const Vec3f &color)				{this->color= color;				 }
	void setTileType(int tileType)					{this->tileType= tileType;			}
	void setTileTexture(const Texture2D *st)		{this->tileTexture= st;			   }
	void setObject(Object *object)					{this->object= object;			   }
	void setFowTexCoord(const Vec2f &ftc)			{this->fowTexCoord= ftc;			}
	void setTileTexCoord(const Vec2f &stc)			{this->surfTexCoord= stc;			 }
	void setExplored(int teamIndex, bool explored)	{this->explored[teamIndex]= explored; }
	void setVisible(int teamIndex, bool visible)	{this->visible[teamIndex]= visible;	   }
	void setNearSubmerged(bool nearSubmerged)		{this->nearSubmerged= nearSubmerged;	}

	//misc
	bool isFree() const						{ return !object || object->getWalkable();	}
	void deleteResource()					{ delete object; object= NULL;				}
	void resetVertex()						{ vertex = originalVertex;					}
	void alterVertex(const Vec3f &offset)	{ vertex += offset;							}
	void updateObjectVertex()				{ if (object) object->setPos(vertex);		}
};

// =====================================================
// 	class Map
//
///	Represents the game map (and loads it from a gbm file)
// =====================================================

class Map {
public:
	typedef vector<Earthquake*> Earthquakes;

private:
	string title;
	float waterLevel;
	float heightFactor;
	float avgHeight;
	int w;
	int h;
	int tileW;
	int tileH;
	int maxPlayers;
	Cell *cells;
	Tile *tiles;
	Vec2i *startLocations;
	//float *surfaceHeights;

	Earthquakes earthquakes;

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
	Cell *getCell(int x, int y) const {
		assert ( this->isInside ( x,y ) );
		return &cells[y * w + x];
	}
	Cell *getCell(const Vec2i &pos) const { return getCell(pos.x, pos.y); }
	Tile *getTile(int sx, int sy) const {
		assert(this->isInsideTile(sx, sy));
		return &tiles[sy*tileW+sx];
	}
	Tile *getTile(const Vec2i &sPos) const { return getTile(sPos.x, sPos.y); }

	int getW() const							{ return w;				}
	int getH() const							{ return h;				 }
	int getTileW() const						{ return tileW;			  }
	int getTileH() const						{ return tileH;			   }
	int getMaxPlayers() const					{ return maxPlayers;		}
	float getHeightFactor() const				{ return heightFactor;		 }
	float getAvgHeight() const					{ return avgHeight;			  }
	float getWaterLevel() const					{ return waterLevel;		   }
	Rect2i getBounds() const					{ return Rect2i(0,0, w,h);	    }
	Vec2i getStartLocation(int locNdx) const	{ return startLocations[locNdx]; }

	// these should be in their respective cell classes...
	bool getSubmerged(const Tile *sc) const		{return sc->getHeight()<waterLevel;}
	//bool getSubmerged(const Cell *c) const				{return c->getHeight()<waterLevel;}
	bool getDeepSubmerged(const Tile *sc) const	{return sc->getHeight()<waterLevel-(1.5f/heightFactor);}
	//bool getDeepSubmerged(const Cell *c) const			{return c->getHeight()<waterLevel-(1.5f/heightFactor);}
	//float getSurfaceHeight(const Vec2i &pos) const;

	const Earthquakes &getEarthquakes() const			{return earthquakes;}

	//is
	bool isInside(int x, int y) const					{return x >= 0 && y >= 0 && x < w && y < h;}
	bool isInside(const Vec2i &pos) const				{return isInside(pos.x, pos.y);}
	bool isInsideTile(int sx, int sy) const			{return sx >= 0 && sy >= 0 && sx < tileW && sy < tileH;}
	bool isInsideTile(const Vec2i &sPos) const		{return isInsideTile(sPos.x, sPos.y);}
	bool isResourceNear(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos) const;

	//free cells
	// This should just do a look up in the map metrics (currently maintained by the Cartographer object)
	// Is a cell of a given field 'free' to be occupied
	bool isFreeCell(const Vec2i &pos, Field field) const;

	bool areFreeCells(const Vec2i &pos, int size, Field field) const;
	bool areFreeCells ( const Vec2i &pos, int size, char *fieldMap ) const;

	bool isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const;
	bool areFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field, const Unit *unit) const;

	bool isFreeCellOrHaveUnits(const Vec2i &pos, Field field, const Selection::UnitContainer &units) const;
	bool areFreeCellsOrHaveUnits(const Vec2i &pos, int size, Field field, const Selection::UnitContainer &units) const;

	bool isAproxFreeCell(const Vec2i &pos, Field field, int teamIndex) const;
	bool areAproxFreeCells(const Vec2i &pos, int size, Field field, int teamIndex) const;

	void getOccupants(vector<Unit *> &results, const Vec2i &pos, int size, Zone field) const;

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
	void evict(Unit *unit, const Vec2i &pos, vector<Unit*> &evicted);

	//misc
	bool isNextTo(const Vec2i &pos, const Unit *unit) const;
	void clampPos(Vec2i &pos) const;

	void prepareTerrain(const Unit *unit);
	void flatternTerrain(const Unit *unit);

	void computeNormals();
	void computeInterpolatedHeights();

	void computeNormals(Rect2i range);
	void computeInterpolatedHeights(Rect2i range);

	void add(Earthquake *earthquake) 			{earthquakes.push_back(earthquake);}
	void update(float slice);

	//assertions
	#ifdef DEBUG
		void assertUnitCells(const Unit * unit);
	#else
		void assertUnitCells(const Unit * unit){}
	#endif

	//static
	static Vec2i toTileCoords(Vec2i cellPos)	{return cellPos / GameConstants::cellScale;}
	static Vec2i toUnitCoords(Vec2i tilePos)	{return tilePos * GameConstants::cellScale;}
	static string getMapPath(const string &mapName)	{return "maps/"+mapName+".gbm";}

private:
	//compute
	void smoothSurface();
	void computeNearSubmerged();
	void computeCellColors();
	void setCellTypes();
	void calcAvgAltitude();

	static void findNearest(Vec2i &result, const Vec2i &start, const Vec2i &candidate, fixed &minDistance);
	void findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, fixed &minDistance) const;
	void findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, fixed &minDistance, const Unit *unit) const;
};

}} //end namespace

#endif
