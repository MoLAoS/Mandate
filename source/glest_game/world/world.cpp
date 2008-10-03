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

#include "world.h"

#include <algorithm>
#include <cassert>

#include "config.h"
#include "faction.h"
#include "unit.h"
#include "game.h"
#include "logger.h"
#include "sound_renderer.h"
#include "game_settings.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class World
// =====================================================

const float World::airHeight= 5.f;

// ===================== PUBLIC ========================

World::World(){
	Config &config= Config::getInstance();

	fogOfWar= config.getFogOfWar();
	fogOfWarSmoothing= config.getFogOfWarSmoothing();
	fogOfWarSmoothingFrameSkip= config.getFogOfWarSmoothingFrameSkip();

	frameCount= 0;
	nextUnitId= 0;
}

void World::end(){
    Logger::getInstance().add("World", true);

	for(int i= 0; i<factions.size(); ++i){
		factions[i].end();
	}
	//stats will be deleted by BattleEnd
}

void World::save(XmlNode *node) const {
	node->addChild("frameCount", frameCount);
	node->addChild("nextUnitId", nextUnitId);

	stats.save(node->addChild("stats"));
	XmlNode *factionsNode = node->addChild("factions");
	for(Factions::const_iterator i = factions.begin(); i != factions.end(); ++i) {
		i->save(factionsNode->addChild("faction"));
	}
}

// ========================== init ===============================================

void World::init(Game *game, const XmlNode *worldNode) {

	unitUpdater.init(game);

	initFactionTypes(game->getGameSettings());
	initCells(); //must be done after knowing faction number and dimensions
	initMap();
	initSplattedTextures();

	//minimap must be init after sum computation
	initMinimap();

	if(worldNode) {
		loadSaved(game->getGameSettings(), worldNode);
	} else {
		initUnits();
	}
	initExplorationState();
	computeFow();
}

//load tileset
void World::loadTileset(const string &dir, Checksum *checksum){
	tileset.load(dir, checksum);
	timeFlow.init(&tileset);
}

//load tech
void World::loadTech(const string &dir, Checksum *checksum){
	 techTree.load(dir, checksum);
}

//load map
void World::loadMap(const string &path, Checksum *checksum){
	checksum->addFile(path);
	map.load(path, &techTree, &tileset);
}

//load saved game
void World::loadSaved(GameSettings *gs, const XmlNode *worldNode) {
	Logger::getInstance().add("Loading saved game", true);
	this->thisFactionIndex = gs->getThisFactionIndex();
	this->thisTeamIndex = gs->getTeam(thisFactionIndex);

	frameCount = worldNode->getChildIntValue("frameCount");
	nextUnitId = worldNode->getChildIntValue("nextUnitId");

	stats.load(worldNode->getChild("stats"));

	const XmlNode *factionsNode = worldNode->getChild("factions");
	factions.resize(factionsNode->getChildCount());
	for (int i = 0; i < factionsNode->getChildCount(); ++i){
		const XmlNode *n = factionsNode->getChild("faction", i);
		const FactionType *ft= techTree.getType(gs->getFactionTypeName(i));
		factions[i].load(n, this, ft, gs->getFactionControl(i), &techTree);
	}

	map.computeNormals();
	map.computeInterpolatedHeights();

	thisTeamIndex= getFaction(thisFactionIndex)->getTeam();
	//TODO: load map resouces & fog of war (explored cells) for each faction or team
}

// ==================== misc ====================

void World::update(){

	++frameCount;
	int jack = 0;

	//time
	timeFlow.update();

	//water effects
	waterEffects.update();

	//units for(Faction *f = getFaction(0); f < getFaction(getFactionCount()); f++) { for(Unit *u = f->getUnit(0); u < f->getUnit(getUnitCount()); u++) {
	for(int i=0; i<getFactionCount(); ++i){
		for(int j=0; j<getFaction(i)->getUnitCount(); ++j){
			unitUpdater.updateUnit(getFaction(i)->getUnit(j));
		}
	}

	//undertake the dead
	for(int i=0; i<getFactionCount(); ++i){
		for(int j=0; j<getFaction(i)->getUnitCount(); ++j){
			Unit *unit= getFaction(i)->getUnit(j);
			if(unit->getToBeUndertaken()){
				unit->undertake();
				delete unit;
				j--;
			}
		}
	}

	//food costs
	for(int i=0; i<techTree.getResourceTypeCount(); ++i){
		const ResourceType *rt= techTree.getResourceType(i);
		if(rt->getClass()==rcConsumable && frameCount % (rt->getInterval()*GameConstants::updateFps)==0){
			for(int i=0; i<getFactionCount(); ++i){
				getFaction(i)->applyCostsOnInterval();
			}
		}
	}

	//fow smoothing
	if(fogOfWarSmoothing && ((frameCount+1) % (fogOfWarSmoothingFrameSkip+1))==0){
		float fogFactor= static_cast<float>(frameCount%GameConstants::updateFps)/GameConstants::updateFps;
		minimap.updateFowTex(clamp(fogFactor, 0.f, 1.f));
	}

	//tick
	if(frameCount%GameConstants::updateFps==0){
		computeFow();
		tick();
	}
}

void World::doKill(Unit *killer, Unit *killed) {
	int kills = 1 + killed->killPets();
	for(int i = 0; i < kills; i++){
		stats.kill(killer->getFactionIndex(), killed->getFactionIndex());
		killer->incKills();
	}
}

void World::tick(){
	if(!fogOfWarSmoothing){
		minimap.updateFowTex(1.f);
	}

	//apply regen/degen
	for(int i=0; i<getFactionCount(); ++i){
		for(int j=0; j<getFaction(i)->getUnitCount(); ++j){
			Unit *killer = getFaction(i)->getUnit(j)->tick();
			if(killer) {
				doKill(killer, getFaction(i)->getUnit(j));
			}
		}
	}

	//compute resources balance
	for(int k=0; k<getFactionCount(); ++k){
		Faction *faction= getFaction(k);

		//for each resource
		for(int i=0; i<techTree.getResourceTypeCount(); ++i){
			const ResourceType *rt= techTree.getResourceType(i);

			//if consumable
			if(rt->getClass()==rcConsumable){
				int balance= 0;
				for(int j=0; j<faction->getUnitCount(); ++j){

					//if unit operative and has this cost
					const Unit *u=  faction->getUnit(j);
					if(u->isOperative()){
						const Resource *r= u->getType()->getCost(rt);
						if(r!=NULL){
							balance-= u->getType()->getCost(rt)->getAmount();
						}
					}
				}
				faction->setResourceBalance(rt, balance);
			}
		}
	}
}

Unit* World::findUnitById(int id){
	for(int i= 0; i<getFactionCount(); ++i){
		Faction* faction= getFaction(i);

		for(int j= 0; j<faction->getUnitCount(); ++j){
			Unit* unit= faction->getUnit(j);

			if(unit->getId()==id){
				return unit;
			}
		}
	}
	return NULL;
}

const UnitType* World::findUnitTypeById(const FactionType* factionType, int id){
	for(int i= 0; i<factionType->getUnitTypeCount(); ++i){
		const UnitType* unitType= factionType->getUnitType(i);

		if(unitType->getId()==id){
			return unitType;
		}
	}
	return NULL;
}

//looks for a place for a unit around a start lociacion, returns true if succeded
bool World::placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated){
    bool freeSpace;
	int size= unit->getType()->getSize();
	Field currField= unit->getCurrField();

    for(int r=1; r<radius; r++){
        for(int i=-r; i<r; ++i){
            for(int j=-r; j<r; ++j){
                Vec2i pos= Vec2i(i,j)+startLoc;
				if(spaciated){
                    const int spacing= 2;
					freeSpace= map.isFreeCells(pos-Vec2i(spacing), size+spacing*2, currField);
				}
				else{
                    freeSpace= map.isFreeCells(pos, size, currField);
				}

                if(freeSpace){
                    unit->setPos(pos);
					unit->setMeetingPos(pos-Vec2i(1), false);
                    return true;
                }
            }
        }
    }
    return false;
}

//clears a unit old position from map and places new position
void World::moveUnitCells(Unit *unit){
	Vec2i newPos= unit->getTargetPos();

	//newPos must be free or the same pos as current
	assert(map.getCell(unit->getPos())->getUnit(unit->getCurrField())==unit || map.isFreeCell(newPos, unit->getCurrField()));
	map.clearUnitCells(unit, unit->getPos());
	map.putUnitCells(unit, newPos);

	//water splash
	if(tileset.getWaterEffects() && unit->getCurrField()==fLand){
		if(map.getSubmerged(map.getCell(unit->getLastPos()))){
			for(int i=0; i<3; ++i){
				waterEffects.addWaterSplash(
					Vec2f(unit->getLastPos().x+random.randRange(-0.4f, 0.4f), unit->getLastPos().y+random.randRange(-0.4f, 0.4f)));
			}
		}
	}
}

//returns the nearest unit that can store a type of resource given a position and a faction
Unit *World::nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt){
    float currDist= infinity;
    Unit *currUnit= NULL;

    for(int i=0; i<getFaction(factionIndex)->getUnitCount(); ++i){
		Unit *u= getFaction(factionIndex)->getUnit(i);
		float tmpDist= u->getPos().dist(pos);
        if(tmpDist<currDist && u->getType()->getStore(rt)>0 && u->isOperative()){
            currDist= tmpDist;
            currUnit= u;
        }
    }
    return currUnit;
}

bool World::toRenderUnit(const Unit *unit, const Quad2i &visibleQuad) const{
    //a unit is rendered if it is in a visible cell or is attacking a unit in a visible cell
    return
		visibleQuad.isInside(unit->getPos()) &&
		toRenderUnit(unit);
}

bool World::toRenderUnit(const Unit *unit) const{

	return
        map.getSurfaceCell(Map::toSurfCoords(unit->getCenteredPos()))->isVisible(thisTeamIndex) ||
        (unit->getCurrSkill()->getClass()==scAttack &&
        map.getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()))->isVisible(thisTeamIndex));
}


// ==================== PRIVATE ====================

// ==================== private init ====================

//init basic cell state
void World::initCells(){

	Logger::getInstance().add("State cells", true);
    for(int i=0; i<map.getSurfaceW(); ++i){
        for(int j=0; j<map.getSurfaceH(); ++j){

			SurfaceCell *sc= map.getSurfaceCell(i, j);

			sc->setFowTexCoord(Vec2f(
				i/(next2Power(map.getSurfaceW())-1.f),
				j/(next2Power(map.getSurfaceH())-1.f)));

			for(int k=0; k<GameConstants::maxPlayers; k++){
				sc->setExplored(k, false);
				sc->setVisible(k, 0);
            }
		}
    }
}

//init surface textures
void World::initSplattedTextures(){
	for(int i=0; i<map.getSurfaceW()-1; ++i){
        for(int j=0; j<map.getSurfaceH()-1; ++j){
			Vec2f coord;
			const Texture2D *texture;
			SurfaceCell *sc00= map.getSurfaceCell(i, j);
			SurfaceCell *sc10= map.getSurfaceCell(i+1, j);
			SurfaceCell *sc01= map.getSurfaceCell(i, j+1);
			SurfaceCell *sc11= map.getSurfaceCell(i+1, j+1);
			tileset.addSurfTex(
				sc00->getSurfaceType(),
				sc10->getSurfaceType(),
				sc01->getSurfaceType(),
				sc11->getSurfaceType(),
				coord, texture);
			sc00->setSurfTexCoord(coord);
			sc00->setSurfaceTexture(texture);
		}
	}
}

//creates each faction looking at each faction name contained in GameSettings
void World::initFactionTypes(GameSettings *gs){
	Logger::getInstance().add("Faction types", true);

	if(gs->getFactionCount() > map.getMaxPlayers()){
		throw runtime_error("This map only supports "+intToStr(map.getMaxPlayers())+" players");
	}

	//create stats
	stats.init(gs->getFactionCount(), gs->getThisFactionIndex(), gs->getDescription());

	//create factions
	this->thisFactionIndex= gs->getThisFactionIndex();
	factions.resize(gs->getFactionCount());
	for(int i=0; i<factions.size(); ++i){
		const FactionType *ft= techTree.getType(gs->getFactionTypeName(i));
		factions[i].init(
			ft, gs->getFactionControl(i), &techTree, i, gs->getTeam(i),
			gs->getStartLocationIndex(i), i==thisFactionIndex);

		stats.setTeam(i, gs->getTeam(i));
		stats.setFactionTypeName(i, formatString(gs->getFactionTypeName(i)));
		stats.setControl(i, gs->getFactionControl(i));
	}

	thisTeamIndex= getFaction(thisFactionIndex)->getTeam();
}

void World::initMinimap(){
    minimap.init(map.getW(), map.getH(), this);
	Logger::getInstance().add("Compute minimap surface", true);
}

//place units randomly aroud start location
void World::initUnits(){

	Logger::getInstance().add("Generate elements", true);

	//put starting units
	for(int i=0; i<getFactionCount(); ++i){
		Faction *f= &factions[i];
		const FactionType *ft= f->getType();
		for(int j=0; j<ft->getStartingUnitCount(); ++j){
			const UnitType *ut= ft->getStartingUnit(j);
			int initNumber= ft->getStartingUnitAmount(j);
			for(int l=0; l<initNumber; l++){
				Unit *unit= new Unit(getNextUnitId(), Vec2i(0), ut, f, &map);
				int startLocationIndex= f->getStartLocationIndex();

				if(placeUnit(map.getStartLocation(startLocationIndex), generationArea, unit, true)){
					unit->create(true);
					unit->born();
				}
				else{
					throw runtime_error("Unit cant be placed, this error is caused because there is no enough place to put the units near its start location, make a better map: "+unit->getType()->getName() + " Faction: "+intToStr(i));
				}
				if(unit->getType()->hasSkillClass(scBeBuilt)){
                    map.flatternTerrain(unit);
				}
            }
		}
	}
	map.computeNormals();
	map.computeInterpolatedHeights();
}

void World::initMap(){
	map.init();
}

void World::initExplorationState(){
	if(!fogOfWar){
		for(int i=0; i<map.getSurfaceW(); ++i){
			for(int j=0; j<map.getSurfaceH(); ++j){
				map.getSurfaceCell(i, j)->setVisible(thisTeamIndex, true);
				map.getSurfaceCell(i, j)->setExplored(thisTeamIndex, true);
			}
		}
	}
}


// ==================== exploration ====================

void World::exploreCells(const Vec2i &newPos, int sightRange, int teamIndex){

	Vec2i newSurfPos= Map::toSurfCoords(newPos);
	int surfSightRange= sightRange/Map::cellScale+1;

	//explore
    for(int i=-surfSightRange-indirectSightRange-1; i<=surfSightRange+indirectSightRange+1; ++i){
        for(int j=-surfSightRange-indirectSightRange-1; j<=surfSightRange+indirectSightRange+1; ++j){
			Vec2i currRelPos= Vec2i(i, j);
            Vec2i currPos= newSurfPos + currRelPos;
            if(map.isInsideSurface(currPos)){

				SurfaceCell *sc= map.getSurfaceCell(currPos);

				//explore
				if(Vec2i(0).dist(currRelPos) < surfSightRange+indirectSightRange+1){
                    sc->setExplored(teamIndex, true);
				}

				//visible
				if(Vec2i(0).dist(currRelPos) < surfSightRange){
					sc->setVisible(teamIndex, true);
				}
            }
        }
    }
}

//computes the fog of war texture, contained in the minimap
void World::computeFow(){

	//reset texture
	minimap.resetFowTex();

	//reset cells
	for(int i=0; i<map.getSurfaceW(); ++i){
		for(int j=0; j<map.getSurfaceH(); ++j){
			for(int k=0; k<GameConstants::maxPlayers; ++k){
				if(fogOfWar || k!=thisTeamIndex){
					map.getSurfaceCell(i, j)->setVisible(k, false);
				}
			}
		}
	}

	//compute cells
	for(int i=0; i<getFactionCount(); ++i){
		for(int j=0; j<getFaction(i)->getUnitCount(); ++j){
			Unit *unit= getFaction(i)->getUnit(j);

			//exploration
			if(unit->isOperative()){
				exploreCells(unit->getCenteredPos(), unit->getSight(), unit->getTeam());
			}
		}
	}

	//fire
	for(int i=0; i<getFactionCount(); ++i){
		for(int j=0; j<getFaction(i)->getUnitCount(); ++j){
			Unit *unit= getFaction(i)->getUnit(j);

			//fire
			ParticleSystem *fire= unit->getFire();
			if(fire!=NULL){
				fire->setActive(map.getSurfaceCell(Map::toSurfCoords(unit->getPos()))->isVisible(thisTeamIndex));
			}
		}
	}

	//compute texture
	for(int i=0; i<getFactionCount(); ++i){
		Faction *faction= getFaction(i);
		if(faction->getTeam()==thisTeamIndex){
			for(int j=0; j<faction->getUnitCount(); ++j){
				const Unit *unit= faction->getUnit(j);
				if(unit->isOperative()){
					int sightRange= unit->getSight();

					//iterate through all cells
					PosCircularIterator pci(&map, unit->getPos(), sightRange+indirectSightRange);
					while(pci.next()){
						Vec2i pos= pci.getPos();
						Vec2i surfPos= Map::toSurfCoords(pos);


						//compute max alpha
						float maxAlpha;
						if(surfPos.x>1 && surfPos.y>1 && surfPos.x<map.getSurfaceW()-2 && surfPos.y<map.getSurfaceH()-2){
							maxAlpha= 1.f;
						}
						else if(surfPos.x>0 && surfPos.y>0 && surfPos.x<map.getSurfaceW()-1 && surfPos.y<map.getSurfaceH()-1){
							maxAlpha= 0.3f;
						}
						else{
							maxAlpha= 0.0f;
						}

						//compute alpha
						float alpha;
						float dist= unit->getPos().dist(pos);
						if(dist>sightRange){
							alpha= clamp(1.f-(dist-sightRange)/(indirectSightRange), 0.f, maxAlpha);
						}
						else{
							alpha= maxAlpha;
						}
						minimap.incFowTextureAlphaSurface(surfPos, alpha);
					}
				}
			}
		}
	}
}

}}//end namespace
