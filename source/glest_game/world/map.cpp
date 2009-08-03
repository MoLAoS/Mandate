// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa,
//				  2008 Jaagup Rep�n <jrepan@gmail.com>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//            2009 James McCulloch <silnarm@gmail.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "map.h"
//#include "earthquake.h"

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

#include "leak_dumper.h"

//#ifndef M_PI
//#define M_PI 3.14159265
//#endif

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Cell
// =====================================================


// ==================== misc ====================

//returns if the cell is free
bool Cell::isFree(Zone field) const {
	return getUnit(field)==NULL || getUnit(field)->isPutrefacting();
}

// =====================================================
// 	class Tile
// =====================================================

Tile::Tile() :
		vertex(0.f),
		normal(0.f, 1.f, 0.f),
		originalVertex(0.f),
		tileType(-1),
		tileTexture(NULL),
		object(NULL) {
}

Tile::~Tile(){
	if(object) {
		delete object;
	}
}

void Tile::deleteResource() {
	delete object;
	object= NULL;
}

/*
This version of the function does it the right way by creating the new object if it doesn't exist,
etc.
void Tile::load(const XmlNode *node, World *world) {
	XmlNode *n;
	Vec2i pos;
	pos.x = node->getAttribute("x")->getIntValue();
	pos.y = node->getAttribute("y")->getIntValue();
	n = node->getChild("object", 0, false);
	if (n) {
		int objectClass = n->getAttribute("value")->getIntValue();
		ObjectType *objectType = world->getTileset()->getObjectType(objectClass);
		object = new Object(objectType, vertex);
	}
	n = node->getChild("resource", 0, false);
	if (n) {
		if (!object) {
			object = new Object(NULL, vertex);
		}
		object->setResource(world->getTechTree()->getResourceType(n->getAttribute("type")->getValue()), world->getMap()->toUnitCoords(pos));
		object->getResource()->setAmount(n->getAttribute("amount")->getIntValue());
	}
	n = node->getChild("explored", 0, false);
	if (n) {
		for (int i = 0; i < n->getChildCount(); i++) {
			explored[n->getChildIntValue("team")] = true;
		}
	}
}
*/
/** Reads saved data for this cell from a binary source. */
void Tile::read(NetworkDataBuffer &buf) {
	uint8 a;
	buf.read(a);
	explored[0] = a & 0x01;
	explored[1] = a & 0x02;
	explored[2] = a & 0x04;
	explored[3] = a & 0x08;

	if(a & 0x10) {
		/* We're not using object or resource type right now, but leave them in comments in case
		   we do later.
		if(a & 0x20) {
			uint8 objectType;
			buf.read(objectType);
		}

		if(a & 0x40) {
			uint8 resourceTypeId;
			buf.read(resourceTypeId);

			if(a & 0x80) {
				uint32 resourceAmount;
				buf.read(resourceAmount);
				assert(object && object->getResource());
				if(object && object->getResource()) {
					object->getResource()->setAmount(resourceAmount);
				}
			}
		}*/

		// If there is an object, with a resource and that amount differs from the default
		if((a & 0x60) == 0x60) {
			// sanity check, let's have a real exception rather than crash if these don't match.
			if(!object) {
				throw runtime_error("Expected Tile at " + floatToStr(vertex.x) + ", "
						+ floatToStr(vertex.z) + " to have an object, but it did not.  This "
						"probably means that you modified the map since this game was saved or if "
						"this is a network game, that you have a different map than the server.");
			}
			if(!object->getResource()) {
				throw runtime_error("Expected Tile at " + floatToStr(vertex.x) + ", "
						+ floatToStr(vertex.z) + " to have a resource, but it did not.  This "
						"probably means that you modified the map since this game was saved or if "
						"this is a network game, that you have a different map than the server.");
			}

			if(a & 0x80) {
				uint32 amount32;
				buf.read(amount32);
				object->getResource()->setAmount(amount32);
			} else {
				uint16 amount16;
				buf.read(amount16);
				object->getResource()->setAmount(amount16);
			}
		}
	} else if(object) {
		delete object;
		object = NULL;
	}
}

/**
 * Saves data for this surface cell. No bit left behind.
 * byte 0:
 *   bits 0-3: explored states for each team (only 4 teams supported)
 *   bit 4: high if there is an object
 *   bit 5: high if object has a resource
 *   bit 6: high if resource amount differs from default (amount coming in next bytes)
 *   bit 7: high if amount is 32 bits, low if amount is 16 bits.
 * bytes 1-4 (optional, depending upon byte 0, bits 6 and 7):
 */
void Tile::write(NetworkDataBuffer &buf) const {
	uint8 a = (explored[0] ? 0x01 : 0)
			| (explored[1] ? 0x02 : 0)
			| (explored[2] ? 0x04 : 0)
			| (explored[3] ? 0x08 : 0);
//	uint8 objectType = 0;
//	uint8 resourceTypeId = 0;
	uint32 amount32 = 0;
	if(object) {
		a = a | 0x10;
//		const ObjectType *ot = object->getType();
//		if(ot) {
//			objectType = ot->getClass();
//			a = a | 0x20;
//		}
		Resource *r = object->getResource();
		if(r) {
			a = a | 0x20;
			const ResourceType *rt = r->getType();
//			resourceTypeId = rt->getId();
			amount32 = r->getAmount();
			if(amount32 != rt->getDefResPerPatch()) {
				a = a | 0x40;
				if(amount32 > USHRT_MAX) {
					a = a | 0x80;
				}
			}
		}
	}
	buf.write(a);

	/*	We have no reason to store these right now, so let's leave them out but keep them in
		comments for now in case we decide we need it for something in the near future.
	if(a & 0x20) {
		buf.write(objectType);
	}

	if(a & 0x40) {
		buf.write(resourceTypeId);
	}
	*/

	// If there is an object, with a resource and that amount differs from the default
	if((a & 0x70) == 0x70) {
		// do we need 32 bits or can we use 16?
		if(a & 0x80) {
			buf.write(amount32);
		} else {
			uint16 amount16 = (uint16)amount32;
			buf.write(amount16);
		}
	}
}


// =====================================================
// 	class Map
// =====================================================

// ===================== PUBLIC ========================

const int Map::cellScale = 2;
const int Map::mapScale = 2;

Map::Map() :
		title(),
		waterLevel(0.f),
		heightFactor(0.f),
		w(0),
		h(0),
		tileW(0),
		tileH(0),
		maxPlayers(0),
		cells(NULL),
		tiles(NULL),
		startLocations(NULL),
		surfaceHeights(NULL),
		earthquakes() {

	// If this is expanded, maintain Tile::read() and write()
	assert(Tileset::objCount < 256);

	// same as above
	assert(GameConstants::maxPlayers == 4);
}

Map::~Map() {
	Logger::getInstance().add("Cells", true);

	if(cells)			{delete[] cells;}
	if(tiles)	{delete[] tiles;}
	if(startLocations)	{delete[] startLocations;}
	if (surfaceHeights)	{delete[] surfaceHeights;}
}
//get
Cell *Map::getCell(int x, int y) const {
	assert ( this->isInside ( x,y ) );
   return &cells[y * w + x];
}
Cell *Map::getCell(const Vec2i &pos) const {
   return getCell(pos.x, pos.y);
}
Tile *Map::getTile(int sx, int sy) const { 
	assert ( this->isInsideTile ( sx,sy ) );
	return &tiles[sy*tileW+sx];
}
Tile *Map::getTile(const Vec2i &sPos) const {  
   return getTile(sPos.x, sPos.y);
}
void Map::load(const string &path, TechTree *techTree, Tileset *tileset) {
	FILE *f = NULL;

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
		if (!(f = fopen(path.c_str(), "rb"))) {
			throw runtime_error("Can't open file");
		}

		//read header
		MapFileHeader header;
		
		// FIXME: Better error handling starting here: report bad map instead of partially loading,
		// etc.
		fread(&header, sizeof(MapFileHeader), 1, f);

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
		w = tileW * cellScale;
		h = tileH * cellScale;


		//start locations
		startLocations = new Vec2i[maxPlayers];
		for (int i = 0; i < maxPlayers; ++i) {
			int x, y;
			fread(&x, sizeof(int32), 1, f);
			fread(&y, sizeof(int32), 1, f);
			startLocations[i] = Vec2i(x, y) * cellScale;
		}


		//cells
		cells = new Cell[w * h];
		tiles = new Tile[tileW * tileH];
		surfaceHeights = new float[tileW * tileH];

		float *surfaceHeight = surfaceHeights;

		//read heightmap
		for (int y = 0; y < tileH; ++y) {
			for (int x = 0; x < tileW; ++x) {
				float32 alt;
				fread(&alt, sizeof(float32), 1, f);
				Tile *sc = getTile(x, y);
				sc->setVertex(Vec3f((float)(x * mapScale), alt / heightFactor, (float)(y * mapScale)));
			}
		}

		//read surfaces
		for (int y = 0; y < tileH; ++y) {
			for (int x = 0; x < tileW; ++x) {
				int8 surf;
				fread(&surf, sizeof(int8), 1, f);
				getTile(x, y)->setTileType(surf - 1);
			}
		}

		//read objects and resources
		for (int y = 0; y < h; y += cellScale) {
			for (int x = 0; x < w; x += cellScale) {

				int8 objNumber;
				fread(&objNumber, sizeof(int8), 1, f);
				Tile *sc = getTile(toTileCoords(Vec2i(x, y)));
				if (objNumber == 0) {
					sc->setObject(NULL);
				} else if (objNumber <= Tileset::objCount) {
					Object *o = new Object(tileset->getObjectType(objNumber - 1), sc->getVertex());
					sc->setObject(o);
					for (int i = 0; i < techTree->getResourceTypeCount(); ++i) {
						const ResourceType *rt = techTree->getResourceType(i);
						if (rt->getClass() == rcTileset && rt->getTilesetObject() == objNumber) {
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

		fclose(f);
	} catch (const exception &e) {
		if (f) {
			fclose(f);
		}
		throw MapException(path, "Error loading map: " + path + "\n" + e.what());
	}
}

void Map::init() {
	Logger::getInstance().add("Heightmap computations", true);
	smoothSurface();
	computeNormals();
	computeInterpolatedHeights();
	computeNearSubmerged();
	computeCellColors();
	setCellTypes ();
}
void Map::setCellTypes ()
{
   for ( int y = 0; y < getH(); ++y )
      for ( int x = 0; x < getW(); ++x )
      {
         Cell *cell = getCell ( x, y );
         if ( cell->getHeight () < waterLevel - ( 1.5f / heightFactor ) )
            cell->setType ( SurfaceTypeDeepWater );
         else if ( cell->getHeight () < waterLevel )
            cell->setType ( SurfaceTypeFordable );
      }
}
/*
void Map::setCellType ( Vec2i pos )
{
   Cell *cell = getCell ( pos );
   if ( cell->getHeight () < waterLevel - ( 1.5f / heightFactor ) )
      cell->setType ( SurfaceTypeDeepWater );
   else if ( cell->getHeight () < waterLevel )
      cell->setType ( SurfaceTypeFordable );
   else
      cell->setType ( SurfaceTypeLand );
}
*/
void Map::read(NetworkDataBuffer &buf) {

	for(int y = 0; y < tileH; ++y) {
		for(int x = 0; x < tileW; ++x) {
			getTile(x, y)->read(buf);
		}
	}
/*
	int thisTeam = world->getThisTeamIndex();
	Minimap *minimap = (Minimap *)world->getMinimap();

	// smooth edges of fow alpha
	for(int y = 0; y < tileH; ++y) {
		for(int x = 0; x < tileW; ++x) {
			if(!getTile(x, y)->isExplored(thisTeam)) {
				continue;
			}
			// count the number of adjacent cells that are explored, allocating .5 for diagonal
			// matches with a total max of 6.0
			float adjacent =
					  (x + 1 < tileW	&& getTile(x + 1, y)->isExplored(thisTeam) ? 1.f : 0.0f)
					+ (x - 1 >= 0		&& getTile(x - 1, y)->isExplored(thisTeam) ? 1.f : 0.0f)
					+ (y + 1 < tileH	&& getTile(x, y + 1)->isExplored(thisTeam) ? 1.f : 0.0f)
					+ (y - 1 >= 0		&& getTile(x, y - 1)->isExplored(thisTeam) ? 1.f : 0.0f)
					+ (x + 1 < tileW	&& y + 1 < tileH	&& getTile(x + 1, y + 1)->isExplored(thisTeam) ? 0.5f : 0.0f)
					+ (x + 1 < tileW	&& y - 1 >= 0		&& getTile(x + 1, y - 1)->isExplored(thisTeam) ? 0.5f : 0.0f)
					+ (x - 1 >= 0		&& y + 1 < tileH	&& getTile(x - 1, y + 1)->isExplored(thisTeam) ? 0.5f : 0.0f)
					+ (x - 1 >= 0		&& y - 1 >= 0		&& getTile(x - 1, y - 1)->isExplored(thisTeam) ? 0.5f : 0.0f);
			// if less than 5, it remains
			if(adjacent >= 5.f) {
				// alpha between 0.25 and 0.5 depending upon how many surrounding are explored
				minimap->incFowTextureAlphaSurface(Vec2i(x, y), 0.25 + (adjacent - 5.f) / 4.f);
			}
		}
	}

	minimap->updateFowTex(1.f);
*/
}

void Map::write(NetworkDataBuffer &buf) const {
	for(int y = 0; y < tileH; ++y) {
		for(int x = 0; x < tileW; ++x) {
			getTile(x, y)->write(buf);
		}
	}
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
bool Map::fieldsCompatible ( Cell *cell, Field mf ) const
{
   if ( mf == FieldAir || mf == FieldAmphibious
   ||  (mf == FieldWalkable && ! cell->isDeepSubmerged ()) 
   ||  (mf == FieldAnyWater && cell->isSubmerged ())
   ||  (mf == FieldDeepWater && cell->isDeepSubmerged ()) )
      return true;
   return false;
}

bool Map::isFreeCell(const Vec2i &pos, Field field) const
{
   if ( !isInside(pos) || !getCell(pos)->isFree(field==FieldAir?ZoneAir:ZoneSurface) )
      return false;
   if ( field != FieldAir && !getTile(toTileCoords(pos))->isFree() )
      return false;
   // leave ^^^ that
   // replace all the rest with lookups into the map metrics...
   //return PathFinder::getInstance()->canOccupy ( pos, 1, field );
   // placeUnit() needs to use this... and the metrics aren't constructed at that point...
   if ( field == FieldWalkable && getCell(pos)->isDeepSubmerged () )
      return false;
   if ( field == FieldWalkable && ( pos.x > getW() - 4 || pos.y > getH() - 4 ) )
      return false;
   if ( field == FieldAnyWater && !getCell(pos)->isSubmerged () )
      return false;
   if ( field == FieldDeepWater && !getCell(pos)->isDeepSubmerged () )
      return false;
   return true;
}
/*
bool Map::isFreeCell(const Vec2i &pos, Zone field) const
{
   if ( !isInside(pos) || !getCell(pos)->isFree(field==FieldAir?ZoneAir:ZoneSurface) )
      return false;
   return true;
}
*/
// Is the Cell at 'pos' (for the given 'field') either free or does it contain 'unit'
/*
bool Map::isFreeCellOrHasUnit(const Vec2i &pos, Zone field, const Unit *unit) const {
	if(isInside(pos)) {
		Cell *c = getCell(pos);
		if(c->getUnit(unit->getCurrField()) == unit) {
			return true;
		} else {
			return isFreeCell(pos, field);
		}
	}
	return false;
}
*/
bool Map::isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const 
{
	if ( isInside ( pos ) ) 
   {
		Cell *c = getCell(pos);
		if ( c->getUnit(unit->getCurrField()) == unit 
      &&   fieldsCompatible ( c, field) ) 
			return true;
		else 
			return isFreeCell ( pos, field );
	}
	return false;
}
// Is the Cell at 'pos' (for the given 'field') either free or does it contain any of the units in 'units'
bool Map::isFreeCellOrHaveUnits(const Vec2i &pos, Field field, const Selection::UnitContainer &units) const {
	if(isInside(pos)) {
      Unit *containedUnit = getCell(pos)->getUnit(field);
		if ( containedUnit && fieldsCompatible ( getCell(pos), field ) ) 
      {
			Selection::UnitContainer::const_iterator i;
			for(i = units.begin(); i != units.end(); ++i) 
				if(containedUnit == *i)
					return true;
		}
		return isFreeCell(pos, field);
	}
	return false;
}

// if the Cell is visible, true if free, false if occupied
// if the Cell is explored, true if Tile is free (no object), false otherwise
// if the Cell is unexplored, true
bool Map::isAproxFreeCell(const Vec2i &pos, Field field, int teamIndex) const 
{
	if (isInside(pos)) // on map ?
   {
		const Tile *sc = getTile(toTileCoords(pos));

		if (sc->isVisible(teamIndex))
			return sc->isFree() && isFreeCell(pos, field);
		else if (sc->isExplored(teamIndex))
			return field == FieldWalkable ? sc->isFree() && !getCell(pos)->isDeepSubmerged() : true;
		else
			return true;
	}

	return false;
}

bool Map::areFreeCells(const Vec2i & pos, int size, Field field) const{
	for(int i=pos.x; i<pos.x+size; ++i){
		for(int j=pos.y; j<pos.y+size; ++j){
			if(!isFreeCell(Vec2i(i,j), field)){
                return false;
			}
		}
	}
    return true;
}

bool Map::areFreeCells( const Vec2i &pos, int size, char *fieldMap ) const
{
	for ( int i = 0; i < size; ++i)
   {
		for ( int j = 0; j < size; ++j)
      {
         Field field;
         switch ( fieldMap[j*size+i] )
         {
         case 'l': field = FieldWalkable; break;
         case 'a': field = FieldAmphibious; break;
         case 'w': field = FieldDeepWater; break;
         case 'r': field = FieldWalkable; break;
         default: throw runtime_error ( "Bad value in fieldMap" );
         }
			if ( ! isFreeCell (Vec2i(pos.x+i, pos.y+j), field) )
                return false;
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
				vector<Unit *>::const_iterator i;

				for (i = results.begin(); i != results.end() && *i != occupant; ++i) ;

				if (i == results.end()) {
					results.push_back(occupant);
				}
			}
		}
	}
}
/*
bool Map::isFieldMapCompatible ( const Vec2i &pos, const UnitType *building ) const
{
   for ( int i=0; i < building->getSize(); ++i )
   {
      for ( int j=0; j < building->getSize(); ++j )
      {
         Field field;
         switch ( building->getFieldMapCell ( i, j ) )
         {
         case 'l': field = FieldWalkable; break;
         case 'a': field = FieldAmphibious; break;
         case 'w': field = FieldDeepWater; break;
         default: throw runtime_error ( "Illegal value in FieldMap." );
         }
         if ( !fieldsCompatible ( getCell ( pos ), field ) )
            return false;
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
	Vec2f ftarget;

	if(start == target) {
		return start;
/*		Vec2i ret;
		if(map->getNearestAdjacentFreePos(ret, NULL, 1, start, target, minRange, ZoneSurface)) {
			return ret;
		} else {
			return start;
		}*/
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

	Vec2f fstart(start.x, start.y);
	//Vec2f ftarget(target.x, target.y);
	float len = fstart.dist(ftarget);

	if((int)len >= minRange && (int)len <= maxRange) {
		return start;
	} else {
		// make sure we don't divide by zero, that's how this whole universe
		// thing got started in the first place (fyi, it wasn't a big bang).
		assert(minRange == 0 || start.x != target.x || start.y != target.y);
		Vec2f result = ftarget.lerp((float)(len <= minRange ? minRange : maxRange) / len, fstart);
 		return Vec2i((int)roundf(result.x), (int)roundf(result.y));
	}
}

// FIXME: function should take field instead of deriving it from unit
bool Map::getNearestFreePos(Vec2i &result, const Unit *unit, const Vec2i &target, int minRange, int maxRange, int targetSize) const {
	Vec2i start = unit->getPos();
	int size = unit->getSize();
	Field field = unit->getCurrField();//==FieldAir?ZoneAir:ZoneSurface;
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
	float minDistance = 10000.f;

	for(int sideSize = targetSize + size + minRange - 1; sideSize < targetSize + size + maxRange; ++sideSize) {
		int topY = target.y - size;
		int bottomY = target.y + targetSize;
		int leftX = target.x - size;
		int rightX = target.x + targetSize;

		for(int i = 0; i < sideSize; ++i) 
      {
         findNearestFree(result, start, size, field, Vec2i(leftX + i, topY), minDistance, unit);		// above
         findNearestFree(result, start, size, field, Vec2i(rightX, topY + i), minDistance, unit);	// right
         findNearestFree(result, start, size, field, Vec2i(rightX - i, bottomY), minDistance, unit);	// below
         findNearestFree(result, start, size, field, Vec2i(leftX, bottomY - i), minDistance, unit);	// left
		}
	}

	return minDistance <= 10000.f;

}

/**
 * Used for adjacent position calculations -- if candidate is closer to start than minDistance, then
 * minDistance is set to that distance and the value of candicate is copied into result.
 */
inline void Map::findNearest(Vec2i &result, const Vec2i &start, const Vec2i &candidate, float &minDistance) {
	float dist = candidate.dist(start);
	if(dist < minDistance) {
		minDistance = dist;
		result = candidate;
	}
}

/**
 * Same as findNearest(Vec2i &, const Vec2i &, const Vec2i &, float &), except that all cells from
 * candidate to candidate + size - 1 must be free (i.e., a the candidate's size).
 */
inline void Map::findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, float &minDistance) const {
	if(areFreeCells(candidate, size, field)) {
		findNearest(result, start, candidate, minDistance);
	}
}

/**
 * Same as findNearestFree(Vec2i &, const Vec2i &, int, Field, const Vec2i &, float &), except that
 * candidate may be occupied by the specified unit.
 */
inline void Map::findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, float &minDistance, const Unit *unit) const {
	if(areFreeCellsOrHasUnit(candidate, size, field, unit)) {
		findNearest(result, start, candidate, minDistance);
	}
}

/**
 * Finds a location for a unit of the specified size to be adjacent to target of the specified size.
 */
Vec2i Map::getNearestAdjacentPos(const Vec2i &start, int size, const Vec2i &target, Field field,
		int targetSize) {
	int sideSize = targetSize + size;
	Vec2i result(0.f);
	float minDistance = 100000.f;

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
	float noneFound = h + w;
	float minDistance = noneFound;

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


// ==================== unit placement ====================
/*
//checks if a unit can move from between 2 cells
bool Map::canMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const {
	int size = unit->getType()->getSize();
   Field mf = unit->getCurrField();
   Zone cf = mf == FieldAir ? ZoneAir : ZoneSurface;

	for(int x = 0; x < size; ++x) {
		for(int y = 0; y < size; ++y) {
			Vec2i currPos(x + pos2.x, y + pos2.y);

			if(!isInside(currPos)) {
				return false;
			}

			if(getCell(currPos)->getUnit(cf) != unit && !isFreeCell(currPos, mf)) {
				return false;
			}
		}
	}

	return true;
}

// Called from PathFinder::aStar ()'s INNER LOOP, can this be tightened up???
// Ditch it! Use clearance values to handle size, check cell 'availability' in pathfinder...
// Check if a unit could theoreticaly move from pos1 to pos2 (pos1 & pos2 must be 'connected')
bool Map::aproxCanMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const{
	int size = unit->getType()->getSize();
	int teamIndex = unit->getTeam();
	Field field = unit->getCurrField();

	if (size == 1) //single cell units
   {
		if (!isAproxFreeCell(pos2, field, teamIndex)) 
			return false; // can not move to pos2, object or unit there
		
      if (pos1.x != pos2.x && pos1.y != pos2.y)
      {
         // Proposed move is diagonal, check if cells either 'side' are free.
         //  eg..  XXXX
         //        X1FX  The Cells marked 'F' must both be free
         //        XF2X  for the move 1->2 to be legit
         //        XXXX
			if ( !isAproxFreeCell(Vec2i(pos1.x, pos2.y), field, teamIndex) 
         &&   !isAproxFreeCell(Vec2i(pos2.x, pos1.y), field, teamIndex) ) 
				return false; // obstruction, can not move to pos2
		}
      // pos2 is free, and nothing is in the way
		return true;
	} 
   else //multi cell units
   {
		for (int i = pos2.x; i < pos2.x + size; ++i) {
			for (int j = pos2.y; j < pos2.y + size; ++j) {
				if (isInside(i, j)) {
					if (getCell(i, j)->getUnit(unit->getCurrField()) != unit) {
						if (!isAproxFreeCell(Vec2i(i, j), field, teamIndex)) {
							return false;
						}
					}
				} else {
					return false;
				}
			}
		}
		return true;
	}
}
*/
//put a units into the cells
void Map::putUnitCells(Unit *unit, const Vec2i &pos){

	assert(unit);
	const UnitType *ut = unit->getType();
	int size = ut->getSize();
	Zone field = unit->getCurrField()==FieldAir?ZoneAir:ZoneSurface;

	for(int x = 0; x < size; ++x) {
		for(int y = 0; y < size; ++y) {
			Vec2i currPos = pos + Vec2i(x, y);
			assert(isInside(currPos));

			if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
				if (getCell(currPos)->getUnit(field) != NULL)
            {
               throw runtime_error ( "Ooops..." );
            }
            assert(getCell(currPos)->getUnit(field) == NULL);
				getCell(currPos)->setUnit(field, unit);
			}
		}
	}
	unit->setPos(pos);
}

//removes a unit from cells
void Map::clearUnitCells(Unit *unit, const Vec2i &pos){

	assert(unit);
	const UnitType *ut = unit->getType();
	int size = ut->getSize();
	Zone field = unit->getCurrField()==FieldAir?ZoneAir:ZoneSurface;

	for(int x = 0; x < size; ++x) {
		for(int y = 0; y < size; ++y) {
			Vec2i currPos = pos + Vec2i(x, y);
			assert(isInside(currPos));

			if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
				assert(getCell(currPos)->getUnit(field) == unit);
				getCell(currPos)->setUnit(field, NULL);
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
	Zone field = unit->getCurrField()==FieldAir?ZoneAir:ZoneSurface;

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
				if(getCell(pos.x+i, pos.y+j)->getUnit(ZoneSurface)==unit){
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
			if(isInside(pos) && sc->getObject()==NULL && (c->getUnit(ZoneSurface)==NULL || c->getUnit(ZoneSurface)==unit)){
				sc->setHeight(refHeight);
            }
        }
    }
}
/*
void Map::flattenTerrain ( const Unit *unit )
{
   // need to make sure unit->getCenteredPos() is on a 'l' fieldMapCell
	float refHeight= getTile(toTileCoords(unit->getFlattenPos()))->getHeight();
   
	for(int i=-1; i<=unit->getType()->getSize(); ++i)
   {
      for(int j=-1; j<=unit->getType()->getSize(); ++j)
      {
         Vec2i relPos = Vec2i(i, j);
         Vec2i pos= unit->getPos() + relPos;
         // Only flatten for parts of the building on 'land'
         if ( unit->getType ()->hasFieldMap ()
         &&   unit->getType ()->getFieldMapCell ( relPos ) != 'l'
         &&   unit->getType ()->getFieldMapCell ( relPos ) != 'f' ) 
            continue;
         Cell *c= getCell(pos);
         Tile *sc= getTile(toTileCoords(pos));
         //we change height if pos is inside world, if its free or ocupied by the currenty building
         if ( isInside(pos) && sc->getObject()==NULL 
         &&   (c->getUnit(ZoneSurface)==NULL || c->getUnit(ZoneSurface)==unit) )
	         sc->setHeight(refHeight);
      }
    }
}*/

//compute normals
void Map::computeNormals(Rect2i range) {
	if (range == Rect2i(0, 0, 0, 0)) {
		range = Rect2i(0, 0, h, w);
	}

	//compute center normals
	for (int i = range.p[0].x / cellScale + 1; i < range.p[1].x / cellScale - 1; ++i) {
		for (int j = range.p[0].y / cellScale + 1; j < range.p[1].y / cellScale - 1; ++j) {
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

	for (int i = range.p[0].x / cellScale + 1; i < range.p[1].x / cellScale - 1; ++i) {
		for (int j = range.p[0].y / cellScale + 1; j < range.p[1].y / cellScale - 1; ++j) {
			for(int k=0; k<cellScale; ++k){
				for(int l=0; l<cellScale; ++l){
					if(k==0 && l==0){
						getCell(i*cellScale, j*cellScale)->setHeight(getTile(i, j)->getHeight());
					}
					else if(k!=0 && l==0){
						getCell(i*cellScale+k, j*cellScale)->setHeight((
							getTile(i, j)->getHeight()+
							getTile(i+1, j)->getHeight())/2.f);
					}
					else if(l!=0 && k==0){
						getCell(i*cellScale, j*cellScale+l)->setHeight((
							getTile(i, j)->getHeight()+
							getTile(i, j+1)->getHeight())/2.f);
					}
					else{
						getCell(i*cellScale+k, j*cellScale+l)->setHeight((
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

         //FIXME
         // if the cells have been init already, we could use the new 
         // SurfaceType enum, it'll be a bit clunky (the code anyway), 
         // but it will reduce redundant calculation...

         //FIXME
         // Also, I meant to move getSubmerged(Tile*) into Tile ages ago
         // That's the logical place for it, getSubmerged(Cell*) now lives
         // in Cell ( as Cell::isSubmerged() )... 

			// Daniel's optimized version: +speed, +code size
			/*
			bool anySubmerged = getSubmerged(getTile(x, y))
					|| (x + 1 < tileW && getSubmerged(getTile(x + 1, y)))
					|| (x - 1 >= 0		 && getSubmerged(getTile(x - 1, y)))
					|| (y + 1 < tileH && getSubmerged(getTile(x, y + 1)))
					|| (y - 1 >= 0		 && getSubmerged(getTile(x, y - 1)))
					|| (x + 1 < tileW && y + 1 < tileH && getSubmerged(getTile(x + 1, y + 1)))
					|| (x + 1 < tileW && y - 1 >= 0		 && getSubmerged(getTile(x + 1, y - 1)))
					|| (x - 1 >= 0		 && y + 1 < tileH && getSubmerged(getTile(x - 1, y + 1)))
					|| (x - 1 >= 0		 && y - 1 >= 0		 && getSubmerged(getTile(x - 1, y - 1)));
			*/

			// Marti�o's version: slower, but more compact (altered from original)
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
				if(unit->getCurrSkill()->getClass() != scDie) {
					assert(getCell(currPos)->getUnit(field) == unit);
				} else {
					assert(getCell(currPos)->getUnit(field) != unit);
				}
			}
		}
	}
}
#endif

// =====================================================
// 	class PosCircularIterator
// =====================================================

PosCircularIteratorSimple::PosCircularIteratorSimple(const Map &map, const Vec2i &center, int radius) :
		map(map), center(center), radius(radius), pos(center - Vec2i(radius + 1, radius)) {
}

// =====================================================
// 	class PosQuadIterator
// =====================================================

PosQuadIterator::PosQuadIterator(const Quad2i &quad, int step) :
		quad(quad),
		step(step),
		boundingRect(quad.computeBoundingRect()),
		pos(boundingRect.p[0]) {
	--pos.x;
	pos.x = (pos.x / step) * step;
	pos.y = (pos.y / step) * step;
}


//////////////////////////////////////////////////////////////////
// Cut Here 
//////////////////////////////////////////////////////////////////
// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Jaagup Rep�n <jrepan@gmail.com>,
//				     2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
/*
#include "pch.h"
#include "earthquake.h"

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

#include "leak_dumper.h"

//#ifndef M_PI
//#define M_PI 3.14159265
//#endif

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{
*/

// =====================================================
// 	class EarthquakeType
// =====================================================

EarthquakeType::EarthquakeType(float maxDps, const AttackType *attackType) :
		maxDps(maxDps), attackType(attackType), affectsAllies(true) {
}

/** Load the EarthquakeType from xml */
void EarthquakeType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	magnitude = n->getChildFloatValue("magnitude");
	frequency = n->getChildFloatValue("frequency");
	speed = n->getChildFloatValue("speed");
	duration = n->getChildFloatValue("duration");
	radius = n->getChildFloatValue("radius");
	initialRadius = n->getChildFloatValue("initial-radius");

	const XmlNode *soundFileNode= n->getChild("sound-file", 0, false);
	if(soundFileNode) {
		string path= soundFileNode->getAttribute("path")->getRestrictedValue();
		sound= new StaticSound();
		sound->load(dir + "/" + path);
	} else {
		sound = NULL;
	}

	shiftsPerSecond = n->getChildFloatValue("shifts-per-second");
	shiftsPerSecondVar = n->getChildFloatValue("shifts-per-second-var");
	shiftLengthMult = n->getChildVec3fValue("shift-length-multiplier");
	affectsAllies = n->getOptionalBoolValue("affects-allies", true);
}

void EarthquakeType::spawn(Map &map, Unit *cause, const Vec2i &epicenter, float strength) const {
	map.add(new Earthquake(map, cause, epicenter, this, strength));
}

// =====================================================
// 	class Earthquake
// =====================================================

Earthquake::Earthquake(Map &map, Unit *cause, const Vec2i &uepicenter, const EarthquakeType* type, float strength) :
		map(map),
		cause(cause),
		type(type),
		epicenter(map.toTileCoords(uepicenter)),
		magnitude(type->getMagnitude() * strength),
		frequency(type->getFrequency()),
		speed(type->getSpeed()),
		duration(type->getDuration()),
		radius(type->getRadius() / map.cellScale),
		initialRadius(type->getInitialRadius() / map.cellScale),
		progress(0.f),
		nextShiftTime(0.f),
		currentShift(0.f),
		random(uepicenter.x) {
	bounds.p[0].x = max((int)(epicenter.x - radius), 0);
	bounds.p[0].y = max((int)(epicenter.y - radius), 0);
	bounds.p[1].x = min((int)(epicenter.x + radius), map.getTileW() - 1);
	bounds.p[1].y = min((int)(epicenter.y + radius), map.getTileH() - 1);
	uBounds = bounds * map.cellScale;
	size.x = bounds.p[1].x - bounds.p[0].x + 1;
	size.y = bounds.p[1].y - bounds.p[0].y + 1;
	cellCount = size.x * size.y;
}

/**
 * Calculates the magnitude of the earthquake (expressed as a float from zero to one from min to
 * max) at a given point in time (also expressed as zero to one, from begining to end). A sine
 * pattern is used that quickly rises at the start and quickly falls at the end.  Full strength
 * is reached at T=0.25 and the taper occurs at 3/4 the speed of rise.
 */
inline float Earthquake::temporalMagnitude(float temporalProgress) {
	// early peek, gradual decline
	float effectiveProgress = temporalProgress < 0.25f
			? temporalProgress * 2.f
			: 0.5f + (temporalProgress - 0.25f) * 2.f / 3.f;

	// non-linear rise and fall of intensity
	return sinf(effectiveProgress * pi);
}

/** Calculates the positive or negative pressure at a given point in time. */
inline float Earthquake::temporalPressure(float time, float &tm) {
	if(time < -initialRadius / speed || time > duration) {
		return 0.f;
	}

	tm = temporalMagnitude(time / duration) * magnitude;
	return sinf(time * frequency * twopi) * tm;
}

/** Updates the earthquake advancing it slice seconds forward in time. */
void Earthquake::update(float slice) {
	progress += slice / duration;
	float time = progress * duration;
	float sps = type->getshiftsPerSecond();
	float spsvar = type->getshiftsPerSecondVar();

	if(nextShiftTime <= time) {
		do {
			// skip any shifts we missed
			nextShiftTime += 1.f / (sps + random.randRange(-spsvar, spsvar));
		} while(nextShiftTime <= time);

		const Vec3f &slm = type->getshiftLengthMult();
		currentShift.x = random.randRange(-slm.x, slm.x);
		currentShift.y = random.randRange(-slm.y, slm.y),
		currentShift.z = random.randRange(-slm.z, slm.z);
	}

	for(int x = bounds.p[0].x; x <= bounds.p[1].x; ++x) {
		for(int y = bounds.p[0].y; y <= bounds.p[1].y; ++y) {
			Vec2i pos(x, y);
			float dist = epicenter.dist(pos);
			if(dist > radius) {
				continue;
			}
			float timeLag = dist / speed;
			float decayMultiplier = dist == 0 ? 1.f : sinf(dist / radius * pi);
			float tm = 0.f;
			float pressure = temporalPressure(time - timeLag, tm) * decayMultiplier;

			Tile *sc = map.getTile(pos);
			sc->alterVertex(Vec3f(0.f, pressure, 0.f) + (currentShift * tm));
			sc->updateObjectVertex();

		}
	}
	map.computeNormals(uBounds);
	map.computeInterpolatedHeights(uBounds);
}

/**
 * Calculate effective values for damage report given a specific surface cell and point in time.
 * @param effectiveTime the effective time based upon the distance surfacePos is from the epicenter,
 * the speed of the earthquake and the supplied actual time.
 * @param effectiveMagnitude the effective magnitude based upon the calculated effective time, lag
 * (the earthquake disturbance may not have reached this cell yet) and decay.
 */
void Earthquake::calculateEffectiveValues(float &effectiveTime, float &effectiveMagnitude, const Vec2i &surfacePos, float time) {
	float dist = epicenter.dist(surfacePos);
	float timeLag = dist / speed;
	effectiveTime = time - timeLag;
	if(effectiveTime < 0.f || effectiveTime > duration || dist > radius) {
		effectiveMagnitude = 0.f;
		return;
	}
	float decayMultiplier = dist == 0 ? 1.f : sinf(dist / radius * pi);
	effectiveMagnitude = temporalMagnitude(time / duration) * magnitude * decayMultiplier;
}

void Earthquake::getDamageReport(DamageReport &results, float slice) {
	float time = progress * duration - slice;
	DamageReport::iterator i;

	for(int x = uBounds.p[0].x; x <= uBounds.p[1].x; ++x) {
		for(int y = uBounds.p[0].y; y <= uBounds.p[1].y; ++y) {
			Vec2i pos(x, y);
			Vec2i surfacePos = map.toTileCoords(pos);
			Unit *unit = map.getCell(pos)->getUnit(ZoneSurface);
			if(!unit) {
				continue;
			}

			float effectiveTime;
			float effectiveMagnitude;
			calculateEffectiveValues(effectiveTime, effectiveMagnitude, surfacePos, time);
			float damage = effectiveMagnitude / magnitude * slice;
			if(damage < 0.f) {
				damage = 0.f;
			}

			i = results.find(unit);
			if(i == results.end()) {
				results[unit].impact(damage);
			} else {
				i->second.impact(damage);
			}
		}
	}
}

void Earthquake::resetSurface() {
	for(int x = bounds.p[0].x; x <= bounds.p[1].x; ++x) {
		for(int y = bounds.p[0].y; y <= bounds.p[1].y; ++y) {
			map.getTile(x, y)->resetVertex();
		}
	}
}


//}}//end namespace
//////////////////////////////////////////////////////////////////
// Cut Here 
//////////////////////////////////////////////////////////////////


}}//end namespace


