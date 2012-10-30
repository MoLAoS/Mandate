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

void GoalSystem::ownerLoad(Unit *unit) {
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
}

void GoalSystem::ownerUnload(Unit *unit) {
    const CommandType *oct = unit->owner->getType()->getFirstCtOfClass(CmdClass::UNLOAD);
    const UnloadCommandType *ouct = static_cast<const UnloadCommandType*>(oct);
    int maxRange = ouct->getUnloadSkillType()->getMaxRange();
    if (g_world.placeUnit(unit->owner->getCenteredPos(), maxRange, unit)) {
        g_map.putUnitCells(unit, unit->getPos());
        unit->setCarried(0);
        unit->owner->getCarriedUnits().erase(std::find(unit->owner->getCarriedUnits().begin(), unit->owner->getCarriedUnits().end(), unit->getId()));
    }
}

void GoalSystem::shop(Unit *unit) {
    Unit *shop = unit->getGoalStructure();
    if (shop != NULL) {
        for (int i = 0; i < unit->getEquipment().size(); ++i) {
            string typeTag = unit->getEquipment()[i].getTypeTag();
            for (int j = 0; j < shop->getItemsStored(); ++j) {
                string tagType = shop->getStoredItem(j)->getType()->getTypeTag();
                if (tagType == typeTag) {
                    if (unit->getEquipment()[i].getCurrent() == 0) {
                        int ident = shop->getStoredItem(j)->id;
                        shop->accessStorageRemove(ident);
                        unit->accessStorageAdd(ident);
                        unit->equipItem(unit->getStoredItems().size()-1);
                    }
                }
            }
        }
        unit->setGoalStructure(NULL);
    }
}

Unit* GoalSystem::findShop(Unit *unit) {
    Faction *f = unit->getFaction();
    vector<Unit*> buildingsList;
    for (int i = 0; i < f->getUnitCount(); ++i) {
        Unit *building = f->getUnit(i);
        if (building->getType()->hasTag("building") && building->getType()->hasTag("shop")) {
            buildingsList.push_back(building);
        }
    }
    int distance = 1000;
    Vec2i uPos = unit->getCenteredPos();
    Unit *finalPick = NULL;
    Vec2i tPos = Vec2i(NULL);
    for (int i = 0; i < buildingsList.size(); ++i) {
        Unit *building = buildingsList[i];
        Vec2i bPos = building->getCenteredPos();
        int newDistance = sqrt(pow(float(abs(uPos.x - bPos.x)), 2) + pow(float(abs(uPos.y - bPos.y)), 2));
        if (newDistance < distance) {
            if (building->getType()->getCreatedItemCount() > 0) {
                for (int j = 0; j < building->getType()->getCreatedItemCount(); ++j) {
                    const ItemType *itemType = building->getType()->getCreatedItem(j, unit->getFaction()).getType();
                    string typeTag = itemType->getTypeTag();
                    bool stored = false;
                    for (int l = 0; l < building->getStorage().size(); ++l) {
                        if (building->getStorage()[l].getTypeTag() == typeTag && building->getStorage()[l].getCurrent() > 0) {
                            stored = true;
                            break;
                        }
                    }
                    if (stored == true) {
                        for (int k = 0; k < unit->getEquipment().size(); ++k) {
                            if (typeTag == unit->getEquipment()[k].getTypeTag()) {
                                if (unit->getEquipment()[k].getCurrent() == 0) {
                                    distance = newDistance;
                                    finalPick = building;
                                    tPos = bPos;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return finalPick;
}

Unit* GoalSystem::findNearbyAlly(Unit* unit, Focus focus) {
    Unit *targetAlly = NULL;
    if (focus.getName() == "heal") {
        Vec2i uPos = unit->getPos();
        int maxDistance = unit->getSight();
        int closest = maxDistance;
        for (int i = 0; i < unit->getFaction()->getUnitCount(); ++i) {
            Unit *target = unit->getFaction()->getUnit(i);
            if (target->getMaxHp() < target->getHp()) {
                Vec2i tPos = target->getPos();
                int distance = sqrt(pow(float(abs(uPos.x - tPos.x)), 2) + pow(float(abs(uPos.y - tPos.y)), 2));
                if (distance < maxDistance) {
                    closest = distance;
                    targetAlly = target;
                }
            }
        }
    }
    return targetAlly;
}

const CommandType* GoalSystem::selectHealSpell(Unit *unit, Unit *target) {
    const CastSpellCommandType *healCommandType = NULL;
    int healthToHeal = target->getMaxHp() - target->getHp();
    for (int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
        const CommandType *testingCommandType = unit->getType()->getCommandType(i);
        const CastSpellCommandType *testCommandType = static_cast<const CastSpellCommandType*>(testingCommandType);
        if (testCommandType->getClass() == CmdClass::CAST_SPELL) {
            const SkillType *testSkillType = testCommandType->getCastSpellSkillType();
            if (testSkillType->hasEffects()) {
                for (int j = 0; j < testSkillType->getEffectTypes().size(); ++j) {
                    if (testSkillType->getEffectTypes()[i]->getHpBoost() > 0) {
                        if (healCommandType == NULL) {
                            healCommandType = testCommandType;
                        } else if (testSkillType->getEffectTypes()[i]->getHpBoost() >
                                   healCommandType->getCastSpellSkillType()->getEffectTypes()[i]->getHpBoost()) {
                            healCommandType = testCommandType;
                            if (healCommandType->getCastSpellSkillType()->getEffectTypes()[i]->getHpBoost() >= healthToHeal) {
                                return healCommandType;
                            }
                        }
                    }
                }
            }
        }
    }
    return healCommandType;
}

const CommandType* GoalSystem::selectBuffSpell(Unit *unit, Unit *target) {
    const CastSpellCommandType *buffCommandType = NULL;
    bool useSpell = true;
    for (int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
        const CommandType *testingCommandType = unit->getType()->getCommandType(i);
        const CastSpellCommandType *testCommandType = static_cast<const CastSpellCommandType*>(testingCommandType);
        if (testCommandType->getClass() == CmdClass::CAST_SPELL) {
            const SkillType *testSkillType = testCommandType->getCastSpellSkillType();
            if (testSkillType->hasEffects()) {
                for (int j = 0; j < target->buffNames.size(); ++j) {
                    if (testCommandType->getName() == target->buffNames[i]) {
                        useSpell = false;
                        break;
                    }
                }
                if (useSpell == true) {
                    buffCommandType = testCommandType;
                    return buffCommandType;
                }
            }
        }
    }
    return buffCommandType;
}

Unit* GoalSystem::findLair(Unit *unit) {
    Vec2i uPos = unit->getPos();
    Faction *faction;
    Unit *lair = NULL;
    for (int i = 0; i < g_world.getFactionCount(); ++i) {
        if (g_world.getFaction(i)->getTeam() != unit->getFaction()->getTeam()) {
            faction = g_world.getFaction(i);
        }
    }
    int distance = 1000;
    for (int i = 0; i < faction->getUnitCount(); ++i) {
        Unit *possibleLair = faction->getUnit(i);
        if (possibleLair->getType()->hasTag("building")) {
            Vec2i bPos = possibleLair->getPos();
            Tile *tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(bPos.x, bPos.y)));
            if(tile->isExplored(faction->getTeam())) {
                int newDistance = sqrt(pow(float(abs(uPos.x - bPos.x)), 2) + pow(float(abs(uPos.y - bPos.y)), 2));
                if (newDistance < distance) {
                    distance = newDistance;
                    lair = possibleLair;
                }
            }
        }
    }
    return lair;
}

Unit* GoalSystem::findCreature(Unit *unit) {
    Vec2i uPos = unit->getPos();
    Faction *faction;
    Unit *creature = NULL;
    for (int i = 0; i < g_world.getFactionCount(); ++i) {
        if (g_world.getFaction(i)->getTeam() != unit->getFaction()->getTeam()) {
            faction = g_world.getFaction(i);
        }
    }
    int distance = 1000;
    for (int i = 0; i < faction->getUnitCount(); ++i) {
        Unit *possibleCreature = faction->getUnit(i);
        if (!possibleCreature->getType()->hasTag("building")) {
            Vec2i bPos = possibleCreature->getPos();
            Tile *tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(bPos.x, bPos.y)));
            if(tile->isExplored(faction->getTeam())) {
                int newDistance = sqrt(pow(float(abs(uPos.x - bPos.x)), 2) + pow(float(abs(uPos.y - bPos.y)), 2));
                if (newDistance < distance) {
                    distance = newDistance;
                    creature = possibleCreature;
                }
            }
        }
    }
    return creature;
}

UnitDirection GoalSystem::newDirection(UnitDirection oldDirection) {
    UnitDirection newUnitDirection;
    srand ( g_world.getFrameCount() );
    int directionChange = rand() % 4 + 1;
    int value = 0;
    int directional = 0;
    switch (oldDirection) {
        case UnitDirection::NORTH: value = 1; break;
        case UnitDirection::SOUTH: value = 2; break;
        case UnitDirection::EAST: value = 3; break;
        case UnitDirection::WEST: value = 4; break;
        case UnitDirection::NORTHWEST: value = 5; break;
        case UnitDirection::NORTHEAST: value = 6; break;
        case UnitDirection::SOUTHWEST: value = 7; break;
        case UnitDirection::SOUTHEAST: value = 8; break;
    }
    switch (value) {
        case 1: directional = 10 + directionChange; break;
        case 2: directional = 20 + directionChange; break;
        case 3: directional = 30 + directionChange; break;
        case 4: directional = 40 + directionChange; break;
        case 5: directional = 50 + directionChange; break;
        case 6: directional = 60 + directionChange; break;
        case 7: directional = 70 + directionChange; break;
        case 8: directional = 80 + directionChange; break;
    }
    switch (directional) {
        case 11: newUnitDirection = UnitDirection::NORTHEAST; break;
        case 12: case 13: newUnitDirection = UnitDirection::NORTH; break;
        case 14: newUnitDirection = UnitDirection::NORTHWEST; break;
        case 21: newUnitDirection = UnitDirection::SOUTHEAST; break;
        case 22: case 23: newUnitDirection = UnitDirection::SOUTH; break;
        case 24: newUnitDirection = UnitDirection::SOUTHWEST; break;
        case 31: newUnitDirection = UnitDirection::NORTHEAST; break;
        case 32: case 33: newUnitDirection = UnitDirection::EAST; break;
        case 34: newUnitDirection = UnitDirection::SOUTHEAST; break;
        case 41: newUnitDirection = UnitDirection::NORTHWEST; break;
        case 42: case 43: newUnitDirection = UnitDirection::WEST; break;
        case 44: newUnitDirection = UnitDirection::SOUTHWEST; break;
        case 51: newUnitDirection = UnitDirection::NORTH; break;
        case 52: case 53: newUnitDirection = UnitDirection::NORTHEAST; break;
        case 54: newUnitDirection = UnitDirection::EAST; break;
        case 61: newUnitDirection = UnitDirection::NORTH; break;
        case 62: case 63: newUnitDirection = UnitDirection::NORTHWEST; break;
        case 64: newUnitDirection = UnitDirection::WEST; break;
        case 71: newUnitDirection = UnitDirection::SOUTH; break;
        case 72: case 73: newUnitDirection = UnitDirection::SOUTHEAST; break;
        case 74: newUnitDirection = UnitDirection::EAST; break;
        case 81: newUnitDirection = UnitDirection::SOUTH; break;
        case 82: case 83: newUnitDirection = UnitDirection::SOUTHWEST; break;
        case 84: newUnitDirection = UnitDirection::WEST; break;
    }
    return newUnitDirection;
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
        ownerLoad(unit);
    } else if (goal == "shop") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
        }
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            Unit *finalPick = findShop(unit);
            Vec2i tPos = Vec2i(NULL);
            if (finalPick != NULL) {
                tPos = finalPick->getPos();
            }
            const CommandType *ct = unit->getType()->getFirstCtOfClass(CmdClass::MOVE);
            if (tPos != Vec2i(NULL)) {
                    if (unit->isCarried()) {
                        ownerUnload(unit);
                    } else {
                        unit->setGoalStructure(finalPick);
                        unit->giveCommand(g_world.newCommand(ct, CmdFlags(), tPos));
                    }
            } else {
                ownerLoad(unit);
            }
        }
    } else if (goal == "attack") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
        }
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            if (unit->attackers.size() > 0) {
                const CommandType *ct = unit->getType()->getFirstCtOfClass(CmdClass::ATTACK);
                const AttackCommandType *act = static_cast<const AttackCommandType*>(ct);
                unit->giveCommand(g_world.newCommand(act, CmdFlags(), unit->attackers[0].getUnit()));
            }
        }
    } else if (goal == "defend") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
        }
        if (unit->owner->getId() != unit->getId()) {
            if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
                Vec2i uPos = unit->getPos();
                Vec2i tPos = unit->owner->getPos();
                int distance = sqrt(pow(float(abs(uPos.x - tPos.x)), 2) + pow(float(abs(uPos.y - tPos.y)), 2));
                if (unit->owner->attackers.size() > 0) {
                    if (distance < 100 + unit->owner->attackers.size() * 10) {
                        const CommandType *ct = unit->getType()->getFirstCtOfClass(CmdClass::ATTACK);
                        const AttackCommandType *act = static_cast<const AttackCommandType*>(ct);
                        unit->giveCommand(g_world.newCommand(act, CmdFlags(), unit->owner->attackers[0].getUnit()));
                    }
                }
            }
        }
    } else if (goal == "hunt") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setGoalStructure(NULL);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
        }
        if (unit->getGoalStructure() != NULL && unit->getCurrSkill()->getClass() != SkillClass::ATTACK) {
            Vec2i uPos = unit->getPos();
            Vec2i tPos = unit->getGoalStructure()->getPos();
            int distance = sqrt(pow(float(abs(uPos.x - tPos.x)), 2) + pow(float(abs(uPos.y - tPos.y)), 2));
            if (distance < 21) {
                unit->finishCommand();
                const CommandType *ct = unit->getType()->getFirstCtOfClass(CmdClass::ATTACK);
                const AttackCommandType *act = static_cast<const AttackCommandType*>(ct);
                unit->giveCommand(g_world.newCommand(act, CmdFlags(), unit->getGoalStructure()));
            }
        }
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP && unit->getGoalStructure() == NULL) {
            Vec2i tPos = Vec2i(NULL);
            Unit *creature = findCreature(unit);
            if (creature != NULL) {
                tPos = creature->getPos();
            }
            if (tPos != Vec2i(NULL)) {
                unit->setGoalStructure(creature);
                const CommandType *ct = unit->getType()->getFirstCtOfClass(CmdClass::MOVE);
                unit->giveCommand(g_world.newCommand(ct, CmdFlags(), tPos));
            }
        }
    } else if (goal == "raid") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setGoalStructure(NULL);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
        }
        if (unit->getGoalStructure() != NULL && unit->getCurrSkill()->getClass() != SkillClass::ATTACK) {
            Vec2i uPos = unit->getPos();
            Vec2i tPos = unit->getGoalStructure()->getPos();
            int distance = sqrt(pow(float(abs(uPos.x - tPos.x)), 2) + pow(float(abs(uPos.y - tPos.y)), 2));
            if (distance < 21) {
                unit->finishCommand();
                const CommandType *ct = unit->getType()->getFirstCtOfClass(CmdClass::ATTACK);
                const AttackCommandType *act = static_cast<const AttackCommandType*>(ct);
                unit->giveCommand(g_world.newCommand(act, CmdFlags(), unit->getGoalStructure()));
            }
        }
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP && unit->getGoalStructure() == NULL) {
            Vec2i tPos = Vec2i(NULL);
            Unit *lair = findLair(unit);
            if (lair != NULL) {
                tPos = lair->getPos();
            }
            if (tPos != Vec2i(NULL)) {
                unit->setGoalStructure(lair);
                Vec2i uPos = unit->getPos();
                const CommandType *ct = unit->getType()->getFirstCtOfClass(CmdClass::MOVE);
                unit->giveCommand(g_world.newCommand(ct, CmdFlags(), tPos));
            }
        }
    } else if (goal == "explore") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
        }
        Vec2i unitPos = unit->getCenteredPos();
        Vec2i pos = unitPos;
        Vec2i position = exploredMap.findNearestUnexploredTile(unitPos, unit->getFaction(), unit->previousDirection);
        pos = position;
        const CommandType *ct = unit->getType()->getFirstCtOfClass(CmdClass::MOVE);
        if (pos != unitPos && pos != Vec2i(NULL)) {
            unit->giveCommand(g_world.newCommand(ct, CmdFlags(), pos));
            unit->previousDirection = newDirection(unit->previousDirection);
        } else {

        }
    } else if (goal == "build") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
        }

        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
        Faction *f = unit->getFaction();
        vector<Unit*> buildingsList;
        for (int i = 0; i < f->getUnitCount(); ++i) {
            Unit *building = f->getUnit(i);
            if (building->getType()->hasTag("building")) {
                if (!building->isBuilt()) {
                    buildingsList.push_back(building);
                }
            }
        }
        int distance = 1000;
        Vec2i uPos = unit->getCenteredPos();
        Unit *finalPick;
        Vec2i tPos = Vec2i(NULL);
        for (int i = 0; i < buildingsList.size(); ++i) {
            Unit *building = buildingsList[i];
            Vec2i bPos = building->getCenteredPos();
            int newDistance = sqrt(pow(float(abs(uPos.x - bPos.x)), 2) + pow(float(abs(uPos.y - bPos.y)), 2));
            if (newDistance < distance) {
                distance = newDistance;
                finalPick = building;
                tPos = bPos;
            }
        }
        const CommandType *ct = unit->getType()->getFirstCtOfClass(CmdClass::REPAIR);
        if (tPos != Vec2i(NULL)) {
            if (unit->isCarried()) {
                ownerUnload(unit);
            } else {
                unit->giveCommand(g_world.newCommand(ct, CmdFlags(), finalPick));
            }
        } else {
            ownerLoad(unit);
        }
        }
    } else if (goal == "buff") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
        }
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            Unit *buffTarget = NULL;
            buffTarget = findNearbyAlly(unit, focus);
            if (buffTarget != NULL) {
                const CommandType *buffingSpell = selectBuffSpell(unit, buffTarget);
                unit->giveCommand(g_world.newCommand(buffingSpell, CmdFlags(), buffTarget));
            }
        }
    } else if (goal == "heal") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
        }
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            Unit *healTarget = NULL;
            healTarget = findNearbyAlly(unit, focus);
            if (healTarget != NULL) {
                const CommandType *healingSpell = selectHealSpell(unit, healTarget);
                unit->giveCommand(g_world.newCommand(healingSpell, CmdFlags(), healTarget));
            }
        }
    } else if (goal == "spell") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
        }

    } else if (goal == "collect") {
        if (goal != unit->getCurrentFocus()) {
            unit->setCurrentFocus(goal);
            unit->setCurrSkill(SkillClass::STOP);
            unit->finishCommand();
            unit->productionRoute.setStoreId(unit->owner->getId());
            unit->productionRoute.setDestination(unit->owner->getPos());
        }
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            Faction *f = unit->getFaction();
            vector<Unit*> buildingsList;
            for (int i = 0; i < f->getUnitCount(); ++i) {
                Unit *building = f->getUnit(i);
                if (building->getType()->hasTag("building") && !building->getType()->hasTag("fort") && building->getId() != unit->owner->getId()) {
                    if (building->sresources.size() > 0) {
                        for (int j = 0; j < building->sresources.size(); ++j) {
                            if (building->getSResource(j)->getType()->getName() == "gold") {
                                if (building->getSResource(j)->getAmount() >= 500) {
                                    buildingsList.push_back(building);
                                }
                            }
                        }
                    }
                }
            }
            int distance = 1000;
            Vec2i uPos = unit->getCenteredPos();
            Unit *finalPick;
            Vec2i tPos = Vec2i(NULL);
            for (int i = 0; i < buildingsList.size(); ++i) {
                Unit *building = buildingsList[i];
                Vec2i bPos = building->getCenteredPos();
                int newDistance = sqrt(pow(float(abs(uPos.x - bPos.x)), 2) + pow(float(abs(uPos.y - bPos.y)), 2));
                if (newDistance < distance) {
                    distance = newDistance;
                    finalPick = building;
                    tPos = bPos;
                }
            }
            const CommandType *tct = unit->getType()->getFirstCtOfClass(CmdClass::TRANSPORT);
            if (tPos != Vec2i(NULL)) {
                if (unit->isCarried()) {
                    ownerUnload(unit);
                } else {
                    if (unit->productionRoute.getProducerId() == 0 || unit->productionRoute.getProducerId() == -1) {
                        unit->productionRoute.setProducerId(finalPick->getId());
                        unit->productionRoute.setDestination(finalPick->getPos());
                    }
                    if (unit->productionRoute.getProducerId() != -1 && unit->productionRoute.getStoreId() != -1) {
                        unit->giveCommand(g_world.newCommand(tct, CmdFlags(), finalPick));
                    }
                }
            } else {
                const StoredResource *res;
                for (int i = 0; i < unit->getType()->getStoredResourceCount(); ++i) {
                    res = unit->getSResource(i);
                    if (res->getType()->getName() == "gold") {
                        break;
                    }
                }
                if (res->getAmount() == 0) {
                    ownerLoad(unit);
                }
            }
        }
    } else {

    }
}

}}
