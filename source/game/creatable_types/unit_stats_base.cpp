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

void Stat::modify() {
    this->value += layerAdd;
    this->valueMult += layerMult;
    this->value = (value * valueMult).intp();
}

void Stat::doChecksum(Checksum &checksum) const {
	checksum.add(value);
	checksum.add(valueMult);
	checksum.add(layerAdd);
	checksum.add(layerMult);
}

bool Stat::load(const XmlNode *baseNode, const string &dir) {
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

void formatModifier(string &str, const char *pre, string label, int value, fixed multiplier, int layer, fixed layerMult) {
	Lang &lang = Lang::getInstance();
	if (value) {
		str += pre + lang.get(label) + ": ";
		if (value > 0) {
			str += "+";
		}
		str += intToStr(value);
	}
	if (multiplier != 1) {
		if(value) {
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
		if (value > 0) {
			str += "+";
		}
		str += intToStr(value);
	}
	if (layerMult != 1) {
		if(value) {
			str += ", ";
		} else {
			str += pre + lang.get(label) + ": ";
		}
		if (multiplier > 1) {
			str += "+";
		}
		str += intToStr(((multiplier - 1) * 100).intp()) + "%";
	}
}

void Stat::getDesc(string &str, const char *pre, string name) const {
	formatModifier(str, pre, name, value, valueMult, layerAdd, layerMult);
}

// ===============================
// 	class ResourcePools
// ===============================
void ResourcePools::reset() {
	maxHp.reset();
	hpRegeneration.reset();
	maxSp.reset();
	spRegeneration.reset();
	maxEp.reset();
	epRegeneration.reset();
	maxCp.reset();
	hpBoost.reset();
	spBoost.reset();
	epBoost.reset();
}

void ResourcePools::modify() {
	maxHp.modify();
	hpRegeneration.modify();
	maxSp.modify();
	spRegeneration.modify();
	maxEp.modify();
	epRegeneration.modify();
	maxCp.modify();
	hpBoost.modify();
	spBoost.modify();
	epBoost.modify();
}

void ResourcePools::clampMultipliers() {
	maxHp.clampMultipliers();
	hpRegeneration.clampMultipliers();
	maxSp.clampMultipliers();
	spRegeneration.clampMultipliers();
	maxEp.clampMultipliers();
	epRegeneration.clampMultipliers();
	maxCp.clampMultipliers();
	hpBoost.clampMultipliers();
	spBoost.clampMultipliers();
	epBoost.clampMultipliers();
}

void ResourcePools::doChecksum(Checksum &checksum) const {
	checksum.add(maxHp);
	checksum.add(hpRegeneration);
	checksum.add(maxSp);
	checksum.add(spRegeneration);
	checksum.add(maxEp);
	checksum.add(epRegeneration);
	checksum.add(maxCp);
	checksum.add(hpBoost);
	checksum.add(spBoost);
	checksum.add(epBoost);
}

bool ResourcePools::load(const XmlNode *baseNode, const string &dir) {
	bool loadOk = true;
	try {
        const XmlNode *maxHpNode = baseNode->getChild("max-hp", 0, false);
        if (maxHpNode) {
            if (!maxHp.load(maxHpNode, dir)) {
                loadOk = false;
            }
        }
        const XmlNode *hpRegenerationNode = baseNode->getChild("hp-regen", 0, false);
        if (hpRegenerationNode) {
            if (!hpRegeneration.load(hpRegenerationNode, dir)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *maxSpNode = baseNode->getChild("max-sp", 0, false);
        if (maxSpNode) {
            if (!maxSp.load(maxSpNode, dir)) {
                loadOk = false;
            }
        }
        const XmlNode *spRegenerationNode = baseNode->getChild("sp-regen", 0, false);
        if (spRegenerationNode) {
            if (!spRegeneration.load(spRegenerationNode, dir)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *maxEpNode = baseNode->getChild("max-ep", 0, false);
        if (maxEpNode) {
            if (!maxEp.load(maxEpNode, dir)) {
                loadOk = false;
            }
        }
        const XmlNode *epRegenerationNode = baseNode->getChild("ep-regen", 0, false);
        if (epRegenerationNode) {
            if (!epRegeneration.load(epRegenerationNode, dir)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *maxCpNode = baseNode->getChild("max-cp", 0, false);
        if (maxCpNode) {
            if (!maxCp.load(maxCpNode, dir)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *boostHpNode = baseNode->getChild("hp-boost", 0, false);
        if (boostHpNode) {
            if (!hpBoost.load(boostHpNode, dir)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *boostSpNode = baseNode->getChild("sp-boost", 0, false);
        if (boostSpNode) {
            if (!hpBoost.load(boostSpNode, dir)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *boostEpNode = baseNode->getChild("ep-boost", 0, false);
        if (boostEpNode) {
            if (!hpBoost.load(boostEpNode, dir)) {
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
	/*node->addChild("max-hp", maxHp);
	node->addChild("hp-regeneration", hpRegeneration);
	node->addChild("max-sp", maxSp);
	node->addChild("sp-regeneration", spRegeneration);
	node->addChild("max-ep", maxEp);
	node->addChild("ep-regeneration", epRegeneration);
	node->addChild("max-cp", maxCp);
	node->addChild("hp-boost", sight);
	node->addChild("sp-boost", expGiven);
	node->addChild("ep-boost", morale);*/
}

void ResourcePools::addStatic(const ResourcePools *rp, fixed strength) {
    int newMaxHp = (rp->getMaxHp().getValue() * strength).intp();
	maxHp.incValue(newMaxHp);
	int newHpRegen = (rp->getHpRegeneration().getValue() * strength).intp();
	hpRegeneration.incValue(newHpRegen);
	int newMaxSp = (rp->getMaxSp().getValue() * strength).intp();
	maxSp.incValue(newMaxSp);
	int newSpRegen = (rp->getSpRegeneration().getValue() * strength).intp();
	spRegeneration.incValue(newSpRegen);
	int newMaxEp = (rp->getMaxEp().getValue() * strength).intp();
	maxEp.incValue(newMaxEp);
	int newEpRegen = (rp->getEpRegeneration().getValue() * strength).intp();
	epRegeneration.incValue(newEpRegen);
	int newMaxCp = (rp->getMaxCp().getValue() * strength).intp();
	maxCp.incValue(newMaxCp);
	int newHpBoost = (rp->getHpBoost().getValue() * strength).intp();
	hpBoost.incValue(newHpBoost);
	int newSpBoost  = (rp->getSpBoost().getValue() * strength).intp();
	spBoost.incValue(newSpBoost);
	int newEpBoost  = (rp->getEpBoost().getValue() * strength).intp();
	epBoost.incValue(newEpBoost);
}

void ResourcePools::addMultipliers(const ResourcePools *rp, fixed strength) {
	maxHp.incValueMult((rp->getMaxHp().getValueMult() - 1) * strength);
	hpRegeneration.incValueMult((rp->getHpRegeneration().getValueMult() - 1) * strength);
	maxSp.incValueMult((rp->getMaxSp().getValueMult() - 1) * strength);
	spRegeneration.incValueMult((rp->getSpRegeneration().getValueMult() - 1) * strength);
	maxEp.incValueMult((rp->getMaxEp().getValueMult() - 1) * strength);
	epRegeneration.incValueMult((rp->getEpRegeneration().getValueMult() - 1) * strength);
	maxCp.incValueMult((rp->getMaxCp().getValueMult() - 1) * strength);
}

void ResourcePools::sanitiseResourcePools() {
	maxHp.sanitiseStat(0);
	maxSp.sanitiseStat(0);
	maxEp.sanitiseStat(0);
    maxCp.sanitiseStat(-1);
	hpBoost.sanitiseStat(0);
	spBoost.sanitiseStat(0);
	epBoost.sanitiseStat(0);
}

void ResourcePools::applyMultipliers(const ResourcePools *rp) {
    int newMaxHp = (maxHp.getValue() * rp->getMaxHp().getValueMult()).intp();
	maxHp.setValue(newMaxHp);
	int newHpRegen = (hpRegeneration.getValue() * rp->getHpRegeneration().getValueMult()).intp();
	hpRegeneration.setValue(newHpRegen);
    int newMaxSp = (maxSp.getValue() * rp->getMaxSp().getValueMult()).intp();
	maxSp.setValue(newMaxSp);
	int newSpRegen = (spRegeneration.getValue() * rp->getSpRegeneration().getValueMult()).intp();
	spRegeneration.setValue(newSpRegen);
    int newMaxEp = (maxEp.getValue() * rp->getMaxEp().getValueMult()).intp();
	maxEp.setValue(newMaxEp);
	int newEpRegen = (epRegeneration.getValue() * rp->getEpRegeneration().getValueMult()).intp();
	epRegeneration.setValue(newEpRegen);
    int newMaxCp = (maxCp.getValue() * rp->getMaxCp().getValueMult()).intp();
	maxCp.setValue(newMaxCp);
}

void addBoostsDesc(string &str, const char *pre, int hpBoost, int epBoost, int spBoost) {
	if (hpBoost) {
		str += pre + g_lang.get("HpBoost") + ": " + intToStr(hpBoost);
	}
	if (epBoost) {
		str += pre + g_lang.get("EpBoost") + ": " + intToStr(epBoost);
	}
    if (spBoost) {
		str += pre + g_lang.get("SpBoost") + ": " + intToStr(spBoost);
	}
}

void ResourcePools::getDesc(string &str, const char *pre) const {
	maxHp.getDesc(str, pre, "MaxHp");
	hpRegeneration.getDesc(str, pre, "HpRegen");
	maxSp.getDesc(str, pre, "MaxSp");
	spRegeneration.getDesc(str, pre, "SpRegen");
	maxEp.getDesc(str, pre, "MaxEp");
	epRegeneration.getDesc(str, pre, "EpRegen");
	maxCp.getDesc(str, pre, "MaxCp");
	addBoostsDesc(str, pre, hpBoost.getValue(), spBoost.getValue(), epBoost.getValue());
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
            prodSpeed.load(prodSpeedNode, dir);
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *repairSpeedNode = baseNode->getChild("repair-speed", 0, false);
        if (repairSpeedNode) {
            repairSpeed.load(repairSpeedNode, dir);
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *harvestSpeedNode = baseNode->getChild("harvest-speed", 0, false);
        if (harvestSpeedNode) {
            harvestSpeed.load(harvestSpeedNode, dir);
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void ProductionSpeeds::save(XmlNode *node) const {
	//node->addChild("prod-speed", prodSpeed);
	//node->addChild("repair-speed", repairSpeed);
	//node->addChild("harvest-speed", harvestSpeed);
}

void ProductionSpeeds::addStatic(const ProductionSpeeds *ps, fixed strength) {
	int newProdSpeed = (ps->getProdSpeed().getValue() * strength).intp();
	prodSpeed.incValue(newProdSpeed);
	int newRepairSpeed  = (ps->getRepairSpeed().getValue() * strength).intp();
	repairSpeed.incValue(newRepairSpeed);
	int newHarvestSpeed  = (ps->getHarvestSpeed().getValue() * strength).intp();
	harvestSpeed.incValue(newHarvestSpeed);
}

void ProductionSpeeds::addMultipliers(const ProductionSpeeds *ps, fixed strength) {
	prodSpeed.incValueMult((ps->getProdSpeed().getValueMult() - 1) * strength);
	repairSpeed.incValueMult((ps->getRepairSpeed().getValueMult() - 1) * strength);
	harvestSpeed.incValueMult((ps->getHarvestSpeed().getValueMult() - 1) * strength);
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
    int newProdSpeed = (prodSpeed.getValue() * ps->getProdSpeed().getValueMult()).intp();
	prodSpeed.setValue(newProdSpeed);
	int newRepairSpeed = (repairSpeed.getValue() * ps->getRepairSpeed().getValueMult()).intp();
	repairSpeed.setValue(newRepairSpeed);
    int newHarvestSpeed = (harvestSpeed.getValue() * ps->getHarvestSpeed().getValueMult()).intp();
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
            if (!attackRange.load(attackRangeNode, dir)) {
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
            if (!attackSpeed.load(attackSpeedNode, dir)) {
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
            if (!attackStrength.load(strengthNode, dir)) {
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
            if (!attackPotency.load(potencyNode, dir)) {
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
	//node->addChild("attack-range", attackRange);
	//node->addChild("attack-speed", attackSpeed);
	//node->addChild("attack-strength", attackStrength);
	//node->addChild("attack-potency", attackPotency);
}

void AttackStats::addStatic(const AttackStats *as, fixed strength) {
	int newAttackRange = (as->getAttackRange().getValue() * strength).intp();
	attackRange.incValue(newAttackRange);
	int newAttackSpeed = (as->getAttackSpeed().getValue() * strength).intp();
	attackSpeed.incValue(newAttackSpeed);
	int newStrength = (as->getAttackStrength().getValue() * strength).intp();
	attackStrength.incValue(newStrength);
	int newPotency = (as->getAttackPotency().getValue() * strength).intp();
	attackPotency.incValue(newPotency);
}

void AttackStats::addMultipliers(const AttackStats *as, fixed strength) {
	attackRange.incValueMult((as->getAttackRange().getValueMult() - 1) * strength);
	attackSpeed.incValueMult((as->getAttackSpeed().getValueMult() - 1) * strength);
	attackStrength.incValueMult((as->getAttackStrength().getValueMult() - 1) * strength);
	attackPotency.incValueMult((as->getAttackPotency().getValueMult() - 1) * strength);
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
    int newAttackRange = (attackRange.getValue() * as->getAttackRange().getValueMult()).intp();
	attackRange.setValue(newAttackRange);
	int newAttackSpeed = (attackSpeed.getValue() * as->getAttackSpeed().getValueMult()).intp();
	attackSpeed.setValue(newAttackSpeed);
    int newStrength = (attackStrength.getValue() * as->getAttackStrength().getValueMult()).intp();
	attackStrength.setValue(newStrength);
	int newPotency = (attackPotency.getValue() * as->getAttackPotency().getValueMult()).intp();
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
            if (!sight.load(sightNode, dir)) {
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
            if (!expGiven.load(expGivenNode, dir)) {
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
            if (!morale.load(moraleNode, dir)) {
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
            if (!moveSpeed.load(moveSpeedNode, dir)) {
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
            if (!effectStrength.load(effectStrengthNode, dir)) {
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
	//node->addChild("sight", sight);
	//node->addChild("exp-given", expGiven);
	//node->addChild("morale", morale);
	//node->addChild("move-speed", moveSpeed);
	//node->addChild("effect-strength", effectStrength);
}

void UnitStats::addStatic(const UnitStats *us, fixed strength) {
	int newSight = (us->getSight().getValue() * strength).intp();
	sight.incValue(newSight);
	int newExpGiven = (us->getExpGiven().getValue() * strength).intp();
	expGiven.incValue(newExpGiven);
	int newMorale = (us->getMorale().getValue() * strength).intp();
	morale.incValue(newMorale);
	int newMoveSpeed = (us->getMoveSpeed().getValue() * strength).intp();
	moveSpeed.incValue(newMoveSpeed);
	int newEffectStrength = (us->getEffectStrength().getValue() * strength).intp();
	effectStrength.incValue(newEffectStrength);
}

void UnitStats::addMultipliers(const UnitStats *us, fixed strength) {
	sight.incValueMult((us->getSight().getValueMult() - 1) * strength);
	expGiven.incValueMult((us->getExpGiven().getValueMult() - 1) * strength);
	morale.incValueMult((us->getMorale().getValueMult() - 1) * strength);
	moveSpeed.incValueMult((us->getMoveSpeed().getValueMult() - 1) * strength);
	effectStrength.incValueMult((us->getEffectStrength().getValueMult() - 1) * strength);
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
    int newSight = (sight.getValue() * us->getSight().getValueMult()).intp();
	sight.setValue(newSight);
	int newExpGiven = (expGiven.getValue() * us->getExpGiven().getValueMult()).intp();
	expGiven.setValue(newExpGiven);
    int newMorale = (morale.getValue() * us->getMorale().getValueMult()).intp();
	morale.setValue(newMorale);
	int newMoveSpeed = (moveSpeed.getValue() * us->getMoveSpeed().getValueMult()).intp();
	moveSpeed.setValue(newMoveSpeed);
    int newEffectStrength = (effectStrength.getValue() * us->getEffectStrength().getValueMult()).intp();
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

void EnhancementType::save(XmlNode *node) const {
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

bool EnhancementType::load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft) {
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
