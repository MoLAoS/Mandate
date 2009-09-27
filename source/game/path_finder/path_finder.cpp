// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2009 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

// Does not use pre-compiled header because it's optimized in debug build.

// Currently DOES use precompiled header, because it's no longer optimized in debug
// because debugging optimized code is not much fun :-)

// Actually, no need for this one to be optimized anymore, when path finding is stable again, 
// graph_search.cpp should get this treatment, that's where the 'hard work' is done now.

#include "pch.h"

#include "path_finder.h"
#include "graph_search.h"

#include "config.h"
#include "map.h"
#include "game.h"
#include "unit.h"
#include "unit_type.h"
#include "world.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{ namespace Search {


// =====================================================
// 	class PathFinder
// =====================================================

// ===================== PUBLIC ========================

//FIXME I was duplicated from GraphSearch because the search functions
// absolutely _need_ this inlined, and then it can't be used here without 
// having been defined... find a better solution :-)
void PathFinder::getDiags ( const Vec2i &s, const Vec2i &d, const int size, Vec2i &d1, Vec2i &d2 ) {
	assert ( s.x != d.x && s.y != d.y );
	if ( size == 1 ) {
		d1.x = s.x; d1.y = d.y;
		d2.x = d.x; d2.y = s.y;
		return;
	}
	if ( d.x > s.x ) {  // travelling east
		if ( d.y > s.y ) {  // se
			d1.x = d.x + size - 1;	d1.y = s.y;
			d2.x = s.x;				d2.y = d.y + size - 1;
		}
		else {  // ne
			d1.x = s.x;				d1.y = d.y;
			d2.x = d.x + size - 1;	d2.y = s.y - size + 1;
		}
	}
	else {  // travelling west
		if ( d.y > s.y ) {  // sw
			d1.x = d.x;				d1.y = s.y;
			d2.x = s.x + size - 1;	d2.y = d.y + size - 1;
		}
		else {  // nw
			d1.x = d.x;				d1.y = s.y - size + 1;
			d2.x = s.x + size - 1;	d2.y = d.y;
		}
	}
}

PathFinder* PathFinder::singleton = NULL;
int pathFindNodesMax;

PathFinder* PathFinder::getInstance () {
	if ( ! singleton )
		singleton = new PathFinder ();
	return singleton;
}

PathFinder::PathFinder() {
	search = new GraphSearch ();
	annotatedMap = NULL;
	singleton = this;
	pathFindNodesMax = Config::getInstance ().getPathFinderMaxNodes ();
}

PathFinder::~PathFinder () {
	delete search;
	delete annotatedMap;
}

void PathFinder::init ( Map *map ) {
	Logger::getInstance().add("Initialising PathFinder", true);
	this->map= map;
	delete annotatedMap;
	annotatedMap = new AnnotatedMap ( map );
	search->init ( map, annotatedMap, Config::getInstance().getPathFinderUseAStar() );
#ifdef _GAE_DEBUG_EDITION_
	if ( Config::getInstance ().getMiscDebugTextures() ) {
		int foo = Config::getInstance ().getMiscDebugTextureMode ();
		debug_texture_action = foo == 0 ? PathFinder::ShowPathOnly
							 : foo == 1 ? PathFinder::ShowOpenClosedSets
										: PathFinder::ShowLocalAnnotations;
	}
#endif
}

bool PathFinder::isLegalMove ( Unit *unit, const Vec2i &pos2 ) const {
	assert ( map->isInside ( pos2 ) );
	if ( unit->getPos().dist ( pos2 ) > 1.5 ) {
		//TODO: figure out why we need this!
		return false;
	}
	const Vec2i &pos1 = unit->getPos ();
	const int &size = unit->getSize ();
	const Field &field = unit->getCurrField ();
	Zone zone = field == FieldAir ? ZoneAir : ZoneSurface;

	if ( ! annotatedMap->canOccupy ( pos2, size, field ) )
		return false; // obstruction in field
	if ( pos1.x != pos2.x && pos1.y != pos2.y ) {
		// Proposed move is diagonal, check if cells either 'side' are free.
		Vec2i diag1, diag2;
		getDiags ( pos1, pos2, size, diag1, diag2 );
		if ( ! annotatedMap->canOccupy (diag1, 1, field) 
		||   ! annotatedMap->canOccupy (diag2, 1, field) ) 
			return false; // obstruction, can not move to pos2
		if ( ! map->getCell (diag1)->isFree (zone)
		||   ! map->getCell (diag2)->isFree (zone) )
			return false; // other unit in the way
	}
	for ( int i = pos2.x; i < unit->getSize () + pos2.x; ++i )
		for ( int j = pos2.y; j < unit->getSize () + pos2.y; ++j )
			if ( map->getCell (i,j)->getUnit (zone) != unit
			&&   ! map->isFreeCell (Vec2i(i,j), field) )
				return false; // blocked by another unit
	// pos2 is free, and nothing is in the way
	return true;
}

const ResourceType* PathFinder::resourceGoal = NULL;
const Unit* PathFinder::storeGoal = NULL;

bool PathFinder::resourceGoalFunc ( const Vec2i &pos ) {
	//FIXME need to take unit size into account...
	Vec2i tmp;
	Map *map = Game::getInstance()->getWorld()->getMap();
	return map->isResourceNear ( pos, resourceGoal, tmp );
}
bool PathFinder::storeGoalFunc ( const Vec2i &pos ) {
	return Game::getInstance()->getWorld()->getMap()->isNextTo(pos, storeGoal);
}
TravelState PathFinder::findPathToGoal(Unit *unit, const Vec2i &finalPos, bool (*func)(const Vec2i&)) {
	//Logger::getInstance ().add ( "findPathToGoal() Called..." );
	static int flipper = 0;
	//route cache
	UnitPath &path = *unit->getPath ();
	if( finalPos == unit->getPos () ) {	//if arrived (where we wanted to go)
		unit->setCurrSkill ( scStop );
		//Logger::getInstance ().add ( "findPathToGoal() returning..." );
		return tsArrived;
	}
	else if( ! path.isEmpty () ) {	//route cache
		Vec2i pos = path.pop();
		if ( isLegalMove ( unit, pos ) ) {
			unit->setNextPos ( pos );
			//Logger::getInstance ().add ( "findPathToGoal() returning..." );
			return tsOnTheWay;
		}
	}
	//route cache miss
	const Vec2i targetPos = computeNearestFreePos ( unit, finalPos );

	//if arrived (as close as we can get to it)
	if ( targetPos == unit->getPos () ) {
		unit->setCurrSkill(scStop);
		//Logger::getInstance ().add ( "findPathToGoal() returning..." );
		return tsArrived;
	}

	bool useAStar = Config::getInstance().getPathFinderUseAStar ();
	// some tricks to determine if we are probably blocked on a short path, without
	// an exhuastive and expensive search through pathFindNodesMax nodes
	/* goal based pathing removes the need for this..
	* Should probably adapt this to check the entire resource 'patch'
	float dist = unit->getPos().dist ( targetPos );
	if ( unit->getCurrField () == FieldWalkable 
	&&   map->getTile (Map::toTileCoords ( targetPos ))->isVisible (unit->getTeam ()) ) {
		int radius;
		if ( dist < 5 ) radius = 2;
		else if ( dist < 10 ) radius = 3;
		else if ( dist < 15 ) radius = 4;
		else radius = 5;
		if( ( useAStar  && !search->canPathOut ( targetPos, radius, FieldWalkable ) )
		||  ( !useAStar && !search->canPathOut_Greedy ( targetPos, radius, FieldWalkable ) ) ) {
			unit->getPath()->incBlockCount ();
			unit->setCurrSkill(scStop);
			//Logger::getInstance ().add ( "findPathToGoal() returning..." );
			return tsBlocked;
		}
	}*/
	SearchParams params (unit);
	if ( func ) params.goalFunc = func;
	params.dest = targetPos;
	list<Vec2i> pathList;

	bool result;
	annotatedMap->annotateLocal ( unit, unit->getCurrField () );
	//TODO annotate target if visible ??
	if ( useAStar )
		result = search->AStarSearch ( params, pathList );
	else
		result = search->GreedySearch ( params, pathList );
	annotatedMap->clearLocalAnnotations ( unit->getCurrField () );
	if ( ! result ) {
		unit->getPath()->incBlockCount ();
		unit->setCurrSkill(scStop);
		//Logger::getInstance ().add ( "findPathToGoal() returning..." );
		return tsBlocked;
	}
	else if ( pathList.size() < 2 ) { // goal might be closer than targetPos.
		unit->setCurrSkill(scStop);
		//Logger::getInstance ().add ( "findPathToGoal() returning..." );
		return tsArrived;
	}
	else //TODO: UnitPath to inherit from list<Vec2i> and then be passed directly
		copyToPath ( pathList, unit->getPath () ); // to the search algorithm

	Vec2i pos = path.pop();
	if ( ! isLegalMove ( unit, pos ) ) {
		unit->setCurrSkill(scStop);
		unit->getPath()->incBlockCount ();
		//Logger::getInstance ().add ( "findPathToGoal() returning..." );
		return tsBlocked;
	}
	unit->setNextPos(pos);
	//Logger::getInstance ().add ( "findPathToGoal() returning..." );
	return tsOnTheWay;
}

//TODO: Make UnitPath inherit from list<Vec2i> then remove all this nonsense...
void PathFinder::copyToPath ( const list<Vec2i> pathList, UnitPath *path ) {
	list<Vec2i>::const_iterator it = pathList.begin();
	// skip start pos, store rest
	for ( ++it; it != pathList.end(); ++it )
		path->push ( *it );
}

// ==================== PRIVATE ====================

// return finalPos if free, else a nearest free pos within maxFreeSearchRadius
// cells, or unit's current position if none found
Vec2i PathFinder::computeNearestFreePos (const Unit *unit, const Vec2i &finalPos) {
	//unit data
	Vec2i unitPos= unit->getPos();
	int size= unit->getType()->getSize();
	Field field = unit->getCurrField();// == FieldAir ? ZoneAir : ZoneSurface;
	int teamIndex= unit->getTeam();

	//if finalPos is free return it
	
	if(map->areAproxFreeCells(finalPos, size, field, teamIndex)){
		return finalPos;
	}

	//find nearest pos
	Vec2i nearestPos= unitPos;
	float nearestDist= unitPos.dist(finalPos);
	for(int i= -maxFreeSearchRadius; i<=maxFreeSearchRadius; ++i){
		for(int j= -maxFreeSearchRadius; j<=maxFreeSearchRadius; ++j){
			Vec2i currPos= finalPos + Vec2i(i, j);
			if(map->areAproxFreeCells(currPos, size, field, teamIndex)){
				float dist= currPos.dist(finalPos);

				//if nearer from finalPos
				if ( dist < nearestDist ) {
					nearestPos= currPos;
					nearestDist= dist;
				}
				//if the distance is the same compare distance to unit
				else if ( dist == nearestDist ) {
					if ( currPos.dist(unitPos) < nearestPos.dist(unitPos) )
						nearestPos= currPos;
				}
			}
		}
	}
	return nearestPos;
}

} // end namespace Glest::Game::PathFinder

#if defined DEBUG && defined VALIDATE_WORLD
// Why is this here?  Doesn't it belong in world.cpp?  It's here because we compile path_finder.cpp
// optimized in debug since it's the only possible way you can really debug and this is a dog slow
// function.
class ValidationMap {
public:
	int h;
	int w;
	char *cells;

	ValidationMap(int h, int w) : h(h), w(w), cells(new char[h * w * ZoneCount]) {
		reset();
	}

	void reset() {
		memset(cells, 0, h * w * ZoneCount);
	}

	void validate(int x, int y, Zone zone) {
		assert(!getCell(x, y, zone));
		getCell(x, y, zone) = 1;
	}

	char &getCell(int x, int y, Zone zone) {
		assert(x >= 0 && x < w);
		assert(y >= 0 && y < h);
		assert(zone >= 0 && zone < ZoneCount);
		return cells[zone * h * w + x * w + y];
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
/*
			if(!((unit->getHp() == 0 && unit->isDead()) || (unit->getHp() > 0 && unit->isAlive()))) {
				cerr << "inconsisteint dead/hp state for unit " << unit->getId()
						<< " (" << unit << ") faction " << fi->getIndex() << endl;
				cout << "inconsisteint dead/hp state for unit " << unit->getId()
						<< " (" << unit << ") faction " << fi->getIndex() << endl;
				cout.flush();
				assert(false);
			}
*/
			if(unit->isDead() && unit->getCurrSkill()->getClass() == scDie) {
				continue;
			}

			const UnitType *ut = unit->getType();
			int size = ut->getSize();
			Zone zone = unit->getCurrZone();
			const Vec2i &pos = unit->getPos();

			for(int x = 0; x < size; ++x) {
				for(int y = 0; y < size; ++y) {
					Vec2i currPos = pos + Vec2i(x, y);
					assert(map.isInside(currPos));

					if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
						Unit *unitInCell = map.getCell(currPos)->getUnit(zone);
						if(unitInCell != unit) {
							cerr << "Unit id " << unit->getId()
									<< " from faction " << fi->getIndex()
									<< "(type = " << unit->getType()->getName() << ")"
									<< " not in cells (" << currPos.x << ", " << currPos.y << ", " << zone << ")";
							if(unitInCell == NULL && !unit->getHp()) {
								cerr << " but has zero HP and is not executing scDie." << endl;
							} else {
								cerr << endl;
								assert(false);
							}
						}
						validationMap.validate(currPos.x, currPos.y, zone);
					}
				}
			}
		}
	}

	// make sure that every cell that was not validated is empty
	for(int x = 0; x < map.getW(); ++x) {
		for(int y = 0; y < map.getH(); ++y ) {
			for(int zone = 0; zone < ZoneCount; ++zone) {
				if(!validationMap.getCell(x, y, (Zone)zone)) {
					Cell *cell = map.getCell(x, y);
					if(cell->getUnit((Zone)zone)) {
						cerr << "Cell not empty at " << x << ", " << y << ", " << zone << endl;
						cerr << "Cell has pointer to unit object at " << cell->getUnit((Zone)zone) << endl;

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

void World::doHackyCleanUp() {
	for(Units::const_iterator u = newlydead.begin(); u != newlydead.end(); ++u) {
		Unit &unit = **u;
		for ( int i=0; i < unit.getSize(); ++i )
			for ( int j=0; j < unit.getSize(); ++j ) {
				Cell *cell = map.getCell ( unit.getPos() + Vec2i(i,j) );
				if(cell->getUnit(unit.getCurrZone()) == &unit)
					cell->setUnit(unit.getCurrZone(), NULL);
			}
	}
	newlydead.clear();
}


}} //end namespace
