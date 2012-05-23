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
	LOG_AI(
		faction->getIndex(), AiComponent::GENERAL, 1,
		"GlestAiInterface::GlestAiInterface: Faction index: " << faction->getIndex() << ", Type:"
		<< faction->getType()->getName() << " Random seed: " << randomSeed
	);
	this->faction = faction;
	this->world= g_simInterface.getWorld();

	timer= 0;

	//init ai
	ai.init(this, randomSeed);
}

// ==================== main ====================

void GlestAiInterface::update(){
	timer++;
	ai.update();
}

// ==================== interaction ====================

CmdResult GlestAiInterface::giveCommand(int unitIndex, CmdClass commandClass, const Vec2i &pos){
	Command *c = g_world.newCommand(faction->getUnit(unitIndex)->getType()->getFirstCtOfClass(commandClass), CmdFlags(), pos);
	return faction->getUnit(unitIndex)->giveCommand(c);
}

CmdResult GlestAiInterface::giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos){
	return faction->getUnit(unitIndex)->giveCommand(g_world.newCommand(commandType, CmdFlags(), pos));
}

CmdResult GlestAiInterface::giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos,
											const ProducibleType* prodType) {
	return faction->getUnit(unitIndex)->giveCommand(
		g_world.newCommand(commandType, CmdFlags(), pos, prodType,CardinalDir::NORTH));
}

CmdResult GlestAiInterface::giveCommand(int unitIndex, const CommandType *commandType, Unit *u){
	return faction->getUnit(unitIndex)->giveCommand(g_world.newCommand(commandType, CmdFlags(), u));
}

CmdResult GlestAiInterface::giveCommand(const Unit *unit, const CommandType *commandType) {
	return const_cast<Unit*>(unit)->giveCommand(g_world.newCommand(commandType, CmdFlags()));
}

CmdResult GlestAiInterface::giveCommand(const Unit *unit, const CommandType *commandType, const Vec2i &pos,
											const ProducibleType* prodType) {
	return const_cast<Unit*>(unit)->giveCommand(
		g_world.newCommand(commandType, CmdFlags(), pos, prodType, CardinalDir::NORTH));
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

void GlestAiInterface::findEnemies(ConstUnitVector &out_list, ConstUnitPtr &out_closest) {
	assert(out_list.empty());
	float closestDist = numeric_limits<float>::infinity();
	Vec2i homePos = getHomeLocation();

	for (int i=0; i < world->getFactionCount(); ++i) {
		Faction *f = world->getFaction(i);
		if (faction == f || faction->getTeam() == f->getTeam()) {
			// don't care about our own or allies units
			continue;
		}
		foreach_const (Units, it, f->getUnits()) {
			const Unit *enemy = *it;
			if (enemy->isAlive() && faction->canSee(enemy)) {
				out_list.push_back(enemy);
				float dist = homePos.dist(enemy->getCenteredPos());
				if (dist < closestDist) {
					out_closest = enemy;
					closestDist = dist;
				}
			}
		}
	}
	if (out_list.empty()) {
		out_closest = 0;
	}
}

const StoredResource *GlestAiInterface::getResource(const ResourceType *rt){
	return faction->getSResource(rt);
}

const Unit *GlestAiInterface::getMyUnit(int unitIndex){
	return faction->getUnit(unitIndex);
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
				MapResource *r= map->getTile(surfPos)->getResource();

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
