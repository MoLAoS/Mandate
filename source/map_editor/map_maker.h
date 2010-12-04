// ==============================================================
//	This file is part of the Glest Advanced Engine
//
//	Copyright (C) 2010	Eric Wilson
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _MAPEDITOR_MAP_MAKER_H_
#define _MAPEDITOR_MAP_MAKER_H_

#include "map.h"

namespace MapEditor {

class MapMaker {
private:
	Random random;
	Map *map;

public:
	MapMaker(Map *map) : map(map) { }

	void randomize(/*int w, int h, int numFactions*/);

private:
	void setStartLocations(float baseHeight, int numFactions = 4, float minDistance = 20.f);
	void diamondSquare(float baseHeight, float delta = 8.f, float roughness = -0.2f);
	void generateResoures(/* something to control 'resource allotment' */);
	void growForests(/* control variables ?? */);
	void growForestsMountaintop(float forestMinHeight, float forestMaxHeight, int maxTrees, float playerRadius);
	//void addEyeCandy();
};

}

#endif
