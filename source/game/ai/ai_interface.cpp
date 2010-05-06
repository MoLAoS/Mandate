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
// 	class GlestAiInterface
// =====================================================

namespace Glest { namespace Plan {

GlestAiInterface::GlestAiInterface(Faction *faction, int32 randomSeed) {
	this->faction = faction;
	this->world= theSimInterface->getWorld();
	//this->commander= theSimInterface->getCommander();
	//this->console= theSimInterface->getGameState()->getConsole();

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

void GlestAiInterface::update(){
	timer++;
	ai.update();
}

// ==================== misc ====================

void GlestAiInterface::printLog(int logLevel, const string &s){
	if(this->logLevel>=logLevel){
		string logString= "(" + intToStr(faction->getIndex()) + ") " + s;

		//print log to file
		FILE *f= fopen(getLogFilename().c_str(), "at");
		if(f==NULL){
			throw runtime_error("Can't open file: "+getLogFilename());
		}
		fprintf(f, "%s\n", logString.c_str());
		fclose(f);

		//redirect to console
		if (redir) {
			theSimInterface->getGameState()->getConsole()->addLine(logString);
		}
	}
}

// ==================== interaction ====================

CommandResult GlestAiInterface::giveCommand(int unitIndex, CommandClass commandClass, const Vec2i &pos){
	Command *c= new Command (faction->getUnit(unitIndex)->getType()->getFirstCtOfClass(commandClass), CommandFlags(), pos);
	return faction->getUnit(unitIndex)->giveCommand(c);
}

CommandResult GlestAiInterface::giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos){
	return faction->getUnit(unitIndex)->giveCommand(new Command(commandType, CommandFlags(), pos));
}

CommandResult GlestAiInterface::giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos, const UnitType *ut){
	return faction->getUnit(unitIndex)->giveCommand(new Command(commandType, CommandFlags(), pos, ut));
}

CommandResult GlestAiInterface::giveCommand(int unitIndex, const CommandType *commandType, Unit *u){
	return faction->getUnit(unitIndex)->giveCommand(new Command(commandType, CommandFlags(), u));
}

// ==================== get data ====================

int GlestAiInterface::getMapMaxPlayers(){
	return world->getMaxPlayers();
}

Vec2i GlestAiInterface::getHomeLocation(){
	return world->getMap()->getStartLocation(faction->getStartLocationIndex());
}

Vec2i GlestAiInterface::getStartLocation(int loactionIndex){
	return world->getMap()->getStartLocation(loactionIndex);
}

int GlestAiInterface::getFactionCount(){
	return world->getFactionCount();
}

int GlestAiInterface::getMyUnitCount() const{
	return faction->getUnitCount();
}

int GlestAiInterface::getMyUpgradeCount() const{
	return faction->getUpgradeManager()->getUpgradeCount();
}

int GlestAiInterface::onSightUnitCount(){
	int count=0;
	Map *map= world->getMap();
	for(int i=0; i<world->getFactionCount(); ++i){
		for(int j=0; j<world->getFaction(i)->getUnitCount(); ++j){
			Tile *sc= map->getTile(Map::toTileCoords(world->getFaction(i)->getUnit(j)->getPos()));
			if(sc->isVisible(faction->getTeam())){
				count++;
			}
		}
	}
	return count;
}

const Resource *GlestAiInterface::getResource(const ResourceType *rt){
	return faction->getResource(rt);
}

const Unit *GlestAiInterface::getMyUnit(int unitIndex){
	return faction->getUnit(unitIndex);
}

const Unit *GlestAiInterface::getOnSightUnit(int unitIndex){

	int count=0;
	Map *map= world->getMap();

	for(int i=0; i<world->getFactionCount(); ++i){
		for(int j=0; j<world->getFaction(i)->getUnitCount(); ++j){
			Unit *u= world->getFaction(i)->getUnit(j);
			if(map->getTile(Map::toTileCoords(u->getPos()))->isVisible(faction->getTeam())){
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

const FactionType * GlestAiInterface::getMyFactionType(){
	return faction->getType();
}

const ControlType GlestAiInterface::getControlType(){
	return faction->getControlType();
}

const TechTree *GlestAiInterface::getTechTree(){
	return world->getTechTree();
}

bool GlestAiInterface::getNearestSightedResource(const ResourceType *rt, const Vec2i &pos, Vec2i &resultPos){
	float tmpDist;

	float nearestDist = numeric_limits<float>::infinity();
	bool anyResource= false;

	const Map *map= world->getMap();

	for(int i=0; i<map->getW(); ++i){
		for(int j=0; j<map->getH(); ++j){
			Vec2i surfPos= Map::toTileCoords(Vec2i(i, j));

			//if explored cell
			if(map->getTile(surfPos)->isExplored(faction->getTeam())){
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

bool GlestAiInterface::isAlly(const Unit *unit) const{
	return faction->isAlly(unit->getFaction());
}
bool GlestAiInterface::reqsOk(const RequirableType *rt){
	return faction->reqsOk(rt);
}

bool GlestAiInterface::reqsOk(const CommandType *ct){
	return faction->reqsOk(ct);
}

bool GlestAiInterface::checkCosts(const ProducibleType *pt){
	return faction->checkCosts(pt);
}

bool GlestAiInterface::areFreeCells(const Vec2i &pos, int size, Field field){
	return world->getMap()->areFreeCells(pos, size, field);
}

}}//end namespace
