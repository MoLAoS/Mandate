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
	gui = NULL;
}

void Selection::init(UserInterface *gui, int factionIndex) {
	empty = true;
	uniform = false;
	commandable = false;
	cancelable = false;
	meetable = false;
	canRepair = false;
	m_autoCmdStates[AutoCmdFlag::REPAIR] = AutoCmdState::NONE;
	this->factionIndex = factionIndex;
	this->gui = gui;
}

Selection::~Selection(){
	clear();
}

void Selection::select(Unit *unit){
	// check selection size, if unit already selected or dead, multi-selection and enemy
	if (selectedUnits.size() >= maxUnits
	|| std::find(selectedUnits.begin(), selectedUnits.end(), unit) != selectedUnits.end()
	|| unit->isDead()
	|| (!unit->getType()->getMultiSelect() && !isEmpty())
	|| (unit->getFactionIndex() != factionIndex && !isEmpty())) {
		return;
	}
	// check existing enemy
	if (selectedUnits.size() == 1 && selectedUnits.front()->getFactionIndex() != factionIndex) {
		clear();
	}
	// check existing multisel
	if (selectedUnits.size() == 1 && !selectedUnits.front()->getType()->getMultiSelect()) {
		clear();
	}
	selectedUnits.push_back(unit);
	unit->StateChanged.connect(this, &Selection::onUnitStateChanged);
	update();
}

/// remove units from current selction
void Selection::unSelect(const UnitVector &units) {
	foreach_const (UnitVector, it, units) {
		foreach (UnitVector, it2, selectedUnits) {
			if (*it == *it2) {
				unSelect(it2);
				break;
			}
		}
	}
}

/// remove unit from current selction
void Selection::unSelect(const Unit *unit) {
	foreach (UnitVector, it, selectedUnits) {
		if (*it == unit) {
			unSelect(it);
			break;
		}
	}
}

void Selection::unSelectAllOfType(const UnitType *type) {
	UnitVector units;
	foreach (UnitVector, it, selectedUnits) {
		if ((*it)->getType() != type) {
			units.push_back(*it);
		}
	}
	clear();
	select(units);
}

void Selection::unSelectAllNotOfType(const UnitType *type) {
	UnitVector units;
	foreach (UnitVector, it, selectedUnits) {
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
	selectedUnits.erase(it);
	update();
	gui->onSelectionChanged();
}

void Selection::clear(){
	// clear list
	foreach (UnitVector, it, selectedUnits) {
		(*it)->StateChanged.disconnect(this);
	}
	selectedUnits.clear();
	update();
}

Vec3f Selection::getRefPos() const{
	RUNTIME_CHECK(!getFrontUnit()->isCarried());
	return getFrontUnit()->getCurrVector();
}

void Selection::assignGroup(int groupIndex){
	// clear group
	groups[groupIndex].clear();

	// assign new group
	foreach (UnitVector, it, selectedUnits) {
		groups[groupIndex].push_back(*it);
	}
}

void Selection::recallGroup(int groupIndex){
	clear();
	foreach (UnitVector, it, groups[groupIndex]) {
		RUNTIME_CHECK(*it != NULL && (*it)->isAlive());
		select(*it);
	}
}

void Selection::clearDeadUnits() {
	UnitVector::iterator it = selectedUnits.begin();
	bool updt = false;
	while (it != selectedUnits.end()) {
		if ((*it)->isDead()) {
			it = selectedUnits.erase(it);
			updt = true;
		} else {
			++it;
		}
	}
	if (updt) {
		update();
		gui->onSelectionChanged();
	}

	for (int i=0; i < maxGroups; ++i) {
		it = groups[i].begin();
		while (it != groups[i].end()) {
			if ((*it)->isDead()) {
				it = groups[i].erase(it);
			} else {
				++it;
			}
		}
	}
}

void Selection::onUnitStateChanged(Unit *unit) {
	update();
	gui->onSelectionStateChanged();
}

void Selection::update() {
	if (selectedUnits.empty()) {
		empty = true;
		enemy = false;
		uniform = false;
		commandable = false;
		cancelable = false;
		meetable = false;
		canRepair = false;
	} else if (selectedUnits.front()->getFactionIndex() != factionIndex) {
		empty = false;
		enemy = true;
		uniform = true;
		commandable = false;
		cancelable = false;
		meetable = false;
		canRepair = false;
	} else {
		const UnitType *frontUT = selectedUnits.front()->getType();
		empty = false;
		enemy = false;
		uniform = true;
		commandable = false;
		cancelable = false;
		canRepair = true;
		if (frontUT->hasCommandClass(CommandClass::REPAIR)) {
			if (selectedUnits.front()->isAutoRepairEnabled()) {
				m_autoCmdStates[AutoCmdFlag::REPAIR] = AutoCmdState::ALL_ON;
			} else {
				m_autoCmdStates[AutoCmdFlag::REPAIR] = AutoCmdState::ALL_OFF;
			}
		} else {
			m_autoCmdStates[AutoCmdFlag::REPAIR] = AutoCmdState::NONE;
		}
		removeCarried(); /// @todo: probably not needed if individual units are removed in load command

		foreach (UnitVector, i, selectedUnits) {
			const UnitType *ut = (*i)->getType();
			if (ut != frontUT) {
				uniform = false;
			}
			if (ut->hasCommandClass(CommandClass::REPAIR)) {
				if (((*i)->isAutoRepairEnabled())) {
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
			} else {
				canRepair = false;
			}
			cancelable = cancelable || ((*i)->anyCommand()
					&& (*i)->getCurrCommand()->getType()->getClass() != CommandClass::STOP);
			commandable = commandable || (*i)->isOperative();
		}
		meetable = uniform && commandable && frontUT->hasMeetingPoint();
	}
	//in case Game::init() isn't called, eg crash at loading data
	if (gui && UserInterface::getCurrentGui()) {
		gui->onSelectionUpdated();
	}
}

void Selection::removeCarried() {
	UnitVector::iterator i = selectedUnits.begin();
	while (i != selectedUnits.end()) {
		if ((*i)->isCarried()) {
			i = selectedUnits.erase(i);
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
		groups[index].clear();
		for(int j = 0; j < groupNode->getChildCount(); ++j) {
			Unit *unit = g_simInterface->getUnitFactory().getUnit(groupNode->getChildIntValue("unit", j));
			if(unit) {
				groups[index].push_back(unit);
			}
		}
	}
}

void Selection::save(XmlNode *node) const {
	for(int i = 0; i < maxGroups; ++i) {
		if(groups[i].empty()) {
			continue;
		}
		XmlNode *groupNode = node->addChild("group");
		groupNode->addAttribute("index", i);
		for(UnitVector::const_iterator j = groups[i].begin(); j != groups[i].end(); ++j) {
			groupNode->addChild("unit", (*j)->getId());
		}
	}
}

}}//end namespace
