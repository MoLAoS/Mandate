// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
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
	m_cap = node->getChildFloatValue("cap");
}

void CreatedUnit::save(XmlNode *node) const {
	node->addChild("type", m_type->getName());
	node->addChild("amount", m_amount);
	node->addChild("plus", m_amount_plus);
	node->addChild("multiply", m_amount_multiply);
	node->addChild("cap", m_cap);
}

void CreatedUnit::init(const UnitType *ut, int amount, int amount_plus, float amount_multiply, int cap) {
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
	m_limit = node->getChildFloatValue("limit");
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

}}//end namespace
