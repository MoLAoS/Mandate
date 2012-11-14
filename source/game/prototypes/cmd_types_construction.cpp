// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//                2008-2009 Daniel Santos
//                2009-2010 Nathan Turner
//                2009-2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "command_type.h"

#include <algorithm>
#include <cassert>
#include <climits>

#include "unit_type.h"
#include "sound.h"
#include "util.h"
#include "graphics_interface.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "renderer.h"
#include "world.h"
#include "unit.h"
#include "sound_renderer.h"
#include "route_planner.h"
#include "cartographer.h"
#include "script_manager.h"
#include "game.h"

#include "leak_dumper.h"
#include "logger.h"
#include "sim_interface.h"

using namespace Glest::Sim;

using namespace Shared::Util;

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class StructureCommandType
// =====================================================

StructureCommandType::~StructureCommandType(){
	deleteValues(m_builtSounds.getSounds().begin(), m_builtSounds.getSounds().end());
	deleteValues(m_startSounds.getSounds().begin(), m_startSounds.getSounds().end());
}

bool StructureCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
    bool loadOk = CommandType::load(n, dir, tt, ft);

	try {
		string skillName = n->getChild("build-skill")->getAttribute("value")->getRestrictedValue();
		m_buildSkillType = static_cast<const BuildSkillType*>(unitType->getSkillType(skillName, SkillClass::BUILD));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	//buildings built
	try {
		const XmlNode *buildingsNode = n->getChild("buildings");
		for(int i=0; i<buildingsNode->getChildCount(); ++i){
			const XmlNode *buildingNode = buildingsNode->getChild("building", i);
			string name = buildingNode->getAttribute("name")->getRestrictedValue();
			m_buildings.push_back(ft->getUnitType(name));
			try {
				m_tipKeys[name] = buildingNode->getRestrictedAttribute("tip");
			} catch (runtime_error &e) {
				m_tipKeys[name] = "";
		}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	//start sound
	try {
		const XmlNode *startSoundNode = n->getChild("start-sound");
		if(startSoundNode->getAttribute("enabled")->getBoolValue()){
			m_startSounds.resize(startSoundNode->getChildCount());
			for(int i = 0; i < startSoundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode = startSoundNode->getChild("sound-file", i);
				string path = soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound = new StaticSound();
				sound->load(dir + "/" + path);
				m_startSounds[i] = sound;
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	//built sound
	try {
		const XmlNode *builtSoundNode= n->getChild("built-sound");
		if(builtSoundNode->getAttribute("enabled")->getBoolValue()){
			m_builtSounds.resize(builtSoundNode->getChildCount());
			for(int i=0; i<builtSoundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode= builtSoundNode->getChild("sound-file", i);
				string path= soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				m_builtSounds[i]= sound;
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void StructureCommandType::doChecksum(Checksum &checksum) const {
	checksum.add(m_buildSkillType->getName());
	for (int i=0; i < m_buildings.size(); ++i) {
		checksum.add(m_buildings[i]->getName());
	}
}

void StructureCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	if (!pt) {
		Lang &lang = g_lang;
		const string factionName = unit->getFaction()->getType()->getName();
		callback->addElement(g_lang.get("Buildings") + ":");
		foreach_const (vector<const UnitType*>, it, m_buildings) {
			callback->addItem(*it, lang.getTranslatedFactionName(factionName, (*it)->getName()));
		}
	}
}

void StructureCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	m_buildSkillType->getDesc(msg, unit);
	callback->addElement(msg);
}

ProdTypePtr StructureCommandType::getProduced(int i) const {
	return m_buildings[i];
}

void StructureCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	const UnitType *builtUnitType = static_cast<const UnitType*>(command->getProdType());

	BUILD_LOG( unit,
		__FUNCTION__ << " : Updating unit " << unit->getId() << ", building type = " << builtUnitType->getName()
		<< ", command target = " << command->getPos();
	);
	if (unit->getCurrSkill()->getClass() != SkillClass::BUILD) {
		Map *map = g_world.getMap();
		if (map->canOccupy(command->getPos(), builtUnitType->getField(), builtUnitType, command->getFacing())) {
			acceptBuild(unit, command, builtUnitType);
		} else {
			vector<Unit *> occupants;
			map->getOccupants(occupants, command->getPos(), builtUnitType->getSize(), Zone::LAND);
			Unit *builtUnit = occupants.size() == 1 ? occupants[0] : NULL;
			if (builtUnit && builtUnit->getType() == builtUnitType && !builtUnit->isBuilt()) {
				existingBuild(unit, command, builtUnit);
			} else if (occupants.size() && attemptMoveUnits(occupants)) {
				return;
			} else {
				blockedBuild(unit);
			}
		}
	} else {
		continueBuild(unit, command, builtUnitType);
	}
}

bool StructureCommandType::isBlocked(const UnitType *builtUnitType, const Vec2i &pos, CardinalDir face) const {
	bool blocked = false;
	Map *map = g_world.getMap();
	if (!map->canOccupy(pos, builtUnitType->getField(), builtUnitType, face)) {
		vector<Unit *> occupants;
		map->getOccupants(occupants, pos, builtUnitType->getSize(), Zone::LAND);
		Unit *builtUnit = occupants.size() == 1 ? occupants[0] : NULL;
		if (builtUnit && builtUnit->getType() == builtUnitType && !builtUnit->isBuilt()) {
		} else if (occupants.size() && attemptMoveUnits(occupants)) {
		} else {
			blocked = true;
		}
	}
	return blocked;
}

CmdResult StructureCommandType::check(const Unit *unit, const Command &command) const {
	const UnitType *builtUnit = static_cast<const UnitType*>(command.getProdType());
	if (isBlocked(builtUnit, command.getPos(), command.getFacing())) {
		return CmdResult::FAIL_BLOCKED;
	}
	return CmdResult::SUCCESS;
}

void StructureCommandType::undo(Unit *unit, const Command &command) const {
	if (command.isReserveResources()
			&& unit->getCurrSkill()->getClass() != SkillClass::BUILD
			&& unit->getCurrSkill()->getClass() != SkillClass::DIE) {
		unit->getFaction()->deApplyCosts(command.getProdType());
	}
}

void StructureCommandType::existingBuild(Unit *unit, Command *command, Unit *builtUnit) const {
	if (command->isReserveResources()) {
		unit->getFaction()->deApplyCosts(static_cast<const UnitType*>(command->getProdType()));
		command->setReserveResources(false);
	}
	unit->setTarget(builtUnit, true, true);
	unit->setCurrSkill(this->getBuildSkillType());
	command->setUnit(builtUnit);
	BUILD_LOG( unit, "in position, building already under construction." );
}

bool StructureCommandType::attemptMoveUnits(const vector<Unit *> &occupants) const {
	vector<Unit *>::const_iterator i;
	for (i = occupants.begin();	i != occupants.end(); ++i) {
		if ((*i)->getCurrSkill()->getClass() != SkillClass::MOVE) {
			return false;
		}
	}
	return true;
}

void StructureCommandType::blockedBuild(Unit *unit) const {
	unit->cancelCurrCommand();
	unit->setCurrSkill(SkillClass::STOP);
	if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
		g_console.addStdMessage("BuildingNoPlace");
	}
	BUILD_LOG( unit, "site blocked." << cmdCancelMsg );
}

void StructureCommandType::acceptBuild(Unit *unit, Command *command, const UnitType *builtUnitType) const {
	Map *map = g_world.getMap();
	Unit *builtUnit = NULL;
	if (!command->isReserveResources()) {
		command->setReserveResources(true);
		if (unit->checkCommand(*command) != CmdResult::SUCCESS) {
			if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
				g_console.addStdMessage("BuildingNoRes");
			}
			BUILD_LOG( unit, "in positioin, late resource allocation failed." << cmdCancelMsg );
			unit->finishCommand();
			return;
		}
		unit->getFaction()->applyCosts(static_cast<const UnitType*>(command->getProdType()));
	}
	BUILD_LOG( unit, "in position, starting construction." );
	builtUnit = g_world.newUnit(command->getPos(), builtUnitType, unit->getFaction(), map, command->getFacing());
	builtUnit->create();
	unit->setCurrSkill(m_buildSkillType);
	unit->setTarget(builtUnit, true, true);
	unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), false);
	Vec2i tilePos = Map::toTileCoords(builtUnit->getCenteredPos());
	if (builtUnitType->getField() == Field::LAND
	|| (builtUnitType->getField() == Field::AMPHIBIOUS && !map->isTileSubmerged(tilePos))) {
		map->prepareTerrain(builtUnit);
	}
	command->setUnit(builtUnit);
	if (!builtUnit->isMobile()) {
		g_world.getCartographer()->updateMapMetrics(builtUnit->getPos(), builtUnit->getSize());
	}
	if (unit->getFaction()->isThisFaction()) {
		RUNTIME_CHECK(!unit->isCarried() && !unit->isGarrisoned());
		g_soundRenderer.playFx(getStartSound(), unit->getCurrVector(), g_gameState.getGameCamera()->getPos());
	}
}

void StructureCommandType::continueBuild(Unit *unit, const Command *command, const UnitType *builtUnitType) const {
	BUILD_LOG( unit, "building." );
	Unit *builtUnit = command->getUnit();
	if (builtUnit && builtUnit->getType() != builtUnitType) {
		unit->setCurrSkill(SkillClass::STOP);
	} else if (!builtUnit || builtUnit->isBuilt()) {
		unit->finishCommand();
		unit->setCurrSkill(SkillClass::STOP);
	} else {
        unit->finishCommand();
		unit->setCurrSkill(SkillClass::STOP);
	}
}

// =====================================================
// 	class ConstructCommandType
// =====================================================

ConstructCommandType::~ConstructCommandType(){
	deleteValues(m_builtSounds.getSounds().begin(), m_builtSounds.getSounds().end());
	deleteValues(m_startSounds.getSounds().begin(), m_startSounds.getSounds().end());
}

bool ConstructCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ft);
	try {
		string skillName = n->getChild("construct-skill")->getAttribute("value")->getRestrictedValue();
		m_constructSkillType = static_cast<const ConstructSkillType*>(unitType->getSkillType(skillName, SkillClass::CONSTRUCT));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		const XmlNode *buildingsNode = n->getChild("buildings");
		for(int i=0; i<buildingsNode->getChildCount(); ++i){
			const XmlNode *buildingNode = buildingsNode->getChild("building", i);
			string name = buildingNode->getAttribute("name")->getRestrictedValue();
			m_buildings.push_back(ft->getUnitType(name));
			try {
				m_tipKeys[name] = buildingNode->getRestrictedAttribute("tip");
			} catch (runtime_error &e) {
				m_tipKeys[name] = "";
		}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		const XmlNode *startSoundNode = n->getChild("start-sound");
		if(startSoundNode->getAttribute("enabled")->getBoolValue()){
			m_startSounds.resize(startSoundNode->getChildCount());
			for(int i = 0; i < startSoundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode = startSoundNode->getChild("sound-file", i);
				string path = soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound = new StaticSound();
				sound->load(dir + "/" + path);
				m_startSounds[i] = sound;
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		const XmlNode *builtSoundNode= n->getChild("built-sound");
		if(builtSoundNode->getAttribute("enabled")->getBoolValue()){
			m_builtSounds.resize(builtSoundNode->getChildCount());
			for(int i=0; i<builtSoundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode= builtSoundNode->getChild("sound-file", i);
				string path= soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				m_builtSounds[i]= sound;
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void ConstructCommandType::doChecksum(Checksum &checksum) const {
	MoveBaseCommandType::doChecksum(checksum);
	checksum.add(m_constructSkillType->getName());
	for (int i=0; i < m_buildings.size(); ++i) {
		checksum.add(m_buildings[i]->getName());
	}
}

void ConstructCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	if (!pt) {
		Lang &lang = g_lang;
		const string factionName = unit->getFaction()->getType()->getName();
		callback->addElement(g_lang.get("Buildings") + ":");
		foreach_const (vector<const UnitType*>, it, m_buildings) {
			callback->addItem(*it, lang.getTranslatedFactionName(factionName, (*it)->getName()));
		}
	}
}

void ConstructCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	m_constructSkillType->getDesc(msg, unit);
	callback->addElement(msg);
}

ProdTypePtr ConstructCommandType::getProduced(int i) const {
	return m_buildings[i];
}

const string cmdCancelMsg = " Command cancelled.";

void ConstructCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	const UnitType *builtUnitType = static_cast<const UnitType*>(command->getProdType());
	BUILD_LOG( unit,
		__FUNCTION__ << " : Updating unit " << unit->getId() << ", building type = " << builtUnitType->getName()
		<< ", command target = " << command->getPos();
	);
	if (unit->getCurrSkill()->getClass() != SkillClass::CONSTRUCT) {
		if (!hasArrived(unit, command, builtUnitType)) {
			return;
		}
		Map *map = g_world.getMap();
		if (map->canOccupy(command->getPos(), builtUnitType->getField(), builtUnitType, command->getFacing())) {
			acceptBuild(unit, command, builtUnitType);
		} else {
			vector<Unit *> occupants;
			map->getOccupants(occupants, command->getPos(), builtUnitType->getSize(), Zone::LAND);
			Unit *builtUnit = occupants.size() == 1 ? occupants[0] : NULL;
			if (builtUnit && builtUnit->getType() == builtUnitType && !builtUnit->isBuilt()) {
				existingBuild(unit, command, builtUnit);
			} else if (occupants.size() && attemptMoveUnits(occupants)) {
				return;
			} else {
				blockedBuild(unit);
			}
		}
	}
}

bool ConstructCommandType::isBlocked(const UnitType *builtUnitType, const Vec2i &pos, CardinalDir face) const {
	bool blocked = false;
	Map *map = g_world.getMap();
	if (!map->canOccupy(pos, builtUnitType->getField(), builtUnitType, face)) {
		vector<Unit *> occupants;
		map->getOccupants(occupants, pos, builtUnitType->getSize(), Zone::LAND);
		Unit *builtUnit = occupants.size() == 1 ? occupants[0] : NULL;
		if (builtUnit && builtUnit->getType() == builtUnitType && !builtUnit->isBuilt()) {
		} else if (occupants.size() && attemptMoveUnits(occupants)) {
		} else {
			blocked = true;
		}
	}
	return blocked;
}

CmdResult ConstructCommandType::check(const Unit *unit, const Command &command) const {
	const UnitType *builtUnit = static_cast<const UnitType*>(command.getProdType());
	if (isBlocked(builtUnit, command.getPos(), command.getFacing())) {
		return CmdResult::FAIL_BLOCKED;
	}
	return CmdResult::SUCCESS;
}

void ConstructCommandType::undo(Unit *unit, const Command &command) const {
	if (command.isReserveResources()
			&& unit->getCurrSkill()->getClass() != SkillClass::CONSTRUCT
			&& unit->getCurrSkill()->getClass() != SkillClass::DIE) {
		unit->getFaction()->deApplyCosts(command.getProdType());
	}
}

bool ConstructCommandType::hasArrived(Unit *unit, const Command *command, const UnitType *builtUnitType) const {
    Vec2i targetPos = command->getPos() + Vec2i(builtUnitType->getSize() / 2);
	unit->setTargetPos(targetPos);
	bool arrived = false;
	switch (g_routePlanner.findPathToBuildSite(unit, builtUnitType, command->getPos(), command->getFacing())) {
		case TravelState::MOVING:
			unit->setCurrSkill(this->getMoveSkillType());
			unit->face(unit->getNextPos());
			BUILD_LOG( unit, "Moving." );
			break;
		case TravelState::BLOCKED:
			unit->setCurrSkill(SkillClass::STOP);
			if(unit->getPath()->isBlocked()) {
				g_console.addStdMessage("Blocked");
				unit->clearPath();
				unit->cancelCurrCommand();
				BUILD_LOG( unit, "Blocked." << cmdCancelMsg );
			}
			break;
		case TravelState::ARRIVED:
			arrived = true;
			break;
		case TravelState::IMPOSSIBLE:
			g_console.addStdMessage("Unreachable");
			unit->cancelCurrCommand();
			BUILD_LOG( unit, "Route impossible," << cmdCancelMsg );
			break;
		default:
			throw runtime_error("Error: RoutePlanner::findPath() returned invalid result.");
	}
	return arrived;
}

void ConstructCommandType::existingBuild(Unit *unit, Command *command, Unit *builtUnit) const {
	if (command->isReserveResources()) {
		unit->getFaction()->deApplyCosts(static_cast<const UnitType*>(command->getProdType()));
		command->setReserveResources(false);
	}
	unit->setTarget(builtUnit, true, true);
	unit->setCurrSkill(this->getConstructSkillType());
	command->setUnit(builtUnit);
	BUILD_LOG( unit, "in position, building already under construction." );
}

bool ConstructCommandType::attemptMoveUnits(const vector<Unit *> &occupants) const {
	vector<Unit *>::const_iterator i;
	for (i = occupants.begin();	i != occupants.end(); ++i) {
		if ((*i)->getCurrSkill()->getClass() != SkillClass::MOVE) {
			return false;
		}
	}
	return true;
}

void ConstructCommandType::blockedBuild(Unit *unit) const {
	unit->cancelCurrCommand();
	unit->setCurrSkill(SkillClass::STOP);
	if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
		g_console.addStdMessage("BuildingNoPlace");
	}
	BUILD_LOG( unit, "site blocked." << cmdCancelMsg );
}

void ConstructCommandType::acceptBuild(Unit *unit, Command *command, const UnitType *builtUnitType) const {
	Map *map = g_world.getMap();
	Unit *builtUnit = NULL;
	BUILD_LOG( unit, "in position, starting construction." );
	int preSize = builtUnitType->getSize();
	stringstream pbs;
	pbs << preSize;
	string preUnitType = "pre_built_size" + pbs.str();
	const UnitType *preBuiltUnitType = unit->getFaction()->getType()->getUnitType(preUnitType);
	builtUnit = g_world.newUnit(command->getPos(), preBuiltUnitType, unit->getFaction(), map, command->getFacing());
	builtUnit->create();
	int newHp = builtUnit->getType()->getMaxHp();
	builtUnit->repair(newHp);
	newHp = builtUnitType->getMaxHp() ;
	builtUnit->decHp(newHp);
    unit->finishCommand();
    unit->setCurrSkill(SkillClass::STOP);
}

// =====================================================
// 	class MaintainCommandType
// =====================================================

bool MaintainCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ft);

	//repair
	try {
		string skillName = n->getChild("maintain-skill")->getAttribute("value")->getRestrictedValue();
		maintainSkillType = static_cast<const MaintainSkillType*>(unitType->getSkillType(skillName, SkillClass::MAINTAIN));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}
	//repaired units
	try {
		const XmlNode *unitsNode = n->getChild("maintained-units");
		for(int i = 0; i < unitsNode->getChildCount(); ++i){
			const XmlNode *unitNode = unitsNode->getChild("unit", i);
			maintainableUnits.push_back(ft->getUnitType(unitNode->getAttribute("name")->getRestrictedValue()));
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void MaintainCommandType::doChecksum(Checksum &checksum) const {
	MoveBaseCommandType::doChecksum(checksum);
	checksum.add(maintainSkillType->getName());
	for (int i=0; i < maintainableUnits.size(); ++i) {
		checksum.add(maintainableUnits[i]->getName());
	}
}

void MaintainCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	maintainSkillType->getDesc(str, unit);

	str+="\n" + lang.get("CanRepair") + ":\n";
	if(maintainSkillType->isSelfOnly()) {
		str += lang.get("SelfOnly");
	} else if(maintainSkillType->isPetOnly()) {
		str += lang.get("PetOnly");
	} else {
		for(int i = 0; i < maintainableUnits.size(); ++i){
			const UnitType *ut = (const UnitType*)maintainableUnits[i];
			if(ut->isAvailableInSubfaction(unit->getFaction()->getSubfaction())) {
				str+= ut->getName()+"\n";
			}
		}
	}
}

void MaintainCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	maintainSkillType->getDesc(msg, unit);
	callback->addElement(msg);
}

void MaintainCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	Lang &lang = g_lang;
	const string factionName = unit->getFaction()->getType()->getName();
	callback->addElement(g_lang.get("CanMaintain") + ":");
	if (maintainSkillType->isSelfOnly()) {
		callback->addElement(lang.get("SelfOnly"));
	} else if (maintainSkillType->isPetOnly()) {
		callback->addElement(lang.get("PetOnly"));
	} else {
		foreach_const (vector<const UnitType*>, it, maintainableUnits) {
			string name;
			if (!lang.lookUp((*it)->getName(), factionName, name)) {
				name = formatString((*it)->getName());
			}
			callback->addItem(*it, name);
		}
	}
}

CmdResult MaintainCommandType::check(const Unit *unit, const Command &command) const {
	if (!command.getCommandedUnit() || command.getUnit() && canMaintain(command.getUnit()->getType())) {
		return CmdResult::SUCCESS;
	} else {
		return CmdResult::FAIL_UNDEFINED;
	}
}

/*void ConstructCommandType::continueBuild(Unit *unit, const Command *command, const UnitType *builtUnitType) const {
	BUILD_LOG( unit, "building." );
	Unit *builtUnit = command->getUnit();
	if (builtUnit && builtUnit->getType() != builtUnit->getType()) {
		unit->setCurrSkill(SkillClass::STOP);
	} else if (!builtUnit || builtUnit->isBuilt()) {
		unit->finishCommand();
		unit->setCurrSkill(SkillClass::STOP);
	} else {
    if (unit->getCurrSkill()->getClass() == SkillClass::CONSTRUCT) {
        bool check = true;
        for (int i = 0; i < builtUnitType->getCostCount(); ++i) {
            const ResourceType *rt = builtUnitType->getCost(i, unit->getFaction()).getType();
            int cost = builtUnitType->getCost(i, unit->getFaction()).getAmount() / builtUnitType->getProductionTime()
            * ((unit->getCurrSkill()->getSpeed(unit)) / 100).intp();
            int stored = builtUnit->getSResource(rt)->getAmount();
            if (cost > stored) {
                check = false;
                break;
            }
        }
        if (check == true) {
            for (int i = 0; i < builtUnitType->getCostCount(); ++i) {
                const ResourceType *costType = builtUnitType->getCost(i, unit->getFaction()).getType();
                int cost = builtUnitType->getCost(i, unit->getFaction()).getAmount() / builtUnitType->getProductionTime()
                * ((unit->getCurrSkill()->getSpeed(unit)) / 100).intp();
                builtUnit->incResourceAmount(costType, cost);
            }
            if (builtUnit->repair()) {
                unit->finishCommand();
                unit->setCurrSkill(SkillClass::STOP);
                unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), true);
                ScriptManager::onUnitCreated(builtUnit);
                if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
                    RUNTIME_CHECK(!unit->isCarried() && !unit->isGarrisoned());
                    g_soundRenderer.playFx(getBuiltSound(), unit->getCurrVector(), g_gameState.getGameCamera()->getPos());
                }
                Vec2i samePos = builtUnit->getPos();
                string name = builtUnitType->getUnitName();
                int faction = builtUnit->getFaction()->getIndex();
                ScriptManager::onUnitDied(builtUnit);
                builtUnit->decHp(builtUnit->getHp());
                builtUnit->replace();
                if (!builtUnit->isMobile()) {
                    g_world.getCartographer()->updateMapMetrics(builtUnit->getPos(), builtUnit->getSize());
                }
                g_world.createUnit(name, faction, samePos, true);
            }
        }
	}
	}
}*/

bool MaintainCommandType::canMaintain(const UnitType *unitType) const{
	for(int i=0; i < maintainableUnits.size(); ++i) {
		if (static_cast<const UnitType*>(maintainableUnits[i]) == unitType) {
			return true;
		}
	}
	return false;
}

void MaintainCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	const MaintainSkillType * const &rst = maintainSkillType;
	bool repairThisFrame = unit->getCurrSkill()->getClass() == SkillClass::MAINTAIN;
	Unit *repaired = command->getUnit();

	if (repaired && (repaired->isDead() || !repaired->isDamaged()
			|| !unit->isAlly(repaired))) {
		unit->setCurrSkill(SkillClass::STOP);
		unit->finishCommand();
		return;
	}

	Command *autoCmd;
	if (command->isAuto() && (autoCmd = doAutoCommand(unit))) {
		if (autoCmd->getType()->getClass() == CmdClass::ATTACK) {
			unit->giveCommand(autoCmd);
		}
	}

	if (maintainableInRange(unit, &repaired, this, rst->getMaxRange(), rst->isSelfAllowed())) {
		unit->setTarget(repaired, true, true);
		unit->setCurrSkill(rst);
	} else {
		Vec2i targetPos;
		if (maintainableInSight(unit, &repaired, this, rst->isSelfAllowed())) {
			if (repaired->isMobile()) {
				if (!g_world.getMap()->getNearestFreePos(targetPos, unit, repaired, 1, rst->getMaxRange())) {
					unit->setCurrSkill(SkillClass::STOP);
					unit->finishCommand();
					return;
				}
			} else {
				targetPos = repaired->getPos() + Vec2i(repaired->getSize() / 2);
			}
			if (targetPos != unit->getTargetPos()) {
				unit->setTargetPos(targetPos);
				unit->clearPath();
			}
		} else {
			if (command->isAuto() && command->hasPos2()) {
				if (Config::getInstance().getGsAutoReturnEnabled()) {
					command->popPos();
					unit->clearPath();
				} else {
					unit->finishCommand();
				}
			}
			targetPos = command->getPos();
		}

		TravelState result;
		if (repaired && !repaired->isMobile()) {
			unit->setTargetPos(targetPos);
			result = g_routePlanner.findPathToBuildSite(unit, repaired->getType(), repaired->getPos(), repaired->getModelFacing());
		} else {
			result = g_routePlanner.findPath(unit, targetPos);
		}
		switch (result) {
			case TravelState::ARRIVED:
				if (repaired) {
					unit->setCurrSkill(rst);
				} else {
					unit->setCurrSkill(SkillClass::STOP);
					unit->finishCommand();
				}
				break;

			case TravelState::MOVING:
				unit->setCurrSkill(m_moveSkillType);
				unit->face(unit->getNextPos());
				break;

			case TravelState::BLOCKED:
				unit->setCurrSkill(SkillClass::STOP);
				if(unit->getPath()->isBlocked()){
					unit->setCurrSkill(SkillClass::STOP);
					unit->clearPath();
					unit->finishCommand();
					REPAIR_LOG( unit, "Unit: " << unit->getId() << " path blocked, cancelling." );
				}
				break;

			case TravelState::IMPOSSIBLE:
				unit->setCurrSkill(SkillClass::STOP);
				unit->finishCommand();
				REPAIR_LOG( unit, "Unit: " << unit->getId() << " path impossible, cancelling." );
				break;

			default: throw runtime_error("Error: RoutePlanner::findPath() returned invalid result.");
		}
	}

	if (repaired && !repaired->isDamaged()) {
		unit->setCurrSkill(SkillClass::STOP);
		unit->finishCommand();
	}

	if (repairThisFrame && unit->getCurrSkill()->getClass() == SkillClass::MAINTAIN) {

		if (!repaired) {
			unit->setCurrSkill(SkillClass::STOP);
		} else {
			unit->setTarget(repaired, true, true);

			//shiney
			if (rst->getSplashParticleType()) {
				const Tile *sc = g_world.getMap()->getTile(Map::toTileCoords(repaired->getCenteredPos()));
				bool visible = sc->isVisible(g_world.getThisTeamIndex())
					&& g_renderer.getCuller().isInside(repaired->getCenteredPos());

				Splash *psSplash = rst->getSplashParticleType()->createSplashParticleSystem(visible);
				psSplash->setPos(repaired->getCurrVector());
				g_renderer.manageParticleSystem(psSplash, ResourceScope::GAME);
			}

			bool wasBuilt = repaired->isBuilt();

			assert(repaired->isAlive() && repaired->getHp() > 0);

			if (repaired->repair(rst->getAmount(), rst->getMultiplier())) {
				unit->setCurrSkill(SkillClass::STOP);
				if (!wasBuilt) {

					ScriptManager::onUnitCreated(repaired);
					if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {

						BuildCommandType *bct = (BuildCommandType *)unit->getType()->getFirstCtOfClass(CmdClass::CONSTRUCT);
						if (bct) {
							RUNTIME_CHECK(!unit->isCarried() && !unit->isGarrisoned());
							g_soundRenderer.playFx(bct->getBuiltSound(),
								unit->getCurrVector(), g_gameState.getGameCamera()->getPos());
						}
					}
				}
			}
		}
	}
}

void MaintainCommandType::tick(const Unit *unit, Command &command) const {
	replaceDeadReferences(command);
}

Command *MaintainCommandType::doAutoMaintain(Unit *unit) const {
	if (!unit->isAutoCmdEnabled(AutoCmdFlag::MAINTAIN) || !unit->getFaction()->isAvailable(this)) {
		return 0;
	}

	Unit *sighted = NULL;
	if (unit->getEp() >= maintainSkillType->getEpCost()
	&& maintainableInSight(unit, &sighted, this, maintainSkillType->isSelfAllowed())) {
		REPAIR_LOG( unit, __FUNCTION__ << "(): Unit:" << *unit << " @ " << unit->getPos()
			<< ", found someone (" << *sighted << ") to repair @ " << sighted->getPos() );
		Command *newCommand;

		Vec2i pos = Map::getNearestPos(unit->getPos(), sighted, maintainSkillType->getMinRange(), maintainSkillType->getMaxRange());
		REPAIR_LOG( unit, "\tMap::getNearestPos(): " << pos );

		newCommand = g_world.newCommand(this, CmdFlags(CmdProps::QUEUE, CmdProps::AUTO), pos);
		newCommand->setPos2(unit->getPos());
		return newCommand;
	}
	return 0;
}

bool MaintainCommandType::maintainableInRange(const Unit *unit, Vec2i centre, int centreSize,
		Unit **rangedPtr, const MaintainCommandType *rct, const MaintainSkillType *rst,
		int range, bool allowSelf, bool militaryOnly, bool damagedOnly) {
	_PROFILE_COMMAND_UPDATE();
	REPAIR_LOG2( unit,
		__FUNCTION__ << "(): with unit: " << *unit << " @ " << unit->getPos() << ", "
		<< " centre: " << centre << ", centreSize: " << centreSize;
	);

	Targets maintainables;
	fixedVec2 fixedCentre(centre.x + centreSize / fixed(2), centre.y + centreSize / fixed(2));
	fixed targetRange = fixed::max_int();
	Unit *target = *rangedPtr;
	range += centreSize / 2;

	if (target && target->isAlive() && target->isDamaged() && rct->canMaintain(target->getType())) {
		fixed rangeToTarget = fixedCentre.dist(target->getFixedCenteredPos()) - (centreSize + target->getSize()) / fixed(2);
		REPAIR_LOG2( unit, "\tRange check to target : " << (rangeToTarget > range ? "not " : "") << "in range." );

		if (rangeToTarget <= range) { // current target is in range
			return true;
		} else { // current target not in range
			return false;
		}
	}
	// no target unit, or target no longer of interest (dead or fully repaired)
	target = NULL;

	// nearby cells
	Vec2i pos;
	fixed distance;
	const Map* map = g_world.getMap();
	PosCircularIteratorSimple pci(map->getBounds(), centre, range);
	while (pci.getNext(pos, distance)) {
		// all zones
		for (int z = 0; z < Zone::COUNT; z++) {
			Unit *candidate = map->getCell(pos)->getUnit(enum_cast<Field>(z));
			if (candidate == 0) {
				continue;
			}
			const Command *cmd = candidate->anyCommand() ? candidate->getCurrCommand() : 0;
			CmdClass cc = cmd ? (CmdClass::Enum)cmd->getType()->getClass() : CmdClass::INVALID;
			const BuildSelfCommandType *bsct = cc == CmdClass::BUILD_SELF
				? static_cast<const BuildSelfCommandType*>(cmd->getType()) : 0;

			//is it a repairable?
			if ((allowSelf || candidate != unit)
			&& (!rst->isSelfOnly() || candidate == unit)
			&& candidate->isAlive()
			&& unit->isAlly(candidate)
			&& (!bsct || bsct->allowRepair())
			&& (!damagedOnly || candidate->isDamaged())
			&& (!militaryOnly || candidate->getType()->hasCommandClass(CmdClass::ATTACK))
			&& rct->canMaintain(candidate->getType())) {
				//record the nearest distance to target (target may be on multiple cells)
				maintainables.record(candidate, distance);
			}
		}
	}
	// if no repairables or just one then it's a simple choice.
	if (maintainables.empty()) {
		REPAIR_LOG2( unit, "\tSearch found no targets." );
		return false;
	} else if (maintainables.size() == 1) {
		*rangedPtr = maintainables.begin()->first;
		REPAIR_LOG2( unit, "\tSearch found single possible target. Unit: "
			<< **rangedPtr << " @ " << (*rangedPtr)->getPos() );
		return true;
	}
	//heal cloesest ally that can attack (and are probably fighting) first.
	//if none, go for units that are less than 20%
	//otherwise, take the nearest repairable unit
	if(!(*rangedPtr = maintainables.getNearestSkillClass(SkillClass::ATTACK))
	&& !(*rangedPtr = maintainables.getNearestHpRatio(fixed(2) / 10))
	&& !(*rangedPtr = maintainables.getNearest())) {
		REPAIR_LOG2( unit, "\tSomething very odd happened..." );
		// this is unreachable, we've already established repaiables is not empty, so
		// getNearest() will always return something for us...
		return false;
	}
	REPAIR_LOG2( unit, "\tSearch found " << maintainables.size() << " possible targets. Selected Unit: "
		<< **rangedPtr << " @ " << (*rangedPtr)->getPos() );
	return true;
}

bool MaintainCommandType::maintainableInRange(const Unit *unit, Unit **rangedPtr,
		const MaintainCommandType *rct, int range, bool allowSelf, bool militaryOnly, bool damagedOnly) {
	return maintainableInRange(unit, unit->getPos(), unit->getType()->getSize(),
			rangedPtr, rct, rct->getMaintainSkillType(), range, allowSelf, militaryOnly, damagedOnly);
}

bool MaintainCommandType::maintainableInSight(const Unit *unit, Unit **rangedPtr,
							const MaintainCommandType *rct, bool allowSelf) {
	return maintainableInRange(unit, rangedPtr, rct, unit->getSight(), allowSelf);
}

}}
