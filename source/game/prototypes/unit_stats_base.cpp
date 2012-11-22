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
//  class UnitStats
// ===============================

//size_t UnitStats::damageMultiplierCount = 0;

// ==================== misc ====================

void UnitStats::reset() {
	maxHp = 0;
	hpRegeneration = 0;
	maxSp = 0;
	spRegeneration = 0;
	maxEp = 0;
	epRegeneration = 0;
	maxCp = 0;
	sight = 0;
	armor = 0;
	expGiven = 0;
	morale = 0;

	attackStrength = 0;
    attackLifeLeech = 0;
    attackManaBurn = 0;
    attackCapture = 0;
	effectStrength = 0;
	attackPctStolen = 0;
	attackRange = 0;
	moveSpeed = 0;
	attackSpeed = 0;
	prodSpeed = 0;
	repairSpeed = 0;
	harvestSpeed = 0;

    layerMaxHp = 0;
	layerHpRegeneration = 0;
	layerMaxSp = 0;
	layerSpRegeneration = 0;
	layerMaxEp = 0;
	layerEpRegeneration = 0;
	layerMaxCp = 0;
	layerSight = 0;
	layerArmor = 0;
	layerExpGiven = 0;
	layerMorale = 0;

	layerAttackStrength = 0;
    layerAttackLifeLeech = 0;
    layerAttackManaBurn = 0;
    layerAttackCapture = 0;
	layerEffectStrength = 0;
	layerAttackPctStolen = 0;
	layerAttackRange = 0;
	layerMoveSpeed = 0;
	layerAttackSpeed = 0;
	layerProdSpeed = 0;
	layerRepairSpeed = 0;
	layerHarvestSpeed = 0;
}

void UnitStats::setValues(const UnitStats &o) {
	maxHp = o.maxHp;
	hpRegeneration = o.hpRegeneration;
	maxSp = o.maxSp;
	spRegeneration = o.spRegeneration;
	maxEp = o.maxEp;
	epRegeneration = o.epRegeneration;
	maxCp = o.maxCp;
	sight = o.sight;
	armor = o.armor;
	expGiven = o.expGiven;
	morale = o.morale;

	attackStrength = o.attackStrength;
    attackLifeLeech = o.attackLifeLeech;
    attackManaBurn = o.attackManaBurn;
    attackCapture = o.attackCapture;
	effectStrength = o.effectStrength;
	attackPctStolen = o.attackPctStolen;
	attackRange = o.attackRange;
	moveSpeed = o.moveSpeed;
	attackSpeed = o.attackSpeed;
	prodSpeed = o.prodSpeed;
	repairSpeed = o.repairSpeed;
	harvestSpeed = o.harvestSpeed;
}

void UnitStats::addStatic(const EnhancementType &e, fixed strength) {
	maxHp += (e.getMaxHp() * strength).intp();
	hpRegeneration += (e.getHpRegeneration() * strength).intp();
	maxSp += (e.getMaxSp() * strength).intp();
	spRegeneration += (e.getSpRegeneration() * strength).intp();
	maxEp += (e.getMaxEp() * strength).intp();
	epRegeneration += (e.getEpRegeneration() * strength).intp();
	maxCp += (e.getMaxCp() * strength).intp();
	sight += (e.getSight() * strength).intp();
	armor += (e.getArmor() * strength).intp();
	expGiven += (e.getExpGiven() * strength).intp();
	morale += (e.getMorale() * strength).intp();
	attackStrength += (e.getAttackStrength() * strength).intp();
    attackLifeLeech += (e.getAttackLifeLeech() * strength).intp();
    attackManaBurn += (e.getAttackManaBurn() * strength).intp();
    attackCapture += (e.getAttackCapture() * strength).intp();
	effectStrength += e.getEffectStrength() * strength;
	attackPctStolen += e.getAttackPctStolen() * strength;
	attackRange += (e.getAttackRange() * strength).intp();
	moveSpeed += (e.getMoveSpeed() * strength).intp();
	attackSpeed += (e.getAttackSpeed() * strength).intp();
	prodSpeed += (e.getProdSpeed() * strength).intp();
	repairSpeed += (e.getRepairSpeed() * strength).intp();
	harvestSpeed += (e.getHarvestSpeed() * strength).intp();
}

void UnitStats::applyMultipliers(const EnhancementType &e) {
	maxHp = (maxHp * e.getMaxHpMult()).intp();
	hpRegeneration = (hpRegeneration * e.getHpRegenerationMult()).intp();
	maxSp = (maxSp * e.getMaxSpMult()).intp();
	spRegeneration = (spRegeneration * e.getSpRegenerationMult()).intp();
	maxEp = (maxEp * e.getMaxEpMult()).intp();
	epRegeneration = (epRegeneration * e.getEpRegenerationMult()).intp();
	maxCp = (maxCp * e.getMaxCpMult()).intp();
	sight = std::max((sight * e.getSightMult()).intp(), 1);
	armor = (armor * e.getArmorMult()).intp();
	expGiven = (expGiven * e.getExpGivenMult()).intp();
	morale = (morale * e.getMoraleMult()).intp();
	attackStrength = (attackStrength * e.getAttackStrengthMult()).intp();
    attackLifeLeech = (attackLifeLeech * e.getAttackLifeLeechMult()).intp();
    attackManaBurn = (attackManaBurn * e.getAttackManaBurnMult()).intp();
    attackCapture = (attackCapture * e.getAttackCaptureMult()).intp();
	effectStrength = effectStrength * e.getEffectStrengthMult();
	attackPctStolen = attackPctStolen * e.getAttackPctStolenMult();
	attackRange = (attackRange * e.getAttackRangeMult()).intp();
	moveSpeed = (moveSpeed * e.getMoveSpeedMult()).intp();
	attackSpeed = (attackSpeed * e.getAttackSpeedMult()).intp();
	prodSpeed = (prodSpeed * e.getProdSpeedMult()).intp();
	repairSpeed = (repairSpeed * e.getRepairSpeedMult()).intp();
	harvestSpeed = (harvestSpeed * e.getHarvestSpeedMult()).intp();
}

void UnitStats::sanitiseUnitStats() {
	if (maxHp < 0) maxHp = 0;
	if (maxSp < 0) maxSp = 0;
	if (maxEp < 0) maxEp = 0;
    if (maxCp <= 0) maxCp = -1;
	if (sight < 0) sight = 0;
	if (armor < 0) armor = 0;
	if (expGiven < 0) expGiven = 0;
	if (morale < 0) morale = 0;
	if (attackStrength < 0) attackStrength = 0;
    if (attackLifeLeech < 0) attackLifeLeech = 0;
    if (attackManaBurn < 0) attackManaBurn = 0;
    if (attackCapture < 0) attackCapture = 0;
	if (effectStrength < 0) effectStrength = 0;
	if (attackPctStolen < 0) attackPctStolen = 0;
	if (attackRange < 0) attackRange = 0;
	if (moveSpeed < 0) moveSpeed = 0;
	if (attackSpeed < 0) attackSpeed = 0;
	if (prodSpeed < 0) prodSpeed = 0;
	if (repairSpeed < 0) repairSpeed = 0;
	if (harvestSpeed < 0) harvestSpeed = 0;
}

// legacy load for Unit class
bool UnitStats::load(const XmlNode *baseNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
	bool loadOk = true;
	//maxHp
	try { maxHp = baseNode->getChildIntValue("max-hp"); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        //hpRegeneration
	    hpRegeneration = baseNode->getChild("max-hp")->getIntAttribute("regeneration");
	    }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	//maxEp
	try { maxEp = baseNode->getChildIntValue("max-ep"); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		//epRegeneration
		XmlAttribute *epRegenAttr = baseNode->getChild("max-ep")->getAttribute("regeneration", false);
		epRegeneration = epRegenAttr ? epRegenAttr->getIntValue() : 0;
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
    //maxSp
	try { maxSp = baseNode->getChildIntValue("max-sp"); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        //spRegeneration
	    spRegeneration = baseNode->getChild("max-sp")->getIntAttribute("regeneration");
	    }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
    try { maxCp = baseNode->getOptionalIntValue("max-cp"); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	//sight
	try { sight = baseNode->getChildIntValue("sight"); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	//armor
	try { armor = baseNode->getChildIntValue("armor"); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	//exp given
	try { expGiven = baseNode->getOptionalIntValue("exp-given"); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	//morale
	try { morale = baseNode->getOptionalIntValue("morale"); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void UnitStats::doChecksum(Checksum &checksum) const {
	checksum.add(maxHp);
	checksum.add(hpRegeneration);
	checksum.add(maxSp);
	checksum.add(spRegeneration);
	checksum.add(maxEp);
	checksum.add(epRegeneration);
	checksum.add(maxCp);
	checksum.add(sight);
	checksum.add(armor);
	checksum.add(expGiven);
	checksum.add(morale);

	checksum.add(attackStrength);
    checksum.add(attackLifeLeech);
    checksum.add(attackManaBurn);
    checksum.add(attackCapture);
	checksum.add(effectStrength);
	checksum.add(attackPctStolen);
	checksum.add(attackRange);
	checksum.add(moveSpeed);
	checksum.add(attackSpeed);
	checksum.add(prodSpeed);
	checksum.add(repairSpeed);
	checksum.add(harvestSpeed);
}

void UnitStats::save(XmlNode *node) const {
	node->addChild("max-hp", maxHp);
	node->addChild("hp-regeneration", hpRegeneration);
	node->addChild("max-sp", maxSp);
	node->addChild("sp-regeneration", spRegeneration);
	node->addChild("max-ep", maxEp);
	node->addChild("ep-regeneration", epRegeneration);
	node->addChild("max-cp", maxCp);
	node->addChild("sight", sight);
	node->addChild("armor", armor);
	node->addChild("exp-given", expGiven);
	node->addChild("morale", morale);
	node->addChild("attack-strength", attackStrength);
    node->addChild("attack-life-leech", attackLifeLeech);
    node->addChild("attack-mana-burn", attackManaBurn);
    node->addChild("attack-capture", attackCapture);
	node->addChild("effect-strength", effectStrength);
	node->addChild("attack-percent-stolen", attackPctStolen);
	node->addChild("attack-range", attackRange);
	node->addChild("move-speed", moveSpeed);
	node->addChild("attack-speed", attackSpeed);
	node->addChild("production-speed", prodSpeed);
	node->addChild("repair-speed", repairSpeed);
	node->addChild("harvest-speed", harvestSpeed);
}

// ===============================
//  class EnhancementType
// ===============================

// ==================== misc ====================

void EnhancementType::reset() {
	UnitStats::reset();
	maxHpMult = 1;
	hpRegenerationMult = 1;
	maxSpMult = 1;
	spRegenerationMult = 1;
	maxEpMult = 1;
	epRegenerationMult = 1;
	maxCpMult = 1;
	sightMult = 1;
	armorMult = 1;
	expGivenMult = 1;
	moraleMult = 1;
	attackStrengthMult = 1;
    attackLifeLeechMult = 1;
    attackManaBurnMult = 1;
    attackCaptureMult = 1;
	effectStrengthMult = 1;
	attackPctStolenMult = 1;
	attackRangeMult = 1;
	moveSpeedMult = 1;
	attackSpeedMult = 1;
	prodSpeedMult = 1;
	repairSpeedMult = 1;
	harvestSpeedMult = 1;
	hpBoost = epBoost = spBoost = 0;

    layerMaxHpMult = 1;
	layerHpRegenerationMult = 1;
	layerMaxSpMult = 1;
	layerSpRegenerationMult = 1;
	layerMaxEpMult = 1;
	layerEpRegenerationMult = 1;
	layerMaxCpMult = 1;
	layerSightMult = 1;
	layerArmorMult = 1;
	layerExpGivenMult = 1;
	layerMoraleMult = 1;
	layerAttackStrengthMult = 1;
    layerAttackLifeLeechMult = 1;
    layerAttackManaBurnMult = 1;
    layerAttackCaptureMult = 1;
	layerEffectStrengthMult = 1;
	layerAttackPctStolenMult = 1;
	layerAttackRangeMult = 1;
	layerMoveSpeedMult = 1;
	layerAttackSpeedMult = 1;
	layerProdSpeedMult = 1;
	layerRepairSpeedMult = 1;
	layerHarvestSpeedMult = 1;
	layerHpBoost = layerEpBoost = layerSpBoost = 0;
}

void EnhancementType::save(XmlNode *node) const {
	XmlNode *m = node->addChild("multipliers");

	m->addChild("max-hp", maxHpMult);
	m->addChild("hp-regeneration", hpRegenerationMult);
	m->addChild("hp-boost", hpBoost);
	m->addChild("max-sp", maxSpMult);
	m->addChild("sp-regeneration", spRegenerationMult);
	m->addChild("sp-boost", spBoost);
	m->addChild("max-ep", maxEpMult);
	m->addChild("ep-regeneration", epRegenerationMult);
	m->addChild("ep-boost", epBoost);
	m->addChild("max-cp", maxCpMult);
	m->addChild("sight", sightMult);
	m->addChild("armor", armorMult);
	m->addChild("exp-given", expGivenMult);
	m->addChild("morale", moraleMult);

	m->addChild("attack-strength", attackStrengthMult);
    m->addChild("attack-life-leech", attackLifeLeechMult);
    m->addChild("attack-mana-burn", attackManaBurnMult);
    m->addChild("attack-capture", attackCaptureMult);
	m->addChild("effect-strength", effectStrengthMult);
	m->addChild("attack-percent-stolen", attackPctStolenMult);
	m->addChild("attack-range", attackRangeMult);
	m->addChild("move-speed", moveSpeedMult);
	m->addChild("attack-speed", attackSpeedMult);
	m->addChild("production-speed", prodSpeedMult);
	m->addChild("repair-speed", repairSpeedMult);
	m->addChild("harvest-speed", harvestSpeedMult);
}

void EnhancementType::addMultipliers(const EnhancementType &e, fixed strength){
	maxHpMult += (e.getMaxHpMult() - 1) * strength;
	hpRegenerationMult += (e.getHpRegenerationMult() - 1) * strength;
	maxSpMult += (e.getMaxSpMult() - 1) * strength;
	spRegenerationMult += (e.getSpRegenerationMult() - 1) * strength;
	maxEpMult += (e.getMaxEpMult() - 1) * strength;
	epRegenerationMult += (e.getEpRegenerationMult() - 1) * strength;
	maxCpMult += (e.getMaxCpMult() - 1) * strength;
	sightMult += (e.getSightMult() - 1) * strength;
	armorMult += (e.getArmorMult() - 1) * strength;
	expGivenMult += (e.getExpGivenMult() - 1) * strength;
	moraleMult += (e.getMoraleMult() - 1) * strength;
	attackStrengthMult += (e.getAttackStrengthMult() - 1) * strength;
    attackLifeLeechMult += (e.getAttackLifeLeechMult() - 1) * strength;
    attackManaBurnMult += (e.getAttackManaBurnMult() - 1) * strength;
    attackCaptureMult += (e.getAttackCaptureMult() - 1) * strength;
	effectStrengthMult += (e.getEffectStrengthMult() - 1) * strength;
	attackPctStolenMult += (e.getAttackPctStolenMult() - 1) * strength;
	attackRangeMult += (e.getAttackRangeMult() - 1) * strength;
	moveSpeedMult += (e.getMoveSpeedMult() - 1) * strength;
	attackSpeedMult += (e.getAttackSpeedMult() - 1) * strength;
	prodSpeedMult += (e.getProdSpeedMult() - 1) * strength;
	repairSpeedMult += (e.getRepairSpeedMult() - 1) * strength;
	harvestSpeedMult += (e.getHarvestSpeedMult() - 1) * strength;
}

void EnhancementType::clampMultipliers() {
	fixed low;
	low.raw() = (1 << 6); // lowest 'fixed' fraction that is 'safe' to multiply
	if (maxHpMult <= 0) {
		maxHpMult = low;
	}
	if (hpRegenerationMult <= 0) {
		hpRegenerationMult = low;
	}
	if (maxSpMult <= 0) {
		maxSpMult = low;
	}
	if (spRegenerationMult <= 0) {
		spRegenerationMult = low;
	}
	if (maxEpMult <= 0) {
		maxEpMult = low;
	}
	if (epRegenerationMult <= 0) {
		epRegenerationMult = low;
	}
	if (maxCpMult <= 0) {
		maxCpMult = low;
	}
	if (sightMult <= 0) {
		sightMult = low;
	}
	if (armorMult <= 0) {
		armorMult = low;
	}
	if (expGivenMult <= 0) {
		expGivenMult = low;
	}
	if (moraleMult <= 0) {
		moraleMult = low;
	}
	if (attackStrengthMult <= 0) {
		attackStrengthMult = low;
	}
    if (attackLifeLeechMult <= 0) {
    attackLifeLeechMult = low;
	}
    if (attackManaBurnMult <= 0) {
    attackManaBurnMult = low;
	}
	if (attackCaptureMult <= 0) {
    attackCaptureMult = low;
	}
	if (effectStrengthMult <= 0) {
		effectStrengthMult = low;
	}
	if (attackPctStolenMult <= 0) {
		attackPctStolenMult = low;
	}
	if (attackRangeMult <= 0) {
		attackRangeMult = low;
	}
	if (attackSpeedMult <= 0) {
		attackSpeedMult = low;
	}
	if (moveSpeedMult <= 0) {
		moveSpeedMult = low;
	}
	if (prodSpeedMult <= 0) {
		prodSpeedMult = low;
	}
	if (repairSpeedMult <= 0) {
		repairSpeedMult = low;
	}
	if (harvestSpeedMult <= 0) {
		harvestSpeedMult = low;
	}
}

void formatModifier(string &str, const char *pre, const char* label, int value, fixed multiplier) {
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

void EnhancementType::getDesc(string &str, const char *pre) const {
	formatModifier(str, pre, "Hp", maxHp, maxHpMult);
	formatModifier(str, pre, "HpRegeneration", hpRegeneration, hpRegenerationMult);
	formatModifier(str, pre, "Sight", sight, sightMult);
	formatModifier(str, pre, "Ep", maxEp, maxEpMult);
	formatModifier(str, pre, "EpRegeneration", epRegeneration, epRegenerationMult);
    formatModifier(str, pre, "Sp", maxSp, maxSpMult);
	formatModifier(str, pre, "SpRegeneration", spRegeneration, spRegenerationMult);
	formatModifier(str, pre, "Cp", maxCp, maxCpMult);
	formatModifier(str, pre, "AttackStrength", attackStrength, attackStrengthMult);
    formatModifier(str, pre, "AttackLifeLeech", attackLifeLeech, attackLifeLeechMult);
    formatModifier(str, pre, "AttackManaBurn", attackManaBurn, attackManaBurnMult);
    formatModifier(str, pre, "AttackCapture", attackCapture, attackCaptureMult);
	formatModifier(str, pre, "EffectStrength", (effectStrength * 100).intp(), effectStrengthMult);
	formatModifier(str, pre, "AttackPctStolen", (attackPctStolen * 100).intp(), attackPctStolenMult);
	formatModifier(str, pre, "AttackSpeed", attackSpeed, attackSpeedMult);
	formatModifier(str, pre, "AttackDistance", attackRange, attackRangeMult);
	formatModifier(str, pre, "Armor", armor, armorMult);
	formatModifier(str, pre, "ExpGiven", expGiven, expGivenMult);
	formatModifier(str, pre, "Morale", morale, moraleMult);
	formatModifier(str, pre, "WalkSpeed", moveSpeed, moveSpeedMult);
	formatModifier(str, pre, "ProductionSpeed", prodSpeed, prodSpeedMult);
	formatModifier(str, pre, "RepairSpeed", repairSpeed, repairSpeedMult);
	formatModifier(str, pre, "HarvestSpeed", harvestSpeed, harvestSpeedMult);
	addBoostsDesc(str, pre, hpBoost, epBoost, spBoost);
}

//Initialize value from <static-modifiers>
void EnhancementType::initStaticModifier(const XmlNode *node, const string &dir) {
	const string &name = node->getName();
    int value = node->getAttribute("value")->getIntValue();
    int additive = node->getAttribute("additive")->getIntValue();
	if (name == "max-hp") {
		maxHp = value;
        layerMaxHp = additive;
	} else if (name == "max-sp") {
		maxSp = value;
        layerMaxSp = additive;
	} else if (name == "max-ep") {
		maxEp = value;
		layerMaxEp = additive;
    } else if (name == "max-cp") {
		maxCp = value;
		layerMaxCp = additive;
	} else if (name == "hp-regeneration") {
		hpRegeneration = value;
		layerHpRegeneration = additive;
	} else if (name == "sp-regeneration") {
		spRegeneration = value;
		layerSpRegeneration = additive;
	} else if (name == "ep-regeneration") {
		epRegeneration = value;
		layerEpRegeneration = additive;
	} else if (name == "sight") {
		sight = value;
		layerSight = additive;
	} else if (name == "armor") {
		armor = value;
		layerArmor = additive;
	} else if (name == "exp-given") {
		expGiven = value;
		layerExpGiven = additive;
	} else if (name == "morale") {
		morale = value;
		layerMorale = additive;
	} else if (name == "attack-strength") {
        attackStrength = value;
        layerAttackStrength = additive;
    } else if (name == "attack-life-leech") {
        attackLifeLeech = value;
        layerAttackLifeLeech = additive;
	} else if (name == "attack-mana-burn") {
		attackManaBurn = value;
		layerAttackManaBurn = additive;
    } else if (name == "attack-capture") {
		attackCapture = value;
		layerAttackCapture = additive;
	} else if (name == "effect-strength") {
		effectStrength = fixed(value) / 100;
		layerEffectStrength = fixed(additive) / 100;
	} else if (name == "attack-percent-stolen") {
		attackPctStolen	= fixed(value) / 100;
		layerAttackPctStolen = fixed(additive) / 100;
	} else if (name == "attack-range") {
		attackRange = value;
		layerAttackRange = additive;
	} else if (name == "move-speed") {
		moveSpeed = value;
		layerMoveSpeed = additive;
	} else if (name == "attack-speed") {
		attackSpeed = value;
		layerAttackSpeed = additive;
	} else if (name == "production-speed") {
		prodSpeed = value;
		layerProdSpeed = additive;
	} else if (name == "repair-speed") {
		repairSpeed = value;
		layerRepairSpeed = additive;
	} else if (name == "harvest-speed") {
		harvestSpeed = value;
		layerHarvestSpeed = additive;
	} else {
		throw runtime_error("Not a valid child of <static-modifiers>: " + name + ": " + dir);
	}
}

//Initialize value from <multipliers>
void EnhancementType::initMultiplier(const XmlNode *node, const string &dir) {
	const string &name = node->getName();
	fixed value = node->getOptionalFloatValue("value");
	fixed multiplier = node->getOptionalFloatValue("multiplier");
	if (name == "max-hp") {
		maxHpMult = value;
		layerMaxHpMult = multiplier;
	} else if (name == "max-ep") {
		maxEpMult = value;
		layerMaxEpMult = multiplier;
	} else if (name == "max-sp") {
		maxSpMult = value;
		layerMaxSpMult = multiplier;
    } else if (name == "max-cp") {
		maxCpMult = value;
		layerMaxCpMult = multiplier;
	} else if (name == "hp-regeneration") {
		hpRegenerationMult = value;
		layerHpRegenerationMult = multiplier;
	} else if (name == "sp-regeneration") {
		spRegenerationMult = value;
		layerSpRegenerationMult = multiplier;
	} else if (name == "ep-regeneration") {
		epRegenerationMult = value;
		layerEpRegenerationMult = multiplier;
	} else if (name == "sight") {
		sightMult = value;
		layerSightMult = multiplier;
	} else if (name == "armor") {
		armorMult = value;
		layerArmorMult = multiplier;
	} else if (name == "exp-given") {
		expGivenMult = value;
		layerExpGivenMult = multiplier;
	} else if (name == "morale") {
		moraleMult = value;
		layerMoraleMult = multiplier;
	} else if (name == "attack-strength") {
		attackStrengthMult = value;
		layerAttackStrengthMult = multiplier;
	} else if (name == "attack-life-leech") {
		attackLifeLeechMult = value;
		layerAttackLifeLeechMult = multiplier;
	} else if (name == "attack-mana-burn") {
		attackManaBurnMult = value;
		layerAttackManaBurnMult = multiplier;
    } else if (name == "attack-capture") {
		attackCaptureMult = value;
		layerAttackCaptureMult = multiplier;
	} else if (name == "effect-strength") {
		effectStrengthMult = value;
		layerEffectStrengthMult = multiplier;
	} else if (name == "attack-percent-stolen") {
		attackPctStolenMult = value;
		layerAttackPctStolenMult = multiplier;
	} else if (name == "attack-range") {
		attackRangeMult = value;
		layerAttackRangeMult = multiplier;
	} else if (name == "move-speed") {
		moveSpeedMult = value;
		layerMoveSpeedMult = multiplier;
	} else if (name == "attack-speed") {
		attackSpeedMult = value;
		layerAttackSpeedMult = multiplier;
	} else if (name == "production-speed") {
		prodSpeedMult = value;
		layerProdSpeedMult = multiplier;
	} else if (name == "repair-speed") {
		repairSpeedMult = value;
		layerRepairSpeedMult = multiplier;
	} else if (name == "harvest-speed") {
		harvestSpeedMult = value;
		layerHarvestSpeedMult = multiplier;
	} else {
		throw runtime_error("Not a valid child of <multipliers>: <" + name + ">: " + dir);
	}
}

bool EnhancementType::load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = true;
	// static modifiers
	try {
        const XmlNode *addNode = baseNode->getChild("static-modifiers", 0, false);
		if(addNode) {
			for (int i = 0; i < addNode->getChildCount(); ++i) {
				initStaticModifier(addNode->getChild(i), dir);
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	// multipliers
	try {
        const XmlNode *multNode = baseNode->getChild("multipliers", 0, false);
		if (multNode) {
			for (int i = 0; i < multNode->getChildCount(); ++i) {
				initMultiplier(multNode->getChild(i), dir);
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	// stat boost
	try {
        const XmlNode *node = baseNode->getChild("point-boosts", 0, false);
		if (node) {
			hpBoost = node->getOptionalIntValue("hp-boost");
			epBoost = node->getOptionalIntValue("ep-boost");
			spBoost = node->getOptionalIntValue("sp-boost");
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void EnhancementType::doChecksum(Checksum &checksum) const {
	UnitStats::doChecksum(checksum);
	checksum.add(maxHpMult);
	checksum.add(hpRegenerationMult);
	checksum.add(hpBoost);
	checksum.add(maxSpMult);
	checksum.add(spRegenerationMult);
	checksum.add(spBoost);
	checksum.add(maxEpMult);
	checksum.add(epRegenerationMult);
	checksum.add(epBoost);
	checksum.add(sightMult);
	checksum.add(armorMult);
	checksum.add(expGivenMult);
	checksum.add(moraleMult);
	checksum.add(attackStrengthMult);
	checksum.add(attackLifeLeechMult);
	checksum.add(attackManaBurnMult);
	checksum.add(attackCaptureMult);
	checksum.add(effectStrengthMult);
	checksum.add(attackPctStolenMult);
	checksum.add(attackRangeMult);
	checksum.add(moveSpeedMult);
	checksum.add(attackSpeedMult);
	checksum.add(prodSpeedMult);
	checksum.add(repairSpeedMult);
	checksum.add(harvestSpeedMult);
}

}}//end namespace
