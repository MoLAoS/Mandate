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
#include "ai.h"

#include <ctime>
#include <climits>

#include "ai_interface.h"
#include "ai_rule.h"
#include "unit_type.h"
#include "unit.h"
#include "program.h"
#include "config.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Plan {

// =====================================================
// 	class Task
// =====================================================

MEMORY_CHECK_IMPLEMENTATION(Task);

// =====================================================
// 	class ProduceTask
// =====================================================

ProduceTask::ProduceTask(UnitClass unitClass)
		: Task(TaskClass::PRODUCE)
		, unitClass(unitClass)
		, resourceType(0)
		, unitType(0) {
}

ProduceTask::ProduceTask(const ResourceType *resourceType)
		: Task(TaskClass::PRODUCE)
		, unitClass(UnitClass::INVALID)
		, resourceType(resourceType)
		, unitType(0) {
}

ProduceTask::ProduceTask(const UnitType *unitType) 
		: Task(TaskClass::PRODUCE)
		, unitClass(UnitClass::INVALID)
		, resourceType(0)
		, unitType(unitType) {
}

string ProduceTask::toString() const {
	string str = "Produce ";
	if (unitType) {
		str += unitType->getName();
	}
	return str;
}

// =====================================================
// 	class BuildTask
// =====================================================

BuildTask::BuildTask(const UnitType *unitType) 
		: Task(TaskClass::BUILD) {
	this->unitType = unitType;
	resourceType = NULL;
	forcePos = false;
}

BuildTask::BuildTask(const ResourceType *resourceType)
		: Task(TaskClass::BUILD) {
	unitType = NULL;
	this->resourceType = resourceType;
	forcePos = false;
}

BuildTask::BuildTask(const UnitType *unitType, const Vec2i &pos)
		: Task(TaskClass::BUILD) {
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

UpgradeTask::UpgradeTask(const UpgradeType *upgradeType)
		: Task(TaskClass::UPGRADE) {
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

void Ai::init(GlestAiInterface *aiInterface, int32 randomSeed) {
	this->aiInterface = aiInterface;
	random.init(randomSeed);
	startLoc = random.randRange(0, aiInterface->getMapMaxPlayers() - 1);
	upgradeCount = 0;
	minWarriors = minMinWarriors;
	randomMinWarriorsReached= false;
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

	// staticResourceUsed
	for (int i=0; i < aiInterface->getMyFactionType()->getUnitTypeCount(); ++i) {
		const UnitType *ut = aiInterface->getMyFactionType()->getUnitType(i);
		for (int j=0; j < ut->getCostCount(); ++j) {
			ResourceAmount r = ut->getCost(j, aiInterface->getFaction());
			if (r.getType()->getClass() == ResourceClass::STATIC && r.getAmount() > 0) {
				staticResourceUsed.insert(r.getType());
			}
		}
	}
	updateUsableResources();
}

void Ai::updateUsableResources() {
	const Faction *faction = aiInterface->getMyFaction();
	usableResources.clear();

	ResourceTypes typesNeeded;
	for (int i=0; i < aiInterface->getMyFactionType()->getUnitTypeCount(); ++i) {
		const UnitType *ut = aiInterface->getMyFactionType()->getUnitType(i);
		if (!faction->reqsOk(ut)) {
			continue;
		}
		int n = ut->getCommandTypeCount();
		for (int i=0; i < n; ++i) {
			const CommandType *ct = ut->getCommandType(i);
			if (!faction->reqsOk(ct)) {
				continue;
			}
			for (int j=0; j < ct->getProducedCount(); ++j) {
				const ProducibleType *pt = ct->getProduced(j);
				if (!faction->reqsOk(pt)) {
					continue;
				}
				for (int k=0; k < pt->getCostCount(); ++k) {
					ResourceAmount r = pt->getCost(k, aiInterface->getFaction());
					if (r.getType()->getClass() != ResourceClass::STATIC
					&& r.getType()->getClass() != ResourceClass::CONSUMABLE && r.getAmount() > 0) {
						typesNeeded.insert(r.getType());
					}
				}
			}
		}
	}
	foreach (UnitTypeCount, it, unitTypeCount) {
		const UnitType *ut = it->first;
		int n = ut->getCommandTypeCount<HarvestCommandType>();
		for (int i=0; i < n; ++i) {
			const HarvestCommandType *hct = ut->getCommandType<HarvestCommandType>(i);
			int rn = hct->getHarvestedResourceCount();
			for (int j=0; j < rn; ++j) {
				const ResourceType *rt = hct->getHarvestedResource(j);
				if (typesNeeded.find(rt) != typesNeeded.end()) {
					usableResources.insert(rt);
					typesNeeded.erase(rt);
				}
			}
		}
	}
}

Ai::~Ai() {
	deleteValues(tasks.begin(), tasks.end());
	deleteValues(aiRules.begin(), aiRules.end());
}

void Ai::update() {
	_PROFILE_FUNCTION();
	//process ai rules
	for (AiRules::iterator it = aiRules.begin(); it != aiRules.end(); ++it) {
		if ((aiInterface->getTimer() % ((*it)->getTestInterval() * WORLD_FPS / 1000)) == 0) {
			if ((*it)->test()) {
				LOG_AI(
					aiInterface->getFactionIndex(), AiComponent::GENERAL, 2,
					"Executing rule: " << (*it)->getName()
				);
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
	int amount = numeric_limits<int>::max();
	const ResourceType *neededResource = 0;

	foreach (ResourceTypes, it, usableResources) {
		const ResourceAmount *r = aiInterface->getResource(*it);
		if (r->getAmount() < amount) {
			amount = r->getAmount();
			neededResource = *it;
		}
	}
	return neededResource;
}

bool Ai::beingAttacked(Vec2i &pos, Field &field, int radius) {
	ConstUnitVector enemies;
	aiInterface->getUnitsSeen(enemies);
	foreach_const (ConstUnitVector, it, enemies) {
		pos = (*it)->getPos();
		field = (*it)->getCurrField();
		if (pos.dist(aiInterface->getHomeLocation()) < radius) {
			baseSeen = true;
			LOG_AI( aiInterface->getFactionIndex(), AiComponent::MILITARY, 1, "Being attacked at pos " << pos);
			return true;
		}
	}
	return false;
}

bool Ai::isStaticResourceUsed(const ResourceType *rt) const {
	assert(rt->getClass() == ResourceClass::STATIC);
	return (staticResourceUsed.find(rt) != staticResourceUsed.end());
}

bool Ai::isStableBase() {
	if (getCountOfClass(UnitClass::WARRIOR) > minWarriors) {
		LOG_AI( aiInterface->getFactionIndex(), AiComponent::MILITARY, 1, "Base is stable." );
		return true;
	} else {
		LOG_AI( aiInterface->getFactionIndex(), AiComponent::MILITARY, 1, "Base is not stable" );
		return false;
	}
}

bool Ai::findAbleUnit(int *unitIndex, CommandClass ability, bool idleOnly) {
	vector<int> units;

	*unitIndex = -1;
	for (int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		const Unit *unit = aiInterface->getMyUnit(i);
		if (unit->getType()->hasCommandClass(ability)) {
			if ((!idleOnly || !unit->anyCommand() || unit->getCurrCommand()->getType()->getClass() == CommandClass::STOP)) {
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
			if (unit->anyCommand() && unit->getCurrCommand()->getType()->getClass() == currentCommand) {
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
	const int spacing = 2;

	for (int currRadius = 0; currRadius < maxBuildRadius; ++currRadius) {
		for (int i = searchPos.x - currRadius; i < searchPos.x + currRadius; ++i) {
			for (int j = searchPos.y - currRadius; j < searchPos.y + currRadius; ++j) {
				outPos = Vec2i(i, j);
				if (aiInterface->areFreeCells(outPos - Vec2i(spacing), building->getSize() + spacing*2, Field::LAND)) {
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

		// build commands
		for (int i=0; i < uti->first->getCommandTypeCount<BuildCommandType>(); ++i) {
			const BuildCommandType *bct = uti->first->getCommandType<BuildCommandType>(i);

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
		}
		// upgrade commands
		for (int i=0; i < uti->first->getCommandTypeCount<UpgradeCommandType>(); ++i) {
			const UpgradeCommandType *uct = uti->first->getCommandType<UpgradeCommandType>(i);
			const UpgradeType *upgrade = uct->getProducedUpgrade();

			if (aiInterface->reqsOk(uct) && aiInterface->reqsOk(upgrade)) {
				availableUpgrades.push_back(upgrade);
			}
		}
		if (uti->first->hasSkillClass(SkillClass::BE_BUILT)) {
			buildingTypeCount[uti->first] = uti->second;
		}
	}

	//discover BuildTasks that are queued
	for(Tasks::iterator ti = tasks.begin(); ti != tasks.end(); ++ti) {
		if((*ti)->getClass() == TaskClass::BUILD) {
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
		} else if((*ti)->getClass() == TaskClass::UPGRADE) {
		}
	}

	//build needed building list
	for(uti = availableBuildings.begin(); uti != availableBuildings.end(); ++uti) {
		utj = buildingTypeCount.find(uti->first);
		if(utj == buildingTypeCount.end()) {
			neededBuildings.push_back(uti->first);
		}
	}
	updateUsableResources();
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
	LOG_AI( aiInterface->getFactionIndex(), AiComponent::GENERAL, 2, "Task added: " + task->toString() );
}

void Ai::addPriorityTask(const Task *task){
	deleteValues(tasks.begin(), tasks.end());
	tasks.clear();

	tasks.push_back(task);
	LOG_AI( aiInterface->getFactionIndex(), AiComponent::GENERAL, 2, "Priority Task added: " + task->toString() );
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
	LOG_AI( aiInterface->getFactionIndex(), AiComponent::GENERAL, 2, "Task removed: " + task->toString() );
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
		if(findAbleUnit(&unit, CommandClass::ATTACK, false)){
			aiInterface->giveCommand(unit, CommandClass::ATTACK, pos);
			LOG_AI( aiInterface->getFactionIndex(), AiComponent::MILITARY, 1, "Scout patrol sent to: " << pos );
		}
	}
}



void Ai::massiveAttack(const Vec2i &pos, Field field, bool ultraAttack){
	int producerWarriorCount=0;
	int maxProducerWarriors=random.randRange(1,11);
    for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
    	bool isWarrior;
        const Unit *unit= aiInterface->getMyUnit(i);
		const AttackCommandType *act= unit->getType()->getAttackCommand(field==Field::AIR?Zone::AIR:Zone::LAND);
		if(act!=NULL && unit->getType()->hasCommandClass(CommandClass::PRODUCE))
		{
			producerWarriorCount++;
		}
		
		if(aiInterface->getControlType()==ControlType::CPU_MEGA)
		{
			if(producerWarriorCount>maxProducerWarriors)
			{
				if(
					unit->getCommandCount()>0 &&
					unit->getCurrCommand()->getType()!=NULL && (
				    unit->getCurrCommand()->getType()->getClass()==CommandClass::BUILD || 
					unit->getCurrCommand()->getType()->getClass()==CommandClass::MORPH || 
					unit->getCurrCommand()->getType()->getClass()==CommandClass::PRODUCE
					)
				)
				{
					isWarrior=false;
				}
				else
				{
					isWarrior=!unit->getType()->hasCommandClass(CommandClass::HARVEST);
				}
			}
			else
			{
				isWarrior= !unit->getType()->hasCommandClass(CommandClass::HARVEST) && !unit->getType()->hasCommandClass(CommandClass::PRODUCE);  
			}
		}
		else
		{
			isWarrior= !unit->getType()->hasCommandClass(CommandClass::HARVEST) && !unit->getType()->hasCommandClass(CommandClass::PRODUCE);
		}
		
		
		bool alreadyAttacking= unit->getCurrSkill()->getClass()==SkillClass::ATTACK; 
		if(!alreadyAttacking && act!=NULL && (ultraAttack || isWarrior)){
			aiInterface->giveCommand(i, act, pos);
		}
    }

    if(aiInterface->getControlType()==ControlType::CPU_EASY)
	{
		minWarriors+= 1;
	}
	else if(aiInterface->getControlType()==ControlType::CPU_MEGA)
	{
		minWarriors+= 3;
		if(minWarriors>maxMinWarriors-1 || randomMinWarriorsReached)
		{
			randomMinWarriorsReached=true;
			minWarriors=random.randRange(maxMinWarriors-10, maxMinWarriors*2);
		}
	}
	else if(minWarriors<maxMinWarriors){
		minWarriors+= 3;
	}
	LOG_AI( aiInterface->getFactionIndex(), AiComponent::MILITARY, 1, "Massive attack to pos: " << pos );
}

void Ai::returnBase(int unitIndex){
    Vec2i pos;
    CommandResult r;

	pos= Vec2i(
		random.randRange(-villageRadius, villageRadius), random.randRange(-villageRadius, villageRadius)) +
		getRandomHomePosition();
    r= aiInterface->giveCommand(unitIndex, CommandClass::MOVE, pos);

	LOG_AI( 
		aiInterface->getFactionIndex(), AiComponent::MILITARY, 2, 
		"Order return to base pos:" << pos << " result: " << CommandResultNames[r]
	);
}

void Ai::harvest(int unitIndex){

	const ResourceType *rt= getNeededResource();

	if(rt!=NULL){
		const HarvestCommandType *hct= aiInterface->getMyUnit(unitIndex)->getType()->getHarvestCommand(rt);
		Vec2i resPos;
		if(hct!=NULL && aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), resPos)){
			resPos= resPos+Vec2i(random.randRange(-2, 2), random.randRange(-2, 2));
			aiInterface->giveCommand(unitIndex, hct, resPos);
			//aiInterface->printLog(4, "Order harvest pos:" + intToStr(resPos.x)+", "+intToStr(resPos.y)+": "+rrToStr(r)+"\n");
		}
	}
}

}}//end namespace
