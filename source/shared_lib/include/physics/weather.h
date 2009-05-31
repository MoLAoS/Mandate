// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2005 Matthias Braun <matze@braunis.de>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
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

using std::string;
using namespace Shared::Graphics;

namespace Shared { namespace Physics {

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
	float flatOrHilly;
	float tropicalOrArid;
	float vegitation;
	float waterDirection;
	float season;
	float latitude;
	float longitude;
	float altitude;
	float orbitalTilt;
	float atmosphericDensity;

	WeatherCell *cells;

public:
	WeatherSystem(int h, int w) : Area(h, w), cells(new WeatherCell[h * w]) {}

	WeatherCell &getCell(int x, int y)	{assert(isValid(x, y)); return cells[x * w + y];}
};

}}//end namespace

#endif
