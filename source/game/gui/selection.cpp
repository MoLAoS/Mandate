// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "selection.h"

#include <algorithm>

#include "unit_type.h"
#include "user_interface.h"

#include "sim_interface.h"
#include "program.h"

#include "leak_dumper.h"

namespace Glest { namespace Gui {

// =====================================================
// 	class Selection
// =====================================================

Selection::Selection()	{
	m_gui = NULL;
}

void Selection::init(UserInterface *m_gui, int m_factionIndex) {
	m_empty = true;
	m_uniform = false;
	m_commandable = false;
	m_tranported = false;
	m_cancelable = false;
	m_meetable = false;
	m_canRepair = false;
	m_autoCmdStates[AutoCmdFlag::REPAIR] = AutoCmdState::NONE;
	this->m_factionIndex = m_factionIndex;
	this->m_gui = m_gui;
}

Selection::~Selection(){
	//if (!m_selectedUnits.empty()) {
	//	// clear list
	//	foreach (UnitVector, it, m_selectedUnits) {
	//		(*it)->StateChanged.disconnect(this);
	//	}
	//	m_selectedUnits.clear();
	//}
}

void Selection::select(Unit *unit){
	// check selection size, if unit already selected or dead, multi-selection and m_enemy
	if (m_selectedUnits.size() >= maxUnits
	|| std::find(m_selectedUnits.begin(), m_selectedUnits.end(), unit) != m_selectedUnits.end()
	|| unit->isDead()
	|| (!unit->getType()->getMultiSelect() && !isEmpty())
	|| (unit->getFactionIndex() != m_factionIndex && !isEmpty())) {
		return;
	}
	// check existing m_enemy
	if (m_selectedUnits.size() == 1 && m_selectedUnits.front()->getFactionIndex() != m_factionIndex) {
		clear();
	}
	// check existing multisel
	if (m_selectedUnits.size() == 1 && !m_selectedUnits.front()->getType()->getMultiSelect()) {
		clear();
	}
	m_selectedUnits.push_back(unit);
	unit->StateChanged.connect(this, &Selection::onUnitStateChanged);
	update();
}

/// remove units from current selction
void Selection::unSelect(const UnitVector &units) {
	foreach_const (UnitVector, it, units) {
		foreach (UnitVector, it2, m_selectedUnits) {
			if (*it == *it2) {
				unSelect(it2);
				break;
			}
		}
	}
}

/// remove unit from current selction
void Selection::unSelect(const Unit *unit) {
	foreach (UnitVector, it, m_selectedUnits) {
		if (*it == unit) {
			unSelect(it);
			break;
		}
	}
}

void Selection::unSelectAllOfType(const UnitType *type) {
	UnitVector units;
	foreach (UnitVector, it, m_selectedUnits) {
		if ((*it)->getType() != type) {
			units.push_back(*it);
		}
	}
	clear();
	select(units);
}

void Selection::unSelectAllNotOfType(const UnitType *type) {
	UnitVector units;
	foreach (UnitVector, it, m_selectedUnits) {
		if ((*it)->getType() == type) {
			units.push_back(*it);
		}
	}
	clear();
	select(units);
}

void Selection::unSelect(UnitVector::iterator it) {
	RUNTIME_CHECK(*it != 0 && this != 0);
	// remove unit from list
	(*it)->StateChanged.disconnect(this);
	m_selectedUnits.erase(it);
	update();
	m_gui->onSelectionChanged();
}

void Selection::clear(){
	if (m_selectedUnits.empty()) {
		return;
	}
	// clear list
	foreach (UnitVector, it, m_selectedUnits) {
		(*it)->StateChanged.disconnect(this);
	}
	m_selectedUnits.clear();
	update();
}

bool Selection::isSharedCommandClass(CmdClass commandClass){
	for (int i = 0; i < getCount(); ++i) {
		if (!getUnit(i)->getFirstAvailableCt(commandClass)) {
			return false;
		}
	}
	return true;
}

Vec3f Selection::getRefPos() const{
	RUNTIME_CHECK(!getFrontUnit()->isCarried());
	return getFrontUnit()->getCurrVector();
}

void Selection::assignGroup(int groupIndex){
	// clear group
	m_groups[groupIndex].clear();

	// assign new group
	foreach (UnitVector, it, m_selectedUnits) {
		m_groups[groupIndex].push_back(*it);
	}
}

void Selection::recallGroup(int groupIndex){
	clear();
	foreach (UnitVector, it, m_groups[groupIndex]) {
		RUNTIME_CHECK(*it != NULL && (*it)->isAlive());
		select(*it);
	}
}

void Selection::clearDeadUnits() {
	UnitVector::iterator it = m_selectedUnits.begin();
	bool updt = false;
	while (it != m_selectedUnits.end()) {
		if ((*it)->isDead()) {
			it = m_selectedUnits.erase(it);
			updt = true;
		} else {
			++it;
		}
	}
	if (updt) {
		update();
		m_gui->onSelectionChanged();
	}

	for (int i=0; i < maxGroups; ++i) {
		it = m_groups[i].begin();
		while (it != m_groups[i].end()) {
			if ((*it)->isDead()) {
				it = m_groups[i].erase(it);
			} else {
				++it;
			}
		}
	}
}

void Selection::onUnitStateChanged(Unit *unit) {
	update();
	m_gui->onSelectionStateChanged();
}

struct UnitTypeIdCompare {
	bool operator()(const Unit *u1, const Unit *u2) {
		return u1->getType()->getId() < u2->getType()->getId();
	}
};

void Selection::update() {
	sort(m_selectedUnits.begin(), m_selectedUnits.end(), UnitTypeIdCompare());
	if (m_selectedUnits.empty()) {
		m_empty = true;
		m_enemy = false;
		m_uniform = false;
		m_commandable = false;
		m_tranported = false;
		m_cancelable = false;
		m_meetable = false;
		m_canRepair = false;
		m_canAttack = false;
		m_canMove = false;
		m_canCloak = false;
	} else if (m_selectedUnits.front()->getFactionIndex() != m_factionIndex) {
		m_empty = false;
		m_enemy = true;
		m_uniform = true;
		m_commandable = false;
		m_tranported = false;
		m_cancelable = false;
		m_meetable = false;
		m_canRepair = false;
		m_canAttack = false;
		m_canMove = false;
		m_canCloak = false;
	} else {
		const UnitType *frontUT = m_selectedUnits.front()->getType();
		m_empty = false;
		m_enemy = false;
		m_uniform = true;
		m_commandable = false;
		m_tranported = false;
		m_cancelable = false;
		m_canAttack = true;
		m_canMove = true;
		m_canRepair = true;
		m_canCloak = false;
		if (frontUT->hasCommandClass(CmdClass::REPAIR)) {
			if (m_selectedUnits.front()->isAutoCmdEnabled(AutoCmdFlag::REPAIR)) {
				m_autoCmdStates[AutoCmdFlag::REPAIR] = AutoCmdState::ALL_ON;
			} else {
				m_autoCmdStates[AutoCmdFlag::REPAIR] = AutoCmdState::ALL_OFF;
			}
		} else {
			m_autoCmdStates[AutoCmdFlag::REPAIR] = AutoCmdState::NONE;
		}
		removeCarried(); /// @todo: probably not needed if individual units are removed in load command

		if (frontUT->hasCommandClass(CmdClass::ATTACK)) {
			if (m_selectedUnits.front()->isAutoCmdEnabled(AutoCmdFlag::ATTACK)) {
				m_autoCmdStates[AutoCmdFlag::ATTACK] = AutoCmdState::ALL_ON;
			} else {
				m_autoCmdStates[AutoCmdFlag::ATTACK] = AutoCmdState::ALL_OFF;
			}
		} else {
			m_autoCmdStates[AutoCmdFlag::ATTACK] = AutoCmdState::NONE;
		}

		if (frontUT->hasCommandClass(CmdClass::MOVE)) {
			if (m_selectedUnits.front()->isAutoCmdEnabled(AutoCmdFlag::FLEE)) {
				m_autoCmdStates[AutoCmdFlag::FLEE] = AutoCmdState::ALL_ON;
			} else {
				m_autoCmdStates[AutoCmdFlag::FLEE] = AutoCmdState::ALL_OFF;
			}
		} else {
			m_autoCmdStates[AutoCmdFlag::FLEE] = AutoCmdState::NONE;
		}

		foreach (UnitVector, i, m_selectedUnits) {
			const UnitType *ut = (*i)->getType();
			if (ut != frontUT) {
				m_uniform = false;
			}
			if ((*i)->getCarriedCount()) {
				m_tranported = true;
			}
			if (ut->hasCommandClass(CmdClass::REPAIR)) {
				if (((*i)->isAutoCmdEnabled(AutoCmdFlag::REPAIR))) {
					if (m_autoCmdStates[AutoCmdFlag::REPAIR] == AutoCmdState::ALL_OFF) {
						m_autoCmdStates[AutoCmdFlag::REPAIR] = AutoCmdState::MIXED;
					} else if (m_autoCmdStates[AutoCmdFlag::REPAIR] == AutoCmdState::NONE) {
						m_autoCmdStates[AutoCmdFlag::REPAIR] = AutoCmdState::ALL_ON;
					} // else MIXED or ALL_ON already
				} else {
					if (m_autoCmdStates[AutoCmdFlag::REPAIR] == AutoCmdState::ALL_ON) {
						m_autoCmdStates[AutoCmdFlag::REPAIR] = AutoCmdState::MIXED;
					} else if (m_autoCmdStates[AutoCmdFlag::REPAIR] == AutoCmdState::NONE) {
						m_autoCmdStates[AutoCmdFlag::REPAIR] = AutoCmdState::ALL_OFF;
					} // else MIXED or ALL_OFF already
				}
			}
			if (ut->hasCommandClass(CmdClass::ATTACK)) {
				if (((*i)->isAutoCmdEnabled(AutoCmdFlag::ATTACK))) {
					if (m_autoCmdStates[AutoCmdFlag::ATTACK] == AutoCmdState::ALL_OFF) {
						m_autoCmdStates[AutoCmdFlag::ATTACK] = AutoCmdState::MIXED;
					} else if (m_autoCmdStates[AutoCmdFlag::ATTACK] == AutoCmdState::NONE) {
						m_autoCmdStates[AutoCmdFlag::ATTACK] = AutoCmdState::ALL_ON;
					} // else MIXED or ALL_ON already
				} else {
					if (m_autoCmdStates[AutoCmdFlag::ATTACK] == AutoCmdState::ALL_ON) {
						m_autoCmdStates[AutoCmdFlag::ATTACK] = AutoCmdState::MIXED;
					} else if (m_autoCmdStates[AutoCmdFlag::ATTACK] == AutoCmdState::NONE) {
						m_autoCmdStates[AutoCmdFlag::ATTACK] = AutoCmdState::ALL_OFF;
					} // else MIXED or ALL_OFF already
				}
			}
			if (ut->hasCommandClass(CmdClass::MOVE)) {
				if (((*i)->isAutoCmdEnabled(AutoCmdFlag::FLEE))) {
					if (m_autoCmdStates[AutoCmdFlag::FLEE] == AutoCmdState::ALL_OFF) {
						m_autoCmdStates[AutoCmdFlag::FLEE] = AutoCmdState::MIXED;
					} else if (m_autoCmdStates[AutoCmdFlag::FLEE] == AutoCmdState::NONE) {
						m_autoCmdStates[AutoCmdFlag::FLEE] = AutoCmdState::ALL_ON;
					} // else MIXED or ALL_ON already
				} else {
					if (m_autoCmdStates[AutoCmdFlag::FLEE] == AutoCmdState::ALL_ON) {
						m_autoCmdStates[AutoCmdFlag::FLEE] = AutoCmdState::MIXED;
					} else if (m_autoCmdStates[AutoCmdFlag::FLEE] == AutoCmdState::NONE) {
						m_autoCmdStates[AutoCmdFlag::FLEE] = AutoCmdState::ALL_OFF;
					} // else MIXED or ALL_OFF already
				}
			}
			m_cancelable = m_cancelable || ((*i)->anyCommand()
					&& (*i)->getCurrCommand()->getType()->getClass() != CmdClass::STOP);
			m_commandable = m_commandable || (*i)->isOperative();

			if ((*i)->getType()->getCloakClass() == CloakClass::ENERGY) {
				m_canCloak = true;
			}
		}
		if (m_canRepair && m_autoCmdStates[AutoCmdFlag::REPAIR] == AutoCmdState::NONE) {
			m_canRepair = false;
		}
		m_meetable = m_uniform && m_commandable && frontUT->hasMeetingPoint();
	}
	//in case Game::init() isn't called, eg crash at loading data
	if (m_gui && UserInterface::getCurrentGui()) {
		m_gui->onSelectionUpdated();
	}
}

const Texture2D* Selection::getCommandImage(CmdClass cc) const {
	foreach_const (UnitVector, it, m_selectedUnits) {
		const CommandType *ct = (*it)->getType()->getFirstCtOfClass(cc);
		if (ct) {
			return ct->getImage();
		}
	}
	return 0;
}

const Texture2D* Selection::getCloakImage() const {
	foreach_const (UnitVector, it, m_selectedUnits) {
		if ((*it)->getType()->getCloakImage()) {
			return (*it)->getType()->getCloakImage();
		}
	}
	return 0;
}

void Selection::removeCarried() {
	UnitVector::iterator i = m_selectedUnits.begin();
	while (i != m_selectedUnits.end()) {
		if ((*i)->isCarried()) {
			i = m_selectedUnits.erase(i);
		} else {
			++i;
		}
	}
}

void Selection::load(const XmlNode *node) {
	for(int i = 0; i < node->getChildCount(); ++i) {
		const XmlNode *groupNode = node->getChild("group", i);
		int index = groupNode->getIntAttribute("index");

		if(index < 0 || index > maxGroups) {
			throw runtime_error("invalid group index");
		}
		m_groups[index].clear();
		for(int j = 0; j < groupNode->getChildCount(); ++j) {
			Unit *unit = g_world.getUnit(groupNode->getChildIntValue("unit", j));
			if(unit) {
				m_groups[index].push_back(unit);
			}
		}
	}
}

void Selection::save(XmlNode *node) const {
	for(int i = 0; i < maxGroups; ++i) {
		if(m_groups[i].empty()) {
			continue;
		}
		XmlNode *groupNode = node->addChild("group");
		groupNode->addAttribute("index", i);
		for(UnitVector::const_iterator j = m_groups[i].begin(); j != m_groups[i].end(); ++j) {
			groupNode->addChild("unit", (*j)->getId());
		}
	}
}

}}//end namespace
