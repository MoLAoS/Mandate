// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include <algorithm>
#include <cassert>

#include "unit_type.h"
#include "util.h"
#include "logger.h"
#include "lang.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "resource.h"
#include "renderer.h"

#include "leak_dumper.h"

using Glest::Util::Logger;
using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest { namespace ProtoTypes {
// ===============================
// 	class Stat
// ===============================
void Stat::init(int value, fixed valueMult, int layerAdd, fixed layerMult) {
    this->value = value;
    this->valueMult = valueMult;
    this->layerAdd = layerAdd;
    this->layerMult = layerMult;
}

void Stat::reset() {
	this->value = 0;
	this->valueMult = 1;
	this->layerAdd = 0;
	this->layerMult = 0;
}

bool Stat::isEmpty() const {
    bool empty = true;
	if (this->value != 0) empty = false;
	if (this->valueMult != 1) empty = false;
	if (this->layerAdd != 0) empty = false;
	if (this->layerMult != 0) empty = false;
	return empty;
}

void Stat::modify() {
    this->value += layerAdd;
    this->valueMult += layerMult;
    this->value = (value * valueMult).intp();
}

void Stat::save(XmlNode *node) const {
    if (value != 0) {
        node->addAttribute("value", value);
    }
    if (valueMult.intp() != 1) {
        node->addAttribute("value-mult", valueMult.intp());
    }
    if (layerAdd != 0) {
        node->addAttribute("add", layerAdd);
    }
    if (layerMult.intp() != 0) {
        node->addAttribute("mult", layerMult.intp());
    }
}

void Stat::doChecksum(Checksum &checksum) const {
	checksum.add(value);
	checksum.add(valueMult);
	checksum.add(layerAdd);
	checksum.add(layerMult);
}

bool Stat::load(const XmlNode *baseNode) {
    bool loadOk = true;
    reset();
    const XmlAttribute* statAttibute = baseNode->getAttribute("value", false);
    if (statAttibute) {
        value = statAttibute->getIntValue();
    }
    statAttibute = baseNode->getAttribute("value-mult", false);
    if (statAttibute) {
        valueMult = statAttibute->getIntValue();
    }
    statAttibute = baseNode->getAttribute("add", false);
    if (statAttibute) {
        layerAdd = statAttibute->getIntValue();
    }
    statAttibute = baseNode->getAttribute("mult", false);
    if (statAttibute) {
        layerMult = statAttibute->getIntValue();
    }
    return loadOk;
}

void Stat::sanitiseStat(int safety) {
    if (value < 0) value = safety;
}

void Stat::clampMultipliers() {
	fixed low;
	low.raw() = (1 << 6);
	if (valueMult <= 0) {
		valueMult = low;
	}
	if (layerMult <= 0) {
		layerMult = low;
	}
}

void Stat::formatModifier(string &str, const char *pre, string label, int base, fixed multiplier, int layer, fixed layerMultiplier) const {
	Lang &lang = Lang::getInstance();
	if (base) {
		str += pre + lang.get(label) + ": ";
		if (base > 0) {
			str += "+";
		}
		str += intToStr(base);
	}
	if (multiplier != 1) {
		if(base) {
			str += ", ";
		} else {
			str += pre + lang.get(label) + ": ";
		}
		if (multiplier > 1) {
			str += "+";
		}
		str += intToStr(((multiplier - 1) * 100).intp()) + "%";
	}
	if (layer) {
		str += pre + lang.get(label) + ": ";
		if (base > 0) {
			str += "+";
		}
		str += intToStr(layer);
	}
	if (layerMultiplier != 0) {
		if(base) {
			str += ", ";
		} else {
			str += pre + lang.get(label) + ": ";
		}
		if (layerMultiplier > 1) {
			str += "+";
		}
		str += intToStr(((layerMultiplier - 1) * 100).intp()) + "%";
	}
}

void Stat::getDesc(string &str, const char *pre, string name) const {
	formatModifier(str, pre, name, value, valueMult, layerAdd, layerMult);
}

// ===============================
// 	class StatGroup
// ===============================
void StatGroup::reset() {
    name = "stat";
    this->maxStat.reset();
    this->regenStat.reset();
    this->boostStat.reset();
}

void StatGroup::modify() {
    this->maxStat.modify();
    this->regenStat.modify();
    this->boostStat.modify();
}

bool StatGroup::isEmpty() const {
    bool empty = true;
    if (!maxStat.isEmpty()) empty = false;
    if (!regenStat.isEmpty()) empty = false;
    if (!boostStat.isEmpty()) empty = false;
    return empty;
}

void StatGroup::doChecksum(Checksum &checksum) const {
	checksum.add(maxStat);
	checksum.add(regenStat);
	checksum.add(boostStat);
}

void StatGroup::clampMultipliers() {
	maxStat.clampMultipliers();
	regenStat.clampMultipliers();
	boostStat.clampMultipliers();
}

bool StatGroup::load(const XmlNode *baseNode, const string &dir) {
    bool loadOk = true;
    reset();
    string statName = baseNode->getAttribute("name")->getRestrictedValue();
    name = statName;
    const XmlNode* maxStatNode = baseNode->getChild("max-stat", 0, false);
    if (maxStatNode) {
        loadOk = maxStat.load(maxStatNode) && loadOk;
    }
    const XmlNode* regenStatNode = baseNode->getChild("regen-stat", 0, false);
    if (regenStatNode) {
        loadOk = regenStat.load(regenStatNode) && loadOk;
    }
    const XmlNode* boostStatNode = baseNode->getChild("boost-stat", 0, false);
    if (boostStatNode) {
        loadOk = boostStat.load(boostStatNode) && loadOk;
    }
    return loadOk;
}


void StatGroup::save(XmlNode *node) const {
    node->addAttribute("name", name);
    if (!maxStat.isEmpty()) {
        maxStat.save(node->addChild("max-stat"));
    }
    if (!regenStat.isEmpty()) {
        regenStat.save(node->addChild("regen-stat"));
    }
    if (!boostStat.isEmpty()) {
        boostStat.save(node->addChild("boost-stat"));
    }
}

void StatGroup::addStatic(const StatGroup *sg, fixed strength) {
    int newMaxStat = (sg->getMaxStat()->getValue() * strength).intp();
	maxStat.incValue(newMaxStat);
	int newRegenStat = (sg->getRegenStat()->getValue() * strength).intp();
	regenStat.incValue(newRegenStat);
	int newBoostStat = (sg->getBoostStat()->getValue() * strength).intp();
	boostStat.incValue(newBoostStat);
}

void StatGroup::addMultipliers(const StatGroup *sg, fixed strength) {
	maxStat.incValueMult((sg->getMaxStat()->getValueMult() - 1) * strength);
	regenStat.incValueMult((sg->getRegenStat()->getValueMult() - 1) * strength);
}

void StatGroup::sanitiseStatGroup() {
	maxStat.sanitiseStat(0);
	boostStat.sanitiseStat(0);
}

void StatGroup::applyMultipliers(const StatGroup *sg) {
    int newMaxStat = (maxStat.getValue() * sg->getMaxStat()->getValueMult()).intp();
	maxStat.setValue(newMaxStat);
	int newRegenStat = (regenStat.getValue() * sg->getRegenStat()->getValueMult()).intp();
	regenStat.setValue(newRegenStat);
}

void StatGroup::getDesc(string &str, const char *pre) const {
    str += pre;
    str += name;
    maxStat.getDesc(str, pre, "Max");
    regenStat.getDesc(str, pre, "Regen");
    boostStat.getDesc(str, pre, "Boost");
}

// ===============================
// 	class ResourcePools
// ===============================
void ResourcePools::reset() {
    health.reset();
    health.setName("health");
    for (int i = 0; i < getResourceCount(); ++i) {
        resources[i].reset();
    }
    for (int i = 0; i < getDefenseCount(); ++i) {
        defenses[i].reset();
    }
	maxCp.reset();
}

bool ResourcePools::isEmpty() const {
    bool empty = true;
    if (!health.isEmpty()) empty = false;
    for (int i = 0; i < getResourceCount(); ++i) {
        if (!resources[i].isEmpty()) empty = false;
    }
    for (int i = 0; i < getDefenseCount(); ++i) {
        if (!defenses[i].isEmpty()) empty = false;
    }
	if (!maxCp.isEmpty()) empty = false;
	return empty;
}

void ResourcePools::modify() {
    health.modify();
    for (int i = 0; i < getResourceCount(); ++i) {
        resources[i].modify();
    }
    for (int i = 0; i < getDefenseCount(); ++i) {
        defenses[i].modify();
    }
	maxCp.modify();
}

void ResourcePools::clampMultipliers() {
    health.clampMultipliers();
    for (int i = 0; i < getResourceCount(); ++i) {
        resources[i].clampMultipliers();
    }
    for (int i = 0; i < getDefenseCount(); ++i) {
        defenses[i].clampMultipliers();
    }
	maxCp.clampMultipliers();
}

void ResourcePools::doChecksum(Checksum &checksum) const {
    checksum.add(health);
    for (int i = 0; i < getResourceCount(); ++i) {
        checksum.add(resources[i]);
    }
    for (int i = 0; i < getDefenseCount(); ++i) {
        checksum.add(defenses[i]);
    }
	checksum.add(maxCp);
}

bool ResourcePools::load(const XmlNode *baseNode, const string &dir) {
	bool loadOk = true;
	const XmlNode *healthNode = baseNode->getChild("health");
    try {
        if (!health.load(healthNode, dir)) {
            loadOk = false;
        }
    }
    catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
	const XmlNode *resourcesNode = baseNode->getChild("pools", 0, false);
	if (resourcesNode) {
        resources.resize(resourcesNode->getChildCount());
        for (int i = 0; i < resourcesNode->getChildCount(); ++i) {
            try {
                const XmlNode *resourceNode = resourcesNode->getChild("pool", i);
                if (!resources[i].load(resourceNode, dir)) {
                    loadOk = false;
                }
            }
            catch (runtime_error e) {
                g_logger.logXmlError(dir, e.what());
                loadOk = false;
            }
        }
	}
	const XmlNode *defensesNode = baseNode->getChild("defenses", 0, false);
	if (defensesNode) {
        defenses.resize(defensesNode->getChildCount());
        for (int i = 0; i < defensesNode->getChildCount(); ++i) {
            try {
                const XmlNode *defenseNode = defensesNode->getChild("defense", i);
                if (!defenses[i].load(defenseNode, dir)) {
                    loadOk = false;
                }
            }
            catch (runtime_error e) {
                g_logger.logXmlError(dir, e.what());
                loadOk = false;
            }
        }
	}
	try {
        const XmlNode *maxCpNode = baseNode->getChild("max-cp", 0, false);
        if (maxCpNode) {
            if (!maxCp.load(maxCpNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void ResourcePools::save(XmlNode *node) const {
    XmlNode *n;
    health.save(node->addChild("health"));
    if (!resources.empty()) {
        n = node->addChild("pools");
        for (int i = 0; i < resources.size(); ++i) {
            resources[i].save(n->addChild("pool"));
        }
    }
    if (!defenses.empty()) {
        n = node->addChild("defenses");
        for (int i = 0; i < defenses.size(); ++i) {
            defenses[i].save(n->addChild("defense"));
        }
    }
    if (!maxCp.isEmpty()) {
        maxCp.save(node->addChild("max-cp"));
    }
}

void ResourcePools::addResources(StatGroups statGroups) {
    for (int i = 0; i < statGroups.size(); ++i) {
        resources.push_back(statGroups[i]);
    }
}

void ResourcePools::addDefenses(StatGroups statGroups) {
    for (int i = 0; i < statGroups.size(); ++i) {
        defenses.push_back(statGroups[i]);
    }
}

void ResourcePools::addStatic(const ResourcePools *rp, fixed strength) {
    health.addStatic(rp->getHealth(), strength);
    for (int i = 0; i < rp->getResourceCount(); ++i) {
        string firstName = rp->getResource(i)->getName();
        for (int j = 0; j < getResourceCount(); ++j) {
            string secondName = getResource(j)->getName();
            if (secondName == firstName) {
                resources[i].addStatic(rp->getResource(i), strength);
            }
        }
    }
    for (int i = 0; i < rp->getDefenseCount(); ++i) {
        string firstName = rp->getDefense(i)->getName();
        for (int j = 0; j < getDefenseCount(); ++j) {
            string secondName = getDefense(j)->getName();
            if (secondName == firstName) {
                defenses[i].addStatic(rp->getDefense(i), strength);
            }
        }
    }
	int newMaxCp = (rp->getMaxCp()->getValue() * strength).intp();
	maxCp.incValue(newMaxCp);
}

void ResourcePools::addMultipliers(const ResourcePools *rp, fixed strength) {
    health.addMultipliers(rp->getHealth(), strength);
    for (int i = 0; i < rp->getResourceCount(); ++i) {
        string firstName = rp->getResource(i)->getName();
        for (int j = 0; j < getResourceCount(); ++j) {
            string secondName = getResource(j)->getName();
            if (secondName == firstName) {
                resources[i].addMultipliers(rp->getResource(i), strength);
            }
        }
    }
    for (int i = 0; i < rp->getDefenseCount(); ++i) {
        string firstName = rp->getDefense(i)->getName();
        for (int j = 0; j < getDefenseCount(); ++j) {
            string secondName = getDefense(j)->getName();
            if (secondName == firstName) {
                defenses[i].addMultipliers(rp->getDefense(i), strength);
            }
        }
    }
	maxCp.incValueMult((rp->getMaxCp()->getValueMult() - 1) * strength);
}

void ResourcePools::sanitiseResourcePools() {
	health.sanitiseStatGroup();
    for (int i = 0; i < getResourceCount(); ++i) {
        resources[i].sanitiseStatGroup();
    }
    for (int i = 0; i < getDefenseCount(); ++i) {
        defenses[i].sanitiseStatGroup();
    }
    maxCp.sanitiseStat(-1);
}

void ResourcePools::applyMultipliers(const ResourcePools *rp) {
	health.applyMultipliers(rp->getHealth());
    for (int i = 0; i < rp->getResourceCount(); ++i) {
        string firstName = rp->getResource(i)->getName();
        for (int j = 0; j < getResourceCount(); ++j) {
            string secondName = getResource(j)->getName();
            if (secondName == firstName) {
                resources[j].applyMultipliers(rp->getResource(i));
            }
        }
    }
    for (int i = 0; i < rp->getDefenseCount(); ++i) {
        string firstName = rp->getDefense(i)->getName();
        for (int j = 0; j < getDefenseCount(); ++j) {
            string secondName = getDefense(j)->getName();
            if (secondName == firstName) {
                defenses[j].applyMultipliers(rp->getDefense(i));
            }
        }
    }
    int newMaxCp = (maxCp.getValue() * rp->getMaxCp()->getValueMult()).intp();
	maxCp.setValue(newMaxCp);
}

void ResourcePools::getDesc(string &str, const char *pre) const {
    if (!health.isEmpty()) {
        health.getDesc(str, pre);
    }
    for (int i = 0; i < getResourceCount(); ++i) {
        resources[i].getDesc(str, pre);
    }
    for (int i = 0; i < getDefenseCount(); ++i) {
        defenses[i].getDesc(str, pre);
    }
	maxCp.getDesc(str, pre, "MaxCp");
}

// ===============================
// 	class ProductionSpeeds
// ===============================
void ProductionSpeeds::reset() {
	prodSpeed.reset();
	repairSpeed.reset();
	harvestSpeed.reset();
}

void ProductionSpeeds::modify() {
	prodSpeed.modify();
	repairSpeed.modify();
	harvestSpeed.modify();
}

bool ProductionSpeeds::isEmpty() const {
    bool empty = true;
	if (!prodSpeed.isEmpty()) empty = false;
	if (!repairSpeed.isEmpty()) empty = false;
	if (!harvestSpeed.isEmpty()) empty = false;
	return empty;
}

void ProductionSpeeds::doChecksum(Checksum &checksum) const {
	checksum.add(prodSpeed);
	checksum.add(repairSpeed);
	checksum.add(harvestSpeed);
}

bool ProductionSpeeds::load(const XmlNode *baseNode, const string &dir) {
	bool loadOk = true;
	try {
        const XmlNode *prodSpeedNode = baseNode->getChild("prod-speed", 0, false);
        if (prodSpeedNode) {
            prodSpeed.load(prodSpeedNode);
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *repairSpeedNode = baseNode->getChild("repair-speed", 0, false);
        if (repairSpeedNode) {
            repairSpeed.load(repairSpeedNode);
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *harvestSpeedNode = baseNode->getChild("harvest-speed", 0, false);
        if (harvestSpeedNode) {
            harvestSpeed.load(harvestSpeedNode);
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void ProductionSpeeds::save(XmlNode *node) const {
    if (!prodSpeed.isEmpty()) {
        prodSpeed.save(node->addChild("prod-speed"));
    }
    if (!repairSpeed.isEmpty()) {
        repairSpeed.save(node->addChild("repair-speed"));
    }
    if (!harvestSpeed.isEmpty()) {
        harvestSpeed.save(node->addChild("harvest-speed"));
    }
}

void ProductionSpeeds::addStatic(const ProductionSpeeds *ps, fixed strength) {
	int newProdSpeed = (ps->getProdSpeed()->getValue() * strength).intp();
	prodSpeed.incValue(newProdSpeed);
	int newRepairSpeed  = (ps->getRepairSpeed()->getValue() * strength).intp();
	repairSpeed.incValue(newRepairSpeed);
	int newHarvestSpeed  = (ps->getHarvestSpeed()->getValue() * strength).intp();
	harvestSpeed.incValue(newHarvestSpeed);
}

void ProductionSpeeds::addMultipliers(const ProductionSpeeds *ps, fixed strength) {
	prodSpeed.incValueMult((ps->getProdSpeed()->getValueMult() - 1) * strength);
	repairSpeed.incValueMult((ps->getRepairSpeed()->getValueMult() - 1) * strength);
	harvestSpeed.incValueMult((ps->getHarvestSpeed()->getValueMult() - 1) * strength);
}

void ProductionSpeeds::clampMultipliers() {
	prodSpeed.clampMultipliers();
	repairSpeed.clampMultipliers();
	harvestSpeed.clampMultipliers();
}

void ProductionSpeeds::sanitiseProductionSpeeds() {
	prodSpeed.sanitiseStat(0);
	repairSpeed.sanitiseStat(0);
	harvestSpeed.sanitiseStat(0);
}

void ProductionSpeeds::applyMultipliers(const ProductionSpeeds *ps) {
    int newProdSpeed = (prodSpeed.getValue() * ps->getProdSpeed()->getValueMult()).intp();
	prodSpeed.setValue(newProdSpeed);
	int newRepairSpeed = (repairSpeed.getValue() * ps->getRepairSpeed()->getValueMult()).intp();
	repairSpeed.setValue(newRepairSpeed);
    int newHarvestSpeed = (harvestSpeed.getValue() * ps->getHarvestSpeed()->getValueMult()).intp();
	harvestSpeed.setValue(newHarvestSpeed);
}

void ProductionSpeeds::getDesc(string &str, const char *pre) const {
	prodSpeed.getDesc(str, pre, "ProdSpeed");
	repairSpeed.getDesc(str, pre, "RepairSpeed");
	harvestSpeed.getDesc(str, pre, "HarvestSpeed");
}

// ===============================
// 	class AttackStats
// ===============================
void AttackStats::reset() {
	attackRange.reset();
	attackSpeed.reset();
	attackStrength.reset();
	attackPotency.reset();
}

bool AttackStats::isEmpty() const {
    bool empty = true;
	if (!attackRange.isEmpty()) empty = false;
	if (!attackSpeed.isEmpty()) empty = false;
	if (!attackStrength.isEmpty()) empty = false;
	if (!attackPotency.isEmpty()) empty = false;
	return empty;
}

void AttackStats::modify() {
	attackRange.modify();
	attackSpeed.modify();
	attackStrength.modify();
	attackPotency.modify();
}

void AttackStats::doChecksum(Checksum &checksum) const {
	checksum.add(attackRange);
	checksum.add(attackSpeed);
	checksum.add(attackStrength);
	checksum.add(attackPotency);
}

bool AttackStats::load(const XmlNode *baseNode, const string &dir) {
	bool loadOk = true;
	try {
        const XmlNode *attackRangeNode = baseNode->getChild("attack-range", 0, false);
        if (attackRangeNode) {
            if (!attackRange.load(attackRangeNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *attackSpeedNode = baseNode->getChild("attack-speed", 0, false);
        if (attackSpeedNode) {
            if (!attackSpeed.load(attackSpeedNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *strengthNode = baseNode->getChild("attack-strength", 0, false);
        if (strengthNode) {
            if (!attackStrength.load(strengthNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *potencyNode = baseNode->getChild("attack-potency", 0, false);
        if (potencyNode) {
            if (!attackPotency.load(potencyNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void AttackStats::save(XmlNode *node) const {
    if (!attackRange.isEmpty()) {
        attackRange.save(node->addChild("attack-range"));
    }
    if (!attackSpeed.isEmpty()) {
        attackSpeed.save(node->addChild("attack-speed"));
    }
    if (!attackStrength.isEmpty()) {
        attackStrength.save(node->addChild("attack-strength"));
    }
    if (!attackPotency.isEmpty()) {
        attackPotency.save(node->addChild("attack-potency"));
    }
}

void AttackStats::addStatic(const AttackStats *as, fixed strength) {
	int newAttackRange = (as->getAttackRange()->getValue() * strength).intp();
	attackRange.incValue(newAttackRange);
	int newAttackSpeed = (as->getAttackSpeed()->getValue() * strength).intp();
	attackSpeed.incValue(newAttackSpeed);
	int newStrength = (as->getAttackStrength()->getValue() * strength).intp();
	attackStrength.incValue(newStrength);
	int newPotency = (as->getAttackPotency()->getValue() * strength).intp();
	attackPotency.incValue(newPotency);
}

void AttackStats::addMultipliers(const AttackStats *as, fixed strength) {
	attackRange.incValueMult((as->getAttackRange()->getValueMult() - 1) * strength);
	attackSpeed.incValueMult((as->getAttackSpeed()->getValueMult() - 1) * strength);
	attackStrength.incValueMult((as->getAttackStrength()->getValueMult() - 1) * strength);
	attackPotency.incValueMult((as->getAttackPotency()->getValueMult() - 1) * strength);
}

void AttackStats::clampMultipliers() {
	attackRange.clampMultipliers();
	attackSpeed.clampMultipliers();
	attackStrength.clampMultipliers();
	attackPotency.clampMultipliers();
}

void AttackStats::sanitiseAttackStats() {
	attackRange.sanitiseStat(0);
	attackSpeed.sanitiseStat(0);
	attackStrength.sanitiseStat(0);
	attackPotency.sanitiseStat(0);
}

void AttackStats::applyMultipliers(const AttackStats *as) {
    int newAttackRange = (attackRange.getValue() * as->getAttackRange()->getValueMult()).intp();
	attackRange.setValue(newAttackRange);
	int newAttackSpeed = (attackSpeed.getValue() * as->getAttackSpeed()->getValueMult()).intp();
	attackSpeed.setValue(newAttackSpeed);
    int newStrength = (attackStrength.getValue() * as->getAttackStrength()->getValueMult()).intp();
	attackStrength.setValue(newStrength);
	int newPotency = (attackPotency.getValue() * as->getAttackPotency()->getValueMult()).intp();
	attackPotency.setValue(newPotency);
}

void AttackStats::getDesc(string &str, const char *pre) const {
	attackRange.getDesc(str, pre, "AttackRange");
	attackSpeed.getDesc(str, pre, "AttackSpeed");
	attackStrength.getDesc(str, pre, "AttackStrength");
	attackPotency.getDesc(str, pre, "AttackPotency");
}

// ===============================
//  class UnitStats
// ===============================
void UnitStats::reset() {
	sight.reset();
	expGiven.reset();
	morale.reset();
	moveSpeed.reset();
	effectStrength.reset();
}

bool UnitStats::isEmpty() const {
    bool empty = true;
	if (!sight.isEmpty()) empty = false;
	if (!expGiven.isEmpty()) empty = false;
	if (!morale.isEmpty()) empty = false;
	if (!moveSpeed.isEmpty()) empty = false;
	if (!effectStrength.isEmpty()) empty = false;
	return empty;
}

void UnitStats::modify() {
	sight.modify();
	expGiven.modify();
	morale.modify();
	moveSpeed.modify();
	effectStrength.modify();
}

void UnitStats::doChecksum(Checksum &checksum) const {
	checksum.add(sight);
	checksum.add(expGiven);
	checksum.add(morale);
	checksum.add(moveSpeed);
	checksum.add(effectStrength);
}

bool UnitStats::load(const XmlNode *baseNode, const string &dir) {
	bool loadOk = true;
	try {
        const XmlNode *sightNode = baseNode->getChild("sight", 0, false);
        if (sightNode) {
            if (!sight.load(sightNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *expGivenNode = baseNode->getChild("exp-given", 0, false);
        if (expGivenNode) {
            if (!expGiven.load(expGivenNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *moraleNode = baseNode->getChild("morale", 0, false);
        if (moraleNode) {
            if (!morale.load(moraleNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *moveSpeedNode = baseNode->getChild("move-speed", 0, false);
        if (moveSpeedNode) {
            if (!moveSpeed.load(moveSpeedNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *effectStrengthNode = baseNode->getChild("effect-strength", 0, false);
        if (effectStrengthNode) {
            if (!effectStrength.load(effectStrengthNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void UnitStats::save(XmlNode *node) const {
    if (!sight.isEmpty()) {
        sight.save(node->addChild("sight"));
    }
    if (!expGiven.isEmpty()) {
        expGiven.save(node->addChild("exp-given"));
    }
    if (!morale.isEmpty()) {
        morale.save(node->addChild("morale"));
    }
    if (!moveSpeed.isEmpty()) {
        moveSpeed.save(node->addChild("move-speed"));
    }
    if (!effectStrength.isEmpty()) {
        effectStrength.save(node->addChild("effect-strength"));
    }
}

void UnitStats::addStatic(const UnitStats *us, fixed strength) {
	int newSight = (us->getSight()->getValue() * strength).intp();
	sight.incValue(newSight);
	int newExpGiven = (us->getExpGiven()->getValue() * strength).intp();
	expGiven.incValue(newExpGiven);
	int newMorale = (us->getMorale()->getValue() * strength).intp();
	morale.incValue(newMorale);
	int newMoveSpeed = (us->getMoveSpeed()->getValue() * strength).intp();
	moveSpeed.incValue(newMoveSpeed);
	int newEffectStrength = (us->getEffectStrength()->getValue() * strength).intp();
	effectStrength.incValue(newEffectStrength);
}

void UnitStats::addMultipliers(const UnitStats *us, fixed strength) {
	sight.incValueMult((us->getSight()->getValueMult() - 1) * strength);
	expGiven.incValueMult((us->getExpGiven()->getValueMult() - 1) * strength);
	morale.incValueMult((us->getMorale()->getValueMult() - 1) * strength);
	moveSpeed.incValueMult((us->getMoveSpeed()->getValueMult() - 1) * strength);
	effectStrength.incValueMult((us->getEffectStrength()->getValueMult() - 1) * strength);
}

void UnitStats::clampMultipliers() {
	sight.clampMultipliers();
	expGiven.clampMultipliers();
	morale.clampMultipliers();
	moveSpeed.clampMultipliers();
	effectStrength.clampMultipliers();
}

void UnitStats::sanitiseUnitStats() {
	sight.sanitiseStat(0);
	expGiven.sanitiseStat(0);
	morale.sanitiseStat(0);
	moveSpeed.sanitiseStat(0);
	effectStrength.sanitiseStat(0);
}

void UnitStats::applyMultipliers(const UnitStats *us) {
    int newSight = (sight.getValue() * us->getSight()->getValueMult()).intp();
	sight.setValue(newSight);
	int newExpGiven = (expGiven.getValue() * us->getExpGiven()->getValueMult()).intp();
	expGiven.setValue(newExpGiven);
    int newMorale = (morale.getValue() * us->getMorale()->getValueMult()).intp();
	morale.setValue(newMorale);
	int newMoveSpeed = (moveSpeed.getValue() * us->getMoveSpeed()->getValueMult()).intp();
	moveSpeed.setValue(newMoveSpeed);
    int newEffectStrength = (effectStrength.getValue() * us->getEffectStrength()->getValueMult()).intp();
	effectStrength.setValue(newEffectStrength);
}

void UnitStats::getDesc(string &str, const char *pre) const {
	sight.getDesc(str, pre, "Sight");
	expGiven.getDesc(str, pre, "ExpGiven");
	morale.getDesc(str, pre, "Morale");
	moveSpeed.getDesc(str, pre, "MoveSpeed");
	effectStrength.getDesc(str, pre, "EffectStrength");
}

// ===============================
//  class EnhancementType
// ===============================
void EnhancementType::reset() {
    resourcePools.reset();
    productionSpeeds.reset();
    attackStats.reset();
	unitStats.reset();
}

void EnhancementType::modify() {
	resourcePools.modify();
	productionSpeeds.modify();
	attackStats.modify();
	unitStats.modify();
}

bool EnhancementType::isEmpty() const {
    bool empty = true;
    if (!resourcePools.isEmpty()) empty = false;
    if (!productionSpeeds.isEmpty()) empty = false;
    if (!attackStats.isEmpty()) empty = false;
    if (!unitStats.isEmpty()) empty = false;
    return empty;
}

void EnhancementType::save(XmlNode *node) const {
    if (!resourcePools.isEmpty()) {
        resourcePools.save(node->addChild("resource-pools"));
    }
    if (!productionSpeeds.isEmpty()) {
        productionSpeeds.save(node->addChild("production-speeds"));
    }
    if (!attackStats.isEmpty()) {
        attackStats.save(node->addChild("attack-stats"));
    }
    if (!unitStats.isEmpty()) {
        unitStats.save(node->addChild("unit-stats"));
    }
}

void EnhancementType::addStatic(const EnhancementType *e, fixed strength){
    resourcePools.addStatic(e->getResourcePools(), strength);
    productionSpeeds.addStatic(e->getProductionSpeeds(), strength);
    attackStats.addStatic(e->getAttackStats(), strength);
    unitStats.addStatic(e->getUnitStats(), strength);
}

void EnhancementType::addMultipliers(const EnhancementType *e, fixed strength){
    resourcePools.addMultipliers(e->getResourcePools(), strength);
    productionSpeeds.addMultipliers(e->getProductionSpeeds(), strength);
    attackStats.addMultipliers(e->getAttackStats(), strength);
    unitStats.addMultipliers(e->getUnitStats(), strength);
}

void EnhancementType::applyMultipliers(const EnhancementType *e){
    resourcePools.applyMultipliers(e->getResourcePools());
    productionSpeeds.applyMultipliers(e->getProductionSpeeds());
    attackStats.applyMultipliers(e->getAttackStats());
    unitStats.applyMultipliers(e->getUnitStats());
}

void EnhancementType::clampMultipliers() {
	resourcePools.clampMultipliers();
	productionSpeeds.clampMultipliers();
	attackStats.clampMultipliers();
	unitStats.clampMultipliers();
}

void EnhancementType::getDesc(string &str, const char *pre) const {
	resourcePools.getDesc(str, pre);
	productionSpeeds.getDesc(str, pre);
	attackStats.getDesc(str, pre);
	unitStats.getDesc(str, pre);
}

void EnhancementType::sanitiseEnhancement() {
	resourcePools.sanitiseResourcePools();
	productionSpeeds.sanitiseProductionSpeeds();
	attackStats.sanitiseAttackStats();
	unitStats.sanitiseUnitStats();
}

bool EnhancementType::load(const XmlNode *baseNode, const string &dir) {
	bool loadOk = true;
	try {
        const XmlNode *resourcePoolsNode = baseNode->getChild("resource-pools", 0, false);
		if(resourcePoolsNode) {
		    if(!resourcePools.load(resourcePoolsNode, dir)) {
                loadOk = false;
		    }
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *productionSpeedsNode = baseNode->getChild("production-speeds", 0, false);
		if(productionSpeedsNode) {
		    if(!productionSpeeds.load(productionSpeedsNode, dir)) {
                loadOk = false;
		    }
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *attackStatsNode = baseNode->getChild("attack-stats", 0, false);
		if(attackStatsNode) {
		    if(!attackStats.load(attackStatsNode, dir)) {
                loadOk = false;
		    }
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *unitStatsNode = baseNode->getChild("unit-stats", 0, false);
		if(unitStatsNode) {
		    if(!unitStats.load(unitStatsNode, dir)) {
                loadOk = false;
		    }

		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void EnhancementType::doChecksum(Checksum &checksum) const {
	resourcePools.doChecksum(checksum);
	productionSpeeds.doChecksum(checksum);
	attackStats.doChecksum(checksum);
	unitStats.doChecksum(checksum);
}

}}//end namespace
