// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PHYSICS_WEATHER_H_
#define _SHARED_PHYSICS_WEATHER_H_

#include <string>
#include <cassert>

#include "vec.h"
#include "cosmology.h"

using std::string;
using namespace Shared::Graphics;

namespace Shared { namespace Physics {

class Wind {
private:
	float direction;
	float velocity;
};

class WindEvent : public Wind {
private:
	float duration;
};

class SimpleWindSystem {

};

#if defined(USE_COMPLEX_WEATHER_MODEL)

#warning "This code isn't even close to usable.  Don't define USE_COMPLEX_WEATHER_MODEL."

class Area {
protected:
	int h;
	int w;

public:
	Area(int h, int w) : h(h), w(w) {}
	int getH() const	{return h;}
	int getW() const	{return w;}

	bool isValid(int x, int y)			{return x >= 0 && x < w && y >= 0 && y < h;}
};


class WeatherCell {
public:
	Vec3f wind;
	float temp;
	float humidity;
	float pressure;

	WeatherCell() :
		wind(0.f),
		temp(0.f),
		humidity(0.f),
		pressure(0.f) {
	}
};

class WeatherSystem  : public Area {
protected:
	const CelestialBody &planet;
	float timeOfDay;		/**< Time of day expressed as a value from zero to <1.  Zero is midnight, 0.5 is noon. */
	float season;			/**< Season of the year expressed as a value from zero to <1.  Zero is Winter Solstice, 0.25f is Spring Equinox, 0.5f is Summer Solstice and 0.75f is Fall Equinox. */
	float flatOrHilly;		/**< The shape of the landscape from zero to one.  Zero represents a landscape that is impossibly flat and smooth while one represents a landscape that is impossibly hilly and jagged.  Values that represent what you will find on earth should be between 0.05 (for a flat plain) to 0.8 (steep and jagged mountains). */
	float altitudeRelation;	/**< Relationship of the altitude at the center of your map to the surrounding area expressed as a value from zero to 1.  Zero indicates an unrealistically exteme basin, 0.5f the same altitude and one the top of an unrealistically steep mountain.  Generally, normal values will be between 0.05f for the bottom of a chasm and 0.95f for the top of a mountain. */
	float vegitation;		/**< One for a tropical forest, zero for a barren desert. (presumes carbon-H20-based life forms) */
	float tropicalOrArid;	/**< How much of the surrounding landscape is water.  Use zero for in-land and no lakes nearby, one for middle of the ocean.  This value should include the landscape of the map its self, so an island should never be one, but something like 0.95. */
	float waterDirection;	/**< Which direction the central body of water indicated by tropicalOrArid is in rads (zero being North). Ignored if tropicalOrArid is zero. */
	float waterDistance;	/**< Distance the center of your map is from the body of water described by tropicalOrArid and waterDirection in multiples of map size ((mapHeight + mapWidth) / 2). Ignored if tropicalOrArid is zero. */
	float latitude;			/**< Latitude (from -90 to +90) of the center of your map in the cordinate system of the planet your map resides upon. */
	float longitude;		/**< Longitude (from -180 to +180) of the center of your map in the cordinate system of the planet your map resides upon. */
	float altitude;			/**< The altitude above sea level of the center of your map. */

	WeatherCell *cells;

public:
	WeatherSystem(int h, int w) : Area(h, w), cells(new WeatherCell[h * w]) {}

	WeatherCell &getCell(int x, int y)	{assert(isValid(x, y)); return cells[x * w + y];}
};

#endif // defined(USE_COMPLEX_WEATHER_MODEL)

}}//end namespace

#endif
