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
#include "command_type.h"

#include <algorithm>
#include <cassert>
#include <climits>

#include "upgrade_type.h"
#include "unit_type.h"
#include "sound.h"
#include "sound_renderer.h"
#include "util.h"
#include "leak_dumper.h"
#include "graphics_interface.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "renderer.h"
#include "world.h"
#include "game.h"
#include "route_planner.h"
#include "script_manager.h"

#include "leak_dumper.h"
#include "logger.h"
#include "sim_interface.h"

using std::min;
using namespace Glest::Sim;
using namespace Shared::Util;

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class CommandType
// =====================================================

CommandType::CommandType(const char* name, Clicks clicks, bool queuable)
		: RequirableType(-1, name, NULL)
		, clicks(clicks)
		, queuable(queuable)
		, unitType(NULL) {
}

bool CommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	name = n->getChildRestrictedValue("name");

	bool ok = DisplayableType::load(n, dir);
	return RequirableType::load(n, dir, tt, ft) && ok;
}

void CommandType::doChecksum(Checksum &checksum) const {
	RequirableType::doChecksum(checksum);
	checksum.add<CommandClass>(getClass());
}

typedef const AttackStoppedCommandType* AttackStoppedCmd;
typedef const MoveBaseCommandType* MoveBaseCmdType;
typedef const RepairCommandType* RepairCmd;

///@todo fixme
Command* CommandType::doAutoCommand(Unit *unit) const {
	Command *autoCmd;
	const UnitType *ut = unit->getType();
	// can we attack any enemy ? ///@todo check all attack commands
	const AttackCommandType *act = ut->getAttackCommand(Zone::LAND);
	if (act && (autoCmd = act->doAutoAttack(unit))) {
		return autoCmd;
	}
	///@todo check all attack-stopped commands
	AttackStoppedCmd asct = static_cast<AttackStoppedCmd>(ut->getFirstCtOfClass(CommandClass::ATTACK_STOPPED));
	if (asct && (autoCmd = asct->doAutoAttack(unit))) {
		return autoCmd;
	}
	// can we repair any ally ? ///@todo check all repair commands
	RepairCmd rct = static_cast<RepairCmd>(ut->getFirstCtOfClass(CommandClass::REPAIR));
	if (!unit->getFaction()->getCpuControl() && rct && (autoCmd = rct->doAutoRepair(unit))) {
		//REMOVE
		if (autoCmd->getUnit()) {
			REPAIR_LOG( "Auto-Repair command generated, target unit: "
				<< *autoCmd->getUnit() << ", command pos:" << autoCmd->getPos()
			);
		} else {
			REPAIR_LOG( "Auto-Repair command generated, no target unit, command pos:" << autoCmd->getPos() );
		}
		return autoCmd;
	}
	// can we see an enemy we cant attack ? can we run ?
	MoveBaseCmdType mct = static_cast<MoveBaseCmdType>(ut->getFirstCtOfClass(CommandClass::MOVE));
	if (mct && (autoCmd = mct->doAutoFlee(unit))) {
		return autoCmd;
	}
	return 0;
}

// =====================================================
// 	class MoveBaseCommandType
// =====================================================

bool MoveBaseCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//move
	try { 
		string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
		const SkillType *st = unitType->getSkillType(skillName, SkillClass::MOVE);
		moveSkillType= static_cast<const MoveSkillType*>(st);
	} catch (runtime_error e) {
		g_errorLog.addXmlError ( dir, e.what() );
		loadOk = false;
	}
	return loadOk;
}

Command *MoveBaseCommandType::doAutoFlee(Unit *unit) const {
	Unit *sighted = NULL;
	if (attackerInSight(unit, &sighted)) {
		Vec2i escapePos = unit->getPos() * 2 - sighted->getPos();
		return new Command(this, CommandFlags(CommandProperties::AUTO), escapePos);
	}
	return 0;
}


// =====================================================
// 	class MoveCommandType
// =====================================================

void MoveCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);

	Vec2i pos;
	if (command->getUnit()) {
		pos = command->getUnit()->getCenteredPos();
		if (!command->getUnit()->isAlive()) {
			command->setPos(pos);
			command->setUnit(NULL);
		}
	} else {
		pos = command->getPos();
	}

	switch (g_routePlanner.findPath(unit, pos)) {
		case TravelState::MOVING:
			unit->setCurrSkill(moveSkillType);
			unit->face(unit->getNextPos());
			//MOVE_LOG( g_world.getFrameCount() << "::Unit:" << unit->getId() << " updating move " 
			//	<< "Unit is at " << unit->getPos() << " now moving into " << unit->getNextPos() );
			break;
		case TravelState::BLOCKED:
			unit->setCurrSkill(SkillClass::STOP);
			if (unit->getPath()->isBlocked() && !command->getUnit()) {
				unit->finishCommand();
				return;
			}
			break;	
		default: // TravelState::ARRIVED or TravelState::IMPOSSIBLE
			unit->finishCommand();
			return;
	}

	// if we're doing an auto command, let's make sure we still want to do it
	Command *autoCmd;
	if (command->isAuto() && (autoCmd = doAutoCommand(unit))) {
		unit->giveCommand(autoCmd);
	}
}


// =====================================================
// 	class StopBaseCommandType
// =====================================================

bool StopBaseCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//stop
	try {
		string skillName= n->getChild("stop-skill")->getAttribute("value")->getRestrictedValue();
		stopSkillType= static_cast<const StopSkillType*>(unitType->getSkillType(skillName, SkillClass::STOP));
	} catch (runtime_error e) {
		g_errorLog.addXmlError ( dir, e.what() );
		return false;
	}
	return loadOk;
}

// =====================================================
// 	class StopCommandType
// =====================================================

void StopCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);

	Unit *sighted = NULL;
	Command *autoCmd;

	// if we have another command then stop sitting on your ass
	if (unit->getCommands().size() > 1) {
		unit->finishCommand();
		UNIT_LOG( g_world.getFrameCount() << "::Unit:" << unit->getId() << " cancelling stop" );
		return;
	}
	unit->setCurrSkill(stopSkillType);

	// check auto commands
	if (autoCmd = doAutoCommand(unit)) {
		unit->giveCommand(autoCmd);
		return;
	}
}

// =====================================================
// 	class ProduceCommandType
// =====================================================

//varios
bool ProduceCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//produce
	try { 
		string skillName= n->getChild("produce-skill")->getAttribute("value")->getRestrictedValue();
		produceSkillType= static_cast<const ProduceSkillType*>(unitType->getSkillType(skillName, SkillClass::PRODUCE));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		loadOk = false;
	}

	if (n->getOptionalChild("produced-unit")) {
		try { 
			string producedUnitName= n->getChild("produced-unit")->getAttribute("name")->getRestrictedValue();
			m_producedUnits.push_back(ft->getUnitType(producedUnitName));
		} catch (runtime_error e) {
			g_errorLog.addXmlError(dir, e.what ());
			loadOk = false;
		}
	} else {
		try {
			const XmlNode *un = n->getChild("produced-units");
			for (int i=0; i < un->getChildCount(); ++i) {
				string name = un->getChild("produced-unit", i)->getAttribute("name")->getRestrictedValue();
				m_producedUnits.push_back(ft->getUnitType(name));
			}
		} catch (runtime_error e) {
			g_errorLog.addXmlError(dir, e.what ());
			loadOk = false;
		}
	}
	// finished sound
	try {
		const XmlNode *finishSoundNode = n->getOptionalChild("finished-sound");
		if (finishSoundNode && finishSoundNode->getAttribute("enabled")->getBoolValue()) {
			finishedSounds.resize(finishSoundNode->getChildCount());
			for (int i=0; i < finishSoundNode->getChildCount(); ++i) {
				const XmlNode *soundFileNode = finishSoundNode->getChild("sound-file", i);
				string path = soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound = new StaticSound();
				sound->load(dir + "/" + path);
				finishedSounds[i] = sound;
			}
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void ProduceCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(produceSkillType->getName());
	foreach_const (vector<const UnitType*>, it, m_producedUnits) {
		checksum.add((*it)->getName());
	}
}

void ProduceCommandType::getDesc(string &str, const Unit *unit) const {
	produceSkillType->getDesc(str, unit);
	if (m_producedUnits.size() == 1) {
		str += "\n" + m_producedUnits[0]->getReqDesc();
	}
}

string ProduceCommandType::getReqDesc() const {
	string res = RequirableType::getReqDesc();
	if (m_producedUnits.size() == 1) {
		res += "\n" + m_producedUnits[0]->getReqDesc();
	}
	return res;
}

/// 0: start, 1: produce, 2: finsh (ok), 3: cancel (could not place new unit)
void ProduceCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	
	if (unit->getCurrSkill()->getClass() != SkillClass::PRODUCE) {
		//if not producing
		unit->setCurrSkill(produceSkillType);
	} else {
		unit->update2();
		const UnitType *prodType = static_cast<const UnitType*>(command->getProdType());
		if (unit->getProgress2() > prodType->getProductionTime()) {
			Unit *produced = g_simInterface->getUnitFactory().newInstance(
				Vec2i(0), prodType, unit->getFaction(), g_world.getMap(), CardinalDir::NORTH);
			if (!g_world.placeUnit(unit->getCenteredPos(), 10, produced)) {
				unit->cancelCurrCommand();
				g_simInterface->getUnitFactory().deleteUnit(unit);
			} else {
				produced->create();
				produced->born();
				ScriptManager::onUnitCreated(produced);
				g_simInterface->getStats()->produce(unit->getFactionIndex());
				const CommandType *ct = produced->computeCommandType(unit->getMeetingPos());

				if (ct) {
					produced->giveCommand(new Command(ct, CommandFlags(), unit->getMeetingPos()));
				}
				unit->finishCommand();
				if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
					g_soundRenderer.playFx(getFinishedSound(), unit->getCurrVector(), 
						g_gameState.getGameCamera()->getPos());
				}
			}
			unit->setCurrSkill(SkillClass::STOP);
		}
	}
}

const ProducibleType* ProduceCommandType::getProduced(int i) const {
	return m_producedUnits[i];
}

// =====================================================
// 	class GenerateCommandType
// =====================================================

bool GenerateCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = CommandType::load(n, dir, tt, ft);

	// produce skill
	try { 
		string skillName= n->getChild("produce-skill")->getAttribute("value")->getRestrictedValue();
		m_produceSkillType= static_cast<const ProduceSkillType*>(unitType->getSkillType(skillName, SkillClass::PRODUCE));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		loadOk = false;
	}

	// Producible(s)
	const XmlNode *producibleNode = 0;
	try {
		producibleNode = n->getChild("produced");
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		return false;
	}
	ProducibleType *pt = g_world.getProducibleFactory().newInstance();
	if (!pt->load(producibleNode, dir, tt, ft)) {
		loadOk = false;
	}
	m_producibles.push_back(pt);

	// finished sound
	try {
		const XmlNode *finishSoundNode = n->getOptionalChild("finished-sound");
		if (finishSoundNode && finishSoundNode->getAttribute("enabled")->getBoolValue()) {
			m_finishedSounds.resize(finishSoundNode->getChildCount());
			for (int i=0; i < finishSoundNode->getChildCount(); ++i) {
				const XmlNode *soundFileNode = finishSoundNode->getChild("sound-file", i);
				string path = soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound = new StaticSound();
				sound->load(dir + "/" + path);
				m_finishedSounds[i] = sound;
			}
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void GenerateCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(m_produceSkillType->getName());
	foreach_const (vector<const ProducibleType*>, it, m_producibles) {
		checksum.add((*it)->getName());
	}
}

void GenerateCommandType::getDesc(string &str, const Unit *unit) const {
	m_produceSkillType->getDesc(str, unit);
	if (m_producibles.size() == 1) {
		str += "\n" + m_producibles[0]->getReqDesc();
	}
}

string GenerateCommandType::getReqDesc() const {
	string res = RequirableType::getReqDesc();
	if (m_producibles.size() == 1) {
		res += "\n" + m_producibles[0]->getReqDesc();
	}
	return res;
}

void GenerateCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	
	// should compare _exact_ skill type, not class... in every command update...

	if (unit->getCurrSkill()->getClass() != SkillClass::PRODUCE) {
		// if not producing
		unit->setCurrSkill(m_produceSkillType);
	} else {
		unit->update2();
		if (unit->getProgress2() > command->getProdType()->getProductionTime()) {
			unit->getFaction()->addProduct(command->getProdType());
			unit->getFaction()->applyStaticProduction(command->getProdType());
			unit->setCurrSkill(SkillClass::STOP);
			unit->finishCommand();
			if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
				g_soundRenderer.playFx(getFinishedSound(), unit->getCurrVector(),
						g_gameState.getGameCamera()->getPos());
			}
		}
	}
}

// =====================================================
// 	class UpgradeCommandType
// =====================================================

bool UpgradeCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//upgrade
	try {
		string skillName= n->getChild("upgrade-skill")->getAttribute("value")->getRestrictedValue();
		upgradeSkillType= static_cast<const UpgradeSkillType*>(unitType->getSkillType(skillName, SkillClass::UPGRADE));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		loadOk = false;
	}
	try {
		string producedUpgradeName= n->getChild("produced-upgrade")->getAttribute("name")->getRestrictedValue();
		producedUpgrade= ft->getUpgradeType(producedUpgradeName);
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		loadOk = false;
	}
	// finished sound
	try {
		const XmlNode *finishSoundNode = n->getOptionalChild("finished-sound");
		if (finishSoundNode && finishSoundNode->getAttribute("enabled")->getBoolValue()) {
			finishedSounds.resize(finishSoundNode->getChildCount());
			for (int i=0; i < finishSoundNode->getChildCount(); ++i) {
				const XmlNode *soundFileNode = finishSoundNode->getChild("sound-file", i);
				string path = soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound = new StaticSound();
				sound->load(dir + "/" + path);
				finishedSounds[i] = sound;
			}
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void UpgradeCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(upgradeSkillType->getName());
	checksum.add(producedUpgrade->getName());
}

string UpgradeCommandType::getReqDesc() const {
	return RequirableType::getReqDesc()+"\n"+getProducedUpgrade()->getReqDesc();
}

const ProducibleType *UpgradeCommandType::getProduced() const {
	return producedUpgrade;
}

void UpgradeCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);

	if (unit->getCurrSkill()->getClass() != SkillClass::UPGRADE) {
		//if not producing
		unit->setCurrSkill(upgradeSkillType);
	} else {
		//if producing
		unit->update2();
		if (unit->getProgress2() >= producedUpgrade->getProductionTime()) {
			unit->finishCommand();
			unit->setCurrSkill(SkillClass::STOP);
			unit->getFaction()->finishUpgrade(producedUpgrade);
			if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
				g_soundRenderer.playFx(getFinishedSound(), unit->getCurrVector(), 
					g_gameState.getGameCamera()->getPos());
			}
		}
	}
}

// =====================================================
// 	class MorphCommandType
// =====================================================

bool MorphCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);
	// morph skill
	try {
		string skillName = n->getChild("morph-skill")->getAttribute("value")->getRestrictedValue();
		m_morphSkillType = static_cast<const MorphSkillType*>(unitType->getSkillType(skillName, SkillClass::MORPH));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		loadOk = false;
	}
	// morph unit(s)
	if (n->getOptionalChild("morph-unit")) {
		try {
			string morphUnitName = n->getChild("morph-unit")->getAttribute("name")->getRestrictedValue();
			m_morphUnits.push_back(ft->getUnitType(morphUnitName));
		} catch (runtime_error e) {
			g_errorLog.addXmlError(dir, e.what ());
			loadOk = false;
		}
	} else {
		try {
			const XmlNode *un = n->getChild("morph-units");
			for (int i=0; i < un->getChildCount(); ++i) {
				string name = un->getChild("morph-unit", i)->getAttribute("name")->getRestrictedValue();
				m_morphUnits.push_back(ft->getUnitType(name));
			}
		} catch (runtime_error e) {
			g_errorLog.addXmlError(dir, e.what ());
			loadOk = false;
		}
	}
	// discount
	try { m_discount= n->getChild("discount")->getAttribute("value")->getIntValue(); }
	catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		loadOk = false;
	}
	// finished sound
	try {
		const XmlNode *finishSoundNode = n->getOptionalChild("finished-sound");
		if (finishSoundNode && finishSoundNode->getAttribute("enabled")->getBoolValue()) {
			m_finishedSounds.resize(finishSoundNode->getChildCount());
			for (int i=0; i < finishSoundNode->getChildCount(); ++i) {
				const XmlNode *soundFileNode = finishSoundNode->getChild("sound-file", i);
				string path = soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound = new StaticSound();
				sound->load(dir + "/" + path);
				m_finishedSounds[i] = sound;
			}
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void MorphCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(m_morphSkillType->getName());
	foreach_const (vector<const UnitType*>, it, m_morphUnits) {
		checksum.add((*it)->getName());
	}
	checksum.add(m_discount);
}

void MorphCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();
	m_morphSkillType->getDesc(str, unit);
	if (m_discount != 0) { // discount
		str += lang.get("Discount") + ": " + intToStr(m_discount) + "%\n";
	}
	if (m_morphUnits.size() == 1) {
		str += "\n" + m_morphUnits[0]->getReqDesc();
	}
}

string MorphCommandType::getReqDesc() const{
	string res = RequirableType::getReqDesc();
	if (m_morphUnits.size() == 1) {
		res += "\n" + m_morphUnits[0]->getReqDesc();
	}
	return res;
}

void MorphCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	const Map *map = g_world.getMap();

	const UnitType *morphToUnit = static_cast<const UnitType*>(command->getProdType());

	if (unit->getCurrSkill()->getClass() != SkillClass::MORPH) {
		// if not morphing, check space
		Field mf = morphToUnit->getField();
		if (map->areFreeCellsOrHasUnit(unit->getPos(), morphToUnit->getSize(), mf, unit)) {
			unit->setCurrSkill(m_morphSkillType);
			unit->getFaction()->checkAdvanceSubfaction(morphToUnit, false);
		} else {
			if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
				g_console.addStdMessage("InvalidPosition");
			}
			unit->cancelCurrCommand();
		}
	} else {
		unit->update2();
		if (unit->getProgress2() > morphToUnit->getProductionTime()) {
			bool mapUpdate = unit->isMobile() != morphToUnit->isMobile();
			int biggerSize = std::max(unit->getSize(), morphToUnit->getSize());
			if (unit->morph(this, morphToUnit)) { // finish the command
				unit->finishCommand();
				if (g_userInterface.isSelected(unit)) {
					g_userInterface.onSelectionChanged();
				}
				ScriptManager::onUnitCreated(unit);
				if (mapUpdate) {
					// obstacle added or removed, update annotated maps
					g_world.getCartographer()->updateMapMetrics(unit->getPos(), biggerSize);
				}
				if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
					g_soundRenderer.playFx(getFinishedSound(), unit->getCurrVector(), 
						g_gameState.getGameCamera()->getPos());
				}
			} else {
				unit->cancelCurrCommand();
				if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
					g_console.addStdMessage("InvalidPosition");
				}
			}
			unit->setCurrSkill(SkillClass::STOP);
		}
	}
}

const ProducibleType* MorphCommandType::getProduced(int i) const {
	return m_morphUnits[i];
}

// =====================================================
// 	class LoadCommandType
// =====================================================

bool LoadCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ft);
	
	//load skill
	try {
		string skillName= n->getChild("load-skill")->getAttribute("value")->getRestrictedValue();
		loadSkillType= static_cast<const LoadSkillType*>(unitType->getSkillType(skillName, SkillClass::LOAD));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}

	return loadOk;
}

void LoadCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(loadSkillType->getName());
}

void LoadCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();
	str+= "\n" + loadSkillType->getName();
}

string LoadCommandType::getReqDesc() const{
	return RequirableType::getReqDesc() /*+ "\n" + getProduced()->getReqDesc()*/;
}

void LoadCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	const Map *map = g_world.getMap();

	UnitContainer &unitsToCarry = unit->getUnitsToCarry();

	if (unit->getCurrSkill()->getClass() != SkillClass::LOAD) {
		unit->setCurrSkill(SkillClass::LOAD);
	} else if (unitsToCarry.empty()) {
		unit->finishCommand();
		unit->setCurrSkill(SkillClass::STOP);
	} else {
		UnitContainer::iterator i = unitsToCarry.begin();
		while (i != unitsToCarry.end()) {
			Unit *targetUnit = *i;

			// prevent the unit being loaded multiple times
			if (targetUnit->isCarried()) {
				g_console.addLine("targetUnit already being carried");
				unit->finishCommand();
				unit->setCurrSkill(SkillClass::STOP);
				return;
			} else if (targetUnit == unit) {
				g_console.addLine("carrier trying to load itself");
				i = unitsToCarry.erase(i);
				continue;
			}

			if (targetUnit->getCurrSkill()->getClass() != SkillClass::MOVE) {
				// move the carrier to the unit
				/// @todo this was taken from MoveCommandType so there might be a need 
				/// for a common function to move a unit or there might be a way to 
				/// add the command to move?
				Vec2i pos;
				if (command->getUnit()) {
					pos = command->getUnit()->getCenteredPos();
					if (!command->getUnit()->isAlive()) {
						command->setPos(pos);
						command->setUnit(NULL);
					}
				} else {
					pos = command->getPos();
				}

				// ERROR: don't do this multiple times in a command update... find the closest,
				// path to there... ONE path request. (this in a while loop)

				switch (g_routePlanner.findPath(unit, pos)) {
					case TravelState::MOVING:
						unit->setCurrSkill(moveSkillType);
						unit->face(unit->getNextPos());
						break;
					case TravelState::BLOCKED:
						unit->setCurrSkill(SkillClass::STOP);
						if (unit->getPath()->isBlocked() && !command->getUnit()) {
							unit->finishCommand();
							break;
						}
						break;	
					default: // TravelState::ARRIVED or TravelState::IMPOSSIBLE
						unit->finishCommand();
						break;
				}
			}

			const SkillType *st = unit->getType()->getFirstStOfClass(SkillClass::LOAD);

			// if not full and in range
			if (unit->getCarriedUnits().size() < static_cast<const LoadSkillType*>(st)->getMaxUnits()) {
				if (inRange(unit->getPos(), targetUnit->getPos(), st->getMaxRange())) {
					g_console.addLine("doing load");
					targetUnit->removeCommands();
					targetUnit->setVisible(false);
					/// @bug in below function: size 2 units may overlap with the carrier or something else related to golem unit
					g_map.clearUnitCells(targetUnit, targetUnit->getPos());
					targetUnit->setCarried(true);
					targetUnit->setPos(Vec2i(-1));
					g_userInterface.getSelection()->unSelect(targetUnit);
					unit->getCarriedUnits().push_back(targetUnit);
					//selection.unselect(targetUnit);

					i = unitsToCarry.erase(i);
				} else {
					++i; // process next unit to be carried
				}
			} else {
				g_console.addLine("carrier full"); /// @todo add to localised version
				unit->finishCommand();
				unit->setCurrSkill(SkillClass::STOP);
			}
		}
	}
}

// --- Private ---

bool LoadCommandType::inRange(const Vec2i &thisPos, const Vec2i &targetPos, int maxRange) const {
	return (thisPos.dist(targetPos) < maxRange);
}

// =====================================================
// 	class UnloadCommandType
// =====================================================

bool UnloadCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//unload skill
	try {
		string skillName= n->getChild("unload-skill")->getAttribute("value")->getRestrictedValue();
		unloadSkillType= static_cast<const UnloadSkillType*>(unitType->getSkillType(skillName, SkillClass::UNLOAD));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	
	return loadOk;
}

void UnloadCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(unloadSkillType->getName());
}

void UnloadCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();
	str+= "\n" + unloadSkillType->getName();
}

string UnloadCommandType::getReqDesc() const{
	return RequirableType::getReqDesc() /*+ "\n" + getProduced()->getReqDesc()*/;
}

void UnloadCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	const Map *map = g_world.getMap();

	if (unit->getCurrSkill()->getClass() != SkillClass::UNLOAD) {
		unit->setCurrSkill(SkillClass::UNLOAD);
	} else {
		UnitContainer &units = unit->getCarriedUnits();

		int maxRange = unloadSkillType->getMaxRange();
		//PosCircularIteratorSimple posIter(g_map.getBounds(), unit->getPos(), maxRange);
		UnitContainer::iterator i = units.begin();
		// unload each carried unit to a free space if possible
		while (i != units.end()) {
			Unit *targetUnit = (*i);
			//Vec2i pos;
			g_console.addLine("unloading unit");
		
			//fixed d;
			//if (posIter.getNext(pos, d)) { // g_map.getNearestFreePos(pos, unit, targetUnit, 1, maxRange)????
			if (g_world.placeUnit(unit->getCenteredPos(), maxRange, targetUnit)) {
				// pick a free space to move the unit to
				/// @todo perhaps combine the next two statements into a unit move/relocate(pos) method
				//targetUnit->setPos(pos);
				g_map.putUnitCells(targetUnit, /*pos*/targetUnit->getPos());
				targetUnit->setVisible(true);
				targetUnit->setCarried(false);
				i = units.erase(i);
			} else {
				// must be crowded, stop unloading
				g_console.addLine("too crowded to unload"); /// @todo change with localised version
				break;
			}		
		}

		// no more units to deal with
		unit->finishCommand();
		unit->setCurrSkill(SkillClass::STOP);
	}
}

// Update helpers...

/** Check for enemies unit can smite (or who can smite him)
  * @param rangedPtr [in/out] should point to a pointer that is either NULL or a valid Unit (a target unit)
  * @param asts [in] The attack skill types available to unit
  * @param past [out] Preferred attack skill
  * @return true if the unit has any enemy in range, with results written to rangedPtr and past.
  *  false if no enemies in range, rangedPtr & past will be unchanged.
  */
bool CommandType::unitInRange(const Unit *unit, int range, Unit **rangedPtr, 
					const AttackSkillTypes *asts, const AttackSkillType **past) {
	_PROFILE_COMMAND_UPDATE();
	fixedVec2 fixedCentre = unit->getFixedCenteredPos();
	fixed halfSize = unit->getType()->getHalfSize();
	fixed distance;
	bool needDistance = false;

	if (*rangedPtr && ((*rangedPtr)->isDead() || !asts->getZone((*rangedPtr)->getCurrZone()))) {
		*rangedPtr = NULL;
	}
	if (*rangedPtr) {
		needDistance = true;
	} else {
		Targets enemies;
		Vec2i pos;
		PosCircularIteratorOrdered pci(g_world.getMap()->getBounds(), unit->getPos(), 
			g_world.getPosIteratorFactory().getInsideOutIterator(1, range + halfSize.intp()));

		Map *map = g_world.getMap();
		while (pci.getNext(pos, distance)) {
			foreach_enum (Zone, z) { // all zones
				if (!asts || asts->getZone(z)) { // looking for target in z?
					// does cell contain a bad guy?
					Unit *possibleEnemy = map->getCell(pos)->getUnit(z);
					if (possibleEnemy && possibleEnemy->isAlive() && !unit->isAlly(possibleEnemy)) {
						// If bad guy has an attack command we can short circut this loop now
						if (possibleEnemy->getType()->hasCommandClass(CommandClass::ATTACK)) {
							*rangedPtr = possibleEnemy;
							goto unitOnRange_exitLoop;
						}
						// otherwise, we'll record it and figure out who to slap later.
						enemies.record(possibleEnemy, distance);
					}
				}
			}
		}
	
		if (!enemies.size()) {
			return false;
		}
	
		*rangedPtr = enemies.getNearest();
		needDistance = true;
	}
unitOnRange_exitLoop:
	assert(*rangedPtr);
	
	if (needDistance) {
		const fixed &targetHalfSize = (*rangedPtr)->getType()->getHalfSize();
		distance = fixedCentre.dist((*rangedPtr)->getFixedCenteredPos()) - targetHalfSize;
	}

	// check to see if we like this target.
	if (asts && past) {
		return bool(*past = asts->getPreferredAttack(unit, *rangedPtr, (distance - halfSize).intp()));
	}
	return true;
}

bool CommandType::attackerInSight(const Unit *unit, Unit **rangedPtr) {
	return unitInRange(unit, unit->getSight(), rangedPtr, NULL, NULL);
}

bool CommandType::attackableInRange(const Unit *unit, Unit **rangedPtr, 
			const AttackSkillTypes *asts, const AttackSkillType **past) {
	int range = min(unit->getMaxRange(asts), unit->getSight());
	return unitInRange(unit, range, rangedPtr, asts, past);
}

bool CommandType::attackableInSight(const Unit *unit, Unit **rangedPtr, 
			const AttackSkillTypes *asts, const AttackSkillType **past) {
	return unitInRange(unit, unit->getSight(), rangedPtr, asts, past);
}

// =====================================================
// 	class CommandFactory
// =====================================================

CommandTypeFactory::CommandTypeFactory()
		: m_idCounter(0) {
	registerClass<StopCommandType>("stop");
	registerClass<MoveCommandType>("move");
	registerClass<AttackCommandType>("attack");
	registerClass<AttackStoppedCommandType>("attack_stopped");
	registerClass<BuildCommandType>("build");
	registerClass<HarvestCommandType>("harvest");
	registerClass<RepairCommandType>("repair");
	registerClass<ProduceCommandType>("produce");
	registerClass<GenerateCommandType>("generate");
	registerClass<UpgradeCommandType>("upgrade");
	registerClass<MorphCommandType>("morph");
	registerClass<LoadCommandType>("load");
	registerClass<UnloadCommandType>("unload");
	registerClass<GuardCommandType>("guard");
	registerClass<PatrolCommandType>("patrol");
	registerClass<SetMeetingPointCommandType>("set-meeting-point");
}

CommandTypeFactory::~CommandTypeFactory() {
	deleteValues(m_types);
	m_types.clear();
	m_checksumTable.clear();
}

CommandType* CommandTypeFactory::newInstance(string classId, UnitType *owner) {
	CommandType *ct = MultiFactory<CommandType>::newInstance(classId);
	ct->setIdAndUnitType(m_idCounter++, owner);
	m_types.push_back(ct);
	return ct;
}

CommandType* CommandTypeFactory::getType(int id) {
	if (id < 0 || id >= m_types.size()) {
		throw runtime_error("Error: Unknown command type id: " + intToStr(id));
	}
	return m_types[id];
}

int32 CommandTypeFactory::getChecksum(CommandType *ct) {
	assert(m_checksumTable.find(ct) != m_checksumTable.end());
	return m_checksumTable[ct];
}

void CommandTypeFactory::setChecksum(CommandType *ct) {
	assert(m_checksumTable.find(ct) == m_checksumTable.end());
	Checksum checksum;
	ct->doChecksum(checksum);
	m_checksumTable[ct] = checksum.getSum();
}

}}//end namespace
