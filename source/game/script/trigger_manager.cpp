// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2009-2010 James McCulloch
//                2009-2010 Nathan Turner
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "script_manager.h"

#include "world.h"
#include "lang.h"
#include "game_camera.h"
#include "game.h"
#include "renderer.h"

#include "leak_dumper.h"
#include "sim_interface.h"

namespace Glest { namespace Script {

using namespace Sim;

// =====================================================
//	class ScriptTimer
// =====================================================

bool ScriptTimer::isReady() const {
	if (real) {
		return Chrono::getCurMillis() >= targetTime;
	} else {
		return g_world.getFrameCount() >= targetTime;
	}
}

void ScriptTimer::reset() {
	if (real) {
		targetTime = Chrono::getCurMillis() + interval;
	} else {
		targetTime = g_world.getFrameCount() + interval;
	}
}

// =====================================================
//	class TriggerManager
// =====================================================

TriggerManager::~TriggerManager() {
	for (Regions::iterator it = regions.begin(); it != regions.end(); ++it) {
		delete it->second;
	}
}

void TriggerManager::reset() {
	deleteMapValues(regions.begin(), regions.end());
	regions.clear();
	events.clear();
	unitPosTriggers.clear();
	attackedTriggers.clear();
	hpBelowTriggers.clear();
	hpAboveTriggers.clear();
	commandCallbacks.clear();
}

bool TriggerManager::registerRegion(const string &name, const Rect &rect) {
	if (regions.find(name) != regions.end()) return false;
 	Region *region = new Rect(rect);
 	regions[name] = region;
 	return true;
 }

int TriggerManager::registerEvent(const string &name) {
	if (events.find(name) != events.end()) return -1;
	events.insert(name);
	return 0;
}

SetTriggerRes TriggerManager::addCommandCallback(int unitId, const string &eventName, int userData) {
	Unit *unit = g_world.findUnitById(unitId);
	if (!unit || !unit->isAlive()) return SetTriggerRes::BAD_UNIT_ID;
	unit->setCommandCallback();
	commandCallbacks[unitId].evnt = eventName;
	commandCallbacks[unitId].user_dat = userData;
	return SetTriggerRes::OK;
}

SetTriggerRes TriggerManager::addHPBelowTrigger(int unitId, int threshold, const string &eventName, int userData) {
	Unit *unit = g_world.findUnitById(unitId);
	if (!unit || !unit->isAlive()) return SetTriggerRes::BAD_UNIT_ID;
	if (unit->getHp() < threshold) return SetTriggerRes::INVALID_THRESHOLD;
	unit->setHPBelowTrigger(threshold);
	hpBelowTriggers[unit->getId()].evnt = eventName;
	hpBelowTriggers[unit->getId()].user_dat = userData;
	return SetTriggerRes::OK;
}

SetTriggerRes TriggerManager::addHPAboveTrigger(int unitId, int threshold, const string &eventName, int userData) {
	Unit *unit = g_world.findUnitById(unitId);
	if (!unit || !unit->isAlive()) return SetTriggerRes::BAD_UNIT_ID;
	if (unit->getHp() > threshold) return SetTriggerRes::INVALID_THRESHOLD;
	unit->setHPAboveTrigger(threshold);
	hpAboveTriggers[unit->getId()].evnt = eventName;
	hpAboveTriggers[unit->getId()].user_dat = userData;
	return SetTriggerRes::OK;
}

SetTriggerRes TriggerManager::addAttackedTrigger(int unitId, const string &eventName, int userData) {
	Unit *unit = g_world.findUnitById(unitId);
	if (!unit || !unit->isAlive()) return SetTriggerRes::BAD_UNIT_ID;
	attackedTriggers[unitId].evnt = eventName;
	attackedTriggers[unitId].user_dat = userData;
	unit->setAttackedTrigger(true);
	return SetTriggerRes::OK;
}

SetTriggerRes TriggerManager::addDeathTrigger(int unitId, const string &eventName, int userData) {
	Unit *unit = g_world.findUnitById(unitId);
	if (!unit || !unit->isAlive()) return SetTriggerRes::BAD_UNIT_ID;
	deathTriggers[unitId].evnt = eventName;
	deathTriggers[unitId].user_dat = userData;
	return SetTriggerRes::OK;
}

void TriggerManager::unitMoved(const Unit *unit) {
	// check id
	int id = unit->getId();
	PosTriggerMap::iterator tmit = unitPosTriggers.find(id);

	if (tmit != unitPosTriggers.end()) { // if any pos triggers for this specific unit
		PosTriggers &triggers = tmit->second;
	start:
		PosTriggers::iterator it = triggers.begin();
 		while (it != triggers.end()) {
			if (it->region->isInside(unit->getPos())) {
				// setting another trigger on this unit in response to this trigger will cause our
				// iterators here to go bad... :(
				string evnt = it->evnt;
				int ud = it->user_dat;
				it = triggers.erase(it);
				ScriptManager::onTrigger(evnt, id, ud);
				goto start;
 			} else {
 				++it;
 			}
		}
	}
	tmit = factionPosTriggers.find(unit->getFactionIndex());
	if (tmit != factionPosTriggers.end()) { // if any pos triggers for this unit's faction
		PosTriggers &triggers = tmit->second;
	start2:
		PosTriggers::iterator it = triggers.begin();
 		while (it != triggers.end()) {
			if (it->region->isInside(unit->getPos())) {
				string evnt = it->evnt;
				int ud = it->user_dat;
				it = triggers.erase(it);
				ScriptManager::onTrigger(evnt, id, ud);
				goto start2;
 			} else {
 				++it;
 			}
		}
	}
}

void TriggerManager::unitDied(const Unit *unit) {
	const int &id = unit->getId();
	unitPosTriggers.erase(id);
	commandCallbacks.erase(id);
	attackedTriggers.erase(id);
	hpBelowTriggers.erase(id);
	hpAboveTriggers.erase(id);
	TriggerMap::iterator it = deathTriggers.find(id);
	if (it != deathTriggers.end()) {
		ScriptManager::onTrigger(it->second.evnt, id, it->second.user_dat);
		deathTriggers.erase(it);
	}
}

void TriggerManager::checkTrigger(TriggerMap &triggerMap, const Unit *unit) {
	TriggerMap::iterator it = triggerMap.find(unit->getId());
	if (it == triggerMap.end()) return;
	string evnt = it->second.evnt;
	int ud = it->second.user_dat;
	triggerMap.erase(it);
	ScriptManager::onTrigger(evnt, unit->getId(), ud);
}

/** @return 0 if ok, -1 if bad unit id, -2 if event not found, -3 region not found,
  * -4 unit already has a trigger for this region,event pair */
SetTriggerRes TriggerManager::addUnitPosTrigger	(int unitId, const string &region, const string &eventName, int userData) {
	//g_logger.logProgramEvent("adding unit="+intToStr(unitId)+ ", event=" + eventName + " trigger");
	Unit *unit = g_world.findUnitById(unitId);
	if (!unit) return SetTriggerRes::BAD_UNIT_ID;
	if (events.find(eventName) == events.end()) return SetTriggerRes::UNKNOWN_EVENT;
	Region *rgn = NULL;
	if (regions.find(region) != regions.end()) rgn = regions[region];
	if (!rgn) return SetTriggerRes::UNKNOWN_REGION;
	if (unitPosTriggers.find(unitId) == unitPosTriggers.end()) {
		unitPosTriggers.insert(pair<int,PosTriggers>(unitId,PosTriggers()));
 	}
	PosTriggers &triggers = unitPosTriggers.find(unitId)->second;
	PosTriggers::iterator it = triggers.begin();
	for (; it != triggers.end(); ++it) {
		if (it->region == rgn && it->evnt == eventName) {
			return SetTriggerRes::DUPLICATE_TRIGGER;
		}
 	}
	triggers.push_back(PosTrigger());
	triggers.back().region = rgn;
	triggers.back().evnt = eventName;
	triggers.back().user_dat = userData;
	return SetTriggerRes::OK;
}

bool TriggerManager::removeUnitPosTriggers(int unitId) {
	PosTriggerMap::iterator it = unitPosTriggers.find(unitId);
	if (it != unitPosTriggers.end()) {
		unitPosTriggers.erase(it);
		return true;
	}
	return false;
}

/** @return 0 if ok, -1 if bad index id, -2 if event not found, -3 region not found,
  * -4 faction already has a trigger for this region,event pair */
SetTriggerRes TriggerManager::addFactionPosTrigger (int ndx, const string &region, const string &eventName, int userData) {
	//g_logger.logProgramEvent("adding unit="+intToStr(unitId)+ ", event=" + eventName + " trigger");
	if (ndx < 0 || ndx >= GameConstants::maxPlayers) return SetTriggerRes::BAD_FACTION_INDEX;
	if (events.find(eventName) == events.end()) return SetTriggerRes::UNKNOWN_EVENT;
	Region *rgn = NULL;
	if (regions.find(region) != regions.end()) rgn = regions[region];
	if (!rgn) return SetTriggerRes::UNKNOWN_REGION;

	if (factionPosTriggers.find(ndx) == factionPosTriggers.end()) {
		factionPosTriggers.insert(pair<int,PosTriggers>(ndx,PosTriggers()));
 	}
	PosTriggers &triggers = factionPosTriggers.find(ndx)->second;
	PosTriggers::iterator it = triggers.begin();
	for (; it != triggers.end(); ++it) {
		if (it->region == rgn && it->evnt == eventName) {
			return SetTriggerRes::DUPLICATE_TRIGGER;
		}
 	}
	triggers.push_back(PosTrigger());
	triggers.back().region = rgn;
	triggers.back().evnt = eventName;
	triggers.back().user_dat = userData;
	return SetTriggerRes::OK;
}

}}
