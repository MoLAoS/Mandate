// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Jaagup Repän <jrepan@gmail.com>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//            2009 James McCulloch <silnarm@gmail.com>
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

namespace Shared{ namespace Platform{
	class NetworkDataBuffer;
}}

namespace Glest{ namespace Game{

using Shared::Graphics::Vec4f;
using Shared::Graphics::Quad2i;
using Shared::Graphics::Rect2i;
using Shared::Graphics::Vec4f;
using Shared::Graphics::Vec2f;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Texture2D;
using Shared::Platform::NetworkDataBuffer;
using Glest::Game::Util::PosCircularIteratorFactory;

class Tileset;
class Unit;
class Resource;
class TechTree;
class World;
class UnitContainer;
class Earthquake;

// =====================================================
// 	class Cell
//
///	A map cell that holds info about units present on it
// =====================================================

class Cell{
private:
   Unit *units[ZoneCount];	//units on this cell
   float height;
   SurfaceType surfaceType;
   Cell(Cell&);
   void operator=(Cell&);

public:
   Cell() {
		memset(units, 0, sizeof(units));
		height= 0;
      surfaceType = SurfaceTypeLand;
	}

	// get
	Unit *getUnit (Zone zone) const		{return units[zone];}
   Unit *getUnit ( Field field ) {return getUnit(field==FieldAir?ZoneAir:ZoneSurface);}
	float getHeight () const				{return height;}
   SurfaceType getType () const { return surfaceType; }

   bool isSubmerged () const { return surfaceType != SurfaceTypeLand; }
   bool isDeepSubmerged () const { return surfaceType == SurfaceTypeDeepWater; }

   // set
   void setUnit ( Zone field, Unit *unit) {units[field]= unit;}
	void setHeight (float h) { height= h; }
   void setType ( SurfaceType type ) { surfaceType = type; }

   //misc
	bool isFree(Zone field) const;
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
	Tile();
	~Tile();

	//get
	const Vec3f &getVertex() const				{return vertex;}
	float getHeight() const						{return vertex.y;}
	const Vec3f &getColor() const				{return color;}
	const Vec3f &getNormal() const				{return normal;}
	int getTileType() const					{return tileType;}
	const Texture2D *getTileTexture() const	{return tileTexture;}
	Object *getObject() const					{return object;}
	Resource *getResource() const				{return object==NULL? NULL: object->getResource();}
	const Vec2f &getFowTexCoord() const			{return fowTexCoord;}
	const Vec2f &getSurfTexCoord() const		{return surfTexCoord;}
	bool getNearSubmerged() const				{return nearSubmerged;}

	bool isVisible(int teamIndex) const			{return visible[teamIndex];}
	bool isExplored(int teamIndex) const		{return explored[teamIndex];}

	//set
	void setVertex(const Vec3f &vertex)				{originalVertex = this->vertex = vertex;}
	void setHeight(float height)					{originalVertex.y = vertex.y = height;}
	void setNormal(const Vec3f &normal)				{this->normal= normal;}
	void setColor(const Vec3f &color)				{this->color= color;}
	void setTileType(int tileType)			{this->tileType= tileType;}
	void setTileTexture(const Texture2D *st)		{this->tileTexture= st;}
	void setObject(Object *object)					{this->object= object;}
	void setFowTexCoord(const Vec2f &ftc)			{this->fowTexCoord= ftc;}
	void setTileTexCoord(const Vec2f &stc)			{this->surfTexCoord= stc;}
	void setExplored(int teamIndex, bool explored)	{this->explored[teamIndex]= explored;}
	void setVisible(int teamIndex, bool visible)	{this->visible[teamIndex]= visible;}
	void setNearSubmerged(bool nearSubmerged)		{this->nearSubmerged= nearSubmerged;}

	//misc
	void deleteResource();
	bool isFree() const								{return object==NULL || object->getWalkable();}
	void resetVertex()								{vertex = originalVertex;}
	void alterVertex(const Vec3f &offset)			{vertex += offset;}
	void updateObjectVertex() {
		if(object) {
			object->setPos(vertex); // should be centered ???
		}
	}

	// I know it looks stupid using NetworkDataBuffer to save these, but then I
	// get my multi-byte values in platform portable format, so that saved game
	// files will work across platforms (especially when resuming an interrupted
	// network game).
	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;
};


// =====================================================
// 	class Map
//
///	Represents the game map (and loads it from a gbm file)
// =====================================================

class Map {
public:
	static const int cellScale;	//number of cells per tile
	static const int mapScale;	//horizontal scale of surface
	typedef vector<Earthquake*> Earthquakes;

private:
	string title;
	float waterLevel;
	float heightFactor;
	int w;
	int h;
	int tileW;
	int tileH;
	int maxPlayers;
	Cell *cells;
	Tile *tiles;
	Vec2i *startLocations;
	float *surfaceHeights;

	Earthquakes earthquakes;

private:
	Map(Map&);
	void operator=(Map&);

public:
	Map();
	~Map();

	void init();
	void load(const string &path, TechTree *techTree, Tileset *tileset);

	//get
	Cell *getCell(int x, int y) const;
	Cell *getCell(const Vec2i &pos) const;
	Tile *getTile(int sx, int sy) const;
	Tile *getTile(const Vec2i &sPos) const;

	int getW() const									{return w;}
	int getH() const									{return h;}
	int getTileW() const								{return tileW;}
	int getTileH() const								{return tileH;}
	int getMaxPlayers() const							{return maxPlayers;}
	float getHeightFactor() const						{return heightFactor;}
	float getWaterLevel() const							{return waterLevel;}
	Vec2i getStartLocation(int loactionIndex) const		{return startLocations[loactionIndex];}
	
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
   // This should just do a look up in the map metrics (currently maintained by the PathFinder object)
   // Is a cell of a given field 'free' to be occupied
   //bool isFreeCell(const Vec2i &pos, Zone field) const;
   bool isFreeCell(const Vec2i &pos, Field field) const;

	//bool areFreeCells(const Vec2i &pos, int size, Zone field) const;
   bool areFreeCells(const Vec2i &pos, int size, Field field) const;
   bool areFreeCells ( const Vec2i &pos, int size, char *fieldMap ) const;

   bool isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const;
	bool areFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field, const Unit *unit) const;

   bool isFreeCellOrHaveUnits(const Vec2i &pos, Field field, const Selection::UnitContainer &units) const;
	bool areFreeCellsOrHaveUnits(const Vec2i &pos, int size, Field field, const Selection::UnitContainer &units) const;

   bool isAproxFreeCell(const Vec2i &pos, Field field, int teamIndex) const;
	bool areAproxFreeCells(const Vec2i &pos, int size, Field field, int teamIndex) const;

   void getOccupants(vector<Unit *> &results, const Vec2i &pos, int size, Zone field) const;
	//bool isFreeCell(const Vec2i &pos, Field field) const;

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
//	bool aproxCanMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const;
	//bool canMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const;
    void putUnitCells(Unit *unit, const Vec2i &pos);
	void clearUnitCells(Unit *unit, const Vec2i &pos);
	void evict(Unit *unit, const Vec2i &pos, vector<Unit*> &evicted);

	//misc
	bool isNextTo(const Vec2i &pos, const Unit *unit) const;
	void clampPos(Vec2i &pos) const;

	void prepareTerrain(const Unit *unit);
	void flatternTerrain(const Unit *unit);

	//void flattenTerrain(const Unit *unit);
	void computeNormals(Rect2i range = Rect2i(0, 0, 0, 0));
	void computeInterpolatedHeights(Rect2i range = Rect2i(0, 0, 0, 0));
	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;

	void add(Earthquake *earthquake) 			{earthquakes.push_back(earthquake);}
	void update(float slice);

	//assertions
	#ifdef DEBUG
		void assertUnitCells(const Unit * unit);
	#else
		void assertUnitCells(const Unit * unit){}
	#endif

	//static
	static Vec2i toTileCoords(Vec2i unitPos)	{return unitPos/cellScale;}
	static Vec2i toUnitCoords(Vec2i surfPos)	{return surfPos*cellScale;}
   static string getMapPath(const string &mapName)	{return "maps/"+mapName+".gbm";}

private:
	//compute
	void smoothSurface();
	void computeNearSubmerged();
	void computeCellColors();
   void setCellTypes ();
   //void setCellType ( Vec2i pos );

	static void findNearest(Vec2i &result, const Vec2i &start, const Vec2i &candidate, float &minDistance);
	void findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, float &minDistance) const;
	void findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, float &minDistance, const Unit *unit) const;
};


// ===============================
// 	class PosCircularIteratorOrdered
// ===============================
/**
 * A position circular iterator who's results are garaunteed to be ordered by distance from the
 * center.  This iterator is slightly slower than PosCircularIteratorSimple, but can produce faster
 * calculations when either the nearest or furthest of a certain object is desired and the loop
 * can termainte as soon as a match is found, rather than iterating through all possibilities and
 * then calculating the nearest or furthest later.
 */
class PosCircularIteratorOrdered {
private:
	const Map &map;
	Vec2i center;
	Glest::Game::Util::PosCircularIterator *i;

public:
	PosCircularIteratorOrdered(const Map &map, const Vec2i &center,
			Glest::Game::Util::PosCircularIterator *i) : map(map), center(center), i(i) {} 
	~PosCircularIteratorOrdered() {
		delete i;
	}

	bool getNext(Vec2i &result) {
		Vec2i offset;
		do {
			if(!i->getNext(offset)) {
				return false;
			}
			result = center + offset;
		} while(!map.isInside(result));

		return true;
	}

	bool getPrev(Vec2i &result) {
		Vec2i offset;
		do {
			if(!i->getPrev(offset)) {
				return false;
			}
			result = center + offset;
		} while(!map.isInside(result));

		return true;
	}

	bool getNext(Vec2i &result, float &dist) {
		Vec2i offset;
		do {
			if(!i->getNext(offset, dist)) {
				return false;
			}
			result = center + offset;
		} while(!map.isInside(result));

		return true;
	}

	bool getPrev(Vec2i &result, float &dist) {
		Vec2i offset;
		do {
			if(!i->getPrev(offset, dist)) {
				return false;
			}
			result = center + offset;
		} while(!map.isInside(result));

		return true;
	}
};
// ===============================
// 	class PosCircularIteratorSimple
// ===============================
/**
 * A position circular iterator that is more primitive and light weight than the
 * PosCircularIteratorOrdered class.  It's best used when order is not important as it uses less CPU
 * cycles for a full iteration than PosCircularIteratorOrdered (and makes code smaller).
 */
class PosCircularIteratorSimple {
private:
	const Map &map;
	Vec2i center;
	int radius;
	Vec2i pos;

public:
	PosCircularIteratorSimple(const Map &map, const Vec2i &center, int radius);
	bool getNext(Vec2i &result, float &dist) {	
		//iterate while dont find a cell that is inside the world
		//and at less or equal distance that the radius
		do {
			pos.x++;
			if (pos.x > center.x + radius) {
				pos.x = center.x - radius;
				pos.y++;
			}
			if (pos.y > center.y + radius) {
				return false;
			}
			result = pos;
			dist = pos.dist(center);
		} while (floor(dist) >= (radius + 1) || !map.isInside(pos));
		//while(!(pos.dist(center) <= radius && map.isInside(pos)));
	
		return true;
	}

	const Vec2i &getPos() {
		return pos;
	}
};

/*
class PosCircularIterator{
private:
	Vec2i center;
	int radius;
	const Map *map;
	Vec2i pos;

public:
	PosCircularIterator(const Map *map, const Vec2i &center, int radius);
	bool next(){

		//iterate while dont find a cell that is inside the world
		//and at less or equal distance that the radius
		do{
			pos.x++;
			if(pos.x > center.x+radius){
				pos.x= center.x-radius;
				pos.y++;
			}
			if(pos.y>center.y+radius)
				return false;
		}
		while(floor(pos.dist(center)) >= (radius+1) || !map->isInside(pos));
		//while(!(pos.dist(center) <= radius && map->isInside(pos)));

		return true;
	}

	const Vec2i &getPos(){
		return pos;
	}
};
*/
// ==================================
// 	class PosCircularInsideOutIterator
// ==================================

/* *
 * A position iterator which traverses (mostly) from the nearest points to the center to the
 * furthest.  The "mostly" is because it may return a position that is more diagonal before
 * returning a position that is less diagonal, but one step further out on either the x or y
 * axis.  None the less, the behavior of this iterator should be sufficient for targeting
 * purposes where small differences in distance will not have a negative effect and are acceptable
 * to reduce CPU cycles.
 */
 /*
class PosCircularInsideOutIterator {
private:
	Vec2i center;
	int radius;
	const Map *map;
	Vec2i pos;
	int step;
	int off;
	int cycle;
	bool cornerOutOfRange;

public:
	PosCircularInsideOutIterator(const Map *map, const Vec2i &center, int radius);
	
	bool isOutOfRange(const Vec2i &p) {
		return p.length() > radius;
	}

	bool next() {
		do {
			if(off ? cycle == 7 : cycle == 3) {
				cycle = 0;
				if(off && (off == step || (cornerOutOfRange && isOutOfRange(Vec2i(step, off))))) {
					if(step > radius) {
						return false;
					} else {
						++step;
						off = 0;
						cornerOutOfRange = isOutOfRange(Vec2i(step));
					}
				}
			} else {
				++cycle;
			}
			pos = center;
			switch(cycle) {
				case 0: pos += Vec2i( off,  -step);	break;
				case 1: pos += Vec2i( step,  off);	break;
				case 2: pos += Vec2i( off,   step);	break;
				case 3: pos += Vec2i(-step,  off);	break;
				case 4: pos += Vec2i(-off,  -step);	break;
				case 5: pos += Vec2i( step, -off);	break;
				case 7: pos += Vec2i(-off,   step);	break;
				case 8: pos += Vec2i(-step, -off);	break;
			}
		} while(!map->isInside(pos));

		return true;
	}

	bool next(float &dist) {
		do {
			if(off ? cycle == 7 : cycle == 3) {
				cycle = 0;
				dist = Vec2i(step, off).length();
				if(off == step || dist > radius) {
					if(step > radius) {
						return false;
					} else {
						++step;
						off = 0;
					}
				}
			} else {
				++cycle;
			}
			pos = center;
			switch(cycle) {
				case 0: pos += Vec2i( off,  -step);	break;
				case 1: pos += Vec2i( step,  off);	break;
				case 2: pos += Vec2i( off,   step);	break;
				case 3: pos += Vec2i(-step,  off);	break;
				case 4: pos += Vec2i(-off,  -step);	break;
				case 5: pos += Vec2i( step, -off);	break;
				case 7: pos += Vec2i(-off,   step);	break;
				case 8: pos += Vec2i(-step, -off);	break;
			}
		} while(!map->isInside(pos));

		return true;
	}

	const Vec2i &getPos() {
		return pos;
	}
};
*/

// ===============================
// 	class PosQuadIterator
// ===============================

class PosQuadIterator {
private:
	Quad2i quad;
	int step;
	Rect2i boundingRect;
	Vec2i pos;

public:
	PosQuadIterator(const Quad2i &quad, int step = 1);

	bool next() {
		do {
			pos.x += step;
			if (pos.x > boundingRect.p[1].x) {
				pos.x = (boundingRect.p[0].x / step) * step;
				pos.y += step;
			}
			if (pos.y > boundingRect.p[1].y)
				return false;
		} while (!quad.isInside(pos));

		return true;
	}

	void skipX() {
		pos.x += step;
	}

	const Vec2i &getPos() {
		return pos;
	}
};

//////////////////////////////////////////////////////////////////
// Cut Here 
//////////////////////////////////////////////////////////////////
// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Jaagup Repän <jrepan@gmail.com>,
//				     2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

/*
#ifndef _GLEST_GAME_EARTHQUAKE_H_
#define _GLEST_GAME_EARTHQUAKE_H_

#include <cassert>
#include <map>

#include "vec.h"
#include "math_util.h"
#include "command_type.h"
#include "logger.h"
#include "object.h"
#include "game_constants.h"
#include "selection.h"
#include "exceptions.h"
#include "pos_iterator.h"

namespace Glest{ namespace Game{

using Shared::Graphics::Vec4f;
using Shared::Graphics::Quad2i;
using Shared::Graphics::Rect2i;
using Shared::Graphics::Vec4f;
using Shared::Graphics::Vec2f;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Texture2D;
using Shared::Platform::NetworkDataBuffer;
using Glest::Game::Util::PosCircularIteratorFactory;

class Tileset;
class Unit;
class Resource;
class TechTree;
class World;
class UnitContainer;
*/
class EarthquakeType {
private:
	float magnitude;			/** max variance in height */
	float frequency;			/** oscilations per second */
	float speed;				/** speed siesmic waves travel per second (in surface cells) */
	float duration;				/** duration in seconds */
	float radius;				/** radius in surface cells */
	float initialRadius;		/** radius of area of initial activity (in surface cells) */
	float shiftsPerSecond;		/** number of sudden shifts per second */
	float shiftsPerSecondVar;	/** variance of shiftsPerSecond (+-) */
	Vec3f shiftLengthMult;		/** multiplier of magnitude to determine max length of each shift */
	float maxDps;				/** maxiumum damage per second caused */
	const AttackType *attackType;/** the damage type to use for damage reports */
	bool affectsAllies;			/** rather or not this earthquake affects allies */
	StaticSound *sound;


public:
	EarthquakeType(float maxDps, const AttackType *attackType);
	void load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	void spawn(Map &map, Unit *cause, const Vec2i &epicenter, float strength) const;

	float getMagnitude() const				{return magnitude;}
	float getFrequency() const				{return frequency;}
	float getSpeed() const					{return speed;}
	float getDuration() const				{return duration;}
	float getRadius() const					{return radius;}
	float getInitialRadius() const			{return initialRadius;}
	float getshiftsPerSecond() const		{return shiftsPerSecond;}
	float getshiftsPerSecondVar() const		{return shiftsPerSecondVar;}
	const Vec3f &getshiftLengthMult() const	{return shiftLengthMult;}
	float getMaxDps() const					{return maxDps;}
	const AttackType *getAttackType() const	{return attackType;}
	bool isAffectsAllies() const			{return affectsAllies;}
	StaticSound *getSound() const			{return sound;}
};

class Earthquake {
public:
	class UnitImpact {
	public:
		int count;		/** total number of cells unit occupies that are affected */
		float intensity;/** total intensity (average = intensity / count) */

		UnitImpact() : count(0), intensity(0.f) {}
		void impact(float intensity)			{++count; this->intensity += intensity;}
	};
	typedef std::map<Unit *, UnitImpact> DamageReport;

private:
	Map &map;
	UnitReference cause;	/** unit that caused this earthquake */
	const EarthquakeType *type; /** the type */
	Vec2i epicenter;		/** epicenter in surface coordinates */
	float magnitude;		/** max variance in height */
	float frequency;		/** oscilations per second */
	float speed;			/** unit cells traveled per second */
	float duration;			/** duration in seconds */
	float radius;			/** radius in surface cells */
	float initialRadius;	/** radius of area of initial activity (in surface cells) */
	float progress;			/** 0 when started, 1 when complete at epicenter (continues longer for waves to even out) */
	Rect2i bounds;			/** surface cell area effected */
	Rect2i uBounds;			/** unit cell area effected */
	float nextShiftTime;	/** when the shift should occurred */
	Vec3f currentShift;		/** the current xyz offset caused by seismic shift (before magnitude multiplier) */
	Vec2i size;
	size_t cellCount;
	Random random;

public:
	Earthquake(Map &map, Unit *cause, const Vec2i &epicenter, const EarthquakeType* type, float strength);

	Unit *getCause() const					{return cause;}
	const EarthquakeType *getType() const	{return type;};
	const Vec2i getEpicenter() const		{return epicenter;}
	void update(float slice);
	void getDamageReport(DamageReport &results, float slice);
   bool isDone() { return progress >= 1.f + radius / speed / duration; };
	void resetSurface();

private:
	float temporalMagnitude(float temporalProgress);
	float temporalPressure(float time, float &tm);
	void calculateEffectiveValues(float &effectiveTime, float &effectiveMagnitude, const Vec2i &surfacePos, float time);
};


//}} //end namespace

//#endif

//////////////////////////////////////////////////////////////////
// Cut Here 
//////////////////////////////////////////////////////////////////


}} //end namespace

#endif
