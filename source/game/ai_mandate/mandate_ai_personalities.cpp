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
void Focus::init(Goal newName, int newImportance) {
    name = newName;
    importance = newImportance;
}

// ===============================
// 	class Personality
// ===============================
void Personality::load(const XmlNode *node) {
    GoalList goalList;
    goalList.push_back("empty");
    goalList.push_back("live");
    goalList.push_back("build");
    goalList.push_back("collect");
    goalList.push_back("transport");
    goalList.push_back("trade");
    goalList.push_back("explore");
    goalList.push_back("shop");
    goalList.push_back("demolish");
    goalList.push_back("raid");
    goalList.push_back("hunt");
    goalList.push_back("patrol");
    goalList.push_back("attack");
    goalList.push_back("defend");
    goalList.push_back("buff");
    goalList.push_back("heal");
    goalList.push_back("spell");
    goalList.push_back("rest");
    personalityName = node->getAttribute("name")->getRestrictedValue();
    goals.resize(node->getChildCount());
    for (int i = 0; i < node->getChildCount(); ++i) {
        const XmlNode *goalNode = node->getChild("goal", i);
        string name = goalNode->getAttribute("name")->getRestrictedValue();
        Goal newGoal = Goal::EMPTY;
        for (int j = 0; j < goalList.size(); ++j) {
            if (name == goalList[j]) {
                break;
            } else {
                ++newGoal;
            }
        }
        int importance = goalNode->getAttribute("importance")->getIntValue();
        goals[i].init(newGoal, importance);
    }
}

// ===============================
// 	class Goal System
// ===============================
void GoalSystem::init() {
    goalList.push_back("empty");
    goalList.push_back("live");
    goalList.push_back("build");
    goalList.push_back("collect");
    goalList.push_back("transport");
    goalList.push_back("trade");
    goalList.push_back("explore");
    goalList.push_back("shop");
    goalList.push_back("demolish");
    goalList.push_back("raid");
    goalList.push_back("hunt");
    goalList.push_back("patrol");
    goalList.push_back("attack");
    goalList.push_back("defend");
    goalList.push_back("buff");
    goalList.push_back("heal");
    goalList.push_back("spell");
    goalList.push_back("rest");
}

void GoalSystem::ownerLoad(Unit *unit) {
    if (unit->owner->getId() != unit->getId() && unit->owner->getType()->getActions()->hasCommandClass(CmdClass::LOAD)) {
        const CommandType *oct = unit->owner->getType()->getActions()->getFirstCtOfClass(CmdClass::LOAD);
        const LoadCommandType *olct = static_cast<const LoadCommandType*>(oct);
        if (unit->getCurrSkill()->getClass() != SkillClass::MOVE) {
            const CommandType *ct = unit->getType()->getActions()->getFirstCtOfClass(CmdClass::MOVE);
            Vec2i pos = unit->owner->getPos();
            float distanceX = (pos.x - unit->getPos().x) * (pos.x - unit->getPos().x);
            float distanceY = (pos.y - unit->getPos().y) * (pos.y - unit->getPos().y);
            int distance = sqrt(distanceX + distanceY);
            if (!unit->isCarried()) {
                if (olct->getLoadSkillType()->getMaxRange() < distance) {
                    unit->giveCommand(g_world.newCommand(ct, CmdFlags(), pos));
                }
                Unit *home = unit->owner;
                if (!unit->getType()->hasTag("ordermember")) {
                    for (int i = 0; i < unit->getType()->getResourceProductionSystem()->getStoredResourceCount(); ++i) {
                        const ResourceType *rt = unit->getType()->getResourceProductionSystem()->getStoredResource(i, unit->getFaction()).getType();
                        int carried = unit->getSResource(rt)->getAmount();
                        if (carried > 0) {
                            unit->incResourceAmount(rt, -carried);
                            if (!home->getType()->hasTag("fort")) {
                                home->incResourceAmount(rt, carried);
                            } else if (home->getType()->hasTag("fort")) {
                                home->getFaction()->incResourceAmount(rt, carried);
                            }
                        }
                    }
                }
                if (olct->getLoadSkillType()->getMaxRange() >= distance && unit->garrisonTest == false) {
                    unit->garrisonTest = true;
                    unit->owner->giveCommand(g_world.newCommand(olct, CmdFlags(), unit, unit->owner));
                }
            }
        }
    }
}

void GoalSystem::ownerUnload(Unit *unit) {
    const CommandType *oct = unit->owner->getType()->getActions()->getFirstCtOfClass(CmdClass::UNLOAD);
    const UnloadCommandType *ouct = static_cast<const UnloadCommandType*>(oct);
    int maxRange = ouct->getUnloadSkillType()->getMaxRange();
    if (g_world.placeUnit(unit->owner->getCenteredPos(), maxRange, unit)) {
        g_map.putUnitCells(unit, unit->getPos());
        unit->setCarried(0);
        unit->owner->getCarriedUnits().erase(std::find(unit->owner->getCarriedUnits().begin(), unit->owner->getCarriedUnits().end(), unit->getId()));
        unit->garrisonTest = false;
    }
}

void GoalSystem::shop(Unit *unit) {
    Unit *shop = unit->getGoalStructure();
    if (shop != NULL) {
        for (int i = 0; i < unit->getEquipmentSize(); ++i) {
            string typeTag = unit->getEquipment(i)->getTypeTag();
            for (int j = 0; j < shop->getItemsStored(); ++j) {
                string tagType = shop->getStoredItem(j)->getType()->getTypeTag();
                int qualityTier = shop->getStoredItem(j)->getType()->getQualityTier();
                if (tagType == typeTag) {
                    if (unit->getEquipment(i)->getCurrent() == 0 || qualityTier > unit->getEquipment(i)->getItem()->getType()->getQualityTier()) {
                        bool costs = true;
                        for (int m = 0; m < shop->getStoredItem(j)->getType()->getCostCount(); ++m) {
                            const ResourceType *rt = shop->getStoredItem(j)->getType()->getCost(m, unit->getFaction()).getType();
                            int cost = shop->getStoredItem(j)->getType()->getCost(m, unit->getFaction()).getAmount();
                            int gold = unit->getSResource(rt)->getAmount();
                            if (gold < cost) {
                                costs = false;
                                break;
                            }
                        }
                        if (costs == true) {
                            for (int m = 0; m < shop->getStoredItem(j)->getType()->getCostCount(); ++m) {
                                const ResourceType *rt = shop->getStoredItem(j)->getType()->getCost(m, unit->getFaction()).getType();
                                int cost = shop->getStoredItem(j)->getType()->getCost(m, unit->getFaction()).getAmount();
                                unit->incResourceAmount(rt, -cost);
                                shop->incResourceAmount(rt, cost);
                            }
                            int ident = shop->getStoredItem(j)->id;
                            shop->accessStorageRemove(ident);
                            unit->accessStorageAdd(ident);
                            if (tagType == "consumable_buff") {
                                unit->consumeItem(unit->getStoredItems().size()-1);
                            } else if (tagType != "consumable" && tagType != "consumable_buff") {
                                unit->equipItem(unit->getStoredItems().size()-1);
                            }
                            break;
                        }
                    }
                }
            }
        }
        for (int i = 0; i < unit->getEquipmentSize(); ++i) {
            string typeTag = unit->getEquipment(i)->getTypeTag();
            for (int j = 0; j < shop->getType()->getModifications().size(); ++j) {
                for (int k = 0; k < shop->getType()->getModifications()[j].getEquipment().size(); ++k) {
                    string tagType = shop->getType()->getModifications()[j].getEquipment(k);
                    if (tagType == typeTag) {
                        bool costs = true;
                        for (int m = 0; m < shop->getType()->getModifications()[j].getCostCount(); ++m) {
                            const ResourceType *rt = shop->getType()->getModifications()[j].getCost(m).getType();
                            int cost = shop->getType()->getModifications()[j].getCost(m).getAmount();
                            int gold = unit->getSResource(rt)->getAmount();
                            if (gold < cost) {
                                costs = false;
                                break;
                            }
                        }
                        if (costs == true) {
                            for (int m = 0; m < shop->getType()->getModifications()[j].getCostCount(); ++m) {
                                const ResourceType *rt = shop->getType()->getModifications()[j].getCost(m).getType();
                                int cost = shop->getType()->getModifications()[j].getCost(m).getAmount();
                                unit->incResourceAmount(rt, -cost);
                                shop->incResourceAmount(rt, cost);
                            }
                            unit->modifications.push_back(shop->getType()->getModifications()[j]);
                        }
                    }
                }
            }
        }
        unit->setGoalStructure(NULL);
    }
}

Unit *GoalSystem::buildingList(Unit *unit, vector<Unit*> buildingsList) {
    Unit *targetBuilding = NULL;
    if (buildingsList.size() > 0) {
        int distance = 50;
        Vec2i uPos = unit->getCenteredPos();
        if (unit->isCarried()) {
            uPos = unit->owner->getCenteredPos();
        }
        Vec2i tPos = Vec2i(0,0);
        for (int i = 0; i < buildingsList.size(); ++i) {
            Unit *building = buildingsList[i];
            Vec2i bPos = building->getCenteredPos();
            int newDistance = sqrt(pow(float(abs(uPos.x - bPos.x)), 2) + pow(float(abs(uPos.y - bPos.y)), 2));
            if (newDistance < distance) {
                distance = newDistance;
                targetBuilding = building;
                tPos = bPos;
            }
        }
    }
    return targetBuilding;
}

Unit* GoalSystem::findShop(Unit *unit) {
    Faction *f = unit->getFaction();
    vector<Unit*> buildingsList;
    for (int i = 0; i < f->getUnitCount(); ++i) {
        Unit *building = f->getUnit(i);
        if (building->getType()->hasTag("shop") || building->getType()->hasTag("enhancer")) {
            buildingsList.push_back(building);
        }
    }
    int distance = 50;
    Vec2i uPos = unit->getCenteredPos();
    if (unit->isCarried()) {
        uPos = unit->owner->getCenteredPos();
    }
    Unit *finalPick = NULL;
    Vec2i tPos = Vec2i(0,0);
    for (int i = 0; i < buildingsList.size(); ++i) {
        Unit *building = buildingsList[i];
        Vec2i bPos = building->getCenteredPos();
        int newDistance = sqrt(pow(float(abs(uPos.x - bPos.x)), 2) + pow(float(abs(uPos.y - bPos.y)), 2));
        if (newDistance < distance) {
            if (building->getStoredItems().size() > 0) {
                for (int i = 0; i < unit->getEquipmentSize(); ++i) {
                    string typeTag = unit->getEquipment(i)->getTypeTag();
                    for (int j = 0; j < building->getStoredItems().size(); ++j) {
                        string tagType = building->getStoredItem(j)->getType()->getTypeTag();
                        int qualityTier = building->getStoredItem(j)->getType()->getQualityTier();
                        if (tagType == typeTag) {
                            if (unit->getEquipment(i)->getCurrent() == 0 || qualityTier > unit->getEquipment(i)->getItem()->getType()->getQualityTier()) {
                                bool costs = true;
                                for (int m = 0; m < building->getStoredItem(j)->getType()->getCostCount(); ++m) {
                                    const ResourceType *rt = building->getStoredItem(j)->getType()->getCost(m, unit->getFaction()).getType();
                                    int cost = building->getStoredItem(j)->getType()->getCost(m, unit->getFaction()).getAmount();
                                    int gold = unit->getSResource(rt)->getAmount();
                                    if (gold < cost) {
                                        costs = false;
                                        break;
                                    }
                                }
                                if (costs == true) {
                                    distance = newDistance;
                                    finalPick = building;
                                    tPos = bPos;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (building->getType()->getModifications().size() > 0) {
                for (int j = 0; j < building->getType()->getModifications().size(); ++j) {
                    string service = building->getType()->getModifications()[j].getService();
                    if (service == "faction" || service == "guild" && building == unit->owner) {
                        for (int k = 0; k < building->getType()->getModifications()[j].getEquipmentSize(); ++k) {
                            for (int l = 0; l < unit->getEquipmentSize(); ++l) {
                                if (unit->getEquipment(l)->getTypeTag() == building->getType()->getModifications()[j].getEquipment(k)) {
                                    bool owned = false;
                                    for (int m = 0; m < unit->modifications.size(); ++m) {
                                        if (unit->modifications[m].getModificationName() == building->getType()->getModifications()[j].getModificationName()) {
                                            owned = true;
                                        }
                                    }
                                    if (owned == false) {
                                        bool costs = true;
                                        for (int m = 0; m < building->getType()->getModifications()[j].getCostCount(); ++m) {
                                            const ResourceType *rt = building->getType()->getModifications()[j].getCost(m).getType();
                                            int cost = building->getType()->getModifications()[j].getCost(m).getAmount();
                                            int gold = unit->getSResource(rt)->getAmount();
                                            if (gold < cost) {
                                                costs = false;
                                                break;
                                            }
                                        }
                                        if (costs == true) {
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
        }
    }
    return finalPick;
}

Unit* GoalSystem::findBuilding(Unit* unit) {
    Unit *targetBuilding = NULL;
    Faction *f = unit->getFaction();
    vector<Unit*> buildingsList;
    for (int i = 0; i < f->getUnitCount(); ++i) {
        Unit *building = f->getUnit(i);
        if (building->getType()->hasTag("building")) {
            if (!building->isBuilt() && !unit->getType()->hasTag("householder")) {
                buildingsList.push_back(building);
            } else if (building->getHp() < building->getStatistics()->getEnhancement()->getResourcePools()->getHealth()->getMaxStat()->getValue()) {
                if (building->getType()->hasTag("house") && unit->getType()->hasTag("householder") && building == unit->owner) {
                    buildingsList.push_back(building);
                } else if (!building->getType()->hasTag("house") && !unit->getType()->hasTag("householder")) {
                    buildingsList.push_back(building);
                }
            }
        }
    }
    targetBuilding = buildingList(unit, buildingsList);
    return targetBuilding;
}

Unit* GoalSystem::findProducer(Unit* unit) {
    Unit *producer = NULL;
    Faction *f = unit->getFaction();
    vector<Unit*> buildingsList;
    for (int i = 0; i < f->getUnitCount(); ++i) {
        Unit *building = f->getUnit(i);
        if (building->getType()->hasTag("building") && building->getType()->hasTag("producer")) {
            for (int k = 0; k < building->getType()->getResourceProductionSystem()->getStoredResourceCount(); ++k) {
                const ResourceType *producedType = building->getType()->getResourceProductionSystem()->getStoredResource(k, building->getFaction()).getType();
                int amount = building->getSResource(producedType)->getAmount();
                for (int n = 0; n < building->getType()->getResourceStoreCount(); ++n) {
                    const ResourceType *resType = building->getType()->getResourceStore(n)->getType();
                    if (resType == producedType) {
                        amount = amount - building->getType()->getResourceStore(n)->getAmount();
                    }
                }
                if (amount > 50 && producedType->getName() != "wealth") {
                    for (int j = 0; j < unit->getType()->getResourceProductionSystem()->getStoredResourceCount(); ++j) {
                        const ResourceType *transportedType = unit->getType()->getResourceProductionSystem()->getStoredResource(j, unit->getFaction()).getType();
                        for (int l = 0; l < unit->owner->getType()->getResourceProductionSystem()->getStoredResourceCount(); ++l) {
                            const ResourceType *storedType = unit->owner->getType()->getResourceProductionSystem()->getStoredResource(l, unit->getFaction()).getType();
                            if (transportedType == storedType && storedType == producedType &&
                                transportedType == producedType) {
                                bool previousTarget = false;
                                for (int z = 0; z < unit->getFaction()->getUnitCount(); ++z) {
                                    if (unit->getFaction()->getUnit(z)->getType()->personality == unit->getType()->personality) {
                                        if (unit->getFaction()->getUnit(z)->getGoalStructure() == building) {
                                            previousTarget = true;
                                        }
                                    }
                                }
                                if (previousTarget == false) {
                                    buildingsList.push_back(building);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    producer = buildingList(unit, buildingsList);
    return producer;
}

Unit* GoalSystem::findGuild(Unit* unit) {
    Unit *producer = NULL;
    Faction *f = unit->getFaction();
    vector<Unit*> buildingsList;
    for (int i = 0; i < f->getUnitCount(); ++i) {
        Unit *building = f->getUnit(i);
        bool buy = false;
        if (unit->owner->getType()->hasTag("house") && building->getType()->hasTag("shop")){
            buy = true;
        }
        if (unit->owner->getType()->hasTag("shop") && building->getType()->hasTag("guildhall")) {
            buy = true;
        }
        if (unit->owner->getType()->hasTag("guildhall") && building->getType()->hasTag("guildhall") && unit->owner != building) {
            buy = true;
        }
        if (unit->owner->getType()->hasTag("guildhall") && building->getType()->hasTag("producer")) {
            buy = true;
        }
        if (buy == true) {
            for (int j = 0; j < building->getType()->getResourceProductionSystem()->getStoredResourceCount(); ++j) {
                const ResourceType *producedType = building->getType()->getResourceProductionSystem()->
                                                   getStoredResource(j, building->getFaction()).getType();
                bool status = false;
                for (int n = 0; n < building->getType()->getResourceStoreCount(); ++n) {
                    if (building->getType()->getResourceStore(n)->getType() == producedType) {
                        if (building->getType()->getResourceStore(n)->getStatus() == "stockpile") {
                            status = true;
                        }
                    }
                }
                if (producedType->getName() != "wealth" && !status) {
                    for (int k = 0; k < unit->getType()->getResourceProductionSystem()->getStoredResourceCount(); ++k) {
                        const ResourceType *transportedType = unit->getType()->getResourceProductionSystem()->
                                                              getStoredResource(k, unit->getFaction()).getType();
                        if (producedType == transportedType) {
                            for (int l = 0; l < unit->owner->getType()->getResourceProductionSystem()->getStoredResourceCount(); ++l) {
                                const ResourceType *storedType = unit->owner->getType()->getResourceProductionSystem()->
                                                                 getStoredResource(l, unit->getFaction()).getType();
                                if (producedType == storedType) {
                                    for (int m = 0; m < unit->owner->getType()->getResourceStoreCount(); ++m) {
                                        const ResourceType *resType = unit->owner->getType()->getResourceStore(m)->getType();
                                        if (producedType == resType) {
                                            int homeStored = unit->owner->getSResource(producedType)->getAmount();
                                            int homeStores = unit->owner->getType()->getResourceStore(m)->getAmount();
                                            if (homeStored < homeStores) {
                                                int available = 0;
                                                int amount = building->getSResource(producedType)->getAmount();
                                                if (!building->getType()->hasTag("shop")) {
                                                    available = amount;
                                                    for (int n = 0; n < building->getType()->getResourceStoreCount(); ++n) {
                                                        if (building->getType()->getResourceStore(n)->getType() == producedType) {
                                                            int free = amount - building->getType()->getResourceStore(n)->getAmount();
                                                            if (free >= 10) {
                                                                available = free;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    available = amount;
                                                }
                                                if (available >= 10) {
                                                    bool previousTarget = false;
                                                    for (int o = 0; o < unit->getFaction()->getUnitCount(); ++o) {
                                                        if (unit->getFaction()->getUnit(o)->getType()->personality == unit->getType()->personality) {
                                                            if (unit->getFaction()->getUnit(o)->getGoalStructure() == building) {
                                                                previousTarget = true;
                                                            }
                                                        }
                                                    }
                                                    if (previousTarget == false) {
                                                        buildingsList.push_back(building);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    producer = buildingList(unit, buildingsList);
    return producer;
}

Unit* GoalSystem::findNearbyAlly(Unit* unit, Focus focus) {
    Unit *targetAlly = NULL;
    if (focus.getName() == Goal::HEAL) {
        Vec2i uPos = unit->getPos();
        if (unit->isCarried()) {
            uPos = unit->owner->getCenteredPos();
        }
        int maxDistance = unit->getStatistics()->getEnhancement()->getUnitStats()->getSight()->getValue();
        int closest = maxDistance;
        for (int i = 0; i < unit->getFaction()->getUnitCount(); ++i) {
            Unit *target = unit->getFaction()->getUnit(i);
            if (target->getStatistics()->getEnhancement()->getResourcePools()->getHealth()->getMaxStat()->getValue() < target->getHp()) {
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
    int healthToHeal = target->getStatistics()->getEnhancement()->getResourcePools()->getHealth()->getMaxStat()->getValue() - target->getHp();
    for (int i = 0; i < unit->getType()->getActions()->getCommandTypeCount(); ++i) {
        const CommandType *testingCommandType = unit->getType()->getActions()->getCommandType(i);
        if (testingCommandType->getClass() == CmdClass::CAST_SPELL) {
            const CastSpellCommandType *testCommandType = static_cast<const CastSpellCommandType*>(testingCommandType);
            const CastSpellSkillType *testSkillType = testCommandType->getCastSpellSkillType();
            if (testSkillType->hasEffects()) {
                for (int j = 0; j < testSkillType->getEffectTypes().size(); ++j) {
                    if (testSkillType->getEffectTypes()[i]->getStatistics()->getEnhancement()->getResourcePools()->getHealth()->getBoostStat()->getValue() > 0) {
                        if (healCommandType == NULL) {
                            healCommandType = testCommandType;
                        } else if (testSkillType->getEffectTypes()[i]->getStatistics()->getEnhancement()->
                                   getResourcePools()->getHealth()->getBoostStat()->getValue() >
                                   healCommandType->getCastSpellSkillType()->getEffectTypes()[i]->getStatistics()->getEnhancement()->
                                   getResourcePools()->getHealth()->getBoostStat()->getValue()) {
                            healCommandType = testCommandType;
                            if (healCommandType->getCastSpellSkillType()->getEffectTypes()[i]->getStatistics()->getEnhancement()->
                                getResourcePools()->getHealth()->getBoostStat()->getValue() >= healthToHeal) {
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
    for (int i = 0; i < unit->getType()->getActions()->getCommandTypeCount(); ++i) {
        const CommandType *testingCommandType = unit->getType()->getActions()->getCommandType(i);
        if (testingCommandType->getClass() == CmdClass::CAST_SPELL) {
            const CastSpellCommandType *testCommandType = static_cast<const CastSpellCommandType*>(testingCommandType);
            const CastSpellSkillType *testSkillType = testCommandType->getCastSpellSkillType();
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

const CommandType* GoalSystem::selectAttackSpell(Unit *unit, Unit *target) {
    const CommandType *attackCommandType = NULL;
    int currentDamage = 0;
    for (int i = 0; i < unit->getType()->getActions()->getCommandTypeCount(); ++i) {
        if (unit->currentCommandCooldowns[i].currentStep == 0) {
            const CommandType *testingCommandType = unit->getType()->getActions()->getCommandType(i);
            if (testingCommandType->getClass() == CmdClass::ATTACK) {
                const AttackCommandType *testCommandType = static_cast<const AttackCommandType*>(testingCommandType);
                const AttackSkillType *testSkillType = testCommandType->AttackCommandTypeBase::getAttackSkillTypes()->getFirstAttackSkill();
                if (unit->checkSkillCosts(testCommandType)) {
                    fixed fDamage = 0;
                    if (fDamage < 1) {
                        fDamage = 1;
                    }
                    fixed totalDamage = 0 + fDamage;
                    const UnitType *uType = target->getType();
                    const AttackLevel *aLevel = testSkillType->getLevel(testSkillType->getCurrentLevel());
                    for (int t = 0; t < aLevel->getStatistics()->getDamageTypeCount(); ++t) {
                        const DamageType *dType = aLevel->getStatistics()->getDamageType(t);
                        string damageType = dType->getTypeName();
                        int mDamage = dType->getValue();
                        for (int i = 0; i < uType->getStatistics()->getResistances()->size(); ++i) {
                            const DamageType *rType = uType->getStatistics()->getResistance(i);
                            string resistType = rType->getTypeName();
                            if (damageType==resistType) {
                                int resist = rType->getValue();
                                mDamage -= resist;
                                if (mDamage < 0) {
                                    mDamage = 0;
                                }
                            }
                        }
                        totalDamage += mDamage;
                    }
                    if (currentDamage < totalDamage.intp()) {
                        currentDamage = totalDamage.intp();
                        attackCommandType = testingCommandType;
                    } else if (currentDamage == totalDamage.intp()){
                        currentDamage = totalDamage.intp();
                        attackCommandType = testingCommandType;
                    }
                }
            }
        }
    }
    return attackCommandType;
}

Unit* GoalSystem::findLair(Unit *unit) {
    Vec2i uPos = unit->getPos();
    if (unit->isCarried()) {
        uPos = unit->owner->getCenteredPos();
    }
    Faction *faction;
    Unit *lair = NULL;
    for (int i = 0; i < g_world.getFactionCount(); ++i) {
        if (g_world.getFaction(i)->getTeam() != unit->getFaction()->getTeam()) {
            faction = g_world.getFaction(i);
        }
    }
    int distance = 50;
    for (int i = 0; i < faction->getUnitCount(); ++i) {
        Unit *possibleLair = faction->getUnit(i);
        if (possibleLair->getType()->hasTag("building") && possibleLair->isAlive()) {
            Vec2i bPos = possibleLair->getPos();
            Tile *tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(bPos.x, bPos.y)));
            if(tile->isExplored(unit->getFaction()->getTeam())) {
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

Unit* GoalSystem::findCity(Unit *unit) {
    Vec2i uPos = unit->getPos();
    if (unit->isCarried()) {
        uPos = unit->owner->getCenteredPos();
    }
    Faction *faction;
    Unit *city = NULL;
    for (int i = 0; i < g_world.getFactionCount(); ++i) {
        if (g_world.getFaction(i)->getTeam() != unit->getFaction()->getTeam()) {
            faction = g_world.getFaction(i);
        }
    }
    int distance = 50;
    for (int i = 0; i < faction->getUnitCount(); ++i) {
        Unit *possibleCity = faction->getUnit(i);
        if (possibleCity->getType()->hasTag("building") && possibleCity->isAlive()) {
            Vec2i bPos = possibleCity->getPos();
            int newDistance = sqrt(pow(float(abs(uPos.x - bPos.x)), 2) + pow(float(abs(uPos.y - bPos.y)), 2));
            if (newDistance < distance) {
                distance = newDistance;
                city = possibleCity;
            }
        }
    }
    return city;
}

Unit* GoalSystem::findCreature(Unit *unit, int range) {
    Vec2i uPos = unit->getPos();
    if (unit->isCarried()) {
        uPos = unit->owner->getCenteredPos();
    }
    Faction *faction;
    Unit *creature = NULL;
    for (int i = 0; i < g_world.getFactionCount(); ++i) {
        if (g_world.getFaction(i)->getTeam() != unit->getFaction()->getTeam()) {
            faction = g_world.getFaction(i);
        }
    }
    int distance = range;
    for (int i = 0; i < faction->getUnitCount(); ++i) {
        Unit *possibleCreature = faction->getUnit(i);
        if (!possibleCreature->getType()->hasTag("building") && possibleCreature->isAlive()) {
            Vec2i bPos = possibleCreature->getPos();
            Tile *tile = g_world.getMap()->getTile(Map::toTileCoords(Vec2i(bPos.x, bPos.y)));
            if(tile->isExplored(unit->getFaction()->getTeam())) {
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

void GoalSystem::clearSimAi(Unit *unit, Goal goal) {
    if (goalList[goal] != unit->getCurrentFocus()) {
        unit->setCurrentFocus(goalList[goal]);
        unit->setGoalStructure(NULL);
        unit->setGoalReason("compute");
        unit->setCurrSkill(SkillClass::STOP);
        unit->finishCommand();
    }
}

void GoalSystem::computeAction(Unit *unit, Focus focus) {
    Goal goal = focus.getName();
    int importance = focus.getImportance();
    if (goal == Goal::LIVE) {
        clearSimAi(unit, goal);
        ownerLoad(unit);
        unit->setGoalReason("live");
    } else if (goal == Goal::BUILD) {
        clearSimAi(unit, goal);
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            Unit *finalPick = findBuilding(unit);
            if (finalPick != NULL) {
                Vec2i tPos = finalPick->getCenteredPos();
                const CommandType *ct = unit->getType()->getActions()->getFirstCtOfClass(CmdClass::REPAIR);
                if (tPos != Vec2i(0,0)) {
                    if (unit->isCarried()) {
                        ownerUnload(unit);
                    } else {
                        unit->giveCommand(g_world.newCommand(ct, CmdFlags(), finalPick));
                    }
                }
            } else if (!unit->isCarried()) {
                ownerLoad(unit);
            }
        }
    } else if (goal == Goal::SHOP) {
        clearSimAi(unit, goal);
        if (unit->getGoalReason() == "shop") {
            Vec2i posUnit = unit->getPos();
            if (unit->isCarried()) {
                posUnit = unit->owner->getCenteredPos();
            }
            Vec2i posShop = unit->getGoalStructure()->getPos();
            int distance = sqrt(pow(float(abs(posUnit.x - posShop.x)), 2) + pow(float(abs(posUnit.y - posShop.y)), 2));
            if (distance < 2) {
                unit->shop();
                unit->setGoalReason("");
            }
        }
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            Unit *finalPick = findShop(unit);
            Vec2i tPos = Vec2i(0,0);
            if (finalPick != NULL) {
                tPos = finalPick->getPos();
            }
            const CommandType *ct = unit->getType()->getActions()->getFirstCtOfClass(CmdClass::MOVE);
            if (tPos != Vec2i(0,0)) {
                    if (unit->isCarried()) {
                        ownerUnload(unit);
                    } else {
                        unit->setGoalStructure(finalPick);
                        unit->giveCommand(g_world.newCommand(ct, CmdFlags(), tPos));
                        unit->setGoalReason("shop");
                    }
            }
        }
    } else if (goal == Goal::ATTACK) {
        clearSimAi(unit, goal);
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            if (unit->getGoalStructure() == NULL) {
                if (unit->attackers.size() > 0) {
                    unit->setGoalStructure(unit->attackers[0].getUnit());
                    unit->setGoalReason("kill");
                }
            }
            if (unit->getGoalStructure() != NULL) {
                if (unit->anyCommand()) {
                    if (unit->getCurrCommand()->getType()->getClass() != CmdClass::ATTACK) {
                        const CommandType *act = selectAttackSpell(unit, unit->getGoalStructure());
                        unit->giveCommand(g_world.newCommand(act, CmdFlags(), unit->getGoalStructure()));
                    }
                }
            }
        }
    } else if (goal == Goal::DEFEND) {
        clearSimAi(unit, goal);
        if (unit->owner->getId() != unit->getId()) {
            if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
                if (unit->getGoalStructure() == NULL) {
                    Vec2i uPos = unit->getPos();
                    if (unit->isCarried()) {
                        uPos = unit->owner->getCenteredPos();
                    }
                    Vec2i tPos = unit->owner->getPos();
                    int distance = sqrt(pow(float(abs(uPos.x - tPos.x)), 2) + pow(float(abs(uPos.y - tPos.y)), 2));
                    if (unit->owner->attackers.size() > 0) {
                        if (unit->isCarried()) {
                            ownerUnload(unit);
                        }
                        if (distance < 25 + unit->owner->attackers.size() * 5) {
                            unit->setGoalStructure(unit->owner->attackers[0].getUnit());
                            unit->setGoalReason("kill");
                        }
                    }
                }
                if (unit->getGoalStructure() != NULL) {
                    if (unit->anyCommand()) {
                        if (unit->getCurrCommand()->getType()->getClass() != CmdClass::ATTACK) {
                            const CommandType *act = selectAttackSpell(unit, unit->getGoalStructure());
                            unit->giveCommand(g_world.newCommand(act, CmdFlags(), unit->getGoalStructure()));
                        }
                    }
                }
            }
        }
    } else if (goal == Goal::HUNT) {
        clearSimAi(unit, goal);
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            if (unit->getGoalStructure() == NULL) {
                Unit *creature = findCreature(unit, 50);
                if (creature != NULL) {
                    unit->setGoalStructure(creature);
                    unit->setGoalReason("hunt");
                    if (unit->isCarried()) {
                        ownerUnload(unit);
                    }
                }
            }
            if (unit->getGoalStructure() != NULL) {
                if (unit->anyCommand()) {
                    if (unit->getCurrCommand()->getType()->getClass() != CmdClass::ATTACK) {
                        const CommandType *act = selectAttackSpell(unit, unit->getGoalStructure());
                        unit->giveCommand(g_world.newCommand(act, CmdFlags(), unit->getGoalStructure()));
                    }
                }
            }
        }
    } else if (goal == Goal::PATROL) {
        clearSimAi(unit, goal);
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            if (unit->getGoalStructure() == NULL) {
                Unit *creature = findCreature(unit, 8);
                if (creature != NULL) {
                    unit->setGoalStructure(creature);
                    unit->setGoalReason("patrol");
                    if (unit->isCarried()) {
                        ownerUnload(unit);
                    }
                }
            }
            if (unit->getGoalStructure() != NULL) {
                if (unit->anyCommand()) {
                    if (unit->getCurrCommand()->getType()->getClass() != CmdClass::ATTACK) {
                        const CommandType *act = selectAttackSpell(unit, unit->getGoalStructure());
                        unit->giveCommand(g_world.newCommand(act, CmdFlags(), unit->getGoalStructure()));
                    }
                }
            }
        }
    } else if (goal == Goal::RAID) {
        clearSimAi(unit, goal);
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            if (unit->getGoalStructure() == NULL) {
                Unit *lair = findLair(unit);
                if (lair != NULL) {
                    unit->setGoalStructure(lair);
                    unit->setGoalReason("raid");
                    if (unit->isCarried()) {
                        ownerUnload(unit);
                    }
                }
            }
            if (unit->getGoalStructure() != NULL) {
                if (unit->anyCommand()) {
                    if (unit->getCurrCommand()->getType()->getClass() != CmdClass::ATTACK) {
                        const CommandType *act = selectAttackSpell(unit, unit->getGoalStructure());
                        unit->giveCommand(g_world.newCommand(act, CmdFlags(), unit->getGoalStructure()));
                    }
                }
            }
        }
    } else if (goal == Goal::DEMOLISH) {
        clearSimAi(unit, goal);
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            if (unit->getGoalStructure() == NULL) {
                Unit *city = findCity(unit);
                if (city != NULL) {
                    unit->setGoalStructure(city);
                    unit->setGoalReason("demolish");
                    if (unit->isCarried()) {
                        ownerUnload(unit);
                    }
                }
            }
            if (unit->getGoalStructure() != NULL) {
                if (unit->anyCommand()) {
                    if (unit->getCurrCommand()->getType()->getClass() != CmdClass::ATTACK) {
                        const CommandType *act = selectAttackSpell(unit, unit->getGoalStructure());
                        unit->giveCommand(g_world.newCommand(act, CmdFlags(), unit->getGoalStructure()));
                    }
                }
            }
        }
    } else if (goal == Goal::EXPLORE) {
        clearSimAi(unit, goal);
        Vec2i unitPos = unit->getCenteredPos();
        if (unit->isCarried()) {
            unitPos = unit->owner->getCenteredPos();
        }
        Vec2i pos = unitPos;
        Vec2i position = exploredMap.findNearestUnexploredTile(unitPos, unit->getFaction(), unit->previousDirection);
        pos = position;
        const CommandType *ct = unit->getType()->getActions()->getFirstCtOfClass(CmdClass::MOVE);
        if (pos != unitPos && pos != Vec2i(0,0)) {
            unit->giveCommand(g_world.newCommand(ct, CmdFlags(), pos));
            unit->previousDirection = newDirection(unit->previousDirection);
        } else {

        }
    } else if (goal == Goal::BUFF) {
        clearSimAi(unit, goal);
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            Unit *buffTarget = NULL;
            buffTarget = findNearbyAlly(unit, focus);
            if (buffTarget != NULL) {
                const CommandType *buffingSpell = selectBuffSpell(unit, buffTarget);
                unit->giveCommand(g_world.newCommand(buffingSpell, CmdFlags(), buffTarget));
            }
        }
    } else if (goal == Goal::HEAL) {
        clearSimAi(unit, goal);
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            Unit *healTarget = NULL;
            healTarget = findNearbyAlly(unit, focus);
            if (healTarget != NULL) {
                const CommandType *healingSpell = selectHealSpell(unit, healTarget);
                unit->giveCommand(g_world.newCommand(healingSpell, CmdFlags(), healTarget));
            }
        }
    } else if (goal == Goal::SPELL) {
        clearSimAi(unit, goal);
        if (unit->getCurrSkill()->getClass() == SkillClass::ATTACK) {
            Unit *spellTarget = NULL;
            spellTarget = unit->attackers[0].getUnit();
            if (spellTarget != NULL) {
                const CommandType *attackSpell = selectAttackSpell(unit, spellTarget);
                unit->giveCommand(g_world.newCommand(attackSpell, CmdFlags(), spellTarget));

            }
        }
    } else if (goal == Goal::TRANSPORT) {
        if (goalList[goal] != unit->getCurrentFocus()) {
            clearSimAi(unit, goal);
            unit->productionRoute.setStoreId(unit->owner->getId());
            unit->productionRoute.setDestination(unit->owner->getPos());
        }
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            Unit *transport = NULL;
            transport = findProducer(unit);
            if (transport != NULL) {
                unit->productionRoute.setProducerId(transport->getId());
                unit->setGoalStructure(transport);
                if (unit->productionRoute.getProducerId() != -1 && unit->productionRoute.getStoreId() != -1) {
                    if (unit->isCarried()) {
                        ownerUnload(unit);
                    }
                    const CommandType *tct = unit->getType()->getActions()->getFirstCtOfClass(CmdClass::TRANSPORT);
                    unit->giveCommand(g_world.newCommand(tct, CmdFlags(), unit->getGoalStructure()));
                }
            }
        }
    } else if (goal == Goal::TRADE) {
        if (goalList[goal] != unit->getCurrentFocus()) {
            clearSimAi(unit, goal);
            unit->productionRoute.setStoreId(unit->owner->getId());
            unit->productionRoute.setDestination(unit->owner->getPos());
        }
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            Unit *transport = NULL;
            transport = findGuild(unit);
            if (transport != NULL) {
                unit->productionRoute.setProducerId(transport->getId());
                unit->productionRoute.setDestination(transport->getPos());
                unit->setGoalStructure(transport);
                if (unit->productionRoute.getProducerId() != -1 && unit->productionRoute.getStoreId() != -1) {
                    if (unit->isCarried()) {
                        ownerUnload(unit);
                    }
                    const CommandType *tct = unit->getType()->getActions()->getFirstCtOfClass(CmdClass::TRADE);
                    if (tct != 0) {
                        unit->giveCommand(g_world.newCommand(tct, CmdFlags(), unit->getGoalStructure()));
                    }
                }
            }
        }
    } else if (goal == Goal::COLLECT) {
        if (goalList[goal] != unit->getCurrentFocus()) {
            clearSimAi(unit, goal);
            unit->productionRoute.setStoreId(unit->owner->getId());
            unit->productionRoute.setDestination(unit->owner->getPos());
        }
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            if (unit->getGoalReason() == "deliver") {
                const CommandType *tct = unit->getType()->getActions()->getFirstCtOfClass(CmdClass::MOVE);
                unit->giveCommand(g_world.newCommand(tct, CmdFlags(), unit->getGoalStructure()));
            }
            Faction *f = unit->getFaction();
            vector<Unit*> buildingsList;
            for (int i = 0; i < f->getUnitCount(); ++i) {
                Unit *building = f->getUnit(i);
                if (building->getType()->hasTag("building") && !building->getType()->hasTag("fort") && building->getId() != unit->owner->getId()) {
                    if (building->sresources.size() > 0) {
                        for (int j = 0; j < building->sresources.size(); ++j) {
                            if (building->getSResource(j)->getType()->getName() == "wealth") {
                                if (building->getSResource(j)->getAmount() - building->taxedGold >= 500) {
                                    bool previousTarget = false;
                                    for (int z = 0; z < unit->getFaction()->getUnitCount(); ++z) {
                                        if (unit->getFaction()->getUnit(z)->owner->getId() == unit->owner->getId()) {
                                            if (unit->getFaction()->getUnit(z)->getGoalStructure() == building) {
                                                previousTarget = true;
                                            }
                                        }
                                    }
                                    if (previousTarget == false) {
                                        buildingsList.push_back(building);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            int distance = 50;
            Vec2i uPos = unit->getCenteredPos();
            if (unit->isCarried()) {
                uPos = unit->owner->getCenteredPos();
            }
            Unit *finalPick;
            Vec2i tPos = Vec2i(0,0);
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
            const CommandType *tct = unit->getType()->getActions()->getFirstCtOfClass(CmdClass::MOVE);
            if (tPos != Vec2i(0,0)) {
                if (unit->isCarried()) {
                    ownerUnload(unit);
                }
                if (unit->productionRoute.getProducerId() == -1) {
                    unit->productionRoute.setProducerId(finalPick->getId());
                    unit->productionRoute.setDestination(finalPick->getPos());
                }
                if (unit->productionRoute.getProducerId() != -1 && unit->productionRoute.getStoreId() != -1) {
                    unit->setGoalReason("collect");
                    unit->setGoalStructure(finalPick);
                    unit->giveCommand(g_world.newCommand(tct, CmdFlags(), finalPick->getPos()));
                }
            } else {
                if (unit->productionRoute.getProducerId() == -1) {
                    ownerLoad(unit);
                }
            }
        }
    } else if (goal == Goal::REST) {
        clearSimAi(unit, goal);
        if (unit->getCurrSkill()->getClass() == SkillClass::STOP) {
            ownerLoad(unit);
            unit->setGoalReason("rest");
        }
    }
}

}}
