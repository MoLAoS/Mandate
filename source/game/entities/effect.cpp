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
#include <map>

#include "effect.h"
#include "unit.h"
#include "conversion.h"
#include "tech_tree.h"
#include "network_message.h"
#include "world.h"
#include "program.h"
#include "sim_interface.h"

#include "leak_dumper.h"

namespace Glest { namespace Entities {

using namespace Shared::Util;

// =====================================================
//  class Effect
// =====================================================

MEMORY_CHECK_IMPLEMENTATION(Effect)

// ============================ Constructor & destructor =============================

Effect::Effect(const EffectType* type, Unit *source, Effect *root, fixed strength,
		const Unit *recipient, const TechTree *tt) {
	this->type = type;
	this->source = source->getId();
	this->root = root;
	this->strength = strength;
	this->duration = type->getDuration();
	this->recourse = root != NULL;
	if (type->getHpRegeneration() < 0 && type->getDamageType()) {
		fixed fregen = type->getHpRegeneration() 
			* tt->getDamageMultiplier(type->getDamageType(), recipient->getType()->getArmourType());
		this->actualHpRegen = fregen.intp();
	} else {
		this->actualHpRegen = type->getHpRegeneration();
	}
}

Effect::Effect(const XmlNode *node) {
	source = node->getOptionalIntValue("source", -1);
	const TechTree *tt = World::getCurrWorld()->getTechTree();
	root = NULL;
	type = tt->getEffectType(node->getChildStringValue("type"));
	strength = node->getChildFixedValue("strength");
	duration = node->getChildIntValue("duration");
	recourse = node->getChildBoolValue("recourse");
	actualHpRegen = node->getChildIntValue("actualHpRegen");
}

Effect::~Effect() {
	if (Unit *unit = g_simInterface.getUnitFactory().getUnit(source)) {
		unit->effectExpired(this);
	}
}

void Effect::save(XmlNode *node) const {
	node->addChild("source", source);
	//FIXME: how should I save the root?
	node->addChild("type", type->getName());
	node->addChild("strength", strength);
	node->addChild("duration", duration);
	node->addChild("recourse", recourse);
	node->addChild("actualHpRegen", actualHpRegen);
}

// =====================================================
//  class Effects
// =====================================================


// ============================ Constructor & destructor =============================

Effects::Effects() {
	dirty = true;
}

Effects::Effects(const XmlNode *node) {
	clear();
	for(int i = 0; i < node->getChildCount(); ++i) {
		push_back(new Effect(node->getChild("effect", i)));
	}
	dirty = true;
}

Effects::~Effects() {
	for (iterator i = begin(); i != end(); i++) {
		(*i)->clearSource();
		(*i)->clearRoot();
		delete *i;
	}
}

// ============================ misc =============================

bool Effects::add(Effect *e){
	EffectStacking es = e->getType()->getStacking();
	if (es == EffectStacking::STACK) {
		push_back(e);
		dirty = true;
		return true;
	}

	for (iterator i = begin(); i != end(); i++) {
		if((*i)->getType() == e->getType() && (*i)->getSource() == e->getSource() && (*i)->getRoot() == e->getRoot() ){
			switch (es) {
				case EffectStacking::EXTEND:
					(*i)->setDuration((*i)->getDuration() + e->getDuration());
					if((*i)->getStrength() < e->getStrength()) {
						(*i)->setStrength(e->getStrength());
					}
					delete e;
					dirty = true;
					return false;

				case EffectStacking::OVERWRITE:
					delete *i;
					erase(i);
					push_back(e);
					dirty = true;
					return true;

				case EffectStacking::REJECT:
					delete e;
					return false;

				case EffectStacking::STACK:; // tell compiler to shut up
				case EffectStacking::COUNT:;
			}
		}
	}
	// previous effect wasn't found, add it as new
	push_back(e);
	dirty = true;
	return true;
}

void Effects::remove(Effect *e) {
	for (iterator i = begin(); i != end(); i++) {
		if(*i == e) {
			list<Effect*>::remove(e);
			dirty = true;
			return;
		}
	}
}

void Effects::clearRootRef(Effect *e) {
	for(const_iterator i = begin(); i != end(); i++) {
		if((*i)->getRoot() == e) {
			(*i)->clearRoot();
		}
	}
}

void Effects::tick() {
	for(iterator i = begin(); i != end();) {
		Effect *e = *i;

		if(e->tick()) {
			delete e;
			i = erase(i);
			dirty = true;
		} else {
			i++;
		}
	}
}

struct EffectSummary {
	int maxDuration;
	int count;
};

void Effects::streamDesc(ostream &stream) const {
	string str;
	getDesc(str);
	str = formatString(str);
	stream << str;
}

void Effects::getDesc(string &str) const {
	map<const EffectType*, EffectSummary> uniqueEffects;
	map<const EffectType*, EffectSummary>::iterator uei;
	bool printedFirst = false;
	Lang &lang= Lang::getInstance();

	for(const_iterator i = begin(); i != end(); i++) {
		const EffectType *type = (*i)->getType();
		if(type->isDisplay()) {
			uei = uniqueEffects.find(type);
			if(uei != uniqueEffects.end()) {
				uniqueEffects[type].count++;
				if(uniqueEffects[type].maxDuration < (*i)->getDuration()) {
					uniqueEffects[type].maxDuration = (*i)->getDuration();
				}
			} else {
				uniqueEffects[type].count = 1;
				uniqueEffects[type].maxDuration = (*i)->getDuration();
			}
		}
	}

	for(uei = uniqueEffects.begin(); uei != uniqueEffects.end(); uei++) {
		if(printedFirst){
			str += "\n    ";
		} else {
			str += "\n" + lang.get("Effects") + ": ";
		}
		string rawName = (*uei).first->getName();
		string name = lang.getFactionString(uei->first->getFactionType()->getName(), rawName);
		str += name + " (" + intToStr((*uei).second.maxDuration) + ")";
		if((*uei).second.count > 1) {
			str += " x" + intToStr((*uei).second.count);
		}
		printedFirst = true;
	}
}

// ====================================== get ======================================

//who killed Kenny?
Unit *Effects::getKiller() const {
	for (const_iterator i = begin(); i != end(); i++) {
		Unit *source = g_simInterface.getUnitFactory().getUnit((*i)->getSource());
		//If more than two other units hit this unit with a DOT and it died,
		//credit goes to the one 1st in the list.

		if ((*i)->getType()->getHpRegeneration() < 0 && source != NULL && source->isAlive()) {
			return source;
		}
	}

	return NULL;
}

void Effects::save(XmlNode *node) const {
	for(const_iterator i = begin(); i != end(); ++i) {
		(*i)->save(node->addChild("effect"));
	}
}


}}//end namespace
