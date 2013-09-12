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
void Equipment::init(int newMax, int newCurrent, string newTypeTag, const ItemType *newItemType, Item *newItem) {
    max = newMax;
    current = newCurrent;
    typeTag = newTypeTag;
    itemType = newItemType;
    item = newItem;
}

void Equipment::getDesc(string &str, const char *pre, string name) {
    str += pre;
    str += "Type: " + typeTag;
    str += " |Amount: " + intToStr(max);
}

void Equipment::save(XmlNode *node) const {
    node->addAttribute("type", typeTag);
    node->addAttribute("value", max);
}

// =====================================================
// 	class Attackers
// =====================================================
void Attacker::init(Unit *unit, int value) {
    attacker = unit;
    timer = value;
}

}}//end namespace
