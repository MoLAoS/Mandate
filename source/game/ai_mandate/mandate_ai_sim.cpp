// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "mandate_ai_sim.h"

#include "faction.h"
#include "world.h"

using namespace Glest::Sim;
using namespace Glest::Entities;

namespace Glest { namespace Plan {
// ===============================
// 	class MandateAISim
//
/**< for sim style player units */
//
// ===============================
void MandateAISim::init(World *newWorld, Faction *newFaction) {
    faction = newFaction;
    world = newWorld;
    goalSystem.init();
    personalities.resize(faction->getType()->getPersonalities().size());
    for (int i = 0; i < faction->getType()->getPersonalities().size(); ++i) {
        personalities.push_back(faction->getType()->getPersonalities()[i]);
    }
}

Focus MandateAISim::getTopGoal(Unit *unit, string personality) {
    Focus topGoal;
    topGoal.init(Goal::EMPTY, NULL);
    for (int i = 0; i < getPersonalities().size(); ++i) {
        if (personality == getPersonality(i).getPersonalityName()) {
            for (int k = 0; k < getPersonality(i).getGoals().size(); ++k) {
                Focus goal = getPersonality(i).getGoal(k);
                Goal goalName = goal.getName();
                int goalImportance = goal.getImportance();
                if (goalName == Goal::LIVE) {
                    int importanceLive = goal.getImportance();
                    if (importanceLive > 100) {
                        importanceLive = 100;
                    }
                    int healthModifier = importanceLive;
                    if (unit->getHp() <= (unit->getMaxHp() * (healthModifier / 100))) {
                        topGoal = goal;
                        return topGoal;
                    } else if (unit->getHp() < unit->getMaxHp() && unit->isCarried()) {
                        topGoal = goal;
                        return topGoal;
                    }
                }
                if (goalName == Goal::BUILD) {
                    if (goalSystem.findBuilding(unit) != NULL) {
                        topGoal = goal;
                        return topGoal;
                    }
                }
                if (goalName == Goal::COLLECT) {
                    if (topGoal.getImportance() != NULL) {
                        if (goalImportance > topGoal.getImportance()) {
                            topGoal = goal;
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == Goal::TRANSPORT) {
                    Unit *producer = NULL;
                    producer = goalSystem.findProducer(unit);
                    if (producer != NULL) {
                        topGoal = goal;
                    }
                }
                if (goalName == Goal::TRADE) {
                    const ResourceType *rt = NULL;
                    int minWealth = 0;
                    for (int j = 0; j < unit->owner->getType()->getResourceStores().size(); ++j) {
                        if (unit->owner->getType()->getResourceStores()[j].getType()->getName() == "wealth") {
                            minWealth = unit->owner->getType()->getResourceStores()[j].getAmount();
                            rt = unit->owner->getType()->getResourceStores()[j].getType();
                        }
                    }
                    int freeWealth = unit->owner->getSResource(rt)->getAmount() - minWealth;
                    if (unit->owner->getType()->hasTag("fort")) {
                        freeWealth = unit->owner->getFaction()->getSResource(rt)->getAmount() - minWealth;
                    }
                    if (freeWealth > 50) {
                        Unit *producer = NULL;
                        producer = goalSystem.findGuild(unit);
                        if (producer != NULL) {
                            topGoal = goal;
                        }
                    }
                }
                if (goalName == Goal::EXPLORE) {
                    if (topGoal.getImportance() != NULL) {
                        if (goalImportance > topGoal.getImportance()) {
                            topGoal = goal;
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == Goal::SHOP) {
                    if (topGoal.getImportance() != NULL) {
                        if (goalSystem.findShop(unit) != NULL) {
                            int goldOwned = unit->getSResource(g_world.getTechTree()->getResourceType("wealth"))->getAmount();
                            int importanceShop = goldOwned / 100;
                            if (goalImportance + importanceShop > topGoal.getImportance()) {
                                topGoal = goal;
                            }
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == Goal::DEMOLISH) {
                    if (topGoal.getImportance() != NULL) {
                        int importanceRaid = 0;
                        Vec2i tPos = Vec2i(NULL);
                        Unit *lair = goalSystem.findLair(unit);
                        if (lair != NULL) {
                            tPos = lair->getPos();
                        }
                        if (tPos != Vec2i(NULL)) {
                            Vec2i uPos = unit->getPos();
                            int distance = sqrt(pow(float(abs(uPos.x - tPos.x)), 2) + pow(float(abs(uPos.y - tPos.y)), 2));
                            if (distance < 100) {
                                importanceRaid = 100 - distance;
                            }
                            if (goalImportance + importanceRaid > topGoal.getImportance()) {
                                topGoal = goal;
                            }
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == Goal::RAID) {
                    if (topGoal.getImportance() != NULL) {
                        int importanceRaid = 0;
                        Vec2i tPos = Vec2i(NULL);
                        Unit *lair = goalSystem.findLair(unit);
                        if (lair != NULL) {
                            tPos = lair->getPos();
                        }
                        if (tPos != Vec2i(NULL)) {
                            Vec2i uPos = unit->getPos();
                            int distance = sqrt(pow(float(abs(uPos.x - tPos.x)), 2) + pow(float(abs(uPos.y - tPos.y)), 2));
                            if (distance < 50) {
                                importanceRaid = 50 - distance;
                            }
                            if (goalImportance + importanceRaid > topGoal.getImportance()) {
                                topGoal = goal;
                            }
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == Goal::HUNT) {
                    if (topGoal.getImportance() != NULL) {
                        int importanceHunt = 0;
                        Vec2i tPos = Vec2i(NULL);
                        Unit *creature = goalSystem.findCreature(unit);
                        if (creature != NULL) {
                            tPos = creature->getPos();
                        }
                        if (tPos != Vec2i(NULL)) {
                            Vec2i uPos = unit->getPos();
                            int distance = sqrt(pow(float(abs(uPos.x - tPos.x)), 2) + pow(float(abs(uPos.y - tPos.y)), 2));
                            if (distance < 50) {
                                importanceHunt = 50 - distance;
                            }
                            if (goalImportance + importanceHunt > topGoal.getImportance()) {
                                topGoal = goal;
                            }
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == Goal::ATTACK) {
                    if (unit->attackers.size() > 0) {
                        if (topGoal.getImportance() != NULL) {
                            int importanceAttack = unit->attackers.size() * 5;
                            if (goalImportance + importanceAttack > topGoal.getImportance() && unit->attackers.size() > 0) {
                                topGoal = goal;
                            }
                        } else {
                            topGoal = goal;
                        }
                    }
                }
                if (goalName == Goal::DEFEND) {
                    if (unit->owner->attackers.size() > 0) {
                        if (topGoal.getImportance() != NULL) {
                            Vec2i uPos = unit->getPos();
                            if (unit->isCarried()) {
                                uPos = unit->owner->getPos();
                            }
                            Vec2i tPos = unit->owner->getPos();
                            int distance = sqrt(pow(float(abs(uPos.x - tPos.x)), 2) + pow(float(abs(uPos.y - tPos.y)), 2));
                            int importanceDefend = unit->owner->attackers.size() * 10;
                            if (distance < 100 + importanceDefend) {
                                if (goalImportance + importanceDefend > topGoal.getImportance()) {
                                    topGoal = goal;
                                }
                            }
                        } else {
                            topGoal = goal;
                        }
                    }
                }
                if (goalName == Goal::BUFF) {
                    if (topGoal.getImportance() != NULL) {
                        if (goalImportance > topGoal.getImportance()) {
                            topGoal = goal;
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == Goal::HEAL) {
                    if (topGoal.getImportance() != NULL) {
                        if (goalImportance > topGoal.getImportance()) {
                            topGoal = goal;
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == Goal::SPELL) {
                    if (topGoal.getImportance() != NULL) {
                        if (goalImportance > topGoal.getImportance()) {
                            topGoal = goal;
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == Goal::REST) {
                    if (topGoal.getImportance() != NULL) {
                        if (goalImportance > topGoal.getImportance()) {
                            topGoal = goal;
                        }
                    } else {
                        topGoal = goal;
                    }
                }
            }
        }
    }
    return topGoal;
}

void MandateAISim::computeAction(Unit *unit, string personality, string reason) {
    if (reason == "compute") {
        Focus newFocus = getTopGoal(unit, personality);
        goalSystem.computeAction(unit, newFocus);
    } else if (reason == "kill") {
        if (unit->getGoalStructure() != NULL) {
            if (unit->anyCommand()) {
                if (unit->getCurrCommand()->getType()->getClass() != CmdClass::ATTACK) {
                    const CommandType *act = goalSystem.selectAttackSpell(unit, unit->getGoalStructure());
                    unit->giveCommand(g_world.newCommand(act, CmdFlags(), unit->getGoalStructure()));
                }
            }
        }
    } else if (reason == "collect") {
        if (unit->getGoalStructure() != NULL) {
            Vec2i posUnit = unit->getPos();
            Vec2i posGoal = unit->getGoalStructure()->getPos();
            int distance = sqrt(pow(float(abs(posUnit.x - posGoal.x)), 2) + pow(float(abs(posUnit.y - posGoal.y)), 2));
            if (distance < 3) {
                for (int i = 0; i < unit->getType()->getResourceProductionSystem().getStoredResourceCount(); ++i) {
                    const ResourceType *rt = unit->getType()->getResourceProductionSystem().getStoredResource(i, unit->getFaction()).getType();
                    int taxes = 0;
                    if (unit->getGoalStructure()->getType()->hasTag("orderhouse") || unit->getGoalStructure()->getType()->hasTag("guildhall")
                         || unit->getGoalStructure()->getType()->hasTag("shop") || unit->getGoalStructure()->getType()->hasTag("producer")
                         || unit->getGoalStructure()->getType()->hasTag("house")) {
                        taxes = (unit->getGoalStructure()->getSResource(rt)->getAmount() -
                                 unit->getGoalStructure()->taxedGold) / (100 / unit->getGoalStructure()->taxRate);
                    } else {
                        taxes = unit->getGoalStructure()->getSResource(rt)->getAmount();
                    }
                    unit->incResourceAmount(rt, taxes);
                    unit->getGoalStructure()->incResourceAmount(rt, -taxes);
                    if (unit->getGoalStructure()->getType()->hasTag("orderhouse") || unit->getGoalStructure()->getType()->hasTag("guildhall")
                         || unit->getGoalStructure()->getType()->hasTag("house") || unit->getGoalStructure()->getType()->hasTag("shop")
                         || unit->getGoalStructure()->getType()->hasTag("producer")) {
                        unit->getGoalStructure()->taxedGold = unit->getGoalStructure()->getSResource(rt)->getAmount();
                    }
                }
                unit->setGoalReason("deliver");
                unit->setGoalStructure(unit->owner);
                const CommandType *ct = unit->getType()->getFirstCtOfClass(CmdClass::MOVE);
                unit->giveCommand(g_world.newCommand(ct, CmdFlags(), unit->getGoalStructure()->getPos()));
            }
        }
    } else if (reason == "deliver") {
        if (unit->getGoalStructure() != NULL) {
            Vec2i posUnit = unit->getPos();
            Vec2i posGoal = unit->owner->getPos();
            int distance = sqrt(pow(float(abs(posUnit.x - posGoal.x)), 2) + pow(float(abs(posUnit.y - posGoal.y)), 2));
            if (distance < 3) {
                for (int i = 0; i < unit->getType()->getResourceProductionSystem().getStoredResourceCount(); ++i) {
                    const ResourceType *rt = unit->getType()->getResourceProductionSystem().getStoredResource(i, unit->getFaction()).getType();
                    int taxes = unit->getSResource(rt)->getAmount();
                    unit->incResourceAmount(rt, -taxes);
                    unit->getGoalStructure()->getFaction()->incResourceAmount(rt, taxes);
                }
                unit->productionRoute.setProducerId(-1);
                unit->setGoalReason("compute");
                unit->setGoalStructure(NULL);
            }
        }
    }
}

void MandateAISim::update() {
    for (int i = 0; i < faction->getUnitCount(); ++i) {
        Unit *unit = faction->getUnit(i);
        string personality = unit->getType()->personality;
        string reason = "compute";
        if (unit->getType()->inhuman) {
            if (unit->getGoalStructure() != NULL) {
                if (!unit->getGoalStructure()->isAlive() || unit->getGoalStructure()->isCarried() || unit->getGoalStructure()->isGarrisoned()) {
                    unit->setGoalStructure(NULL);
                    unit->setCurrentFocus("");
                    unit->setGoalReason("compute");
                    if (unit->anyCommand()) {
                        if (unit->getCurrCommand()->getType()->getClass() == CmdClass::ATTACK) {
                            unit->finishCommand();
                        }
                    }
                }
            }
            int minHp = fixed(unit->getMaxHp() * unit->getType()->live / 100).intp();
            if (unit->getType()->hasTag("ordermember") && unit->getHp() <= minHp && !unit->isCarried()) {
                goalSystem.clearSimAi(unit, Goal::LIVE);
                Focus liveFocus;
                liveFocus.init(Goal::LIVE, unit->getType()->live);
                goalSystem.computeAction(unit, liveFocus);
            } else {
                if (unit->isCarried() && unit->getCurrentFocus() == "live") {
                    int heal = unit->getMaxHp() / 10 / 40;
                    unit->repair(heal, 1);
                }
                if (unit->isCarried() && unit->getCurrentFocus() == "live" && unit->getHp() == unit->getMaxHp()) {
                    goalSystem.ownerUnload(unit);
                }
                if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
                    if (unit->getGoalReason() == "kill") {
                        reason = "kill";
                        computeAction(unit, personality, reason);
                    } else if (unit->getGoalReason() == "collect") {
                        reason = "collect";
                        computeAction(unit, personality, reason);
                    } else if (unit->getGoalReason() == "deliver") {
                        reason = "deliver";
                        computeAction(unit, personality, reason);
                    } else {
                        if (unit->currentAiUpdate[0].currentStep == 8) {
                            computeAction(unit, personality, reason);
                            unit->currentAiUpdate[0].currentStep = 0;
                        } else {
                            ++unit->currentAiUpdate[0].currentStep;
                        }
                    }
                }
            }
        }
    }
}

}}
