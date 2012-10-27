// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "mandate_ai_personalities.h"

#include "faction_type.h"
#include "unit.h"
#include "world.h"

namespace Glest { namespace Plan {
// ===============================
// 	class Focus
// ===============================
void Focus::init(string newName, int newImportance) {
    name = newName;
    importance = newImportance;
}

// ===============================
// 	class Personality
// ===============================
void Personality::load(const XmlNode *node, const TechTree* techTree, const FactionType* factionType) {
    personalityName = node->getAttribute("name")->getRestrictedValue();
    goals.resize(node->getChildCount());
    for (int i = 0; i < node->getChildCount(); ++i) {
        const XmlNode *goalNode = node->getChild("goal", i);
        string name = goalNode->getAttribute("name")->getRestrictedValue();
        int importance = goalNode->getAttribute("importance")->getIntValue();
        goals[i].init(name, importance);
    }
}

// ===============================
// 	class Goal System
// ===============================
void GoalSystem::init() {
    goalList.push_back("explore");
    goalList.push_back("fight");
    goalList.push_back("live");
    goalList.push_back("build");
    goalList.push_back("guard");
    goalList.push_back("buff");
    goalList.push_back("heal");
    goalList.push_back("spell");
    goalList.push_back("collect");
}

void GoalSystem::computeAction(Unit *unit, Focus focus) {
    string goal = focus.getName();
    int importance = focus.getImportance();
    if (goal == "live") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
        }
        if (unit->owner->getId() != unit->getId()) {
            const CommandType *oct = unit->owner->getType()->getFirstCtOfClass(CmdClass::LOAD);
            const LoadCommandType *olct = static_cast<const LoadCommandType*>(oct);
            if (unit->getCurrSkill()->getClass() != SkillClass::MOVE) {
                const CommandType *ct = unit->getType()->getFirstCtOfClass(CmdClass::MOVE);
                Vec2i pos = unit->owner->getPos();
                float distanceX = (pos.x - unit->getPos().x) * (pos.x - unit->getPos().x);
                float distanceY = (pos.y - unit->getPos().y) * (pos.y - unit->getPos().y);
                int distance = sqrt(distanceX + distanceY);
                if (!unit->isCarried()) {
                    if (olct->getLoadSkillType()->getMaxRange() < distance) {
                        unit->giveCommand(g_world.newCommand(ct, CmdFlags(), pos));
                    }
                    if (olct->getLoadSkillType()->getMaxRange() >= distance) {
                        unit->owner->giveCommand(g_world.newCommand(olct, CmdFlags(), unit, unit->owner));
                    }
                }
            }
        }
    } else if (goal == "fight") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);

        } else {

        }
    } else if (goal == "explore") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
        }
        Vec2i unitPos = unit->getCenteredPos();
        Vec2i pos = unitPos;
        DirectionalPosition dp = exploredMap.findNearestUnexploredTile(unitPos, unit->getFaction(), exploredMap.getPreviousDirection());
        pos = dp.getPositionResult();
        exploredMap.getPreviousDirection() = dp.getLastDirection();
        const CommandType *ct = unit->getType()->getFirstCtOfClass(CmdClass::MOVE);
        if (pos != unitPos && pos != Vec2i(NULL)) {
            unit->giveCommand(g_world.newCommand(ct, CmdFlags(), pos));
        } else {

        }
    } else if (goal == "build") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);

        } else {

        }
    } else if (goal == "guard") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);

        } else {

        }
    } else if (goal == "buff") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);

        } else {

        }
    } else if (goal == "heal") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);

        } else {

        }
    } else if (goal == "spell") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);

        } else {

        }
    } else if (goal == "collect") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);

        } else {

        }
    } else {

    }
}

}}
