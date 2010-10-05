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

#include <algorithm>
#include <climits>
#include <limits>

#include "ai_rule.h"
#include "ai.h"
#include "ai_interface.h"
#include "unit.h"
#include "game_constants.h"

#include "leak_dumper.h"

using std::numeric_limits;
using Shared::Math::Vec2i;

namespace Glest { namespace Plan {

// =====================================================
//	class AiRule
// =====================================================

MEMORY_CHECK_IMPLEMENTATION(AiRule)

// =====================================================
//	class AiRuleWorkerHarvest
// =====================================================

AiRuleWorkerHarvest::AiRuleWorkerHarvest(Ai *ai)
		: AiRule(ai) {
	stoppedWorkerIndex = -1;
}

bool AiRuleWorkerHarvest::test() {
	return ai->findAbleUnit(&stoppedWorkerIndex, CommandClass::HARVEST, true);
}

void AiRuleWorkerHarvest::execute() {
	ai->harvest(stoppedWorkerIndex);
}

// =====================================================
//	class AiRuleRefreshHarvester
// =====================================================

AiRuleRefreshHarvester::AiRuleRefreshHarvester(Ai *ai)
		: AiRule(ai) {
	workerIndex = -1;
}

bool AiRuleRefreshHarvester::test() {
	return ai->findAbleUnit(&workerIndex, CommandClass::HARVEST, true); // should be idleOnly == false ?
}

void AiRuleRefreshHarvester::execute() {
	ai->harvest(workerIndex);
}

// =====================================================
//	class AiRuleScoutPatrol
// =====================================================

AiRuleScoutPatrol::AiRuleScoutPatrol(Ai *ai)
		: AiRule(ai) {
}

bool AiRuleScoutPatrol::test() {
	return ai->isStableBase();
}

void AiRuleScoutPatrol::execute() {
	ai->sendScoutPatrol();
}
// =====================================================
//	class AiRuleRepair
// =====================================================

AiRuleRepair::AiRuleRepair(Ai *ai)
		: AiRule(ai) {
}

bool AiRuleRepair::test() {
	GlestAiInterface *aiInterface = ai->getAiInterface();
	repairable.clear();

	//look for a damaged unit that we can repair
	for(int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		const Unit *u = aiInterface->getMyUnit(i);
		if(u->getHpRatio() < 1.f && ai->isRepairable(u)) {
			repairable.push_back(u);
		}
	}
	return !repairable.empty();
}

void AiRuleRepair::execute() {
	GlestAiInterface *aiInterface = ai->getAiInterface();

	const RepairCommandType *nearestRct = NULL;
	int nearest = -1;
	fixed minDist = fixed::max_int();

	//try all of our repairable units in case one type of repairer is busy, but
	//others aren't.
	for (Units::iterator ui = repairable.begin(); ui != repairable.end(); ++ui) {
		const Unit *damagedUnit = *ui;

		fixedVec2 fpos = damagedUnit->getFixedCenteredPos();
		int size = damagedUnit->getType()->getSize();

		//find the nearest available repairer and issue command
		for(int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
			const Unit *u = aiInterface->getMyUnit(i);
			const RepairCommandType *rct;
			if ((u->getCurrSkill()->getClass() == SkillClass::STOP 
			||   u->getCurrSkill()->getClass() == SkillClass::MOVE)
			&& (rct = u->getRepairCommandType(damagedUnit))) {
				fixed dist = fpos.dist(u->getFixedCenteredPos()) + size + u->getType()->getHalfSize();
				if (minDist > dist) {
					nearestRct = rct;
					nearest = i;
					minDist = dist;
				}
			}
		}

		if (nearestRct) {
			aiInterface->giveCommand(nearest, nearestRct, const_cast<Unit*>(damagedUnit));
			LOG_AI( "Repairing order issued" );
			return;
		}
	}
}


// =====================================================
//	class AiRuleReturnBase
// =====================================================

AiRuleReturnBase::AiRuleReturnBase(Ai *ai)
		: AiRule(ai) {
	stoppedUnitIndex= -1;
}

bool AiRuleReturnBase::test() {
	return ai->findAbleUnit(&stoppedUnitIndex, CommandClass::MOVE, true);
}

void AiRuleReturnBase::execute() {
	ai->returnBase(stoppedUnitIndex);
}

// =====================================================
//	class AiRuleMassiveAttack
// =====================================================

AiRuleMassiveAttack::AiRuleMassiveAttack(Ai *ai)
		: AiRule(ai) {
}

bool AiRuleMassiveAttack::test() {
	if (ai->isStableBase()) {
		ultraAttack = false;
		return ai->beingAttacked(attackPos, field, INT_MAX);
	} else {
		ultraAttack = true;
		return ai->beingAttacked(attackPos, field, baseRadius);
	}
}

void AiRuleMassiveAttack::execute() {
	ai->massiveAttack(attackPos, field, ultraAttack);
}

// =====================================================
//	class AiRuleAddTasks
// =====================================================

AiRuleAddTasks::AiRuleAddTasks(Ai *ai)
		: AiRule(ai) {
}

bool AiRuleAddTasks::test() {
	return !ai->anyTask() || ai->getCountOfClass(UnitClass::WORKER) < 4;
}

void AiRuleAddTasks::execute() {
	GlestAiInterface *aii = ai->getAiInterface();
	Random *rand = ai->getRandom();
	int buildingCount = ai->getCountOfClass(UnitClass::BUILDING);
	int warriorCount = ai->getCountOfClass(UnitClass::WARRIOR);
	int workerCount = ai->getCountOfClass(UnitClass::WORKER);
	int upgradeCount = ai->getAiInterface()->getMyUpgradeCount();

	float buildingRatio = ai->getRatioOfClass(UnitClass::BUILDING);
	float warriorRatio = ai->getRatioOfClass(UnitClass::WARRIOR);
	float workerRatio = ai->getRatioOfClass(UnitClass::WORKER);
	ai->updateStatistics();

	//standard tasks

	//emergency workers
	if (workerCount < 4) {
		ai->addPriorityTask(new ProduceTask(UnitClass::WORKER));
	}
	else{
		if(ai->getAiInterface()->getControlType()==ControlType::CPU_MEGA)
		{
			//workers
			if(workerCount<5) ai->addTask(new ProduceTask(UnitClass::WORKER));
			if(workerCount<10) ai->addTask(new ProduceTask(UnitClass::WORKER));
			if(workerRatio<0.20) ai->addTask(new ProduceTask(UnitClass::WORKER));
			if(workerRatio<0.30) ai->addTask(new ProduceTask(UnitClass::WORKER));
			
			//warriors
			if(warriorCount<10) ai->addTask(new ProduceTask(UnitClass::WARRIOR));
			if(warriorRatio<0.20) ai->addTask(new ProduceTask(UnitClass::WARRIOR));
			if(warriorRatio<0.30) ai->addTask(new ProduceTask(UnitClass::WARRIOR));
			if(workerCount>=10) ai->addTask(new ProduceTask(UnitClass::WARRIOR));
			if(workerCount>=15) ai->addTask(new ProduceTask(UnitClass::WARRIOR));			
			if(warriorCount<ai->getMinWarriors()+2) 
			{
				ai->addTask(new ProduceTask(UnitClass::WARRIOR));
				if( buildingCount>9 )
				{
					ai->addTask(new ProduceTask(UnitClass::WARRIOR));
					ai->addTask(new ProduceTask(UnitClass::WARRIOR));
				}
				if( buildingCount>12 )
				{
					ai->addTask(new ProduceTask(UnitClass::WARRIOR));
					ai->addTask(new ProduceTask(UnitClass::WARRIOR));
				}
			}
			
			//buildings
			if(buildingCount<6 || buildingRatio<0.20) ai->addTask(new BuildTask());
			if(buildingCount<10 && workerCount>12) ai->addTask(new BuildTask());
			//upgrades
			if(upgradeCount==0 && workerCount>5) ai->addTask(new UpgradeTask());
			if(upgradeCount==1 && workerCount>10) ai->addTask(new UpgradeTask());
			if(upgradeCount==2 && workerCount>15) ai->addTask(new UpgradeTask());
			if(ai->isStableBase()) ai->addTask(new UpgradeTask());
		}
		else if(ai->getAiInterface()->getControlType()==ControlType::CPU_EASY)
		{// Easy CPU
			//workers
			if(workerCount<buildingCount+2) ai->addTask(new ProduceTask(UnitClass::WORKER));
			if(workerCount>5 && workerRatio<0.20) ai->addTask(new ProduceTask(UnitClass::WORKER));
			
			//warriors
			if(warriorCount<10) ai->addTask(new ProduceTask(UnitClass::WARRIOR));
			if(warriorRatio<0.20) ai->addTask(new ProduceTask(UnitClass::WARRIOR));
			if(warriorRatio<0.30) ai->addTask(new ProduceTask(UnitClass::WARRIOR));
			if(workerCount>=10) ai->addTask(new ProduceTask(UnitClass::WARRIOR));
			if(workerCount>=15) ai->addTask(new ProduceTask(UnitClass::WARRIOR));			
			
			//buildings
			if(buildingCount<6 || buildingRatio<0.20) ai->addTask(new BuildTask());
			if(buildingCount<10 && ai->isStableBase()) ai->addTask(new BuildTask());
			
			//upgrades
			if(upgradeCount==0 && workerCount>6) ai->addTask(new UpgradeTask());
			if(upgradeCount==1 && workerCount>7) ai->addTask(new UpgradeTask());
			if(upgradeCount==2 && workerCount>9) ai->addTask(new UpgradeTask());
			//if(ai->isStableBase()) ai->addTask(new UpgradeTask());
		}
		else
		{// normal CPU / UltraCPU ...
			//workers
			if(workerCount<5) ai->addTask(new ProduceTask(UnitClass::WORKER));
			if(workerCount<10) ai->addTask(new ProduceTask(UnitClass::WORKER));
			if(workerRatio<0.20) ai->addTask(new ProduceTask(UnitClass::WORKER));
			if(workerRatio<0.30) ai->addTask(new ProduceTask(UnitClass::WORKER));
			
			//warriors
			if(warriorCount<10) ai->addTask(new ProduceTask(UnitClass::WARRIOR));
			if(warriorRatio<0.20) ai->addTask(new ProduceTask(UnitClass::WARRIOR));
			if(warriorRatio<0.30) ai->addTask(new ProduceTask(UnitClass::WARRIOR));
			if(workerCount>=10) ai->addTask(new ProduceTask(UnitClass::WARRIOR));
			if(workerCount>=15) ai->addTask(new ProduceTask(UnitClass::WARRIOR));			
			
			//buildings
			if(buildingCount<6 || buildingRatio<0.20) ai->addTask(new BuildTask());
			if (buildingCount < 10 && workerCount > 12) ai->addTask(new BuildTask());
			
			//upgrades
			if(upgradeCount==0 && workerCount>5) ai->addTask(new UpgradeTask());
			if(upgradeCount==1 && workerCount>10) ai->addTask(new UpgradeTask());
			if(upgradeCount==2 && workerCount>15) ai->addTask(new UpgradeTask());
			if(ai->isStableBase()) ai->addTask(new UpgradeTask());
		}
	}
}

// =====================================================
//	class AiRuleBuildOneFarm
// =====================================================

AiRuleBuildOneFarm::AiRuleBuildOneFarm(Ai *ai)
		: AiRule(ai) {
}

bool AiRuleBuildOneFarm::test() {
	GlestAiInterface *aiInterface = ai->getAiInterface();

	// for all unit types
	for(int i = 0; i < aiInterface->getMyFactionType()->getUnitTypeCount(); ++i) {
		const UnitType *ut = aiInterface->getMyFactionType()->getUnitType(i);
		// for all produce commands
		for(int j = 0; j < ut->getCommandTypeCount<ProduceCommandType>(); ++j) {
			const ProduceCommandType *pct = ut->getCommandType<ProduceCommandType>(j);
			for (int k=0; k < pct->getProducedCount(); ++k) {
				const UnitType *pt = pct->getProducedUnit(k);
				//for all resources
				for (int n = 0; n < pt->getCostCount(); ++n) {
					const Resource *r = pt->getCost(n);
					// can produce consumables and would be the first of its type?
					if (r->getAmount() < 0 && r->getType()->getClass() == ResourceClass::CONSUMABLE 
					&& ai->getCountOfType(ut) == 0) {
						farm = ut;
						return true;
					}
				}

			}
		}
	}
	return false;
}

void AiRuleBuildOneFarm::execute() {
	ai->addPriorityTask(new BuildTask(farm));
}

// =====================================================
//	class AiRuleProduceResourceProducer
// =====================================================

AiRuleProduceResourceProducer::AiRuleProduceResourceProducer(Ai *ai)
		: AiRule(ai) {
	interval = shortInterval;
}

bool AiRuleProduceResourceProducer::test() {
	// emergency tasks: resource buildings
	GlestAiInterface *aiInterface = ai->getAiInterface();

	// consumables first
	for (int i = 0; i < aiInterface->getTechTree()->getResourceTypeCount(); ++i) {
        rt = aiInterface->getTechTree()->getResourceType(i);
		const Resource *r = aiInterface->getResource(rt);
		if (rt->getClass() == ResourceClass::CONSUMABLE && r->getBalance() < 0) {
			interval = longInterval;
			return true;

        }
    }
	if (!ai->usesStaticResources()) {
		interval = shortInterval;
		return false;
	}
	// statics second
	for (int i = 0; i < aiInterface->getTechTree()->getResourceTypeCount(); ++i) {
        rt = aiInterface->getTechTree()->getResourceType(i);
		const Resource *r = aiInterface->getResource(rt);
		if (rt->getClass() == ResourceClass::STATIC && ai->isStaticResourceUsed(rt)
		&& r->getAmount() < minStaticResources) {
			interval = longInterval;
			return true;
        }
    }
	interval = shortInterval;
	return false;
}

void AiRuleProduceResourceProducer::execute() {
	ai->addPriorityTask(new ProduceTask(rt));
	ai->addTask(new BuildTask(rt));
}

// =====================================================
//	class AiRuleProduce
// =====================================================

#define RAND_RANGE(min, max) (ai->getRandom()->randRange(min, max))

AiRuleProduce::AiRuleProduce(Ai *ai)
		: AiRule(ai) {
	produceTask = NULL;
}

bool AiRuleProduce::test() {
	const Task *task = ai->getTask();

	if (task == NULL || task->getClass() != TaskClass::PRODUCE) {
		return false;
	}

	produceTask = static_cast<const ProduceTask*>(task);
	return true;
}

void AiRuleProduce::execute() {
	if (produceTask != NULL) {
		if (produceTask->getResourceType()) {
			// produce static or consumable resources
			produceResources(produceTask);
		} else if (produceTask->getUnitClass() != UnitClass::INVALID) {
			// generic produce task, produce random unit that has a certain UnitClass
			produceGeneric(produceTask);
		} else {
			// specific produce task, produce if possible, retry if not enough resources
			produceSpecific(produceTask);
		}
		//remove the task
		ai->removeTask(produceTask);
	} else {
		LOG_AI_PRODUCE(__FUNCTION__ << " produceTask is NULL.");
	}
}

void AiRuleProduce::produceResources(const ProduceTask *task) {
	assert(task->getResourceType());
	assert(task->getUnitClass() == UnitClass::INVALID && !task->getUnitType());
	LOG_AI_PRODUCE(__FUNCTION__);
	
	GlestAiInterface *aiInterface = ai->getAiInterface();
	const FactionType *ft = aiInterface->getMyFactionType();
	const Faction *faction = aiInterface->getMyFaction();

	typedef pair<const CommandType*, const ProducibleType*> ProdPair;
	typedef vector<ProdPair> CommandList;
	map<const UnitType*, CommandList> prodMap;

	// 1. Find all UnitTypes that can make ProducibleTypes that satisfy request (cost is 
	// negative for pt->resourceType and has reqsOk() for Producible)
	for (int i=0; i < ft->getUnitTypeCount(); ++i) {
		const UnitType *ut = ft->getUnitType(i);
		for (int j=0; j < ut->getCommandTypeCount(); ++j) {
			const CommandType *ct = ut->getCommandType(j);
			if (!faction->reqsOk(ct)) {
				continue;
			}
			for (int k=0; k < ct->getProducedCount(); ++k) {
				const ProducibleType *pt = ct->getProduced(k);
				const Resource *res = pt->getCost(task->getResourceType());
				if (res && res->getAmount() < 0 && faction->reqsOk(pt)) {
					prodMap[ut].push_back(std::make_pair(ct, pt));
				}
			}
		}
	}
	if (prodMap.empty()) {
		return;
	}

	// 2. Look at all units that are of types found in 1 and find the one with the 
	// least commands
	int lowCmd = numeric_limits<int>::max();
	Unit *unitToCmd = 0;
	for (int i=0; i < faction->getUnitCount(); ++i) {
		Unit *unit = faction->getUnit(i);
		if (prodMap.find(unit->getType()) != prodMap.end()) {
			int cmds = unit->getCommands().size();
			if (cmds == 1 && unit->getCommands().front()->getType()->getClass() == CommandClass::STOP) {
				cmds = 0;
			}
			if (cmds < lowCmd) {
				lowCmd = cmds;
				unitToCmd = unit;
			} else if (cmds == lowCmd && RAND_RANGE(0, 1)) {
				unitToCmd = unit;
			}
		}
	}
	if (!unitToCmd) {
		return;
	}

	// 3. Give new produce/morph/generate command
	int cmdNdx = RAND_RANGE(0, prodMap[unitToCmd->getType()].size() - 1);
	ProdPair pp = prodMap[unitToCmd->getType()][cmdNdx];
	aiInterface->giveCommand(unitToCmd, pp.first, Command::invalidPos, pp.second);
}

void AiRuleProduce::produceGeneric(const ProduceTask *task) {
	assert(task->getUnitClass() != UnitClass::INVALID);
	assert(!task->getResourceType() && !task->getUnitType());
	LOG_AI_PRODUCE(__FUNCTION__);
	
	GlestAiInterface *aiInterface = ai->getAiInterface();
	const FactionType *ft = aiInterface->getMyFactionType();
	const Faction *faction = aiInterface->getMyFaction();

	typedef pair<const CommandType*, const UnitType*> ProdPair;
	typedef vector<ProdPair> CommandList;
	map<const UnitType*, CommandList> prodMap;

	// 1. Find all UnitTypes who can produce/morph-to UnitTypes of UnitClass requested and that have reqsOk
	for (int i=0; i < ft->getUnitTypeCount(); ++i) {
		const UnitType *ut = ft->getUnitType(i);
//		if (!faction->reqsOk(ut)) {
//			continue;
//		}
		for (int j=0; j < ut->getCommandTypeCount(); ++j) {
			const CommandType *ct = ut->getCommandType(j);
			if (!faction->reqsOk(ct)
			|| (ct->getClass() != CommandClass::PRODUCE && ct->getClass() != CommandClass::MORPH)) {
				continue;
			}
			
			if (ct->getClass() == CommandClass::PRODUCE && task->getUnitClass() == UnitClass::WARRIOR
			&& ut->isMobile()) {
				DEBUG_HOOK();
			}

			for (int k=0; k < ct->getProducedCount(); ++k) {
				const UnitType *pt = static_cast<const UnitType*>(ct->getProduced(k));
				if (faction->reqsOk(pt) && pt->isOfClass(task->getUnitClass())) {
					prodMap[ut].push_back(std::make_pair(ct, pt));
				}
			}

		}
	}
	if (prodMap.empty()) {
		return;
	}

	// 2. Find all units of types found in 1
	int lowCmd = numeric_limits<int>::max();
	Unit *unitToCmd = 0;
	for (int i=0; i < faction->getUnitCount(); ++i) {
		Unit *unit = faction->getUnit(i);
		if (prodMap.find(unit->getType()) != prodMap.end()) {
			int cmds = unit->getCommands().size();
			if (cmds == 1 && unit->getCommands().front()->getType()->getClass() == CommandClass::STOP) {
				cmds = 0;
			}
			if (cmds < lowCmd) {
				lowCmd = cmds;
				unitToCmd = unit;
			} else if (cmds == lowCmd && RAND_RANGE(0, 1)) {
				unitToCmd = unit;
			}
		}
	}
	if (!unitToCmd) {
		return;
	}

	// 3. Give produce/morph command.
	int cmdNdx = RAND_RANGE(0, prodMap[unitToCmd->getType()].size() - 1);
	ProdPair pp = prodMap[unitToCmd->getType()][cmdNdx];
	aiInterface->giveCommand(unitToCmd, pp.first, Command::invalidPos, pp.second);
}

void AiRuleProduce::produceSpecific(const ProduceTask *task) {
	LOG_AI_PRODUCE(__FUNCTION__ << " type to build: " << task->getUnitType()->getName());
	assert(task->getUnitType());
	GlestAiInterface *aiInterface = ai->getAiInterface();
	const FactionType *ft = aiInterface->getMyFactionType();
	Faction *faction = aiInterface->getMyFaction();

	typedef pair<const CommandType*, const UnitType*> ProdPair;
	typedef vector<ProdPair> CommandList;
	map<const UnitType*, CommandList> prodMap;

	// 1. Do we meet requirements and costs?
	if (!aiInterface->reqsOk(task->getUnitType())) { // if unit meets requirements
		LOG_AI_PRODUCE(__FUNCTION__ << " unit/upgrade reqs not met.");
		return;
	}
	if (!aiInterface->checkCosts(task->getUnitType())) { //if unit doesnt meet resources retry
		LOG_AI_PRODUCE(__FUNCTION__ << " resource reqs not met, reposting.");
		ai->retryTask(task);
		return;
	}		

	// 2. find all UnitTypes that can produce/morph-to required type
	for (int i=0; i < ft->getUnitTypeCount(); ++i) {
		const UnitType *ut = ft->getUnitType(i);
		//if (!faction->reqsOk(ut)) {
		//	continue;
		//}
		for (int j=0; j < ut->getCommandTypeCount(); ++j) {
			const CommandType *ct = ut->getCommandType(j);
			if (!faction->reqsOk(ct) 
			|| (ct->getClass() != CommandClass::PRODUCE && ct->getClass() != CommandClass::MORPH)) {
				continue;
			}
			for (int k=0; k < ct->getProducedCount(); ++k) {
				const UnitType *pt = static_cast<const UnitType*>(ct->getProduced(k));
				if (pt == task->getUnitType()) {
					prodMap[ut].push_back(std::make_pair(ct, pt));
				}
			}
		}
	}

	// 3. find all units of types found in 2, and remember the best (lowest command count)
	int lowCmd = numeric_limits<int>::max();
	Unit *unitToCmd = 0;
	for (int i=0; i < faction->getUnitCount(); ++i) {
		Unit *unit = faction->getUnit(i);
		if (prodMap.find(unit->getType()) != prodMap.end()) {
			int cmds = unit->getCommands().size();
			if (cmds == 1 && unit->getCommands().front()->getType()->getClass() == CommandClass::STOP) {
				cmds = 0;
			}
			if (cmds < lowCmd) {
				lowCmd = cmds;
				unitToCmd = unit;
			} else if (cmds == lowCmd && RAND_RANGE(0, 1)) {
				unitToCmd = unit;
			}
		}
	}
	if (!unitToCmd) { // no one can currently make it
		ai->retryTask(task);
		return;
	}

	// 4. if producer with lowest command count has many commands, add task to make more producers
	if (lowCmd > 2) {
		if (aiInterface->reqsOk(unitToCmd->getType())) { 
			if (ai->getCountOfClass(UnitClass::BUILDING) > 5) {
				if (unitToCmd->getType()->isOfClass(UnitClass::BUILDING)) {
					ai->addTask(new BuildTask(unitToCmd->getType()));
				} else {
					ai->addTask(new ProduceTask(unitToCmd->getType()));
				}
			}
		}
	}

	// 5. Give produce/morph command.
	int cmdNdx = RAND_RANGE(0, prodMap[unitToCmd->getType()].size() - 1);
	ProdPair pp = prodMap[unitToCmd->getType()][cmdNdx];
	aiInterface->giveCommand(unitToCmd, pp.first, Command::invalidPos, pp.second);
}

// ========================================
// 	class AiRuleBuild
// ========================================

AiRuleBuild::AiRuleBuild(Ai *ai)
		: AiRule(ai) {
	buildTask= NULL;
}

bool AiRuleBuild::test() {
	const Task *task = ai->getTask();
	if (task == NULL || task->getClass() != TaskClass::BUILD) {
		return false;
	}
	buildTask = static_cast<const BuildTask*>(task);
	return true;
}


void AiRuleBuild::execute() {
	if (buildTask) {
		// generic build task, build random building that can be built
		if (buildTask->getUnitType()==NULL) {
			buildGeneric(buildTask);

		// specific building task, build if possible, retry if not enough resources or not position
		} else {
			buildSpecific(buildTask);
		}
		//remove the task
		ai->removeTask(buildTask);
	}
}

void AiRuleBuild::findBuildingTypes(UnitTypeList &utList, const ResourceType *rt) {
	// for each UnitType
	GlestAiInterface *aiInterface= ai->getAiInterface();
	const FactionType *factionType = ai->getAiInterface()->getMyFactionType();
	for (int i=0; i < factionType->getUnitTypeCount(); ++i) {
		const UnitType *ut = factionType->getUnitType(i);
		if (!ai->getCountOfType(ut)) {
			continue;
		}
		for(int j = 0; j < ut->getCommandTypeCount(); ++j) {
			const CommandType *ct = ut->getCommandType(j);
			if (ct->getClass() == CommandClass::BUILD) { // if the command is build
				const BuildCommandType *bct= static_cast<const BuildCommandType*>(ct);
				//for each building
				for (int k=0; k < bct->getBuildingCount(); ++k) {
					const UnitType *buildingType = bct->getBuilding(k);
					if (aiInterface->reqsOk(bct) && aiInterface->reqsOk(buildingType)) {
						//if any building, or produces resource
						const Resource *cost= buildingType->getCost(rt);
						if (!rt || (cost && cost->getAmount() < 0)) {
							//LOG_AI_BUILD(__FUNCTION__ << " candidate building " 
							//	<< buildingType->getName());
							utList.push_back(buildingType);
						}
					}
				}
			}
		}
	}
}

void AiRuleBuild::buildGeneric(const BuildTask *bt){
	LOG_AI_BUILD(__FUNCTION__);
	//find buildings that can be built
	GlestAiInterface *aiInterface= ai->getAiInterface();
	Faction *faction = aiInterface->getMyFaction();
	const Units &units = faction->getUnits();

	UnitTypeList buildingTypes;
	findBuildingTypes(buildingTypes, bt->getResourceType());

	//add specific build task
	buildBestBuilding(buildingTypes);
}

void AiRuleBuild::buildBestBuilding(const UnitTypeList &buildingTypes) {
	if (buildingTypes.empty()) {
		LOG_AI_BUILD(__FUNCTION__ << " no building types to build." );
		return;
	}
	// build the least built buildingType
	for (int i=0; i < 10; ++i) {
		if (i > 0) {
			// Defensive buildings have priority
			for (int j=0; j < buildingTypes.size(); ++j) {
				const UnitType *buildingType = buildingTypes[j];
				if (ai->getCountOfType(buildingType) <= i + 1 && isDefensive(buildingType)) {
					LOG_AI_BUILD(__FUNCTION__ << " adding task, Defensive building, type = "
						<< buildingType->getName());
					ai->addTask(new BuildTask(buildingType));
					return;
				}
			}
			// Warrior producers next
			for (int j=0; j < buildingTypes.size(); ++j) {
				const UnitType *buildingType = buildingTypes[j];
				if (ai->getCountOfType(buildingType) <= i + 1 && isWarriorProducer(buildingType)) {
					LOG_AI_BUILD(__FUNCTION__ << " adding task, Warrior producing building, type = "
						<< buildingType->getName());
					ai->addTask(new BuildTask(buildingType));
					return;
				}
			}
			// Resource producers next
			for (int j=0; j < buildingTypes.size(); ++j) {
				const UnitType *buildingType = buildingTypes[j];
				if (ai->getCountOfType(buildingType) <= i + 1 && isResourceProducer(buildingType)) {
					LOG_AI_BUILD(__FUNCTION__ << " adding task, Resource producing building, type = "
						<< buildingType->getName());
					ai->addTask(new BuildTask(buildingType));
					return;
				}
			}
		}
		// Any buildingType
		for (int j=0; j < buildingTypes.size(); ++j) {
			const UnitType *buildingType = buildingTypes[j];
			if (ai->getCountOfType(buildingType) <= i) {
				LOG_AI_BUILD(__FUNCTION__ << " adding task, low-ratio building, type = "
					<< buildingType->getName());
				ai->addTask(new BuildTask(buildingType));
				return;
			}
		}
	}
}

void AiRuleBuild::findBuilderTypes(const UnitType *buildingType, CmdByUtMap &cmdMap) {
	const FactionType *factionType = ai->getAiInterface()->getMyFactionType();
	for (int i=0; i < factionType->getUnitTypeCount(); ++i) { // for each UnitType
		const UnitType *ut = factionType->getUnitType(i);
		if (!ai->getCountOfType(ut)) {
			continue;
		}
		for(int j = 0; j < ut->getCommandTypeCount(); ++j) {
			const CommandType *ct = ut->getCommandType(j);
			if (ct->getClass() == CommandClass::BUILD) { // if the command is build
				const BuildCommandType *bct= static_cast<const BuildCommandType*>(ct);
				for (int k=0; k < bct->getBuildingCount(); ++k) { // for each building
					const UnitType *canBuildType = bct->getBuilding(k);
					if (buildingType == canBuildType) {
						cmdMap[ut] = ct;
						continue;
					}
				}
			}
		}
	}
}

void AiRuleBuild::buildSpecific(const BuildTask *bt){
	GlestAiInterface *aiInterface= ai->getAiInterface();
	Faction *faction = aiInterface->getMyFaction();
	const Units &units = faction->getUnits();

	if (!aiInterface->reqsOk(bt->getUnitType())) { // if reqs not met, bail
		LOG_AI_BUILD(__FUNCTION__ << " building to build : " << bt->getUnitType()->getName()
			<< ", unit/upgrade reqs not met.");
		return;
	}
	if (!aiInterface->checkCosts(bt->getUnitType())) { // retry if not enough resources
		LOG_AI_BUILD(__FUNCTION__ << " building to build : " << bt->getUnitType()->getName()
			<< ", resource reqs not met.");
		ai->retryTask(bt);
		return;
	}

	CmdByUtMap cmdMap;
	findBuilderTypes(bt->getUnitType(), cmdMap);
	ConstUnitVector potentialBuilders;

	foreach_const (Units, it, units) {
		const Unit *const &unit = *it;
		if (cmdMap.find(unit->getType()) != cmdMap.end()
		&& (!unit->anyCommand() || unit->getCurrCommand()->getType()->getClass() != CommandClass::BUILD)) {
			potentialBuilders.push_back(unit);
		}
	}
	//use random builder to build
	if (potentialBuilders.empty()) {
		LOG_AI_BUILD(__FUNCTION__ << " building to build : " << bt->getUnitType()->getName()
			<< ", could not find a builder unit.");
		return;
	}
	const Unit *unit = potentialBuilders[RAND_RANGE(0, potentialBuilders.size() - 1)];
	Vec2i pos;
	Vec2i searchPos = bt->getForcePos() ? bt->getPos() : ai->getRandomHomePosition();
	// if free pos give command, else retry
	if (ai->findPosForBuilding(bt->getUnitType(), searchPos, pos)) {
		LOG_AI_BUILD(__FUNCTION__ << " building to build : " << bt->getUnitType()->getName()
			<< ", " << cmdMap[unit->getType()]->getName() << " command given to unit"
			<< unit->getId() << " [" << unit->getType()->getName() << "]");
		aiInterface->giveCommand(unit, cmdMap[unit->getType()], pos, bt->getUnitType());
	} else {
		LOG_AI_BUILD(__FUNCTION__ << " building to build : " << bt->getUnitType()->getName()
			<< ", could not find position for building. Re-posting.");
		ai->retryTask(bt);
	}
}

bool AiRuleBuild::isDefensive(const UnitType *building){
	return building->hasSkillClass(SkillClass::ATTACK);
}

bool AiRuleBuild::isResourceProducer(const UnitType *building){
	for(int i= 0; i<building->getCostCount(); i++){
		if(building->getCost(i)->getAmount()<0){
			return true;
		}
	}
	return false;
}

bool AiRuleBuild::isWarriorProducer(const UnitType *building){
	for (int i= 0; i < building->getCommandTypeCount(); i++) {
		const CommandType *ct= building->getCommandType(i);
		if (ct->getClass() == CommandClass::PRODUCE) {
			const ProduceCommandType *pt = static_cast<const ProduceCommandType*>(ct);
			for (int j=0; j < pt->getProducedUnitCount(); ++j) {
				const UnitType *ut= pt->getProducedUnit(j);
				if (ut->isOfClass(UnitClass::WARRIOR)) {
					return true;
				}
			}
		}
	}
	return false;
}

// ========================================
// 	class AiRuleUpgrade
// ========================================

AiRuleUpgrade::AiRuleUpgrade(Ai *ai):
	AiRule(ai)
{
	upgradeTask= NULL;
}

bool AiRuleUpgrade::test(){
	const Task *task= ai->getTask();

	if(task==NULL || task->getClass()!=TaskClass::UPGRADE){
		return false;
	}

	upgradeTask= static_cast<const UpgradeTask*>(task);
	return true;
}

void AiRuleUpgrade::execute(){

	//upgrade any upgrade
	if(upgradeTask->getUpgradeType()==NULL){
		upgradeGeneric(upgradeTask);
	}
	//upgrade specific upgrade
	else{
		upgradeSpecific(upgradeTask);
	}

	//remove the task
	ai->removeTask(upgradeTask);
}

void AiRuleUpgrade::upgradeGeneric(const UpgradeTask *upgt){

	typedef vector<const UpgradeType*> UpgradeTypes;
	GlestAiInterface *aiInterface= ai->getAiInterface();

	//find upgrades that can be upgraded
	UpgradeTypes upgrades;

	//for each upgrade, upgrade it if possible
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){

		//for each command
		const UnitType *ut= aiInterface->getMyUnit(i)->getType();
		for(int j=0; j<ut->getCommandTypeCount(); ++j){
			const CommandType *ct= ut->getCommandType(j);

			//if the command is upgrade
			if(ct->getClass()==CommandClass::UPGRADE){
				const UpgradeCommandType *upgct= static_cast<const UpgradeCommandType*>(ct);
				const UpgradeType *upgrade= upgct->getProducedUpgrade();
				if(aiInterface->reqsOk(upgct)){
					upgrades.push_back(upgrade);
				}
			}
		}
	}

	//add specific upgrade task
	if(!upgrades.empty()){
		ai->addTask(new UpgradeTask(upgrades[ai->getRandom()->randRange(0, upgrades.size()-1)]));
	}
}

void AiRuleUpgrade::upgradeSpecific(const UpgradeTask *upgt){

	GlestAiInterface *aiInterface= ai->getAiInterface();

	//if reqs ok
	if(aiInterface->reqsOk(upgt->getUpgradeType())){

		//if resources dont meet retry
		if(!aiInterface->checkCosts(upgt->getUpgradeType())){
			ai->retryTask(upgt);
			return;
		}

		//for each unit
		for(int i=0; i<aiInterface->getMyUnitCount(); ++i){

			//for each command
			const UnitType *ut= aiInterface->getMyUnit(i)->getType();
			for(int j=0; j<ut->getCommandTypeCount(); ++j){
				const CommandType *ct= ut->getCommandType(j);

				//if the command is upgrade
				if(ct->getClass()==CommandClass::UPGRADE){
					const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
					const UpgradeType *producedUpgrade= uct->getProducedUpgrade();

					//if upgrades match
					if(producedUpgrade == upgt->getUpgradeType()){
						if(aiInterface->reqsOk(uct)){
							aiInterface->giveCommand(i, uct);
						}
					}
				}
			}
		}

	}
}

// ========================================
// 	class AiRuleExpand
// ========================================

AiRuleExpand::AiRuleExpand(Ai *ai):
	AiRule(ai)
{
	storeType= NULL;
}

bool AiRuleExpand::test(){
	GlestAiInterface *aiInterface = ai->getAiInterface();

	for(int i= 0; i<aiInterface->getTechTree()->getResourceTypeCount(); ++i){
		const ResourceType *rt = aiInterface->getTechTree()->getResourceType(i);

		if(rt->getClass()==ResourceClass::TECHTREE){

			// If any resource sighted
			if(aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), expandPos)){

				int minDistance= INT_MAX;
				storeType= NULL;

				//If there is no close store
				for(int j=0; j<aiInterface->getMyUnitCount(); ++j){
					const Unit *u= aiInterface->getMyUnit(j);
					const UnitType *ut= aiInterface->getMyUnit(j)->getType();

					// If this building is a store
					if(ut->getStore(rt)>0){
						storeType = ut;
						int distance= static_cast<int> (u->getPos().dist(expandPos));

						if(distance < minDistance){
							minDistance = distance;
						}
					}
				}

				if(minDistance>expandDistance)
				{
					return true;
				}
			}
			else{
				// send patrol to look for resource
				ai->sendScoutPatrol();
			}
		}
	}

	return false;
}

void AiRuleExpand::execute(){
	ai->addExpansion(expandPos);
	ai->addPriorityTask(new BuildTask(storeType, expandPos));
}

}}//end namespace
