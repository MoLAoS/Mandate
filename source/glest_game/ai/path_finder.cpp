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

// Does not use pre-compiled header because it's optimized in debug build.
#include "path_finder.h"

#include <algorithm>
#include <cassert>
#include <iostream>

#include "config.h"
#include "map.h"
#include "unit.h"
#include "unit_type.h"
#include "world.h"

#include "leak_dumper.h"


using namespace std;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class PathFinder
// =====================================================

// ===================== PUBLIC ========================

const int PathFinder::maxFreeSearchRadius= 10;
const int PathFinder::pathFindNodesMax= 400;
const int PathFinder::pathFindRefresh= 10;


PathFinder::PathFinder(){
	nodePool= NULL;
}

PathFinder::PathFinder(const Map *map){
	init(map);
	nodePool= NULL;
}

void PathFinder::init(const Map *map){
	nodePool= new Node[pathFindNodesMax];
	this->map= map;
}

PathFinder::~PathFinder(){
	delete [] nodePool;
}

PathFinder::TravelState PathFinder::findPath(Unit *unit, const Vec2i &finalPos){
//Timer t(50000, "PathFinder::findPath");
	//route cache
	UnitPath *path= unit->getPath();
	if(finalPos==unit->getPos()){
		//if arrived
		unit->setCurrSkill(scStop);
		return tsArrived;
	}
	else if(!path->isEmpty()){
		//route cache
		Vec2i pos= path->pop();
		if(map->canMove(unit, unit->getPos(), pos)){
			unit->setTargetPos(pos);
			return tsOnTheWay;
		}
	}

	//route cache miss
	TravelState ts= aStar(unit, finalPos);

	//post actions
	switch(ts){
	case tsBlocked:
	case tsArrived:
		unit->setCurrSkill(scStop);
		break;
	case tsOnTheWay:
		Vec2i pos= path->pop();
		if(map->canMove(unit, unit->getPos(), pos)){
			unit->setTargetPos(pos);
		}
		else{
			unit->setCurrSkill(scStop);
			return tsBlocked;
		}
		break;
	}
	return ts;
}

// ==================== PRIVATE ====================

//route a unit using A* algorithm
PathFinder::TravelState PathFinder::aStar(Unit *unit, const Vec2i &targetPos){
//Timer t(50000, "PathFinder::aStar=========================");
	nodePoolCount= 0;
	const Vec2i finalPos= computeNearestFreePos(unit, targetPos);
//t.print("1");
	//if arrived
	if(finalPos==unit->getPos()){
		return tsArrived;
	}

	//path find algorithm

	//a) push starting pos into openNodes
	Node *firstNode= newNode();
	assert(firstNode!=NULL);;
	firstNode->next= NULL;
	firstNode->prev= NULL;
	firstNode->pos= unit->getPos();
	firstNode->heuristic= heuristic(unit->getPos(), finalPos);
	firstNode->exploredCell= true;
	openNodes.push_back(firstNode);

	//b) loop
	bool pathFound= true;
	bool nodeLimitReached= false;
	Node *node= NULL;
//t.print("2");

	while(!nodeLimitReached){
//t.print("2.5a");
		//b1) is open nodes is empty => failed to find the path
		if(openNodes.empty()){
			pathFound= false;
			break;
		}

		//b2) get the minimum heuristic node
		Nodes::iterator it = minHeuristic();
		node= *it;

		//b3) if minHeuristic is the finalNode, or the path is no more explored => path was found
		if(node->pos==finalPos || !node->exploredCell){
			pathFound= true;
			break;
		}

		//b4) move this node from closedNodes to openNodes
		//add all succesors that are not in closedNodes or openNodes to openNodes
		closedNodes.push_back(node);
		openNodes.erase(it);
//t.print("2.5b");
		for(int i=-1; i<=1 && !nodeLimitReached; ++i){
			for(int j=-1; j<=1 && !nodeLimitReached; ++j){
				Vec2i sucPos= node->pos + Vec2i(i, j);
				if(!openPos(sucPos) && map->aproxCanMove(unit, node->pos, sucPos)){
					//if node is not open and canMove then generate another node
					Node *sucNode= newNode();
					if(sucNode!=NULL){
						sucNode->pos= sucPos;
						sucNode->heuristic= heuristic(sucNode->pos, finalPos);
						sucNode->prev= node;
						sucNode->next= NULL;
						sucNode->exploredCell= map->getSurfaceCell(Map::toSurfCoords(sucPos))->isExplored(unit->getTeam());
						openNodes.push_back(sucNode);
					}
					else{
						nodeLimitReached= true;
					}
				}
			}
		}
	//t.print("2.5c");
	}//while
//t.print("3");

	Node *lastNode= node;

	//if consumed all nodes find best node (to avoid strage behaviour)
	if(nodeLimitReached){
		for(Nodes::iterator it= closedNodes.begin(); it!=closedNodes.end(); ++it){
			if((*it)->heuristic < lastNode->heuristic){
				lastNode= *it;
			}
		}
	}
//t.print("4");

	//check results of path finding
	TravelState ts;
	UnitPath *path= unit->getPath();
	if(pathFound==false || lastNode==firstNode){
		//blocked
		ts= tsBlocked;
		path->incBlockCount();
	}
	else {
		//on the way
		ts= tsOnTheWay;

		//build next pointers
		Node *currNode= lastNode;
		while(currNode->prev!=NULL){
			currNode->prev->next= currNode;
			currNode= currNode->prev;
		}
		//store path
		path->clear();

		currNode= firstNode;
		for(int i=0; currNode->next!=NULL && i<pathFindRefresh; currNode= currNode->next, i++){
			path->push(currNode->next->pos);
		}
	}
//t.print("5");

	//clean nodes
	openNodes.clear();
	closedNodes.clear();

	return ts;
}

PathFinder::Node *PathFinder::newNode(){
	if(nodePoolCount<pathFindNodesMax){
		Node *node= &nodePool[nodePoolCount];
		nodePoolCount++;
		return node;
	}
	return NULL;
}

Vec2i PathFinder::computeNearestFreePos(const Unit *unit, const Vec2i &finalPos){
//Timer t(1000, "PathFinder::computeNearestFreePos");
	//unit data
	Vec2i unitPos= unit->getPos();
	int size= unit->getType()->getSize();
	Field field= unit->getCurrField();
	int teamIndex= unit->getTeam();

	//if finalPos is free return it
	if(map->isAproxFreeCells(finalPos, size, field, teamIndex)){
		return finalPos;
	}

	//find nearest pos
	Vec2i nearestPos= unitPos;
	float nearestDist= unitPos.dist(finalPos);
	for(int i= -maxFreeSearchRadius; i<=maxFreeSearchRadius; ++i){
		for(int j= -maxFreeSearchRadius; j<=maxFreeSearchRadius; ++j){
			Vec2i currPos= finalPos + Vec2i(i, j);
			if(map->isAproxFreeCells(currPos, size, field, teamIndex)){
				float dist= currPos.dist(finalPos);

				//if nearer from finalPos
				if(dist<nearestDist){
					nearestPos= currPos;
					nearestDist= dist;
				}
				//if the distance is the same compare distance to unit
				else if(dist==nearestDist){
					if(currPos.dist(unitPos)<nearestPos.dist(unitPos)){
						nearestPos= currPos;
					}
				}
			}
		}
	}
	return nearestPos;
}

float PathFinder::heuristic(const Vec2i &pos, const Vec2i &finalPos){
	return pos.dist(finalPos);
}

//returns an iterator to the lowest heuristic node
PathFinder::Nodes::iterator PathFinder::minHeuristic(){
//Timer t(1000, "PathFinder::minHeuristic");
	Nodes::iterator minNodeIt=  openNodes.begin();

	assert(!openNodes.empty());

	for(Nodes::iterator it= openNodes.begin(); it!=openNodes.end(); ++it){
		if((*it)->heuristic < (*minNodeIt)->heuristic){
			minNodeIt= it;
		}
	}

	return minNodeIt;
}

bool PathFinder::openPos(const Vec2i &sucPos){
	for(Nodes::reverse_iterator it= closedNodes.rbegin(); it!=closedNodes.rend(); ++it){
		if(sucPos==(*it)->pos){
			return true;
		}
	}

	//use reverse iterator to find a node faster
	for(Nodes::reverse_iterator it= openNodes.rbegin(); it!=openNodes.rend(); ++it){
		if(sucPos==(*it)->pos){
			return true;
		}
	}

	return false;
}


#ifdef DEBUG
// Why is this here?  Doesn't it belong in world.cpp?  It's here because we compile path_finder.cpp
// optimized in debug since it's the only possible way you can really debug and this is a dog slow
// function.
class ValidationMap {
public:
	int h;
	int w;
	char *cells;

	ValidationMap(int h, int w) : h(h), w(w), cells(new char[h * w * fCount]) {
		reset();
	}

	void reset() {
		memset(cells, 0, h * w * fCount);
	}

	void validate(int x, int y, Field field) {
		assert(!getCell(x, y, field));
		getCell(x, y, field) = 1;
	}

	char &getCell(int x, int y, Field field) {
		assert(x >= 0 && x < w);
		assert(y >= 0 && y < h);
		assert(field >= 0 && field < fCount);
		return cells[field * h * w + x * w + y];
	}
};

void World::assertConsistiency() {
	// go through each map cell and make sure each unit in cells are supposed to be there
	// go through each unit and make sure they are in the cells they are supposed to be in
	// iterate through unit references: pets, master, target and make sure they are valid or
	//		null-references
	// make sure alive/dead states of all units is good.
	// whatever else I can think of

	static ValidationMap validationMap(map.getH(), map.getW());
	validationMap.reset();

	// make sure that every unit is in their cells and mark those as validated.
	for(Factions::iterator fi = factions.begin(); fi != factions.end(); ++fi) {
		for(int ui = 0; ui < fi->getUnitCount(); ++ui) {
			Unit *unit = fi->getUnit(ui);

			if(!(unit->getHp() == 0 && unit->isDead() || unit->getHp() > 0 && unit->isAlive())) {
				cerr << "inconsisteint dead/hp state for unit " << unit->getId()
						<< " (" << unit << ") faction " << fi->getIndex() << endl;
				cout << "inconsisteint dead/hp state for unit " << unit->getId()
						<< " (" << unit << ") faction " << fi->getIndex() << endl;
				cout.flush();
				assert(false);
			}

			if(unit->getCurrSkill()->getClass() == scDie) {
				continue;
			}

			const UnitType *ut = unit->getType();
			int size = ut->getSize();
			Field field = unit->getCurrField();
			const Vec2i &pos = unit->getPos();

			for(int x = 0; x < size; ++x) {
				for(int y = 0; y < size; ++y) {
					Vec2i currPos = pos + Vec2i(x, y);
					assert(map.isInside(currPos));

					if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
						if(map.getCell(currPos)->getUnit(field) != unit) {
							cerr << "Unit id " << unit->getId()
									<< " from faction " << fi->getIndex()
									<< " not in cells (" << x << ", " << y << ", " << field << ")"
									<< endl;
							assert(false);
						}
						validationMap.validate(currPos.x, currPos.y, field);
					}
				}
			}
		}
	}

	// make sure that every cell that was not validated is empty
	for(int x = 0; x < map.getW(); ++x) {
		for(int y = 0; y < map.getH(); ++y ) {
			for(int field = 0; field < fCount; ++field) {
				if(!validationMap.getCell(x, y, (Field)field)) {
					if(map.getCell(x, y)->getUnit(field)) {
						Cell *cell = map.getCell(x, y);
						cerr << "Cell not empty at " << x << ", " << y << ", " << field << endl;
						cerr << "Cell has pointer to unit object at " << cell->getUnit(field) << endl;

						assert(false);
					}
				}
			}
		}
	}
}
#else
void World::assertConsistiency() {}
#endif
}} //end namespace
