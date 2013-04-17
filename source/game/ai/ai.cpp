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
	string str = "Research ";
	if (upgradeType) {
		str += upgradeType->getName();
	}
	return str;
}

//
// Logging macros...
//

#define FACTION_INDEX aiInterface->getFactionIndex()

#define AI_LOG(component, level, message)                                                 \
	if (g_logger.shouldLogAiEvent(FACTION_INDEX, Util::AiComponent::component, level)) {  \
		LOG_AI( FACTION_INDEX, Util::AiComponent::component, level, message );            \
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

	// 1. Find all tech and tileset resources we can spend on something (anything)
	ResourceTypes typesNeeded;
	for (int i=0; i < aiInterface->getMyFactionType()->getUnitTypeCount(); ++i) {
		const UnitType *ut = aiInterface->getMyFactionType()->getUnitType(i);
		if (!faction->reqsOk(ut)) {
			continue;
		}
		int n = ut->getActions()->getCommandTypeCount();
		for (int i=0; i < n; ++i) {
			const CommandType *ct = ut->getActions()->getCommandType(i);
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
	// 2. Find all currently harvestable resources, and put any also found in step 1 in usableResources
	foreach (UnitTypeCount, it, unitTypeCount) {
		const UnitType *ut = it->first;
		int n = ut->getActions()->getCommandTypeCount<HarvestCommandType>();
		for (int i=0; i < n; ++i) {
			const HarvestCommandType *hct = ut->getActions()->getCommandType<HarvestCommandType>(i);
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
	foreach (AiRules, it, aiRules) { // process ai rules
		if ((aiInterface->getTimer() % ((*it)->getTestInterval() * WORLD_FPS / 1000)) == 0) {
			if ((*it)->test()) {
				AI_LOG( GENERAL, 3, "Ai::update: Executing rule: " << (*it)->getName() );
				(*it)->execute();
			}
		}
	}
}

void Ai::evaluateEnemies() {

	// 1. maintain knownEnemyLocations
	// remove if enemy building is dead and visible, null Unit pointers if dead and not visible
	const int teamIndex = aiInterface->getFaction()->getTeam();
	UnitPositions::iterator it = m_knownEnemyLocations.begin();
	UnitPositions::iterator itEnd = m_knownEnemyLocations.end();
	vector<Vec2i> remList;
	while (it != itEnd) {
		const Vec2i tPos = Map::toTileCoords(it->first);
		bool canSee = g_map.getTile(tPos)->isVisible(teamIndex);
		bool dead = it->second == 0;
		if (canSee) {
			if (dead || it->second->isDead()) {
				remList.push_back(it->first);
			}
		} else {
			if (!dead) {
				if (it->second->isDead()) {
					it->second = 0;
				}
			}
		}
		++it;
	}
	foreach (vector<Vec2i>, it, remList) {
		m_knownEnemyLocations.erase(*it);
	}

	// 2. refresh visibleEnemies and closestEnemy
	m_visibleEnemies.clear();
	aiInterface->findEnemies(m_visibleEnemies, m_closestEnemy);

	// 3. update knownEnemyLocations with any previously unseen enemy buildings
	foreach_const (ConstUnitVector, it, m_visibleEnemies) {
		const Unit &enemy = **it;
		if (enemy.isBuilding()) {
			Vec2i bPos = enemy.getCenteredPos();
			if (m_knownEnemyLocations.find(bPos) == m_knownEnemyLocations.end()) {
				m_knownEnemyLocations[bPos] = &enemy;
			}
		}
	}
}

// ==================== state requests ====================

int Ai::getCountOfType(const UnitType *ut) {
	return aiInterface->getFaction()->getCountOfUnitType(ut);
}

int Ai::getCountOfClass(UnitClass uc) {
	const FactionType *factionType = aiInterface->getFaction()->getType();
	int count = 0;
	for (int i=0; i < factionType->getUnitTypeCount(); ++i) {
		const UnitType *ut = factionType->getUnitType(i);
		if (ut->isOfClass(uc)) {
			count += aiInterface->getFaction()->getCountOfUnitType(ut);
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

bool Ai::beingAttacked(float radius, Vec2i &out_pos, Field &out_field) {
	evaluateEnemies();
	if (m_closestEnemy) {
		Vec2i basePos = aiInterface->getHomeLocation();
		float dist = basePos.dist(m_closestEnemy->getCenteredPos());
		if (dist < radius) {
			baseSeen = true;
			out_pos = m_closestEnemy->getCenteredPos();
			out_field = m_closestEnemy->getCurrField();
			AI_LOG( MILITARY, 2, "Ai::beingAttacked: enemy found at pos " << out_pos );
			return true;
		}
	}
	AI_LOG( MILITARY, 2, "Ai::beingAttacked: no enemies found in range." );
	return false;
}

bool Ai::blindAttack(Vec2i &out_pos) {
	if (m_knownEnemyLocations.empty()) {
		return false;
	}
	int ndx = random.randRange(0, m_knownEnemyLocations.size() - 1);
	int i = 0;
	foreach_const (UnitPositions, it, m_knownEnemyLocations) {
		if (i == ndx) {
			out_pos = it->first;
			return true;
		}
		++i;
	}
	assert(false);
	return false;
}

bool Ai::isStaticResourceUsed(const ResourceType *rt) const {
	assert(rt->getClass() == ResourceClass::STATIC);
	return (staticResourceUsed.find(rt) != staticResourceUsed.end());
}

bool Ai::isStableBase() {
	if (getCountOfClass(UnitClass::WARRIOR) > minWarriors) {
		return true;
	} else {
		return false;
	}
}

bool Ai::findAbleUnit(int *unitIndex, CmdClass ability, bool idleOnly) {
	vector<int> units;

	*unitIndex = -1;
	for (int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		const Unit *unit = aiInterface->getMyUnit(i);
		if (unit->getType()->getActions()->hasCommandClass(ability)) {
			if ((!idleOnly || !unit->anyCommand() || unit->getCurrCommand()->getType()->getClass() == CmdClass::STOP)) {
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

bool Ai::findAbleUnit(int *unitIndex, CmdClass ability, CmdClass currentCommand) {
	vector<int> units;

	*unitIndex = -1;
	for (int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		const Unit *unit = aiInterface->getMyUnit(i);
		if (unit->getType()->getActions()->hasCommandClass(ability)) {
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

	// get a count of all unit types that we have
	const Faction *faction = aiInterface->getFaction();
	for (int i = 0; i < faction->getType()->getUnitTypeCount(); ++i){
		const UnitType *ut = faction->getType()->getUnitType(i);
		int count = faction->getCountOfUnitType(ut);
		if (count) {
			unitTypeCount[ut] = count;
		}
	}

	//get a list of all building types we can build and how many units can build
	//them.  Copy unit types that are buildings into builtOrBuilding.
	for(uti = unitTypeCount.begin(); uti != unitTypeCount.end(); ++uti) {

		// build commands
		for (int i=0; i < uti->first->getActions()->getCommandTypeCount<BuildCommandType>(); ++i) {
			const BuildCommandType *bct = uti->first->getActions()->getCommandType<BuildCommandType>(i);

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
		for (int i=0; i < uti->first->getActions()->getCommandTypeCount<UpgradeCommandType>(); ++i) {
			const UpgradeCommandType *uct = uti->first->getActions()->getCommandType<UpgradeCommandType>(i);
			for (int j=0; j < uct->getProducedCount(); ++j) {
				const UpgradeType *upgrade = uct->getProducedUpgrade(j);
				if (aiInterface->reqsOk(uct) && aiInterface->reqsOk(upgrade)) {
					availableUpgrades.push_back(upgrade);
				}
			}
		}
		if (uti->first->getActions()->hasSkillClass(SkillClass::BE_BUILT)) {
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

void Ai::addTask(const Task *task) {
	tasks.push_back(task);
	LOG_AI( FACTION_INDEX, AiComponent::GENERAL, 2, "Ai::addTask: Task added: " + task->toString() );
}

void Ai::addPriorityTask(const Task *task) {
	deleteValues(tasks.begin(), tasks.end());
	tasks.clear();

	tasks.push_back(task);
	AI_LOG( GENERAL, 2, "Ai::addPriorityTask: Priority Task added: " + task->toString() );
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
	AI_LOG( GENERAL, 2, "Ai::removeTask: Task removed: " + task->toString() );
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
	for (Positions::iterator it = expansionPositions.begin(); it != expansionPositions.end(); ++it) {
		if ((*it).dist(pos) < villageRadius) {
			return;
		}
	}
	//add expansion
	expansionPositions.push_front(pos);

	//remove expansion if queue is list is full
	if (expansionPositions.size()>maxExpansions) {
		expansionPositions.pop_back();
	}
}

Vec2i Ai::getRandomHomePosition() {
	if (expansionPositions.empty() || random.randRange(0, 1) == 0) {
		return aiInterface->getHomeLocation();
	}
	return expansionPositions[random.randRange(0, expansionPositions.size() - 1)];
}

// ==================== actions ====================

void Ai::sendScoutPatrol() {
    Vec2i pos;
    int unit;

	startLoc = (startLoc+1) % aiInterface->getMapMaxPlayers();
	pos = aiInterface->getStartLocation(startLoc);

	if (aiInterface->getFactionIndex() != startLoc) {
		if (findAbleUnit(&unit, CmdClass::ATTACK, false)) {
			aiInterface->giveCommand(unit, CmdClass::ATTACK, pos);
			AI_LOG( MILITARY, 1, "Ai::sendScoutPatrol: Scout patrol sent to: " << pos );
		}
	}
}

void Ai::massiveAttack(const Vec2i &pos, Field field, bool ultraAttack){
	int producerWarriorCount = 0;
	int maxProducerWarriors = random.randRange(1, 11);
	AI_LOG( MILITARY, 1, "Ai::massiveAttack: Massive attack to pos: " << pos );
    for (int i=0; i < aiInterface->getMyUnitCount(); ++i) {
    	bool isWarrior;
        const Unit *unit = aiInterface->getMyUnit(i);
		const AttackCommandType *act = unit->getType()->getActions()->getAttackCommand(field==Field::AIR?Zone::AIR:Zone::LAND);

		if (act && unit->getType()->getActions()->hasCommandClass(CmdClass::PRODUCE)) {
			++producerWarriorCount;
		}

		if (aiInterface->getControlType() == ControlType::CPU_MEGA) {
			//if (producerWarriorCount > maxProducerWarriors) {
				if (unit->getCommandCount() > 0
				&& unit->getCurrCommand()->getType() != 0
				&& ( unit->getCurrCommand()->getType()->getClass() == CmdClass::BUILD
				||   unit->getCurrCommand()->getType()->getClass() == CmdClass::MORPH
				||   unit->getCurrCommand()->getType()->getClass() == CmdClass::PRODUCE )) {
					AI_LOG( MILITARY, 3, "Ai::massiveAttack: candidate is producer and currently producing." );
					isWarrior = false;
				} else {
					isWarrior = !unit->getType()->getActions()->hasCommandClass(CmdClass::HARVEST);
				}
			//} else {
			//	isWarrior = !unit->getType()->getActions()->hasCommandClass(CmdClass::HARVEST) && !unit->getType()->getActions()->hasCommandClass(CmdClass::PRODUCE);
			//}
		} else {
			isWarrior = !unit->getType()->getActions()->hasCommandClass(CmdClass::HARVEST) &&
			!unit->getType()->getActions()->hasCommandClass(CmdClass::PRODUCE);
		}
		if (act) {
			bool alreadyAttacking = unit->getCurrSkill()->getClass() == SkillClass::ATTACK;
			if (alreadyAttacking) {
				AI_LOG( MILITARY, 3, "Ai::massiveAttack: attacker " << unit->getId() << " [type="
					<< unit->getType()->getName() << "] already attacking." );
			} else if (ultraAttack || isWarrior) {
				CmdResult res = aiInterface->giveCommand(i, act, pos);
				AI_LOG( MILITARY, 2, "Ai::massiveAttack: attacker " << unit->getId() << " [type="
					<< unit->getType()->getName() << "] issued attack command to " << pos );
			} else {
				AI_LOG( MILITARY, 3, "Ai::massiveAttack: attacker " << unit->getId() << " [type="
					<< unit->getType()->getName() << "] not ultra-attack, not attacking." );
			}
		}
    }
    if (aiInterface->getControlType() == ControlType::CPU_EASY) {
		minWarriors += 1;
	} else if(aiInterface->getControlType() == ControlType::CPU_MEGA) {
		minWarriors += 3;
		if (minWarriors > maxMinWarriors - 1 || randomMinWarriorsReached) {
			randomMinWarriorsReached = true;
			minWarriors = random.randRange(maxMinWarriors - 10, maxMinWarriors * 2);
		}
	} else if (minWarriors < maxMinWarriors) {
		minWarriors += 3;
	}
}

inline Vec2i randOffset(Random &rand, int radius) {
	return Vec2i(rand.randRange(-radius, radius), rand.randRange(-radius, radius));
}

void Ai::returnBase(int unitIndex) {
    Vec2i pos = getRandomHomePosition() + randOffset(random, villageRadius);
    CmdResult r = aiInterface->giveCommand(unitIndex, CmdClass::MOVE, pos);
	AI_LOG( MILITARY, 2, "Ai::returnBase: Order return to base pos:" << pos
		<< " result: " << CmdResultNames[r] );
}

void Ai::harvest(int unitIndex) {
	const ResourceType *rt = getNeededResource();
	if (!rt) {
		AI_LOG( ECONOMY, 1, "Ai::harvest: No needed resources!?!" );
		return;
	}
	const HarvestCommandType *hct = aiInterface->getMyUnit(unitIndex)->getType()->getActions()->getHarvestCommand(rt);
	if (!hct) {
		AI_LOG( ECONOMY, 1, "Ai::harvest: worker " << unitIndex
			<< " [type=" << aiInterface->getMyUnit(unitIndex)->getType()->getName() << "] "
			<< "can not harvest " << rt->getName() );
		return;
	}
	Vec2i resPos;
	if (aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), resPos)) {
		resPos = resPos + Vec2i(random.randRange(-2, 2), random.randRange(-2, 2));
		aiInterface->giveCommand(unitIndex, hct, resPos);
		AI_LOG( ECONOMY, 2, "Ai::harvest: Harvest command for resource " << rt->getName()
			<< " at pos " << resPos << " issued." );
	}
}

}}//end namespace
