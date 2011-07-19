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
	maxEp = 0;
	epRegeneration = 0;
	sight = 0;
	armor = 0;

	attackStrength = 0;
	effectStrength = 0;
	attackPctStolen = 0;
	attackRange = 0;
	moveSpeed = 0;
	attackSpeed = 0;
	prodSpeed = 0;
	repairSpeed = 0;
	harvestSpeed = 0;
}

void UnitStats::setValues(const UnitStats &o) {
	maxHp = o.maxHp;
	hpRegeneration = o.hpRegeneration;
	maxEp = o.maxEp;
	epRegeneration = o.epRegeneration;
	sight = o.sight;
	armor = o.armor;

	attackStrength = o.attackStrength;
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
	maxEp += (e.getMaxEp() * strength).intp();
	epRegeneration += (e.getEpRegeneration() * strength).intp();
	sight += (e.getSight() * strength).intp();
	armor += (e.getArmor() * strength).intp();
	attackStrength += (e.getAttackStrength() * strength).intp();
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
	maxEp = (maxEp * e.getMaxEpMult()).intp();
	epRegeneration = (epRegeneration * e.getEpRegenerationMult()).intp();
	sight = std::max((sight * e.getSightMult()).intp(), 1);
	armor = (armor * e.getArmorMult()).intp();
	attackStrength = (attackStrength * e.getAttackStrengthMult()).intp();
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
	if (maxEp < 0) maxEp = 0;
	if (sight < 0) sight = 0;
	if (armor < 0) armor = 0;
	if (attackStrength < 0) attackStrength = 0;
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
	//hpRegeneration
	try { hpRegeneration = baseNode->getChild("max-hp")->getIntAttribute("regeneration"); }
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
	return loadOk;
}

void UnitStats::doChecksum(Checksum &checksum) const {
	checksum.add(maxHp);
	checksum.add(hpRegeneration);
	checksum.add(maxEp);
	checksum.add(epRegeneration);
	checksum.add(sight);
	checksum.add(armor);

	checksum.add(attackStrength);
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
	node->addChild("max-ep", maxHp);
	node->addChild("hp-regeneration", hpRegeneration);
	node->addChild("max-ep", maxEp);
	node->addChild("ep-regeneration", epRegeneration);
	node->addChild("sight", sight);
	node->addChild("armor", armor);

	node->addChild("attack-strength", attackStrength);
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
	maxEpMult = 1;
	epRegenerationMult = 1;
	sightMult = 1;
	armorMult = 1;
	attackStrengthMult = 1;
	effectStrengthMult = 1;
	attackPctStolenMult = 1;
	attackRangeMult = 1;
	moveSpeedMult = 1;
	attackSpeedMult = 1;
	prodSpeedMult = 1;
	repairSpeedMult = 1;
	harvestSpeedMult = 1;
	hpBoost = epBoost = 0;
}

void EnhancementType::save(XmlNode *node) const {
	XmlNode *m = node->addChild("multipliers");

	m->addChild("max-ep", maxHpMult);
	m->addChild("hp-regeneration", hpRegenerationMult);
	m->addChild("hp-boost", hpBoost);
	m->addChild("max-ep", maxEpMult);
	m->addChild("ep-regeneration", epRegenerationMult);
	m->addChild("ep-boost", epBoost);
	m->addChild("sight", sightMult);
	m->addChild("armor", armorMult);

	m->addChild("attack-strength", attackStrengthMult);
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
	maxEpMult += (e.getMaxEpMult() - 1) * strength;
	epRegenerationMult += (e.getEpRegenerationMult() - 1) * strength;
	sightMult += (e.getSightMult() - 1) * strength;
	armorMult += (e.getArmorMult() - 1) * strength;
	attackStrengthMult += (e.getAttackStrengthMult() - 1) * strength;
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
	if (maxEpMult <= 0) {
		maxEpMult = low;
	}
	if (epRegenerationMult <= 0) {
		epRegenerationMult = low;
	}
	if (sightMult <= 0) {
		sightMult = low;
	}
	if (armorMult <= 0) {
		armorMult = low;
	}
	if (attackStrengthMult <= 0) {
		attackStrengthMult = low;
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

void addBoostsDesc(string &str, const char *pre, int hpBoost, int epBoost) {
	if (hpBoost) {
		str += pre + g_lang.get("HpBoost") + ": " + intToStr(hpBoost);
	}
	if (epBoost) {
		str += pre + g_lang.get("EpBoost") + ": " + intToStr(epBoost);
	}
}

void EnhancementType::getDesc(string &str, const char *pre) const {
	formatModifier(str, pre, "Hp", maxHp, maxHpMult);
	formatModifier(str, pre, "HpRegeneration", hpRegeneration, hpRegenerationMult);
	formatModifier(str, pre, "Sight", sight, sightMult);
	formatModifier(str, pre, "Ep", maxEp, maxEpMult);
	formatModifier(str, pre, "EpRegeneration", epRegeneration, epRegenerationMult);
	formatModifier(str, pre, "AttackStrength", attackStrength, attackStrengthMult);
	formatModifier(str, pre, "EffectStrength", (effectStrength * 100).intp(), effectStrengthMult);
	formatModifier(str, pre, "AttackPctStolen", (attackPctStolen * 100).intp(), attackPctStolenMult);
	formatModifier(str, pre, "AttackSpeed", attackSpeed, attackSpeedMult);
	formatModifier(str, pre, "AttackDistance", attackRange, attackRangeMult);
	formatModifier(str, pre, "Armor", armor, armorMult);
	formatModifier(str, pre, "WalkSpeed", moveSpeed, moveSpeedMult);
	formatModifier(str, pre, "ProductionSpeed", prodSpeed, prodSpeedMult);
	formatModifier(str, pre, "RepairSpeed", repairSpeed, repairSpeedMult);
	formatModifier(str, pre, "HarvestSpeed", harvestSpeed, harvestSpeedMult);
	addBoostsDesc(str, pre, hpBoost, epBoost);
}

//Initialize value from <static-modifiers>
void EnhancementType::initStaticModifier(const XmlNode *node, const string &dir) {
	const string &name = node->getName();
	int value = node->getAttribute("value")->getIntValue();

	if (name == "max-hp") {
		maxHp = value;
	} else if (name == "max-ep") {
		maxEp = value;
	} else if (name == "hp-regeneration") {
		hpRegeneration = value;
	} else if (name == "ep-regeneration") {
		epRegeneration = value;
	} else if (name == "sight") {
		sight = value;
	} else if (name == "armor") {
		armor = value;
	} else if (name == "attack-strength") {
		attackStrength = value;
	} else if (name == "effect-strength") {
		effectStrength = fixed(value) / 100;
	} else if (name == "attack-percent-stolen") {
		attackPctStolen	= fixed(value) / 100;
	} else if (name == "attack-range") {
		attackRange = value;
	} else if (name == "move-speed") {
		moveSpeed = value;
	} else if (name == "attack-speed") {
		attackSpeed = value;
	} else if (name == "production-speed") {
		prodSpeed = value;
	} else if (name == "repair-speed") {
		repairSpeed = value;
	} else if (name == "harvest-speed") {
		harvestSpeed = value;
	} else {
		throw runtime_error("Not a valid child of <static-modifiers>: " + name + ": " + dir);
	}
}

//Initialize value from <multipliers>
void EnhancementType::initMultiplier(const XmlNode *node, const string &dir) {
	const string &name = node->getName();
	fixed value = node->getAttribute("value")->getFixedValue();

	if (name == "max-hp") {
		maxHpMult = value;
	} else if (name == "max-ep") {
		maxEpMult = value;
	} else if (name == "hp-regeneration") {
		hpRegenerationMult = value;
	} else if (name == "ep-regeneration") {
		epRegenerationMult = value;
	} else if (name == "sight") {
		sightMult = value;
	} else if (name == "armor") {
		armorMult = value;
	} else if (name == "attack-strength") {
		attackStrengthMult = value;
	} else if (name == "effect-strength") {
		effectStrengthMult = value;
	} else if (name == "attack-percent-stolen") {
		attackPctStolenMult = value;
	} else if (name == "attack-range") {
		attackRangeMult = value;
	} else if (name == "move-speed") {
		moveSpeedMult = value;
	} else if (name == "attack-speed") {
		attackSpeedMult = value;
	} else if (name == "production-speed") {
		prodSpeedMult = value;
	} else if (name == "repair-speed") {
		repairSpeedMult = value;
	} else if (name == "harvest-speed") {
		harvestSpeedMult = value;
	} else {
		throw runtime_error("Not a valid child of <multipliers>: <" + name + ">: " + dir);
	}
}

bool EnhancementType::load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	const XmlNode *node;
	bool loadOk = true;
	// static modifiers
	try {
		node = baseNode->getChild("static-modifiers", 0, false);
		if(node) {
			for (int i = 0; i < node->getChildCount(); ++i) {
				initStaticModifier(node->getChild(i), dir);
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	// multipliers
	try {
		node = baseNode->getChild("multipliers", 0, false);
		if (node) {
			for (int i = 0; i < node->getChildCount(); ++i) {
				initMultiplier(node->getChild(i), dir);
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	// stat boost
	try {
		node = baseNode->getChild("point-boosts", 0, false);
		if (node) {
			hpBoost = node->getOptionalIntValue("hp-boost");
			epBoost = node->getOptionalIntValue("ep-boost");
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
	checksum.add(maxEpMult);
	checksum.add(epRegenerationMult);
	checksum.add(epBoost);
	checksum.add(sightMult);
	checksum.add(armorMult);
	checksum.add(attackStrengthMult);
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
