// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _MAPEDITOR_PROGRAM_H_
#define _MAPEDITOR_PROGRAM_H_

#include "map.h"
#include "renderer.h"

namespace MapEditor {

class MainWindow;

// ===============================================
// class Program
// ===============================================

class Program {
private:
	Renderer renderer;
	int ofsetX, ofsetY;
	int cellSize;
	Map *map;

public:
	Program(int w, int h);
	~Program();

	//map cell change
	void changeMapHeight(int x, int y, int Height, int radius);
	void changeMapSurface(int x, int y, int surface, int radius);
	void changeMapObject(int x, int y, int object, int radius);
	void changeMapResource(int x, int y, int resource, int radius);
	void changeStartLocation(int x, int y, int player);

	//map ops
	void reset(int w, int h, int alt, int surf);
	void resize(int w, int h, int alt, int surf);
	void resetFactions(int maxFactions);
	void setRefAlt(int x, int y);
	void flipX();
	void flipY();
	void randomizeMapHeights();
	void randomizeMap();
	void switchMapSurfaces(int surf1, int surf2);
	void loadMap(const string &path);
	void saveMap(const string &path);

	//map misc
	void setMapTitle(const string &title);
	void setMapDesc(const string &desc);
	void setMapAuthor(const string &author);
	void setMapAdvanced(int altFactor, int waterLevel);

	//misc
	void renderMap(int w, int h);
	void setOfset(int x, int y);
	void incCellSize(int i);
	void resetOfset();

	const Map *getMap() {return map;}
};

}// end namespace

#endif
