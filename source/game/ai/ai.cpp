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
#include "ai.h"

#include <ctime>

#include "ai_interface.h"
#include "ai_rule.h"
#include "unit_type.h"
#include "unit.h"
#include "program.h"
#include "config.h"

#include "leak_dumper.h"


using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class ProduceTask
// =====================================================

ProduceTask::ProduceTask(UnitClass unitClass) {
	taskClass = tcProduce;
	this->unitClass = unitClass;
	unitType = NULL;
	resourceType = NULL;
}

ProduceTask::ProduceTask(const UnitType *unitType) {
	taskClass = tcProduce;
	this->unitType = unitType;
	resourceType = NULL;
}

ProduceTask::ProduceTask(const ResourceType *resourceType) {
	taskClass = tcProduce;
	unitType = NULL;
	this->resourceType = resourceType;
}

string ProduceTask::toString() const {
	string str = "Produce ";
	if (unitType != NULL) {
		str += unitType->getName();
	}
	return str;
}

// =====================================================
// 	class BuildTask
// =====================================================

BuildTask::BuildTask(const UnitType *unitType) {
	taskClass = tcBuild;
	this->unitType = unitType;
	resourceType = NULL;
	forcePos = false;
}

BuildTask::BuildTask(const ResourceType *resourceType) {
	taskClass = tcBuild;
	unitType = NULL;
	this->resourceType = resourceType;
	forcePos = false;
}

BuildTask::BuildTask(const UnitType *unitType, const Vec2i &pos) {
	taskClass = tcBuild;
	this->unitType = unitType;
	resourceType = NULL;
	forcePos = true;
	this->pos = pos;
}

string BuildTask::toString() const {
	string str = "Build ";
	if (unitType != NULL) {
		str += unitType->getName();
	}
	return str;
}

// =====================================================
// 	class UpgradeTask
// =====================================================

UpgradeTask::UpgradeTask(const UpgradeType *upgradeType) {
	taskClass = tcUpgrade;
	this->upgradeType = upgradeType;
}

string UpgradeTask::toString() const {
	string str = "Build ";
	if (upgradeType) {
		str += upgradeType->getName();
	}
	return str;
}

// =====================================================
// 	class Ai
// =====================================================

void Ai::init(AiInterface *aiInterface) {
	this->aiInterface = aiInterface;
	startLoc = random.randRange(0, aiInterface->getMapMaxPlayers() - 1);
	upgradeCount = 0;
	minWarriors = minMinWarriors;
	baseSeen = false;

	//add ai rules
	aiRules.resize(14);
	aiRules[0] = new AiRuleWorkerHarvest(this);
	aiRules[1] = new AiRuleRefreshHarvester(this);
	aiRules[2] = new AiRuleScoutPatrol(this);
	aiRules[3] = new AiRuleReturnBase(this);
	aiRules[4] = new AiRuleMassiveAttack(this);
	aiRules[5] = new AiRuleAddTasks(this);
	aiRules[6] = new AiRuleProduceResourceProducer(this);
	aiRules[7] = new AiRuleBuildOneFarm(this);
	aiRules[8] = new AiRuleProduce(this);
	aiRules[9] = new AiRuleBuild(this);
	aiRules[10] = new AiRuleUpgrade(this);
	aiRules[11] = new AiRuleExpand(this);
	aiRules[12] = new AiRuleRepair(this);
	aiRules[13] = new AiRuleRepair(this);
}

Ai::~Ai() {
	deleteValues(tasks.begin(), tasks.end());
	deleteValues(aiRules.begin(), aiRules.end());
}

void Ai::update() {
	//process ai rules
	for (AiRules::iterator it = aiRules.begin(); it != aiRules.end(); ++it) {
		if ((aiInterface->getTimer() % ((*it)->getTestInterval() * Config::getInstance().getGsWorldUpdateFps() / 1000)) == 0) {
			if ((*it)->test()) {
				aiInterface->printLog(3,
						intToStr(1000 * aiInterface->getTimer() / Config::getInstance().getGsWorldUpdateFps())
						+ ": Executing rule: " + (*it)->getName() + '\n');
				(*it)->execute();
			}
		}
	}
}


// ==================== state requests ====================

int Ai::getCountOfType(const UnitType *ut) {
	int count = 0;
	for (int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		if (ut == aiInterface->getMyUnit(i)->getType()) {
			count++;
		}
	}
	return count;
}

int Ai::getCountOfClass(UnitClass uc) {
	int count = 0;
	for (int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		if (aiInterface->getMyUnit(i)->getType()->isOfClass(uc)) {
			++count;
		}
	}
	return count;
}

float Ai::getRatioOfClass(UnitClass uc) {
	if (aiInterface->getMyUnitCount() == 0) {
		return 0;
	} else {
		return static_cast<float>(getCountOfClass(uc)) / aiInterface->getMyUnitCount();
	}
}

const ResourceType *Ai::getNeededResource() {

	int amount = -1;
	const ResourceType *neededResource = NULL;
	const TechTree *tt = aiInterface->getTechTree();

	for (int i = 0; i < tt->getResourceTypeCount(); ++i) {
		const ResourceType *rt = tt->getResourceType(i);
		const Resource *r = aiInterface->getResource(rt);
		if (rt->getClass() != rcStatic && rt->getClass() != rcConsumable && (r->getAmount() < amount || amount == -1)) {
			amount = r->getAmount();
			neededResource = rt;
		}
	}

	return neededResource;
}

bool Ai::beingAttacked(Vec2i &pos, Field &field, int radius) {
	int count = aiInterface->onSightUnitCount();
	const Unit *unit;

	for (int i = 0; i < count; ++i) {
		unit = aiInterface->getOnSightUnit(i);
		if (!aiInterface->isAlly(unit) && unit->isAlive()) {
			pos = unit->getPos();
			field = unit->getCurrField();
			if (pos.dist(aiInterface->getHomeLocation()) < radius) {
				baseSeen = true;
				aiInterface->printLog(2, "Being attacked at pos " + intToStr(pos.x) + "," + intToStr(pos.y) + "\n");
				return true;
			}
		}
	}
	return false;
}

bool Ai::isStableBase() {
	if (getCountOfClass(ucWarrior) > minWarriors) {
		aiInterface->printLog(4, "Base is stable\n");
		return true;
	} else {
		aiInterface->printLog(4, "Base is not stable\n");
		return false;
	}
}

bool Ai::findAbleUnit(int *unitIndex, CommandClass ability, bool idleOnly) {
	vector<int> units;

	*unitIndex = -1;
	for (int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		const Unit *unit = aiInterface->getMyUnit(i);
		if (unit->getType()->hasCommandClass(ability)) {
			if ((!idleOnly || !unit->anyCommand() || unit->getCurrCommand()->getType()->getClass() == ccStop) && !unit->isAPet()) {
				units.push_back(i);
			}
		}
	}

	if (units.empty()) {
		return false;
	} else {
		*unitIndex = units[random.randRange(0, units.size() - 1)];
		return true;
	}
}

bool Ai::findAbleUnit(int *unitIndex, CommandClass ability, CommandClass currentCommand) {
	vector<int> units;

	*unitIndex = -1;
	for (int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		const Unit *unit = aiInterface->getMyUnit(i);
		if (unit->getType()->hasCommandClass(ability)) {
			if (unit->anyCommand() && unit->getCurrCommand()->getType()->getClass() == currentCommand && !unit->isAPet()) {
				units.push_back(i);
			}
		}
	}

	if (units.empty()) {
		return false;
	} else {
		*unitIndex = units[random.randRange(0, units.size() - 1)];
		return true;
	}
}

bool Ai::findPosForBuilding(const UnitType* building, const Vec2i &searchPos, Vec2i &outPos) {
	const int spacing = 1;

	for (int currRadius = 0; currRadius < maxBuildRadius; ++currRadius) {
		for (int i = searchPos.x - currRadius; i < searchPos.x + currRadius; ++i) {
			for (int j = searchPos.y - currRadius; j < searchPos.y + currRadius; ++j) {
				outPos = Vec2i(i, j);
				if (aiInterface->areFreeCells(outPos - Vec2i(spacing), building->getSize() + spacing*2, FieldWalkable)) {
					return true;
				}
			}
		}
	}

	return false;
}

/**
 * statistics
 */
void Ai::updateStatistics() {
	UnitTypeCount::iterator uti;
	UnitTypeCount::iterator utj;

	unitTypeCount.clear();
	buildingTypeCount.clear();
	unspecifiedBuildTasks = 0;
	availableBuildings.clear();
	neededBuildings.clear();
	availableUpgrades.clear();

	//get a count of all unit types that we have
	for (int i = 0; i < aiInterface->getMyUnitCount(); ++i){
		const UnitType *ut = aiInterface->getMyUnit(i)->getType();

		uti = unitTypeCount.find(ut);
		if(uti == unitTypeCount.end()) {
			unitTypeCount[ut] = 1;
		} else {
			++uti->second;
		}
	}

	//get a list of all building types we can build and how many units can build
	//them.  Copy unit types that are buildings into builtOrBuilding.
	for(uti = unitTypeCount.begin(); uti != unitTypeCount.end(); ++uti) {

		//all commands for UnitType
		for(int i = 0; i < uti->first->getCommandTypeCount(); ++i) {
			const CommandType *ct = uti->first->getCommandType(i);
			if(ct->getClass() == ccBuild) {
				const BuildCommandType *bct = (const BuildCommandType *)ct;

				//all buildings that we can build now
				for(int j = 0; j < bct->getBuildingCount(); ++j) {
					const UnitType *buildingType = bct->getBuilding(j);

					if(aiInterface->reqsOk(bct) && aiInterface->reqsOk(buildingType)) {
						utj = availableBuildings.find(buildingType);
						if(utj == availableBuildings.end()) {
							availableBuildings[buildingType] = uti->second;
						} else {
							utj->second += uti->second;
						}
					}
				}
			} else if (ct->getClass() == ccUpgrade) {
				const UpgradeCommandType *uct = (const UpgradeCommandType*)ct;
				const UpgradeType *upgrade = uct->getProducedUpgrade();

				if (aiInterface->reqsOk(uct) && aiInterface->reqsOk(upgrade)) {
					availableUpgrades.push_back(upgrade);
				}
			}
		}

		if(uti->first->hasSkillClass(scBeBuilt)) {
			buildingTypeCount[uti->first] = uti->second;
		}
	}

	//discover BuildTasks that are queued
	for(Tasks::iterator ti = tasks.begin(); ti != tasks.end(); ++ti) {
		if((*ti)->getClass() == tcBuild) {
			const BuildTask *bt = (const BuildTask *)*ti;
			const UnitType *ut = bt->getUnitType();
			if(ut) {
				uti = buildingTypeCount.find(ut);
				if(uti == buildingTypeCount.end()) {
					buildingTypeCount[ut] = 1;
				} else {
					++uti->second;
				}
			} else {
				++unspecifiedBuildTasks;
			}
		} else if((*ti)->getClass() == tcUpgrade) {
		}
	}

	//build needed building list
	for(uti = availableBuildings.begin(); uti != availableBuildings.end(); ++uti) {
		utj = buildingTypeCount.find(uti->first);
		if(utj == buildingTypeCount.end()) {
			neededBuildings.push_back(uti->first);
		}
	}
}

bool Ai::isRepairable(const Unit *u) const {
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		if(aiInterface->getMyUnit(i)->getRepairCommandType(u)) {
			return true;
		}
	}
	return false;
}

// ==================== tasks ====================

void Ai::addTask(const Task *task){
	tasks.push_back(task);
	aiInterface->printLog(2, "Task added: " + task->toString());
}

void Ai::addPriorityTask(const Task *task){
	deleteValues(tasks.begin(), tasks.end());
	tasks.clear();

	tasks.push_back(task);
	aiInterface->printLog(2, "Priority Task added: " + task->toString());
}

bool Ai::anyTask(){
	return !tasks.empty();
}

const Task *Ai::getTask() const{
	if(tasks.empty()){
		return NULL;
	}
	else{
		return tasks.front();
	}
}

void Ai::removeTask(const Task *task){
	aiInterface->printLog(2, "Task removed: " + task->toString());
	tasks.remove(task);
	delete task;
}

void Ai::retryTask(const Task *task){
	tasks.remove(task);
	tasks.push_back(task);
}
// ==================== expansions ====================

void Ai::addExpansion(const Vec2i &pos){

	//check if there is a nearby expansion
	for(Positions::iterator it= expansionPositions.begin(); it!=expansionPositions.end(); ++it){
		if((*it).dist(pos)<villageRadius){
			return;
		}
	}

	//add expansion
	expansionPositions.push_front(pos);

	//remove expansion if queue is list is full
	if(expansionPositions.size()>maxExpansions){
		expansionPositions.pop_back();
	}
}

Vec2i Ai::getRandomHomePosition(){

	if(expansionPositions.empty() || random.randRange(0, 1) == 0){
		return aiInterface->getHomeLocation();
	}

	return expansionPositions[random.randRange(0, expansionPositions.size()-1)];
}

// ==================== actions ====================

void Ai::sendScoutPatrol(){
    Vec2i pos;
    int unit;

	startLoc= (startLoc+1) % aiInterface->getMapMaxPlayers();
	pos= aiInterface->getStartLocation(startLoc);

	if(aiInterface->getFactionIndex()!=startLoc){
		if(findAbleUnit(&unit, ccAttack, false)){
			aiInterface->giveCommand(unit, ccAttack, pos);
			aiInterface->printLog(2, "Scout patrol sent to: " + intToStr(pos.x)+","+intToStr(pos.y)+"\n");
		}
	}
}

void Ai::massiveAttack(const Vec2i &pos, Field field, bool ultraAttack){

    for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
        const Unit *unit= aiInterface->getMyUnit(i);
		const AttackCommandType *act= unit->getType()->getFirstAttackCommand(field==FieldAir?ZoneAir:ZoneSurface);
		bool isWarrior= !unit->getType()->hasCommandClass(ccHarvest) && !unit->getType()->hasCommandClass(ccProduce);
		bool alreadyAttacking= unit->getCurrSkill()->getClass()==scAttack;
		if(!alreadyAttacking && act!=NULL && (ultraAttack || isWarrior)){
			aiInterface->giveCommand(i, act, pos);
		}
    }
    if(minWarriors<maxMinWarriors){
		minWarriors+= 3;
	}
	aiInterface->printLog(2, "Massive attack to pos: "+ intToStr(pos.x)+", "+intToStr(pos.y)+"\n");
}

void Ai::returnBase(int unitIndex){
    Vec2i pos;
    CommandResult r;
    int fi;

    fi= aiInterface->getFactionIndex();
    pos= Vec2i(
		random.randRange(-villageRadius, villageRadius), random.randRange(-villageRadius, villageRadius)) +
		getRandomHomePosition();
    r= aiInterface->giveCommand(unitIndex, ccMove, pos);

    //aiInterface->printLog(1, "Order return to base pos:" + intToStr(pos.x)+", "+intToStr(pos.y)+": "+rrToStr(r)+"\n");
}

void Ai::harvest(int unitIndex){

	const ResourceType *rt= getNeededResource();

	if(rt!=NULL){
		const HarvestCommandType *hct= aiInterface->getMyUnit(unitIndex)->getType()->getFirstHarvestCommand(rt);
		Vec2i resPos;
		if(hct!=NULL && aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), resPos)){
			resPos= resPos+Vec2i(random.randRange(-2, 2), random.randRange(-2, 2));
			aiInterface->giveCommand(unitIndex, hct, resPos);
			//aiInterface->printLog(4, "Order harvest pos:" + intToStr(resPos.x)+", "+intToStr(resPos.y)+": "+rrToStr(r)+"\n");
		}
	}
}

}}//end namespace
