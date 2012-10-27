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
    for (int i = 0; i < getPersonalities().size(); ++i) {
        if (personality == getPersonality(i).getPersonalityName()) {
            for (int k = 0; k < getPersonality(i).getGoals().size(); ++k) {
                if (getPersonality(i).getGoal(k).getName() == "live") {
                    if (unit->getHp() <= (unit->getMaxHp() / 2)) {
                        topGoal = getPersonality(i).getGoal(k);
                        return topGoal;
                    } else if (unit->getHp() < unit->getMaxHp() && unit->isCarried()) {
                        topGoal = getPersonality(i).getGoal(k);
                        return topGoal;
                    }
                }
                if (getPersonality(i).getGoal(k).getName() == "explore") {
                    topGoal = getPersonality(i).getGoal(k);
                    return topGoal;
                }
            }
        }
    }
}

void MandateAISim::computeAction(Unit *unit, string personality, SkillClass currentAction) {
    Focus newFocus = getTopGoal(unit, personality);
    if (currentAction == SkillClass::STOP) {
        goalSystem.computeAction(unit, newFocus);
    } else if (currentAction == SkillClass::MOVE) {
        //goalSystem.computeAction(unit, newFocus);
    } else {
        goalSystem.computeAction(unit, newFocus);
    }
}

void MandateAISim::update() {
    for (int i = 0; i < faction->getUnitCount(); ++i) {
        Unit *unit = faction->getUnit(i);
        string personality = unit->getType()->personality;
        if (unit->getType()->inhuman) {
            if (unit->isCarried() && unit->getCurrentFocus() == "live") {
                int heal = unit->getMaxHp() / 10 / 40;
                unit->repair(heal, 1);
            }
            if (unit->isCarried() && unit->getCurrentFocus() == "live" && unit->getHp() == unit->getMaxHp()) {
                const CommandType *oct = unit->owner->getType()->getFirstCtOfClass(CmdClass::UNLOAD);
                const UnloadCommandType *ouct = static_cast<const UnloadCommandType*>(oct);
                int maxRange = ouct->getUnloadSkillType()->getMaxRange();
                if (g_world.placeUnit(unit->owner->getCenteredPos(), maxRange, unit)) {
                    g_map.putUnitCells(unit, unit->getPos());
                    unit->setCarried(0);
                    unit->owner->getCarriedUnits().erase(std::find(unit->owner->getCarriedUnits().begin(), unit->owner->getCarriedUnits().end(), unit->getId()));
                }
            }
            if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
                computeAction(unit, personality, SkillClass::STOP);
            }
            else if (unit->getCurrSkill()->getClass() == SkillClass::MOVE) {
                computeAction(unit, personality, SkillClass::MOVE);
            } /*else if (unit->getCurrCommand()->getType()->getClass() == SkillClass::ATTACK) {
                computeAction(unit, personality, SkillClass::ATTACK);
            } else {

            }*/
        }
    }

}

}}
