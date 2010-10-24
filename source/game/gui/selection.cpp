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
	this->factionIndex = factionIndex;
	this->gui = gui;
}

Selection::~Selection(){
	clear();
}

void Selection::incRef(Unit *u) {
	UnitRefMap::iterator it = m_referenceMap.find(u);
	if (it != m_referenceMap.end()) {
		++it->second;
	} else {
		m_referenceMap[u] = 1;
		u->StateChanged.connect(this, &Selection::onUnitStateChanged);
		u->Died.connect(this, &Selection::onUnitDied);
	}
}

void Selection::decRef(Unit *u) {
	UnitRefMap::iterator it = m_referenceMap.find(u);
	assert(it != m_referenceMap.end() && it->second > 0);
	if (it->second == 1) {
		m_referenceMap.erase(u);
		u->StateChanged.disconnect(this);
		u->Died.disconnect(this);
	} else {
		--it->second;
	}
}

void Selection::select(Unit *unit){

	// check size
	if (selectedUnits.size() >= maxUnits) {
		return;
	}

	// check if already selected
	for (int i=0; i < selectedUnits.size(); ++i) {
		if (selectedUnits[i] == unit) {
			return;
		}
	}

	// check if dead
	if (unit->isDead()) {
		return;
	}

	// check if multisel
	if (!unit->getType()->getMultiSelect() && !isEmpty()) {
		return;
	}

	// check if enemy
	if (unit->getFactionIndex() != factionIndex && !isEmpty()) {
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
	incRef(unit);
	update();
//	gui->onSelectionChanged();
}
/*
void Selection::select(const UnitVector &units){

	//add units to gui
	for(UnitIterator it= units.begin(); it!=units.end(); ++it){
		select(*it);
	}
}
*/

/// remove units from current selction
void Selection::unSelect(const UnitVector &units){
	for(UnitIterator it= units.begin(); it!=units.end(); ++it){
		for(int i=0; i<selectedUnits.size(); ++i){
			if(selectedUnits[i]==*it){
				unSelect(i);
			}
		}
	}
}

/// remove unit from current selction
void Selection::unSelect(const Unit *unit){
	for(int i = 0; i < selectedUnits.size(); ++i) {
		if(selectedUnits[i] == unit) {
			unSelect(i);
		}
	}
}

void Selection::unSelectAllOfType(const UnitType *type) {
	UnitVector units;
	for (UnitIterator it = selectedUnits.begin(); it != selectedUnits.end(); ++it) {
		if ((*it)->getType() != type) {
			units.push_back(*it);
		}
	}
	clear();
	select(units);
}

void Selection::unSelectAllNotOfType(const UnitType *type) {
	UnitVector units;
	for (UnitIterator it = selectedUnits.begin(); it != selectedUnits.end(); ++it) {
		if ((*it)->getType() == type) {
			units.push_back(*it);
		}
	}
	clear();
	select(units);
}

void Selection::unSelect(int i){
	//remove unit from list
	decRef(selectedUnits[i]);
	selectedUnits.erase(selectedUnits.begin()+i);
	update();
	gui->onSelectionChanged();
}

void Selection::clear(){
	//clear list
	foreach (UnitVector, it, selectedUnits) {
		decRef(*it);
	}
	selectedUnits.clear();
	update();
}

Vec3f Selection::getRefPos() const{
	RUNTIME_CHECK(!getFrontUnit()->isCarried());
	return getFrontUnit()->getCurrVector();
}

void Selection::assignGroup(int groupIndex){
	//clear group
	foreach (UnitVector, it, groups[groupIndex]) {
		decRef(*it);
	}
	groups[groupIndex].clear();

	//assign new group
	for(int i=0; i<selectedUnits.size(); ++i){
		groups[groupIndex].push_back(selectedUnits[i]);
		incRef(selectedUnits[i]);
	}
}

void Selection::recallGroup(int groupIndex){
	clear();
	for (int i=0; i < groups[groupIndex].size(); ++i) {
		RUNTIME_CHECK(groups[groupIndex][i] != NULL && groups[groupIndex][i]->isAlive());
		select(groups[groupIndex][i]);
	}
}

void Selection::onUnitDied(Unit *unit) {
	
	// mutex ... this wll be called in the Simulation thread
	
	// prevent resetting Gui if a unit in a selection group dies
	bool needUpdate = false;

	//remove from selection
	for (int i = 0; i < selectedUnits.size(); ++i) {
		if (selectedUnits[i] == unit) {
			selectedUnits.erase(selectedUnits.begin() + i);
			needUpdate = true;
			break;
		}
	}

	//remove from groups
	for (int i = 0; i < maxGroups; ++i) {
		for (int j = 0; j < groups[i].size(); ++j) {
			if (groups[i][j] == unit) {
				groups[i].erase(groups[i].begin() + j);
				break;
			}
		}
	}

	//notify gui & stuff
	if(needUpdate) {
		update();
		gui->onSelectionChanged();
	}
}

void Selection::onUnitStateChanged(Unit *unit) {

	// mutex ... this wll be called in the Simulation thread

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
		const UnitType *frontUT= selectedUnits.front()->getType();
		empty = false;
		enemy = true;
		uniform = true;
		commandable = false;
		cancelable = false;
		canRepair = true;
		autoRepairState = selectedUnits.front()->isAutoRepairEnabled() ? arsOn : arsOff;

		removeCarried(); /// @todo: probably not needed if individual units are removed in load command

		for(UnitVector::iterator i = selectedUnits.begin(); i != selectedUnits.end(); ++i) {
			const UnitType *ut = (*i)->getType();
			if(ut != frontUT){
				uniform = false;
			}
			if(ut->hasCommandClass(CommandClass::REPAIR)) {
				if(((*i)->isAutoRepairEnabled() ? arsOn : arsOff) != autoRepairState) {
					autoRepairState = arsMixed;
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
