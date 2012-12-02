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

Effect::Effect(CreateParams params)
		: m_id(-1) {
	this->type = params.type;
	this->source = params.source ? params.source->getId() : -1;
	this->root = params.root;
	this->strength = params.strength;
	this->duration = type->getDuration();
	this->recourse = params.root != NULL;
	this->actualHpRegen = type->getHpRegeneration();
	if (type->getHpRegeneration() < 0) {
        if (type->getDamageClass()) {
            fixed fregen = type->getHpRegeneration()
                * params.tt->getDamageMultiplier(type->getDamageClass(), params.recipient->getType()->getArmourType());
            this->actualHpRegen = fregen.intp();
        } else {
            this->actualHpRegen = type->getHpRegeneration();
        }
    } else if (type->getDamageTypeCount() > 0) {
        for (int i = 0; i < type->getDamageTypeCount(); ++i) {
            DamageType damage = type->getDamageType(i);
            for (int j = 0; j < params.recipient->resistances.size(); ++j) {
                DamageType resistance = params.recipient->resistances[j];
                if (damage.getTypeName() == resistance.getTypeName()) {
                    int damageChange = damage.getValue() - resistance.getValue();
                    if (damageChange > 0) {
                        this->actualHpRegen -= damageChange;
                    }
                }
            }
        }
    }
}

Effect::Effect(const XmlNode *node) {
	m_id = node->getChildIntValue("id");
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
	if (World::isConstructed()) {
		if (Unit *unit = g_world.getUnit(source)) {
			unit->effectExpired(this);
		}
	}
}

void Effect::save(XmlNode *node) const {
	node->addChild("id", m_id);
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
		push_back(g_world.newEffect(node->getChild("effect", i)));
	}
	dirty = true;
}

Effects::~Effects() {
	if (World::isConstructed()) {
		for (iterator i = begin(); i != end(); ++i) {
			(*i)->clearSource();
			(*i)->clearRoot();
			g_world.deleteEffect(*i);
		}
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

	for (iterator i = begin(); i != end(); ++i) {
		if((*i)->getType() == e->getType() && (*i)->getSource() == e->getSource() && (*i)->getRoot() == e->getRoot() ){
			switch (es) {
				case EffectStacking::EXTEND:
					(*i)->setDuration((*i)->getDuration() + e->getDuration());
					if((*i)->getStrength() < e->getStrength()) {
						(*i)->setStrength(e->getStrength());
					}
					g_world.deleteEffect(e);
					dirty = true;
					return false;

				case EffectStacking::OVERWRITE:
					g_world.deleteEffect(*i);
					erase(i);
					push_back(e);
					dirty = true;
					return true;

				case EffectStacking::REJECT:
					g_world.deleteEffect(e);
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
	for (iterator i = begin(); i != end(); ++i) {
		if(*i == e) {
			list<Effect*>::remove(e);
			dirty = true;
			return;
		}
	}
}

void Effects::clearRootRef(Effect *e) {
	for(const_iterator i = begin(); i != end(); ++i) {
		if((*i)->getRoot() == e) {
			(*i)->clearRoot();
		}
	}
}

void Effects::tick() {
	for(iterator i = begin(); i != end();) {
		Effect *e = *i;

		if(e->tick()) {
			g_world.deleteEffect(e);
			i = erase(i);
			dirty = true;
		} else {
			++i;
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

	for(const_iterator i = begin(); i != end(); ++i) {
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

	for(uei = uniqueEffects.begin(); uei != uniqueEffects.end(); ++uei) {
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
	for (const_iterator i = begin(); i != end(); ++i) {
		Unit *source = g_world.getUnit((*i)->getSource());
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
