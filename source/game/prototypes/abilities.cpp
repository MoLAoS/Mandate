// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "abilities.h"
#include "tech_tree.h"
#include "faction.h"
#include "unit.h"
#include "world.h"
#include "sim_interface.h"

namespace Glest { namespace ProtoTypes {

// ===============================
// 	class ItemStore
// ===============================
void ItemStore::init(const ItemType *it, int cap) {
    m_type = it;
    m_cap = cap;
}

// =====================================================
// 	class DamageType
// =====================================================
void DamageType::init(string name, int amount) {
    type_name = name;
    value = amount;
}

// ===============================
// 	class Load Bonus
// ===============================
bool LoadBonus::load(const XmlNode *loadBonusNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
	bool loadOk = true;
    const XmlNode *sourceNode = loadBonusNode->getChild("source");
    const XmlNode *loadableUnitNode = sourceNode->getChild("loadable-unit");
    source = loadableUnitNode->getAttribute("name")->getRestrictedValue();
    const XmlNode *enhancementNode = loadBonusNode->getChild("enhancement", 0, false);
    if (enhancementNode) {
        EnhancementType::load(enhancementNode, dir, techTree, factionType);
    }
	return loadOk;
}

// ===============================
// 	class Level
// ===============================
bool Level::load(const XmlNode *levelNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = true;
	try {
		m_name = levelNode->getAttribute("name")->getRestrictedValue();
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		exp = levelNode->getAttribute("exp")->getIntValue();
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		count = levelNode->getAttribute("count")->getIntValue();
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        if (!enLevel.load(levelNode, dir, tt, ft)) {
            loadOk = false;
        }
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

// =====================================================
// 	class Timer
// =====================================================

void Timer::init(const XmlNode *node, const TechTree *tt) {
	timerValue = node->getChildIntValue("timer");
	currentStep = node->getChildIntValue("step");
}

void Timer::save(XmlNode *node) const {
	node->addChild("timer", timerValue);
	node->addChild("step", currentStep);
}

void Timer::init(int timer, int step) {
    timerValue = timer;
    currentStep = step;
}

// =====================================================
// 	class UnitsOwned
// =====================================================

void UnitsOwned::init(const XmlNode *node, const Faction *f) {
	m_type = f->getType()->getUnitType(node->getChildStringValue("type"));
	m_limit = node->getChildIntValue("limit");
}

void UnitsOwned::save(XmlNode *node) const {
	node->addChild("type", m_type->getName());
	node->addChild("owned", m_owned);
	node->addChild("limit", m_limit);
}

void UnitsOwned::init(const UnitType *ut, int owned, int limit) {
    m_type = ut;
    m_owned = owned;
    m_limit = limit;
}

// =====================================================
// 	class Equipment
// =====================================================
void Equipment::init(int newMax, int newCurrent, string newName, string newTypeTag) {
    max = newMax;
    current = newCurrent;
    typeTag = newTypeTag;
    name = newName;
}

// =====================================================
// 	class Attackers
// =====================================================
void Attacker::init(Unit *unit, int value) {
    attacker = unit;
    timer = value;
}

}}//end namespace
