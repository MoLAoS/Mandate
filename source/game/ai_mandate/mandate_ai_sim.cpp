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
    topGoal.init("empty", NULL);
    for (int i = 0; i < getPersonalities().size(); ++i) {
        if (personality == getPersonality(i).getPersonalityName()) {
            for (int k = 0; k < getPersonality(i).getGoals().size(); ++k) {
                Focus goal = getPersonality(i).getGoal(k);
                string goalName = goal.getName();
                int goalImportance = goal.getImportance();
                if (goalName == "live") {
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
                if (goalName == "build") {
                    topGoal = goal;
                    return topGoal;
                }
                if (goalName == "collect") {
                    topGoal = goal;
                    return topGoal;
                }
                if (goalName == "explore") {
                    if (topGoal.getImportance() != NULL) {
                        if (goalImportance > topGoal.getImportance()) {
                            topGoal = goal;
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == "shop") {
                    if (topGoal.getImportance() != NULL) {
                        if (goalSystem.findShop(unit) != NULL) {
                            int goldOwned = unit->getSResource(g_world.getTechTree()->getResourceType("gold"))->getAmount();
                            int importanceShop = goldOwned / 100;
                            if (goalImportance + importanceShop > topGoal.getImportance()) {
                                topGoal = goal;
                            }
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == "demolish") {
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
                if (goalName == "raid") {
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
                if (goalName == "hunt") {
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
                            if (distance < 100) {
                                importanceHunt = 100 - distance;
                            }
                            if (goalImportance + importanceHunt > topGoal.getImportance()) {
                                topGoal = goal;
                            }
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == "attack") {
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
                if (goalName == "defend") {
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
                if (goalName == "buff") {
                    if (topGoal.getImportance() != NULL) {
                        if (goalImportance > topGoal.getImportance()) {
                            topGoal = goal;
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == "heal") {
                    if (topGoal.getImportance() != NULL) {
                        if (goalImportance > topGoal.getImportance()) {
                            topGoal = goal;
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == "spell") {
                    if (topGoal.getImportance() != NULL) {
                        if (goalImportance > topGoal.getImportance()) {
                            topGoal = goal;
                        }
                    } else {
                        topGoal = goal;
                    }
                }
                if (goalName == "rest") {
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
    if (reason != "kill") {
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

}}
