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

#include "leak_dumper.h"

using std::numeric_limits;
using Shared::Math::Vec2i;

namespace Glest { namespace Plan {

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
			aiInterface->printLog(3, "Repairing order issued");
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

	//for all units
	for(int i = 0; i < aiInterface->getMyFactionType()->getUnitTypeCount(); ++i) {
		const UnitType *ut = aiInterface->getMyFactionType()->getUnitType(i);

		//for all production commands
		for(int j = 0; j < ut->getCommandTypeCount(); ++j) {
			const CommandType *ct = ut->getCommandType(j);
			if (ct->getClass() == CommandClass::PRODUCE) {
				const UnitType *producedType = static_cast<const ProduceCommandType*>(ct)->getProducedUnit();

				//for all resources
				for (int k = 0; k < producedType->getCostCount(); ++k) {
					const Resource *r = producedType->getCost(k);

					//find a food producer in the farm produced units
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
	//emergency tasks: resource buildings
	GlestAiInterface *aiInterface = ai->getAiInterface();

	//consumables first
	for (int i = 0; i < aiInterface->getTechTree()->getResourceTypeCount(); ++i) {
        rt = aiInterface->getTechTree()->getResourceType(i);
		const Resource *r = aiInterface->getResource(rt);
		if (rt->getClass() == ResourceClass::CONSUMABLE && r->getBalance() < 0) {
			interval = longInterval;
			return true;

        }
    }

	//statics second
	for (int i = 0; i < aiInterface->getTechTree()->getResourceTypeCount(); ++i) {
        rt = aiInterface->getTechTree()->getResourceType(i);
		const Resource *r = aiInterface->getResource(rt);
		if (rt->getClass() == ResourceClass::STATIC && r->getAmount() < minStaticResources) {
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
	const Task *task= ai->getTask();

	if (task == NULL || task->getClass() != TaskClass::PRODUCE) {
		return false;
	}

	produceTask = static_cast<const ProduceTask*>(task);
	return true;
}

void AiRuleProduce::execute() {
	if (produceTask != NULL) {
		if (produceTask->getUnitType() == NULL) {
			//generic produce task, produce random unit that has the skill or produces the resource
			produceGeneric(produceTask);
		} else {
			//specific produce task, produce if possible, retry if not enough resources
			produceSpecific(produceTask);
		}
		//remove the task
		ai->removeTask(produceTask);
	}
}

void AiRuleProduce::produceGeneric(const ProduceTask *pt) {
	typedef vector<const UnitType*> UnitTypeList;
	typedef set<const UnitType*> UnitTypeSet;

	UnitTypeSet typesSeen;
	UnitTypeList candidateTypes;
	GlestAiInterface *aiInterface = ai->getAiInterface();
	//for each unit, produce it if possible
	for(int i = 0; i < aiInterface->getMyUnitCount(); ++i) {
		//for each command
		const UnitType *ut = aiInterface->getMyUnit(i)->getType();
		for(int j = 0; j < ut->getCommandTypeCount(); ++j) {
			const CommandType *ct = ut->getCommandType(j);
			//if the command is produce
			if (ct->getClass() == CommandClass::PRODUCE || ct->getClass() == CommandClass::MORPH) {
				const UnitType *producedUnit = static_cast<const UnitType*>(ct->getProduced());
				bool produceIt = false;
				//if the unit produces the resource
				if (pt->getResourceType() != NULL) {
					const Resource *r = producedUnit->getCost(pt->getResourceType());
					if (r != NULL && r->getAmount() < 0) {
						produceIt= true;
					}
				} else {
					//if the unit is from the right class
					if (producedUnit->isOfClass(pt->getUnitClass())) {
						if (aiInterface->reqsOk(ct) && aiInterface->reqsOk(producedUnit)) {
							produceIt= true;
						}
					}
				}
				if (produceIt) {
					//if the unit is not already on the list
					if (typesSeen.find(producedUnit) == typesSeen.end()) {
						typesSeen.insert(producedUnit);
						candidateTypes.push_back(producedUnit);
					}
				}
			}
		} // for each command
	} // for each unit

	//add specific produce task
	if (!candidateTypes.empty()) {
		//priority for non produced units
		for (int i=0; i < candidateTypes.size(); ++i) {
			if (ai->getCountOfType(candidateTypes[i]) == 0) {
				if (ai->getRandom()->randRange(0, 1) == 0) {
					ai->addTask(new ProduceTask(candidateTypes[i]));
					return;
				}
			}
		}
		//normal case
		ai->addTask(new ProduceTask( candidateTypes[ai->getRandom()->randRange(0, candidateTypes.size() - 1)] ));
	}
}

void AiRuleProduce::findProducerTypes(const UnitType *produce, CmdByUtMap &cmdMap) {
	const FactionType *factionType = ai->getAiInterface()->getMyFactionType();
	for (int i=0; i < factionType->getUnitTypeCount(); ++i) {
		const UnitType *ut = factionType->getUnitType(i);
		for(int j = 0; j < ut->getCommandTypeCount(); ++j) {
			const CommandType *ct = ut->getCommandType(j);
			//if the command is produce
			if (ct->getClass() == CommandClass::PRODUCE || ct->getClass() == CommandClass::MORPH) {
				const UnitType *producedUnit = static_cast<const UnitType*>(ct->getProduced());
				//if units match
				if (producedUnit == produce) {
					if (ai->getAiInterface()->reqsOk(ct)) {
						cmdMap[ut] = ct;
					}
				}
			}
		}
	}
}

void AiRuleProduce::findProducerTypes(CmdByUtMap &cmdMap) {
	const FactionType *factionType = ai->getAiInterface()->getMyFactionType();
	for (int i=0; i < factionType->getUnitTypeCount(); ++i) {
		const UnitType *ut = factionType->getUnitType(i);
		for(int j = 0; j < ut->getCommandTypeCount(); ++j) {
			const CommandType *ct = ut->getCommandType(j);
			if (ct->getClass() == CommandClass::PRODUCE) { // if the command is produce
				const UnitType *producedUnit = static_cast<const UnitType*>(ct->getProduced());
				if (producedUnit->hasSkillClass(SkillClass::ATTACK)
				&& !producedUnit->hasSkillClass(SkillClass::HARVEST)
				&& ai->getAiInterface()->reqsOk(ct)) {
					cmdMap[ut] = ct;
				}
			}
		}
	}
}

void AiRuleProduce::findLowestCommandQueue(UnitList &list, const Unit **best) {
	int lowestCommandCount = numeric_limits<int>::max();
	*best = 0;
	int cmds;
	foreach (UnitList, it, list) {
		const Unit *&unit = *it;
		cmds = unit->getCommandSize();
		if (!cmds || (cmds == 1 && unit->getCurrCommand()->getType()->getClass() == CommandClass::STOP)) {
			*best = unit;
			return;
		}
		if (cmds < lowestCommandCount) {
			lowestCommandCount = cmds;
			*best = unit;
		}
	}
}

void AiRuleProduce::produceSpecific(const ProduceTask *pt) {
	
	assert(pt->getUnitType());
	GlestAiInterface *aiInterface = ai->getAiInterface();
	const FactionType *factionType = aiInterface->getMyFactionType();
	Faction *faction = aiInterface->getMyFaction();
	const Units &units = faction->getUnits();

	if (!aiInterface->reqsOk(pt->getUnitType())) { // if unit meets requirements
		return;
	}
	if (!aiInterface->checkCosts(pt->getUnitType())) { //if unit doesnt meet resources retry
		ai->retryTask(pt);
		return;
	}		
	// produce specific unit
	// find all units who can produce or morph to required type
	CmdByUtMap l_commandMap;
	findProducerTypes(pt->getUnitType(), l_commandMap);
	UnitList l_producers;
	foreach_const (Units, it, units) {
		if (l_commandMap.find((*it)->getType()) != l_commandMap.end()) {
			l_producers.push_back(*it);
		}
	}
	if (l_producers.empty()) {
		return;
	}
	// produce from random producer
	if (aiInterface->getControlType() != ControlType::CPU_MEGA) {
		const Unit *unit = l_producers[RAND_RANGE(0, l_producers.size() - 1)];
		aiInterface->giveCommand(unit, l_commandMap[unit->getType()]);
	} else {
		// mega cpu trys to balance the commands to the producers
		const Unit *bestSeen = 0;
		findLowestCommandQueue(l_producers, &bestSeen);
		assert(bestSeen);
		if (bestSeen->getCommandSize() > 2) { // maybe we need another producer of this kind if possible!
			if (aiInterface->reqsOk(bestSeen->getType())) { 
				if (ai->getCountOfClass(UnitClass::BUILDING) > 5) {
					ai->addTask(new BuildTask(bestSeen->getType()));
				}
			}
			// need to calculte another producer, maybe its better to produce another warrior with another producer 
			l_producers.clear();
			CmdByUtMap l_commandMap2;
			findProducerTypes(l_commandMap2);
			foreach_const (Units, it, units) {
				if (l_commandMap2.find((*it)->getType()) != l_commandMap2.end()) {
					l_producers.push_back(*it);
				}
			}
			if (!l_producers.empty()) { // a good producer is found, lets choose a warrior production
				findLowestCommandQueue(l_producers, &bestSeen);
				assert(bestSeen);
				const UnitType *ut = bestSeen->getType();
				vector<const CommandType *> usableCommands;
				for (int j=0; j < ut->getCommandTypeCount(); ++j) {
					const CommandType *ct = ut->getCommandType(j);
					if (ct->getClass() == CommandClass::PRODUCE) { // if the command is produce
						const UnitType *unitType= static_cast<const UnitType*>(ct->getProduced());								
						if (unitType->hasSkillClass(SkillClass::ATTACK) 
						&& !unitType->hasSkillClass(SkillClass::HARVEST) 
						&& aiInterface->reqsOk(ct)) { //this can produce a warrior
							usableCommands.push_back(ct);
						} 
					}
				}
				const CommandType *ct = usableCommands[RAND_RANGE(0, usableCommands.size() - 1)];
				aiInterface->giveCommand(bestSeen, l_commandMap2[bestSeen->getType()]);

			} else { // do it like normal CPU
				aiInterface->giveCommand(bestSeen, l_commandMap[bestSeen->getType()]);
			}
		} else {
			if (bestSeen->getCommandSize() == 0) { // idle, double up
				aiInterface->giveCommand(bestSeen, l_commandMap[bestSeen->getType()]);
			}
			aiInterface->giveCommand(bestSeen, l_commandMap[bestSeen->getType()]);
		}
	}
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
						if (rt || (cost && cost->getAmount() < 0)) {
							utList.push_back(buildingType);
						}
					}
				}
			}
		}
	}
}

void AiRuleBuild::buildGeneric(const BuildTask *bt){
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
		return;
	}
	// build the least built buildingType
	for (int i=0; i < 10; ++i) {
		if (i > 0) {
			//Defensive buildings have priority
			for (int j=0; j < buildingTypes.size(); ++j) {
				const UnitType *buildingType = buildingTypes[j];
				if (ai->getCountOfType(buildingType) <= i + 1 && isDefensive(buildingType)) {
					ai->addTask(new BuildTask(buildingType));
					return;
				}
			}
			// Warrior producers next
			for (int j=0; j < buildingTypes.size(); ++j) {
				const UnitType *buildingType = buildingTypes[j];
				if (ai->getCountOfType(buildingType) <= i + 1 && isWarriorProducer(buildingType)) {
					ai->addTask(new BuildTask(buildingType));
					return;
				}
			}
			//Resource producers next
			for (int j=0; j < buildingTypes.size(); ++j) {
				const UnitType *buildingType = buildingTypes[j];
				if (ai->getCountOfType(buildingType) <= i + 1 && isResourceProducer(buildingType)) {
					ai->addTask(new BuildTask(buildingType));
					return;
				}
			}
		}
		// Any buildingType
		for (int j=0; j < buildingTypes.size(); ++j) {
			const UnitType *buildingType = buildingTypes[j];
			if (ai->getCountOfType(buildingType) <= i) {
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
		return;
	}
	if (!aiInterface->checkCosts(bt->getUnitType())) { // retry if not enough resources
		ai->retryTask(bt);
		return;
	}

	CmdByUtMap cmdMap;
	findBuilderTypes(bt->getUnitType(), cmdMap);
	UnitList potentialBuilders;

	foreach_const (Units, it, units) {
		const Unit *const &unit = *it;
		if (cmdMap.find(unit->getType()) != cmdMap.end()
		&& (!unit->anyCommand() || unit->getCurrCommand()->getType()->getClass() != CommandClass::BUILD)) {
			potentialBuilders.push_back(unit);
		}
	}
	//use random builder to build
	if (potentialBuilders.empty()) {
		return;
	}
	const Unit *unit = potentialBuilders[RAND_RANGE(0, potentialBuilders.size() - 1)];
	Vec2i pos;
	Vec2i searchPos = bt->getForcePos() ? bt->getPos() : ai->getRandomHomePosition();
	// if free pos give command, else retry
	if (ai->findPosForBuilding(bt->getUnitType(), searchPos, pos)) {
		aiInterface->giveCommand(unit, cmdMap[unit->getType()], pos, bt->getUnitType());
	} else {
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
	for(int i= 0; i < building->getCommandTypeCount(); i++){
		const CommandType *ct= building->getCommandType(i);
		if(ct->getClass() == CommandClass::PRODUCE){
			const UnitType *ut= static_cast<const ProduceCommandType*>(ct)->getProducedUnit();

			if(ut->isOfClass(UnitClass::WARRIOR)){
				return true;
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
