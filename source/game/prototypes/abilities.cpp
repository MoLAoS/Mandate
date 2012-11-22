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

namespace Glest { namespace ProtoTypes {

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

LoadBonus::LoadBonus()
{}

void LoadBonus::loadResourceModifier(const XmlNode *node, ResModifierMap &map, const TechTree *techTree) {
	for (int i=0; i < node->getChildCount(); ++i) {
		const XmlNode *resNode = node->getChild(i);
		if (resNode->getName() == "resource") {
			string resName = resNode->getAttribute("name")->getRestrictedValue();
			int addition = 0;
			if (const XmlAttribute *addAttrib = resNode->getAttribute("addition", false)) {
				addition = addAttrib->getIntValue();
			}
			fixed mult = 1;
			if (const XmlAttribute *multAttrib = resNode->getAttribute("multiplier", false)) {
				mult = multAttrib->getFixedValue();
			}
			const ResourceType *rt = techTree->getResourceType(resName);
			if (map.find(rt) != map.end()) {
				throw runtime_error("duplicate resource node '" + resName + "'");
			}
			map[rt] = Modifier(addition, mult);
		}
	}
}

bool LoadBonus::loadNewStyle(const XmlNode *node, const string &dir, const TechTree *techTree,
							   const FactionType *factionType) {
	bool loadOk = true;
		const XmlNode *enhanceNode, *enhancementNode;
		try {
			enhanceNode = node->getChild("enhancement", 0, false);
			enhancementNode = enhanceNode->getChild("effects");
		} catch (runtime_error &e) {
			g_logger.logXmlError(dir, e.what());
			loadOk = false;
		}
		if (!m_enhancement.m_enhancement.load(enhancementNode, dir, techTree, factionType)) {
			loadOk = false;
		}
		try { // creation and storage modifiers
			if (const XmlNode *costModsNode = enhancementNode->getOptionalChild("cost-modifiers")) {
				loadResourceModifier(costModsNode, m_enhancement.m_costModifiers, techTree);
			}
			if (const XmlNode *storeModsNode = enhancementNode->getOptionalChild("store-modifiers")) {
				loadResourceModifier(storeModsNode, m_enhancement.m_storeModifiers, techTree);
			}
			if (const XmlNode *createModsNode = enhancementNode->getOptionalChild("create-modifiers")) {
				loadResourceModifier(createModsNode, m_enhancement.m_createModifiers, techTree);
			}
		} catch (runtime_error e) {
			g_logger.logXmlError(dir, e.what());
			loadOk = false;
		}
	return loadOk;
}

bool LoadBonus::load(const XmlNode *loadBonusNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
	bool loadOk = true;
    const XmlNode *sourceNode = loadBonusNode->getChild("source");
    const XmlNode *loadableUnitNode = sourceNode->getChild("loadable-unit");
    source = loadableUnitNode->getAttribute("name")->getRestrictedValue();
    const XmlNode *enhancementsNode = loadBonusNode->getChild("enhancements", 0, false);
    if (enhancementsNode) {
    loadOk = loadNewStyle(enhancementsNode, dir, techTree, factionType) && loadOk;
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
// 	class CreatedUnit
// =====================================================

void CreatedUnit::init(const XmlNode *node, const Faction *f) {
	m_type = f->getType()->getUnitType(node->getChildStringValue("type"));
	m_amount = node->getChildIntValue("amount");
	m_amount_plus = node->getChildIntValue("plus");
	m_amount_multiply = node->getChildFloatValue("multiply");
	m_cap = node->getChildIntValue("cap");
}

void CreatedUnit::save(XmlNode *node) const {
	node->addChild("type", m_type->getName());
	node->addChild("amount", m_amount);
	node->addChild("plus", m_amount_plus);
	node->addChild("multiply", m_amount_multiply);
	node->addChild("cap", m_cap);
}

void CreatedUnit::init(const UnitType *ut, int amount, int amount_plus, fixed amount_multiply, int cap) {
    m_type = ut;
    m_amount = amount;
    m_amount_plus = amount_plus;
    m_amount_multiply = amount_multiply;
    m_cap = cap;
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
// 	class CreatedItem
// =====================================================

void CreatedItem::init(const XmlNode *node, const Faction *f) {
	m_type = f->getType()->getItemType(node->getChildStringValue("type"));
	m_amount = node->getChildIntValue("amount");
	m_amount_plus = node->getChildIntValue("plus");
	m_amount_multiply = node->getChildFloatValue("multiply");
	m_cap = node->getChildIntValue("cap");
}

void CreatedItem::save(XmlNode *node) const {
	node->addChild("type", m_type->getName());
	node->addChild("amount", m_amount);
	node->addChild("plus", m_amount_plus);
	node->addChild("multiply", m_amount_multiply);
	node->addChild("cap", m_cap);
}

void CreatedItem::init(const ItemType *it, int amount, int amount_plus, fixed amount_multiply, int cap) {
    m_type = it;
    m_amount = amount;
    m_amount_plus = amount_plus;
    m_amount_multiply = amount_multiply;
    m_cap = cap;
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
