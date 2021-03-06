// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "program.h"
#include "map_maker.h"
#include "util.h"

using namespace Shared::Util;

namespace MapEditor {

////////////////////////////
// class UndoPoint
////////////////////////////
int UndoPoint::w = 0;
int UndoPoint::h = 0;

UndoPoint::UndoPoint() 
		: change(ctNone)
		, surface(0)
		, object(0)
		, resource(0)
		, height(0) {
	w = Program::map->getW();
	h = Program::map->getH();
}
/*
UndoPoint::UndoPoint(ChangeType change) {
	w = Program::map->getW();
	h = Program::map->getH();

	init(change);
}*/

void UndoPoint::init(ChangeType change) {
	this->change = change;
	switch (change) {
		// Back up everything
		case ctAll:
		// Back up heights
		case ctHeight:
		case ctGradient:
			// Build an array of heights from the map
			//std::cout << "Building an array of heights to use for our UndoPoint" << std::endl;
			height = new float[w * h];
			for (int i = 0; i < w; ++i) {
				for (int j = 0; j < h; ++j) {
					 height[j * w + i] = Program::map->getHeight(i, j);
				}
			}
			//std::cout << "Built the array" << std::endl;
			if (change != ctAll) break;
		// Back up surfaces
		case ctSurface:
			surface = new int[w * h];
			for (int i = 0; i < w; ++i) {
				for (int j = 0; j < h; ++j) {
					 surface[j * w + i] = Program::map->getSurface(i, j);
				}
			}
			if (change != ctAll) break;
		// Back up both objects and resources if either changes
		case ctObject:
		case ctResource:
			object = new int[w * h];
			for (int i = 0; i < w; ++i) {
				for (int j = 0; j < h; ++j) {
					 object[j * w + i] = Program::map->getObject(i, j);
				}
			}
			resource = new int[w * h];
			for (int i = 0; i < w; ++i) {
				for (int j = 0; j < h; ++j) {
					 resource[j * w + i] = Program::map->getResource(i, j);
				}
			}
			break;
	}
}

UndoPoint::~UndoPoint() {
	delete [] height;
	delete [] resource;
	delete [] object;
	delete [] surface;
}

void UndoPoint::revert() {
	//std::cout << "attempting to revert last changes to " << undoID << std::endl;
	switch (change) {
		// Revert Everything
		case ctAll:
		// Revert Heights
		case ctHeight:
		case ctGradient:
			// Restore the array of heights to the map
			//std::cout << "attempting to restore the height array" << std::endl;
			for (int i = 0; i < w; i++) {
				for (int j = 0; j < h; j++) {
					 Program::map->setHeight(i, j, height[j * w + i]);
				}
			}
			if (change != ctAll) break;
		// Revert surfaces
		case ctSurface:
			//std::cout << "attempting to restore the surface array" << std::endl;
			for (int i = 0; i < w; i++) {
				for (int j = 0; j < h; j++) {
					 Program::map->setSurface(i, j, surface[j * w + i]);
				}
			}
			if (change != ctAll) break;
		// Revert objects and resources
		case ctObject:
		case ctResource:
			for (int i = 0; i < w; i++) {
				for (int j = 0; j < h; j++) {
					 Program::map->setObject(i, j, object[j * w + i]);
				}
			}
			for (int i = 0; i < w; i++) {
				for (int j = 0; j < h; j++) {
					 Program::map->setResource(i, j, resource[j * w + i]);
				}
			}
			break;
	}
	//std::cout << "reverted changes (we hope)" << std::endl;
}

// ===============================================
//	class Program
// ===============================================

Map *Program::map = NULL;

Program::Program(int w, int h) {
	cellSize = 6;
	offsetX = 0;
	offsetY = 0;
	map = new Map();
	renderer.init(w, h);
}

Program::~Program() {
	delete map;
	map = NULL;
}

int Program::getObject(int x, int y) {
	int i = (x - offsetX) / cellSize;
	int j = (y + offsetY) / cellSize;
	if (map && map->inside(i, j)) {
		return map->getObject(i,j);
	} else {
		return 0;
	}
}

int Program::getResource(int x, int y) {
	int i = (x - offsetX) / cellSize;
	int j = (y + offsetY) / cellSize;
	if (map && map->inside(i, j)) {
		return map->getResource(i,j);
	} else {
		return 0;
	}
}

Vec2i Program::getCellCoords(int x, int y) {
	return Vec2i((x - offsetX) / cellSize, (y + offsetY) / cellSize);
}

void Program::glestChangeMapHeight(int x, int y, int Height, int radius) {
	assert(map);
	map->glestChangeHeight((x - offsetX) / cellSize, (y + offsetY) / cellSize, Height, radius);
}

void Program::pirateChangeMapHeight(int x, int y, int Height, int radius) {
	assert(map);
	map->pirateChangeHeight((x - offsetX) / cellSize, (y + offsetY) / cellSize, Height, radius);
}

void Program::changeMapSurface(int x, int y, int surface, int radius) {
	assert(map);
	map->changeSurface((x - offsetX) / cellSize, (y + offsetY) / cellSize, surface, radius);
}

void Program::changeMapObject(int x, int y, int object, int radius) {
	assert(map);
	map->changeObject((x - offsetX) / cellSize, (y + offsetY) / cellSize, object, radius);
}

void Program::changeMapResource(int x, int y, int resource, int radius) {
	assert(map);
	map->changeResource((x - offsetX) / cellSize, (y + offsetY) / cellSize, resource, radius);
}

void Program::changeStartLocation(int x, int y, int player) {
	assert(map);
	map->changeStartLocation((x - offsetX) / cellSize, (y + offsetY) / cellSize, player);
}

void Program::setUndoPoint(ChangeType change) {
	if (change == ctLocation) return;

	undoStack.push(UndoPoint());
	undoStack.top().init(change);

	redoStack.clear();
}

bool Program::undo() {
	if (undoStack.empty()) {
		return false;
	}
	// push current state onto redo stack
	redoStack.push(UndoPoint());
	redoStack.top().init(undoStack.top().getChange());

	undoStack.top().revert();
	undoStack.pop();
	return true;
}

bool Program::redo() {
	if (redoStack.empty()) {
		return false;
	}
	// push current state onto undo stack
	undoStack.push(UndoPoint());
	undoStack.top().init(redoStack.top().getChange());

	redoStack.top().revert();
	redoStack.pop();
	return true;
}

void Program::renderMap(int w, int h) {
	renderer.renderMap(map, offsetX, offsetY, w, h, cellSize);
}

void Program::setRefAlt(int x, int y) {
	assert(map);
	map->setRefAlt((x - offsetX) / cellSize, (y + offsetY) / cellSize);
}

void Program::flipX() {
	assert(map);
	map->flipX();
}

void Program::flipY() {
	assert(map);
	map->flipY();
}

void Program::randomizeMapHeights() {
	assert(map);
	map->randomizeHeights();
}

void Program::randomizeMap() {
	assert(map);
	///@todo show dialog with parameters
	MapMaker mm(map);
	///@todo pass parameters
	mm.randomize();
}

void Program::switchMapSurfaces(int surf1, int surf2) {
	assert(map);
	map->switchSurfaces(surf1, surf2);
}

void Program::reset(int w, int h, int alt, int surf) {
	assert(map);
	undoStack.clear();
	redoStack.clear();
	map->reset(w, h, (float) alt, surf);
}

void Program::resize(int w, int h, int alt, int surf) {
	assert(map);
	map->resize(w, h, (float) alt, surf);
}

void Program::resetFactions(int maxFactions) {
	assert(map);
	map->resetFactions(maxFactions);
}

bool Program::setMapTitle(const string &title) {
	assert(map);
	if (map->getTitle() != title) {
		map->setTitle(title);
		return true;
	}
	return false;
}

bool Program::setMapDesc(const string &desc) {
	assert(map);
	if (map->getDesc() != desc) {
		map->setDesc(desc);
		return true;
	}
	return false;
}

bool Program::setMapAuthor(const string &author) {
	assert(map);
	if (map->getAuthor() != author) {
		map->setAuthor(author);
		return true;
	}
	return false;
}

void Program::setOffset(int x, int y) {
	offsetX += x;
	offsetY -= y;
}

void Program::incCellSize(int i) {

	int minInc = 2 - cellSize;

	if (i < minInc) {
		i = minInc;
	}
	cellSize += i;
	offsetX -= (map->getW() * i) / 2;
	offsetY += (map->getH() * i) / 2;
}

void Program::resetOffset() {
	offsetX = 0;
	offsetY = 0;
	cellSize = 6;
}

void Program::setMapAdvanced(int altFactor, int waterLevel) {
	assert(map);
	map->setAdvanced(altFactor, waterLevel);
}

void Program::loadMap(const string &path) {
	assert(map);
	undoStack.clear();
	redoStack.clear();
	map->loadFromFile(path);
}

void Program::saveMap(const string &path) {
	assert(map);
	map->saveToFile(path);
}

}// end namespace
