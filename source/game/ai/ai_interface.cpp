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

#include "pch.h"
#include "ai_interface.h"

#include "ai.h"
#include "command_type.h"
#include "faction.h"
#include "unit.h"
#include "unit_type.h"
#include "object.h"
#include "game.h"
#include "config.h"

#include "leak_dumper.h"
#include "sim_interface.h"

using namespace Glest::Sim;
using namespace Shared::Util;
using namespace Shared::Graphics;

// =====================================================
// 	class AiInterface
// =====================================================

namespace Glest{ namespace Game{

AiInterface::AiInterface(GameState &game, int factionIndex, int teamIndex, int32 randomSeed){
	this->world= theSimInterface->getWorld();
	this->commander= theSimInterface->getCommander();
	this->console= game.getConsole();

	this->factionIndex= factionIndex;
	this->teamIndex= teamIndex;
	timer= 0;

	//init ai
	ai.init(this, randomSeed);

	//config
	logLevel= Config::getInstance().getMiscAiLog();
	redir= Config::getInstance().getMiscAiRedir();

	//clear log file
	if(logLevel>0){
		FILE *f= fopen(getLogFilename().c_str(), "wt");
		if(f==NULL){
			throw runtime_error("Can't open file: "+getLogFilename());
		}
		fprintf(f, "%s", "Glest AI log file\n\n");
		fclose(f);
	}
}

// ==================== main ====================

void AiInterface::update(){
	timer++;
	ai.update();
}

// ==================== misc ====================

void AiInterface::printLog(int logLevel, const string &s){
	if(this->logLevel>=logLevel){
		string logString= "(" + intToStr(factionIndex) + ") " + s;

		//print log to file
		FILE *f= fopen(getLogFilename().c_str(), "at");
		if(f==NULL){
			throw runtime_error("Can't open file: "+getLogFilename());
		}
		fprintf(f, "%s\n", logString.c_str());
		fclose(f);

		//redirect to console
		if(redir) {
			console->addLine(logString);
		}
	}
}

// ==================== interaction ====================

CommandResult AiInterface::giveCommand(int unitIndex, CommandClass commandClass, const Vec2i &pos){
	Command *c= new Command (world->getFaction(factionIndex)->getUnit(unitIndex)->getType()->getFirstCtOfClass(commandClass), CommandFlags(), pos);
	return world->getFaction(factionIndex)->getUnit(unitIndex)->giveCommand(c);
}

CommandResult AiInterface::giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos){
	return world->getFaction(factionIndex)->getUnit(unitIndex)->giveCommand(new Command(commandType, CommandFlags(), pos));
}

CommandResult AiInterface::giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos, const UnitType *ut){
	return world->getFaction(factionIndex)->getUnit(unitIndex)->giveCommand(new Command(commandType, CommandFlags(), pos, ut));
}

CommandResult AiInterface::giveCommand(int unitIndex, const CommandType *commandType, Unit *u){
	return world->getFaction(factionIndex)->getUnit(unitIndex)->giveCommand(new Command(commandType, CommandFlags(), u));
}

// ==================== get data ====================

int AiInterface::getMapMaxPlayers(){
	return world->getMaxPlayers();
}

Vec2i AiInterface::getHomeLocation(){
	return world->getMap()->getStartLocation(world->getFaction(factionIndex)->getStartLocationIndex());
}

Vec2i AiInterface::getStartLocation(int loactionIndex){
	return world->getMap()->getStartLocation(loactionIndex);
}

int AiInterface::getFactionCount(){
	return world->getFactionCount();
}

int AiInterface::getMyUnitCount() const{
	return world->getFaction(factionIndex)->getUnitCount();
}

int AiInterface::getMyUpgradeCount() const{
	return world->getFaction(factionIndex)->getUpgradeManager()->getUpgradeCount();
}

int AiInterface::onSightUnitCount(){
	int count=0;
	Map *map= world->getMap();
	for(int i=0; i<world->getFactionCount(); ++i){
		for(int j=0; j<world->getFaction(i)->getUnitCount(); ++j){
			Tile *sc= map->getTile(Map::toTileCoords(world->getFaction(i)->getUnit(j)->getPos()));
			if(sc->isVisible(teamIndex)){
				count++;
			}
		}
	}
	return count;
}

const Resource *AiInterface::getResource(const ResourceType *rt){
	return world->getFaction(factionIndex)->getResource(rt);
}

const Unit *AiInterface::getMyUnit(int unitIndex){
	return world->getFaction(factionIndex)->getUnit(unitIndex);
}

const Unit *AiInterface::getOnSightUnit(int unitIndex){

	int count=0;
	Map *map= world->getMap();

	for(int i=0; i<world->getFactionCount(); ++i){
		for(int j=0; j<world->getFaction(i)->getUnitCount(); ++j){
			Unit *u= world->getFaction(i)->getUnit(j);
			if(map->getTile(Map::toTileCoords(u->getPos()))->isVisible(teamIndex)){
				if(count==unitIndex){
					return u;
				}
				else{
					count ++;
				}
			}
		}
	}
	return NULL;
}

const FactionType * AiInterface::getMyFactionType(){
	return world->getFaction(factionIndex)->getType();
}

const ControlType AiInterface::getControlType(){
	return world->getFaction(factionIndex)->getControlType();
}

const TechTree *AiInterface::getTechTree(){
	return world->getTechTree();
}

bool AiInterface::getNearestSightedResource(const ResourceType *rt, const Vec2i &pos, Vec2i &resultPos){
	float tmpDist;

	float nearestDist = numeric_limits<float>::infinity();
	bool anyResource= false;

	const Map *map= world->getMap();

	for(int i=0; i<map->getW(); ++i){
		for(int j=0; j<map->getH(); ++j){
			Vec2i surfPos= Map::toTileCoords(Vec2i(i, j));

			//if explored cell
			if(map->getTile(surfPos)->isExplored(teamIndex)){
				Resource *r= map->getTile(surfPos)->getResource();

				//if resource cell
				if(r!=NULL && r->getType()==rt){
					tmpDist= pos.dist(Vec2i(i, j));
					if(tmpDist<nearestDist){
						anyResource= true;
						nearestDist= tmpDist;
						resultPos= Vec2i(i, j);
					}
				}
			}
		}
	}
	return anyResource;
}

bool AiInterface::isAlly(const Unit *unit) const{
	return world->getFaction(factionIndex)->isAlly(unit->getFaction());
}
bool AiInterface::reqsOk(const RequirableType *rt){
	return world->getFaction(factionIndex)->reqsOk(rt);
}

bool AiInterface::reqsOk(const CommandType *ct){
	return world->getFaction(factionIndex)->reqsOk(ct);
}

bool AiInterface::checkCosts(const ProducibleType *pt){
	return world->getFaction(factionIndex)->checkCosts(pt);
}

bool AiInterface::areFreeCells(const Vec2i &pos, int size, Field field){
	return world->getMap()->areFreeCells(pos, size, field);
}

}}//end namespace
