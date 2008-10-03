// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Jaagup Repän <jrepan@gmail.com>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
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

#include "vec.h"
#include "math_util.h"
#include "command_type.h"
#include "logger.h"
#include "object.h"
#include "game_constants.h"
#include "selection.h"

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

class Tileset;
class Unit;
class Resource;
class TechTree;
class World;
class UnitContainer;

// =====================================================
// 	class Cell
//
///	A map cell that holds info about units present on it
// =====================================================

class Cell{
private:
    Unit *units[fCount];	//units on this cell
	float height;

private:
	Cell(Cell&);
	void operator=(Cell&);

public:
	Cell() {
		memset(units, 0, sizeof(units));
		height= 0;
	}

	//get
	Unit *getUnit(int field) const		{return units[field];}
	float getHeight() const				{return height;}

	void setUnit(int field, Unit *unit)	{units[field]= unit;}
	void setHeight(float height)		{this->height= height;}

	//misc
	bool isFree(Field field) const;
};

// =====================================================
// 	class SurfaceCell
//
//	A heightmap cell, each surface cell is composed by more than one Cell
// =====================================================

class SurfaceCell{
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
	int surfaceType;
    const Texture2D *surfaceTexture;

	//object & resource
	Object *object;

	//visibility
	bool visible[GameConstants::maxPlayers];
    bool explored[GameConstants::maxPlayers];

	//cache
	bool nearSubmerged;

public:
	SurfaceCell();
	~SurfaceCell();

	//get
	const Vec3f &getVertex() const				{return vertex;}
	float getHeight() const						{return vertex.y;}
	const Vec3f &getColor() const				{return color;}
	const Vec3f &getNormal() const				{return normal;}
	int getSurfaceType() const					{return surfaceType;}
	const Texture2D *getSurfaceTexture() const	{return surfaceTexture;}
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
	void setSurfaceType(int surfaceType)			{this->surfaceType= surfaceType;}
	void setSurfaceTexture(const Texture2D *st)		{this->surfaceTexture= st;}
	void setObject(Object *object)					{this->object= object;}
	void setFowTexCoord(const Vec2f &ftc)			{this->fowTexCoord= ftc;}
	void setSurfTexCoord(const Vec2f &stc)			{this->surfTexCoord= stc;}
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
			object->setPos(vertex);
		}
	}

	// I know it looks stupid using NetworkDataBuffer to save these, but then I
	// get my multi-byte values in platform portable format, so that saved game
	// files will work across platforms (especially when resuming an interrupted
	// network game).
	void read(NetworkDataBuffer &buf);

	/**
	 * Saves data for this surface cell.  Data written varies bewteen 1 and 7 bytes as described
	 * below.
	 * <ul>
	 * <li> 4 bits: explored flags</li>
	 * <li> 4 bits: object exists?, object has type?, has resource? amount differs from default?</li>
	 * <li> 1 byte: object type class (if exists)</li>
	 * <li> 1 byte: resource id (if exists)</li>
	 * <li> 4 bytes: resource amount (if != default)</li>
	 * </ul>
	 */
	void write(NetworkDataBuffer &buf) const;
};

class EarthquakeType {
private:
	float magnitude;			/// max variance in height
	float frequency;			/// oscilations per second
	float speed;				/// speed siesmic waves travel per second (in surface cells)
	float duration;				/// duration in seconds
	float radius;				/// radius in surface cells
	float initialRadius;		/// radius of area of initial activity (in surface cells)
	float shiftsPerSecond;		/// number of sudden shifts per second
	float shiftsPerSecondVar;	/// variance of shiftsPerSecond (+-)
	Vec3f shiftLengthMult;		/// multiplier of magnitude to determine max length of each shift
	float maxDps;
	const AttackType *attackType;
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
	StaticSound *getSound() const			{return sound;}
};

class Earthquake {
public:
	class UnitImpact {
	public:
		int count;
		float intensity;

		UnitImpact() : count(0), intensity(0.f) {}
		void impact(float intensity)			{++count; this->intensity += intensity;}
	};
	typedef std::map<Unit *, UnitImpact> DamageReport;

private:
	Map &map;
	UnitReference cause;
	const EarthquakeType *type;
	Vec2i epicenter;		// epicenter in surface coordinates
	float magnitude;		// max variance in height
	float frequency;		// oscilations per second
	float speed;			// unit cells traveled per second
	float duration;			// duration in seconds
	float radius;			// radius in surface cells
	float initialRadius;	// radius of area of initial activity (in surface cells)
	float progress;			// 0 when started, 1 when complete at epicenter (continues longer for waves to even out)
	Rect2i bounds;			// surface cell area effected
	Rect2i uBounds;			// unit cell area effected;
	float nextShiftTime;	// when the shift should occurred
	Vec3f currentShift;		// the current xyz offset caused by seismic shift (before magnitude multiplier)
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
	bool isDone();
	void resetSurface();

private:
	float temporalMagnitude(float temporalProgress);
	float temporalPressure(float time, float &tm);
	void calculateEffectiveValues(float &effectiveTime, float &effectiveMagnitude, const Vec2i &surfacePos, float time);
};

// =====================================================
// 	class Map
//
///	Represents the game map (and loads it from a gbm file)
// =====================================================

class Map{
public:
	static const int cellScale;	//number of cells per surfaceCell
	static const int mapScale;	//horizontal scale of surface
	typedef vector<Earthquake*> Earthquakes;

private:
	string title;
	float waterLevel;
	float heightFactor;
	int w;
	int h;
	int surfaceW;
	int surfaceH;
	int maxPlayers;
	Cell *cells;
	SurfaceCell *surfaceCells;
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
	Cell *getCell(int x, int y) const					{assert(isInside(x, y)); return &cells[y * w + x];}
	Cell *getCell(const Vec2i &pos) const				{assert(isInside(pos.x, pos.y)); return getCell(pos.x, pos.y);}
	SurfaceCell *getSurfaceCell(int sx, int sy) const	{assert(isInsideSurface(sx, sy)); return &surfaceCells[sy*surfaceW+sx];}
	SurfaceCell *getSurfaceCell(const Vec2i &sPos) const{assert(isInsideSurface(sPos.x, sPos.y)); return getSurfaceCell(sPos.x, sPos.y);}
	int getW() const									{return w;}
	int getH() const									{return h;}
	int getSurfaceW() const								{return surfaceW;}
	int getSurfaceH() const								{return surfaceH;}
	int getMaxPlayers() const							{return maxPlayers;}
	float getHeightFactor() const						{return heightFactor;}
	float getWaterLevel() const							{return waterLevel;}
	Vec2i getStartLocation(int loactionIndex) const		{return startLocations[loactionIndex];}
	bool getSubmerged(const SurfaceCell *sc) const		{return sc->getHeight()<waterLevel;}
	bool getSubmerged(const Cell *c) const				{return c->getHeight()<waterLevel;}
	bool getDeepSubmerged(const SurfaceCell *sc) const	{return sc->getHeight()<waterLevel-(1.5f/heightFactor);}
	bool getDeepSubmerged(const Cell *c) const			{return c->getHeight()<waterLevel-(1.5f/heightFactor);}
	float getSurfaceHeight(const Vec2i &pos) const;
	const Earthquakes &getEarthquakes() const			{return earthquakes;}

	//is
	bool isInside(int x, int y) const					{return x >= 0 && y >= 0 && x < w && y < h;}
	bool isInside(const Vec2i &pos) const				{return isInside(pos.x, pos.y);}
	bool isInsideSurface(int sx, int sy) const			{return sx >= 0 && sy >= 0 && sx < surfaceW && sy < surfaceH;}
	bool isInsideSurface(const Vec2i &sPos) const		{return isInsideSurface(sPos.x, sPos.y);}
	bool isResourceNear(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos) const;

	//free cells
	bool isFreeCell(const Vec2i &pos, Field field) const;
	bool isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const;
	bool isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Selection::UnitContainer &units) const;
	bool isAproxFreeCell(const Vec2i &pos, Field field, int teamIndex) const;
	bool isFreeCells(const Vec2i &pos, int size, Field field) const;
	bool isFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field, const Unit *unit) const;
	bool isFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field, const Selection::UnitContainer &units) const;
	bool isAproxFreeCells(const Vec2i &pos, int size, Field field, int teamIndex) const;
	void getOccupants(vector<Unit *> &results, const Vec2i &pos, int size, Field field) const;

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
	bool aproxCanMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const;
	bool canMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const;
    void putUnitCells(Unit *unit, const Vec2i &pos);
	void clearUnitCells(Unit *unit, const Vec2i &pos);
	void evict(Unit *unit, const Vec2i &pos, vector<Unit*> &evicted);

	//misc
	bool isNextTo(const Vec2i &pos, const Unit *unit) const;
	void clampPos(Vec2i &pos) const{
		if(pos.x<0){
			pos.x=0;
		}
		if(pos.y<0){
			pos.y=0;
		}
		if(pos.x>=w){
			pos.x=w-1;
		}
		if(pos.y>=h){
			pos.y=h-1;
		}
	}

	void prepareTerrain(const Unit *unit);
	void flatternTerrain(const Unit *unit);
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
	static Vec2i toSurfCoords(Vec2i unitPos)	{return unitPos/cellScale;}
	static Vec2i toUnitCoords(Vec2i surfPos)	{return surfPos*cellScale;}

private:
	//compute
	void smoothSurface();
	void computeNearSubmerged();
	void computeCellColors();

	static void findNearest(Vec2i &result, const Vec2i &start, const Vec2i &candidate, float &minDistance);
	void findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, float &minDistance) const;
	void findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, float &minDistance, const Unit *unit) const;
};


// ===============================
// 	class PosCircularIterator
// ===============================

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

// ===============================
// 	class PosQuadIterator
// ===============================

class PosQuadIterator{
private:
	Quad2i quad;
	Rect2i boundingRect;
	const Map *map;
	Vec2i pos;
	int step;
public:
	PosQuadIterator(const Map *map, const Quad2i &quad, int step=1);

	bool next(){

		do{
			pos.x+= step;
			if(pos.x > boundingRect.p[1].x){
				pos.x= (boundingRect.p[0].x/step)*step;
				pos.y+= step;
			}
			if(pos.y>boundingRect.p[1].y)
				return false;
		}
		while(!quad.isInside(pos));

		return true;
	}

	void skipX(){
		pos.x+= step;
	}

	const Vec2i &getPos(){
		return pos;
	}
};

}} //end namespace

#endif
