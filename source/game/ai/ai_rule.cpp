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

#define FACTION_INDEX ai->getAiInterface()->getFactionIndex()

#define AI_LOG(component, level, message)                                                 \
	if (g_logger.shouldLogAiEvent(FACTION_INDEX, Util::AiComponent::component, level)) {  \
		LOG_AI( FACTION_INDEX, Util::AiComponent::component, level, message );            \
	}


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
	if (ai->findAbleUnit(&stoppedWorkerIndex, CmdClass::HARVEST, true)) {
		AI_LOG( ECONOMY, 3, "AiRuleWorkerHarvest::test: found idle worker, unit index = " << stoppedWorkerIndex );
		return true;
	}
	AI_LOG( ECONOMY, 2, "AiRuleWorkerHarvest::test: failed to find idle worker" );
	return false;
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
	if (ai->findAbleUnit(&workerIndex, CmdClass::HARVEST, false)) {
		AI_LOG( ECONOMY, 3, "AiRuleRefreshHarvester::test: found worker, unit index = " << workerIndex );
		return true;
	}
	AI_LOG( ECONOMY, 2, "AiRuleRefreshHarvester::test: failed to find any worker!" );
	return false;
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

	// look for a damaged unit that we can repair
	for (int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		const Unit *u = aiInterface->getMyUnit(i);
		if (u->getHpRatio() < 1.f && ai->isRepairable(u)) {
			repairable.push_back(u);
		}
	}
	if (!repairable.empty()) {
		AI_LOG( GENERAL, 2, "AiRuleRepair::test: repairable units updated, with unit(s) needing repair." );
		return true;
	}
	AI_LOG( GENERAL, 2, "AiRuleRepair::test: repairable units updated, with no units needing repair." );
	return false;
}

void AiRuleRepair::execute() {
	GlestAiInterface *aiInterface = ai->getAiInterface();

	const RepairCommandType *nearestRct = NULL;
	int nearest = -1;
	fixed minDist = fixed::max_int();

	//try all of our repairable units in case one type of repairer is busy, but
	//others aren't.
	foreach_const (ConstUnitVector, it, repairable) {
		const Unit *damagedUnit = *it;
		AI_LOG( GENERAL, 3, "AiRuleRepair::execute: Trying to find repairer for damaged unit id " << damagedUnit->getId()
			<< " @ " << damagedUnit->getPos() );

		fixedVec2 fpos = damagedUnit->getFixedCenteredPos();
		int size = damagedUnit->getType()->getSize();

		// find the nearest available repairer and issue command
		for (int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
			const Unit *u = aiInterface->getMyUnit(i);
			const RepairCommandType *rct;
			if ( (u->isIdle() || u->isMoving()) && (rct = u->getRepairCommandType(damagedUnit)) ) {
				fixed dist = fpos.dist(u->getFixedCenteredPos()) + size + u->getType()->getHalfSize();
				if (minDist > dist) {
					nearestRct = rct;
					nearest = i;
					minDist = dist;
					AI_LOG( GENERAL, 3, "AiRuleRepair::execute: Candidate repairer id " 
						<< u->getId() << " @ " << u->getPos() << " distance " << dist.toFloat() 
						<< " (closest seen so far)" );
				}
			}
		}

		if (nearestRct) {
			CmdResult res = aiInterface->giveCommand(nearest, nearestRct, const_cast<Unit*>(damagedUnit));
			const Unit *repairer = aiInterface->getMyUnit(nearest);
			AI_LOG( GENERAL, 2, "AiRuleRepair::execute: Repair command issued, repairer id "
				<< repairer->getId() << " @ " << repairer->getPos() << ", repairee id " << damagedUnit->getId()
				<< " [type=" << damagedUnit->getType()->getName() << "] @ " << damagedUnit->getPos()
				<< " Reult: " << CmdResultNames[res] );
			return;
		} else {
			AI_LOG( GENERAL, 2, "AiRuleRepair::execute: Damaged unit id " << damagedUnit->getId() << " [type="
				<< damagedUnit->getType()->getName() << "] @ " << damagedUnit->getPos()
				<< " could not find a repairer." );
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
	if (ai->findAbleUnit(&stoppedUnitIndex, CmdClass::MOVE, true)) {
		AI_LOG( MILITARY, 3, "AiRuleReturnBase::test: found idle unit." );
		return true;
	}
	AI_LOG( MILITARY, 3, "AiRuleReturnBase::test: found no idle units." );
	return false;
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
		AI_LOG( MILITARY, 2, "AiRuleMassiveAttack::test: base stable, will launch "
			<< "attack if any enemy found." );
		ultraAttack = false;
		bool res = ai->beingAttacked(numeric_limits<float>::infinity(), attackPos, field);
		if (!res) {
			if (true != false) { //this_ai_is_mad_keen_on_killing_ya) {
				res = ai->blindAttack(attackPos);
				field = Field::LAND;
			}
		}
		return res;
	} else {
		AI_LOG( MILITARY, 2, "AiRuleMassiveAttack::test: base is not stable, will "
			<< "counter only if being attacked." );
		ultraAttack = true;
		return ai->beingAttacked(float(baseRadius), attackPos, field);
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
	if (!ai->anyTask() || ai->getCountOfClass(UnitClass::WORKER) < 4) {
		AI_LOG( GENERAL, 3, "AiRuleAddTasks::test: No tasks or need emergency worker." );
		return true;
	}
	AI_LOG( GENERAL, 3, "AiRuleAddTasks::test: still tasks to attend to, not running." );
	return false;
}

void AiRuleAddTasks::execute() {
	GlestAiInterface *aiInterface = ai->getAiInterface();

	int buildingCount = ai->getCountOfClass(UnitClass::BUILDING);
	int warriorCount = ai->getCountOfClass(UnitClass::WARRIOR);
	int workerCount = ai->getCountOfClass(UnitClass::WORKER);
	int upgradeCount = aiInterface->getMyUpgradeCount();

	float buildingRatio = ai->getRatioOfClass(UnitClass::BUILDING);
	float warriorRatio = ai->getRatioOfClass(UnitClass::WARRIOR);
	float workerRatio = ai->getRatioOfClass(UnitClass::WORKER);
	ai->updateStatistics();

	//standard tasks
	if (workerCount < 4 && canProduce(UnitClass::WORKER)) { // emergency workers
		AI_LOG( GENERAL, 2, "AiRuleAddTasks::execute: Emergency! Priority task => produce worker." );
		ai->addPriorityTask(new ProduceTask(UnitClass::WORKER));
		return;
	}

	int workers = 0, warriors = 0, buildings = 0, upgrades = 0;

	// workers
	workers += workerCount < 5    ? 2 : workerCount < 10   ? 1 : 0;
	workers += workerRatio < 0.2f ? 2 : workerRatio < 0.3f ? 1 : 0;

	// warriors
	warriors += warriorCount < 10   ? 1 : 0;
	warriors += warriorRatio < 0.2f ? 2 : warriorRatio < 0.3f ? 1 : 0;
	warriors += workerCount >= 15   ? 2 : workerCount >= 10   ? 1 : 0;
	
	// buildings
	if (buildingCount < 6 || buildingRatio < 0.2f) ++buildings;
	if (buildingCount < 10 && workerCount > 12) ++buildings;
		
	// upgrades
	if (workerCount > 5 * (upgradeCount + 1)) ++upgrades;

	if (aiInterface->getControlType() == ControlType::CPU_MEGA) {
		// additional warriors
		if (warriorCount < ai->getMinWarriors() + 2) {
			warriors += buildingCount > 12 ? 5 : buildingCount > 9 ? 3 : 1;
		}
		// additional upgrades
		if (ai->isStableBase()) ++upgrades;

	} else if(aiInterface->getControlType() == ControlType::CPU_EASY) {// Easy CPU
		// workers
		workers = 0; // reset
		if (workerCount < buildingCount + 2) ++workers;
		if (workerCount > 5 && workerRatio < 0.20) ++workers;
		
		// buildings
		buildings = 0; // reset
		if (buildingCount < 6 || buildingRatio < 0.2f) ++buildings;
		if (buildingCount < 10 && ai->isStableBase()) ++buildings;
	
	} else {// normal CPU / UltraCPU ...
		// additional upgrades
		if (ai->isStableBase()) ++upgrades;
	}
	AI_LOG( GENERAL, 2, "AiRuleAddTasks::execute: Adding " << workers << " produce worker tasks." );
	for (int i=0; i < workers; ++i) {
		ai->addTask(new ProduceTask(UnitClass::WORKER));
	}
	AI_LOG( GENERAL, 2, "AiRuleAddTasks::execute: Adding " << warriors << " produce warrior tasks." );
	for (int i=0; i < warriors; ++i) {
		ai->addTask(new ProduceTask(UnitClass::WARRIOR));
	}
	AI_LOG( GENERAL, 2, "AiRuleAddTasks::execute: Adding " << buildings << " build tasks." );
	for (int i=0; i < buildings; ++i) {
		 ai->addTask(new BuildTask());
	}
	AI_LOG( GENERAL, 2, "AiRuleAddTasks::execute: Adding " << upgrades << " research upgrade tasks." );
	for (int i=0; i < upgrades; ++i) {
		ai->addTask(new UpgradeTask());
	}
}

bool AiRuleAddTasks::canProduce(UnitClass unitClass) const {
	GlestAiInterface *aiInterface = ai->getAiInterface();
	const FactionType *ft = aiInterface->getMyFactionType();
	const Faction *faction = aiInterface->getMyFaction();

	// Find the first UnitType who can produce/morph-to UnitTypes of UnitClass requested and that have reqsOk
	for (int i=0; i < ft->getUnitTypeCount(); ++i) {
		const UnitType *ut = ft->getUnitType(i);
		for (int j=0; j < ut->getCommandTypeCount(); ++j) {
			const CommandType *ct = ut->getCommandType(j);
			if (!faction->reqsOk(ct)
			|| (ct->getClass() != CmdClass::PRODUCE && ct->getClass() != CmdClass::MORPH)) {
				continue;
			}

			for (int k=0; k < ct->getProducedCount(); ++k) {
				const UnitType *pt = static_cast<const UnitType*>(ct->getProduced(k));
				if (faction->reqsOk(pt) && pt->isOfClass(unitClass)) {
					return true; // found it
				}
			}
		}
	}
	return false; // haven't found one
}

// =====================================================
//	class AiRuleBuildOneFarm
// =====================================================

AiRuleBuildOneFarm::AiRuleBuildOneFarm(Ai *ai)
		: AiRule(ai)
		, farm(0) {
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
					ResourceAmount r = pt->getCost(n, ai->getAiInterface()->getFaction());
					// can produce consumables and would be the first of its type?
					if (r.getAmount() < 0 && r.getType()->getClass() == ResourceClass::CONSUMABLE 
					&& ai->getCountOfType(ut) == 0) {
						farm = ut;
						AI_LOG( PRODUCTION, 1, "AiRuleBuildOneFarm::test: found unbuilt farm-type " << farm->getName() );
						return true;
					}
				}

			}
		}
	}
	AI_LOG( PRODUCTION, 2, "AiRuleBuildOneFarm::test: found no farms I don't already have." );
	return false;
}

void AiRuleBuildOneFarm::execute() {
	AI_LOG( PRODUCTION, 1, "AiRuleBuildOneFarm::execute: adding priority task to build a " << farm->getName() );
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
		const StoredResource *r = aiInterface->getResource(rt);
		if (rt->getClass() == ResourceClass::CONSUMABLE && r->getBalance() < 0) {
			AI_LOG( PRODUCTION, 2, "AiRuleProduceResourceProducer::test: found consumable resource "
				<< rt->getName() << " with balance less than 0" );
			interval = longInterval;
			return true;
		}
	}
	if (!ai->usesStaticResources()) {
		AI_LOG( PRODUCTION, 3, "AiRuleProduceResourceProducer::test: found no needed consumable resources"
			<< " and don't use any statics" );
		interval = shortInterval;
		return false;
	}
	// statics second
	for (int i = 0; i < aiInterface->getTechTree()->getResourceTypeCount(); ++i) {
		rt = aiInterface->getTechTree()->getResourceType(i);
		const StoredResource *r = aiInterface->getResource(rt);
		if (rt->getClass() == ResourceClass::STATIC && ai->isStaticResourceUsed(rt)
		&& r->getAmount() < minStaticResources) {
			AI_LOG( PRODUCTION, 2, "AiRuleProduceResourceProducer::test: found static resource "
				<< rt->getName() << " below min-static threshold" );
			interval = longInterval;
			return true;
		}
	}
	AI_LOG( PRODUCTION, 3, "AiRuleProduceResourceProducer::test: found no needed consumable or "
		<< "static resources" );
	interval = shortInterval;
	return false;
}

void AiRuleProduceResourceProducer::execute() {
	AI_LOG( PRODUCTION, 1, "AiRuleProduceResourceProducer::execute: adding priority task "
		<< "to produce " << rt->getName() );
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
		AI_LOG( PRODUCTION, 3, "AiRuleProduce::test: head of task queue is not produce task" );
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
		AI_LOG( PRODUCTION, 1, "AiRuleProduce::execute: produceTask is NULL." );
	}
}

void AiRuleProduce::produceResources(const ProduceTask *task) {
	assert(task->getResourceType());
	assert(task->getUnitClass() == UnitClass::INVALID && !task->getUnitType());
	
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
				const ResourceAmount res = pt->getCost(task->getResourceType(), ai->getAiInterface()->getFaction());
				if (res.getType() && res.getAmount() < 0 && faction->reqsOk(pt)) {
					prodMap[ut].push_back(std::make_pair(ct, pt));
				}
			}
		}
	}
	if (prodMap.empty()) {
		AI_LOG( PRODUCTION, 3, "AiRuleProduce::produceResources: unable to find any unit types that can"
			<< " produce " << task->getResourceType()->getName() );
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
			if (cmds == 1 && unit->getCommands().front()->getType()->getClass() == CmdClass::STOP) {
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
		AI_LOG( PRODUCTION, 3, "AiRuleProduce::produceResources: unable to find any active units that can"
			<< " produce " << task->getResourceType()->getName() );
		return;
	}

	// 3. Give new produce/morph/generate command
	int cmdNdx = RAND_RANGE(0, prodMap[unitToCmd->getType()].size() - 1);
	ProdPair pp = prodMap[unitToCmd->getType()][cmdNdx];
	if (pp.first->getClass() == CmdClass::TRANSFORM) {
		Vec2i pos;
		if (ai->findPosForBuilding(static_cast<const UnitType*>(pp.second), ai->getRandomHomePosition(), pos)) {
			AI_LOG( PRODUCTION, 2, "AiRuleProduce::produceResources: giving unit id " << unitToCmd->getId()
				<< "[type=" << unitToCmd->getType()->getName() << "] command to tranform to "
				<< pp.second->getName() );
			aiInterface->giveCommand(unitToCmd, pp.first, pos, pp.second);
		} else {
			AI_LOG( PRODUCTION, 2, "AiRuleProduce::produceResources: could not find location to "
				<< " tranform unit, reposting task." );
			ai->retryTask(task);
		}
	} else {
		AI_LOG( PRODUCTION, 1, "AiRuleProduce::produceResources: giving unit id " << unitToCmd->getId()
			<< "[type=" << unitToCmd->getType()->getName() << "] command to produce " << pp.second->getName() );
		aiInterface->giveCommand(unitToCmd, pp.first, Command::invalidPos, pp.second);
	}
}

void AiRuleProduce::produceGeneric(const ProduceTask *task) {
	assert(task->getUnitClass() != UnitClass::INVALID);
	assert(!task->getResourceType() && !task->getUnitType());
	
	GlestAiInterface *aiInterface = ai->getAiInterface();
	const FactionType *ft = aiInterface->getMyFactionType();
	const Faction *faction = aiInterface->getMyFaction();

	typedef pair<const CommandType*, const UnitType*> ProdPair;
	typedef vector<ProdPair> CommandList;
	map<const UnitType*, CommandList> prodMap;

	// 1. Find all UnitTypes who can produce/morph-to UnitTypes of UnitClass requested and that have reqsOk
	for (int i=0; i < ft->getUnitTypeCount(); ++i) {
		const UnitType *ut = ft->getUnitType(i);
		for (int j=0; j < ut->getCommandTypeCount(); ++j) {
			const CommandType *ct = ut->getCommandType(j);
			if (!faction->reqsOk(ct)
			|| (ct->getClass() != CmdClass::PRODUCE && ct->getClass() != CmdClass::MORPH)) {
				continue;
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
		AI_LOG( PRODUCTION, 3, "AiRuleProduce::produceGeneric: unable to find any unit types that can"
			<< " produce units of class " << UnitClassNames[task->getUnitClass()] );
		return;
	}

	// 2. Find all units of types found in 1
	int lowCmd = numeric_limits<int>::max();
	Unit *unitToCmd = 0;
	for (int i=0; i < faction->getUnitCount(); ++i) {
		Unit *unit = faction->getUnit(i);
		if (prodMap.find(unit->getType()) != prodMap.end()) {
			int cmds = unit->getCommands().size();
			if (cmds == 1 && unit->getCommands().front()->getType()->getClass() == CmdClass::STOP) {
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
		AI_LOG( PRODUCTION, 3, "AiRuleProduce::produceGeneric: unable to find any active units that can"
			<< " produce units of class " << UnitClassNames[task->getUnitClass()] );
		return;
	}

	// 3. Give produce/morph command.
	int cmdNdx = RAND_RANGE(0, prodMap[unitToCmd->getType()].size() - 1);
	ProdPair pp = prodMap[unitToCmd->getType()][cmdNdx];
	AI_LOG( PRODUCTION, 1, "AiRuleProduce::produceGeneric: giving unit id " << unitToCmd->getId()
		<< "[type=" << unitToCmd->getType()->getName() << "] command to produce/morph-to "
		<< pp.second->getName() );
	aiInterface->giveCommand(unitToCmd, pp.first, Command::invalidPos, pp.second);
}

void AiRuleProduce::produceSpecific(const ProduceTask *task) {
	assert(task->getUnitType());
	GlestAiInterface *aiInterface = ai->getAiInterface();
	const FactionType *ft = aiInterface->getMyFactionType();
	Faction *faction = aiInterface->getMyFaction();

	typedef pair<const CommandType*, const UnitType*> ProdPair;
	typedef vector<ProdPair> CommandList;
	map<const UnitType*, CommandList> prodMap;

	// 1. Do we meet requirements and costs?
	if (!aiInterface->reqsOk(task->getUnitType())) { // if unit meets requirements
		AI_LOG( PRODUCTION, 3, "AiRuleProduce::produceSpecific: unit/upgrade reqs not met." );
		return;
	}
	if (!aiInterface->checkCosts(task->getUnitType())) { //if unit doesnt meet resources retry
		AI_LOG( PRODUCTION, 3, "AiRuleProduce::produceSpecific: resource reqs not met, reposting.");
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
			|| (ct->getClass() != CmdClass::PRODUCE && ct->getClass() != CmdClass::MORPH)) {
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
	if (prodMap.empty()) {
		AI_LOG( PRODUCTION, 3, "AiRuleProduce::produceSpecific: unable to find any unit types that can"
			<< " produce unit type " << task->getUnitType()->getName() );
		return;
	}

	// 3. find all units of types found in 2, and remember the best (lowest command count)
	int lowCmd = numeric_limits<int>::max();
	Unit *unitToCmd = 0;
	for (int i=0; i < faction->getUnitCount(); ++i) {
		Unit *unit = faction->getUnit(i);
		if (prodMap.find(unit->getType()) != prodMap.end()) {
			int cmds = unit->getCommands().size();
			if (cmds == 1 && unit->getCommands().front()->getType()->getClass() == CmdClass::STOP) {
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
		AI_LOG( PRODUCTION, 3, "AiRuleProduce::produceSpecific: unable to find any active units that can"
			<< " produce unit type " << task->getUnitType()->getName() << ", reposting." );
		ai->retryTask(task);
		return;
	}

	// 4. if producer with lowest command count has many commands, add task to make more producers
	if (lowCmd > 2) {
		if (aiInterface->reqsOk(unitToCmd->getType())) { 
			if (ai->getCountOfClass(UnitClass::BUILDING) > 5) {
				AI_LOG( PRODUCTION, 3, "AiRuleProduce::produceSpecific: lowest producer command queue is "
					<< lowCmd << ", adding task to build/produce another " << unitToCmd->getType()->getName() );
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
	AI_LOG( PRODUCTION, 1, "AiRuleProduce::produceSpecific: giving unit id " << unitToCmd->getId()
		<< "[type=" << unitToCmd->getType()->getName() << "] command to produce/morph-to "
		<< pp.second->getName() );
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
		AI_LOG( PRODUCTION, 3, "AiRuleProduce::test: head of task queue is not build task" );
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
			if (ct->getClass() == CmdClass::BUILD) { // if the command is build
				const BuildCommandType *bct= static_cast<const BuildCommandType*>(ct);
				//for each building
				for (int k=0; k < bct->getBuildingCount(); ++k) {
					const UnitType *buildingType = bct->getBuilding(k);
					if (aiInterface->reqsOk(bct) && aiInterface->reqsOk(buildingType)) {
						//if any building, or produces resource
						ResourceAmount cost = buildingType->getCost(rt, ai->getAiInterface()->getFaction());
						if (!rt || (cost.getType() && cost.getAmount() < 0)) {
							utList.push_back(buildingType);
						}
					}
				}
			}
		}
	}
}

void AiRuleBuild::buildGeneric(const BuildTask *bt) {
	// find buildings that can be built
	UnitTypeList buildingTypes;
	findBuildingTypes(buildingTypes, bt->getResourceType());
	if (buildingTypes.empty()) {
		AI_LOG( PRODUCTION, 3, "AiRuleBuild::buildGeneric: could not find any buildable unit types" );
	}

	// add specific build task
	buildBestBuilding(buildingTypes);
}

void AiRuleBuild::buildBestBuilding(const UnitTypeList &buildingTypes) {
	if (buildingTypes.empty()) {
		AI_LOG( PRODUCTION, 1, "AiRuleBuild::buildBestBuilding: no building types to build." );
		return;
	}
	// build the least built buildingType
	for (int i=0; i < 10; ++i) {
		if (i > 0) {
			// Defensive buildings have priority
			for (int j=0; j < buildingTypes.size(); ++j) {
				const UnitType *buildingType = buildingTypes[j];
				if (ai->getCountOfType(buildingType) <= i + 1 && isDefensive(buildingType)) {
					AI_LOG( PRODUCTION, 2, "AiRuleBuild::buildBestBuilding: adding build task, "
						<< "Defensive building, type = " << buildingType->getName() );
					ai->addTask(new BuildTask(buildingType));
					return;
				}
			}
			// Warrior producers next
			for (int j=0; j < buildingTypes.size(); ++j) {
				const UnitType *buildingType = buildingTypes[j];
				if (ai->getCountOfType(buildingType) <= i + 1 && isWarriorProducer(buildingType)) {
					AI_LOG( PRODUCTION, 1, "AiRuleBuild::buildBestBuilding: adding build task, "
						<< "Warrior producing building, type = " << buildingType->getName() );
					ai->addTask(new BuildTask(buildingType));
					return;
				}
			}
			// Resource producers next
			for (int j=0; j < buildingTypes.size(); ++j) {
				const UnitType *buildingType = buildingTypes[j];
				if (ai->getCountOfType(buildingType) <= i + 1 && isResourceProducer(buildingType)) {
					AI_LOG( PRODUCTION, 1, "AiRuleBuild::buildBestBuilding: adding build task, "
						<< "Resource producing building, type = " << buildingType->getName() );
					ai->addTask(new BuildTask(buildingType));
					return;
				}
			}
		}
		// Any buildingType
		for (int j=0; j < buildingTypes.size(); ++j) {
			const UnitType *buildingType = buildingTypes[j];
			if (ai->getCountOfType(buildingType) <= i) {
				AI_LOG( PRODUCTION, 1, "AiRuleBuild::buildBestBuilding: adding build task, "
					<< "low-ratio building, type = " << buildingType->getName() );
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
			if (ct->getClass() == CmdClass::BUILD) { // if the command is build
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
		AI_LOG( PRODUCTION, 3, "AiRuleBuild::buildSpecific: building to build : "
			<< bt->getUnitType()->getName() << ", unit/upgrade reqs not met" );
		return;
	}
	if (!aiInterface->checkCosts(bt->getUnitType())) { // retry if not enough resources
		AI_LOG( PRODUCTION, 3, "AiRuleBuild::buildSpecific: building to build : "
			<< bt->getUnitType()->getName() << ", resource reqs not met, re-posting" );
		ai->retryTask(bt);
		return;
	}

	CmdByUtMap cmdMap;
	findBuilderTypes(bt->getUnitType(), cmdMap);
	if (cmdMap.empty()) {
		AI_LOG( PRODUCTION, 3, "AiRuleBuild::buildSpecific: failed to find any unit types who can build a "
			<< bt->getUnitType()->getName() );
		return;
	}

	ConstUnitVector potentialBuilders;

	foreach_const (Units, it, units) {
		const Unit *const &unit = *it;
		if (cmdMap.find(unit->getType()) != cmdMap.end()
		&& (!unit->anyCommand() || unit->getCurrCommand()->getType()->getClass() != CmdClass::BUILD)) {
			potentialBuilders.push_back(unit);
		}
	}
	//use random builder to build
	if (potentialBuilders.empty()) {
		AI_LOG( PRODUCTION, 2, "AiRuleBuild::buildSpecific: failed to find any active units who can build a "
			<< bt->getUnitType()->getName() );
		return;
	}
	const Unit *unit = potentialBuilders[RAND_RANGE(0, potentialBuilders.size() - 1)];
	Vec2i pos;
	Vec2i searchPos = bt->getForcePos() ? bt->getPos() : ai->getRandomHomePosition();
	// if free pos give command, else retry
	if (ai->findPosForBuilding(bt->getUnitType(), searchPos, pos)) {
		AI_LOG( PRODUCTION, 1, "AiRuleBuild::buildSpecific: building to build : " << bt->getUnitType()->getName()
			<< ", " << cmdMap[unit->getType()]->getName() << " command given to unit "
			<< unit->getId() << " [" << unit->getType()->getName() << "]" );
		aiInterface->giveCommand(unit, cmdMap[unit->getType()], pos, bt->getUnitType());
	} else {
		AI_LOG( PRODUCTION, 2, "AiRuleBuild::buildSpecific: building to build : " << bt->getUnitType()->getName()
			<< ", could not find position for building. Re-posting." );
		ai->retryTask(bt);
	}
}

bool AiRuleBuild::isDefensive(const UnitType *building){
	return building->hasSkillClass(SkillClass::ATTACK);
}

bool AiRuleBuild::isResourceProducer(const UnitType *building){
	for (int i= 0; i < building->getCostCount(); i++) {
		if (building->getCost(i, ai->getAiInterface()->getFaction()).getAmount() < 0) {
			return true;
		}
	}
	return false;
}

bool AiRuleBuild::isWarriorProducer(const UnitType *building){
	for (int i= 0; i < building->getCommandTypeCount(); i++) {
		const CommandType *ct= building->getCommandType(i);
		if (ct->getClass() == CmdClass::PRODUCE) {
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

AiRuleUpgrade::AiRuleUpgrade(Ai *ai)
		: AiRule(ai), upgradeTask(0) {
}

bool AiRuleUpgrade::test() {
	const Task *task = ai->getTask();
	if (task == 0 || task->getClass()!=TaskClass::UPGRADE) {
		AI_LOG( RESEARCH, 3, "AiRuleUpgrade::test: head of task queue is not upgrade task" );
		return false;
	}
	upgradeTask = static_cast<const UpgradeTask*>(task);
	return true;
}

void AiRuleUpgrade::execute() {
	// upgrade any upgrade
	if (upgradeTask->getUpgradeType() == 0) {
		upgradeGeneric(upgradeTask);
	} else { // upgrade specific upgrade
		upgradeSpecific(upgradeTask);
	}
	// remove the task
	ai->removeTask(upgradeTask);
}

void AiRuleUpgrade::upgradeGeneric(const UpgradeTask *upgt) {
	typedef vector<const UpgradeType*> UpgradeTypes;
	GlestAiInterface *aiInterface= ai->getAiInterface();

	// 1. find upgrades that can be upgraded
	UpgradeTypes upgrades;
	for (int i=0; i < aiInterface->getMyUnitCount(); ++i) { // foreach unit
		const UnitType *ut = aiInterface->getMyUnit(i)->getType();
		for (int j=0; j < ut->getCommandTypeCount<UpgradeCommandType>(); ++j) { // for each upgrade command
			const UpgradeCommandType *upgct = ut->getCommandType<UpgradeCommandType>(j);
			for (int k=0; k < upgct->getProducedCount(); ++k) {
				const UpgradeType *upgrade = upgct->getProducedUpgrade(k);
				if (aiInterface->reqsOk(upgct)) { ///@todo: does this weed out already/in-progress ?
					upgrades.push_back(upgrade);
				}
			}
		}
	}
	if (upgrades.empty()) {
		AI_LOG( RESEARCH, 3, "AiRuleUpgrade::upgradeGeneric: failed to find any upgrades that can be researched" );
		return;
	}
	// add specific upgrade task
	const UpgradeType *ut = upgrades[ai->getRandom()->randRange(0, upgrades.size()-1)];
	AI_LOG( RESEARCH, 2, "AiRuleUpgrade::upgradeGeneric: adding task to research " << ut->getName() );
	ai->addTask(new UpgradeTask(ut));
}

void AiRuleUpgrade::upgradeSpecific(const UpgradeTask *upgt) {
	GlestAiInterface *aiInterface = ai->getAiInterface();
	
	if (aiInterface->reqsOk(upgt->getUpgradeType())) { // if reqs ok
		// if resources dont meet retry
		if (!aiInterface->checkCosts(upgt->getUpgradeType())) {
			AI_LOG( RESEARCH, 3, "AiRuleUpgrade::upgradeSpecific: can't afford upgrade " 
				<< upgt->getUpgradeType()->getName() << ", re-posting" );
			ai->retryTask(upgt);
			return;
		}
		// for each unit
		for (int i=0; i < aiInterface->getMyUnitCount(); ++i) {
			// for each upgrade command
			const UnitType *ut = aiInterface->getMyUnit(i)->getType();
			for (int j=0; j < ut->getCommandTypeCount<UpgradeCommandType>(); ++j) {
				const UpgradeCommandType *uct = ut->getCommandType<UpgradeCommandType>(j);
				for (int k=0; k < uct->getProducedCount(); ++k) {
					const UpgradeType *producedUpgrade = uct->getProducedUpgrade(k);

					// if upgrades match
					if (producedUpgrade == upgt->getUpgradeType()) {
						if (aiInterface->reqsOk(uct)) {
							AI_LOG( RESEARCH, 1, "AiRuleUpgrade::upgradeSpecific: issueing command to unit "
								<< aiInterface->getMyUnit(i)->getId() << " to research "
								<< upgt->getUpgradeType()->getName() );
							aiInterface->giveCommand(i, uct, invalidPos, producedUpgrade);
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

AiRuleExpand::AiRuleExpand(Ai *ai)
		: AiRule(ai), storeType(0) {
}

bool AiRuleExpand::test() {
	GlestAiInterface *aiInterface = ai->getAiInterface();

	for (int i= 0; i < aiInterface->getTechTree()->getResourceTypeCount(); ++i) {
		const ResourceType *rt = aiInterface->getTechTree()->getResourceType(i);
		if (rt->getClass() == ResourceClass::TECHTREE) {
			// If any resource sighted
			if (aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), expandPos)) {
				int minDistance = numeric_limits<int>::max();
				storeType = 0;
				for (int j=0; j < aiInterface->getMyUnitCount(); ++j) { // foreach unit
					const Unit *u = aiInterface->getMyUnit(j);
					const UnitType *ut = aiInterface->getMyUnit(j)->getType();
					if (ut->getStore(rt, ai->getAiInterface()->getFaction()) > 0) { // store ?
						storeType = ut;
						int distance = static_cast<int>(u->getPos().dist(expandPos));
						if (distance < minDistance) {
							minDistance = distance;
						}
					}
				}
				if (minDistance > expandDistance ) {
					// if min distance from store to any tech resource is more than expandDistance (30)
					AI_LOG( ECONOMY, 1, "AiRuleExpand::test: Need to expand." );
					return true;
				}
			} else {
				// else send patrol to look for resource
				AI_LOG( ECONOMY, 2, "AiRuleExpand::test: Need expansion, but can't find any "
					<< "resources, sending scout." );
				ai->sendScoutPatrol();
			}
		}
	}
	AI_LOG( ECONOMY, 3, "AiRuleExpand::test: No need to expand at this time." );
	return false;
}

void AiRuleExpand::execute() {
	AI_LOG( ECONOMY, 2, "AiRuleExpand::execute: Adding expansion @ " << expandPos );
	ai->addExpansion(expandPos);
	AI_LOG( ECONOMY, 2, "AiRuleExpand::execute: Adding priority task to build a " << storeType->getName()
		<< " at new expansion" );
	ai->addPriorityTask(new BuildTask(storeType, expandPos));
}

}}//end namespace
