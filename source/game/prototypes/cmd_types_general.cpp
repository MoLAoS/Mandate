// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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
	const XmlNode *nameNode = n->getChild("name");
	name = nameNode->getRestrictedValue();
	XmlAttribute *tipAttrib = nameNode->getAttribute("tip", false);
	if (tipAttrib) {
		m_tipKey = tipAttrib->getRestrictedValue();
	} else {
		m_tipKey = "";
	}

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
	if (unit->isCarried()) {
		Unit *carrier = g_simInterface->getUnitFactory().getUnit(unit->getCarrier());
		const LoadCommandType *lct = 
			static_cast<const LoadCommandType *>(carrier->getType()->getFirstCtOfClass(CommandClass::LOAD));
		if (!lct->areProjectilesAllowed() || !unit->getType()->hasProjectileAttack()) {
			return 0;
		}
	}
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
	if (unit->isCarried()) {
		return 0;
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
		string skillName = n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
		const SkillType *st = unitType->getSkillType(skillName, SkillClass::MOVE);
		m_moveSkillType = static_cast<const MoveSkillType*>(st);
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

Command *MoveBaseCommandType::doAutoFlee(Unit *unit) const {
	Unit *sighted = NULL;
	if (attackerInSight(unit, &sighted)) {
		Vec2i escapePos = unit->getPos() * 2 - sighted->getPos();
		return new Command(this, CommandFlags(CommandProperties::AUTO, true), escapePos);
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
			unit->setCurrSkill(m_moveSkillType);
			unit->face(unit->getNextPos());
			//MOVE_LOG( g_world.getFrameCount() << "::Unit:" << unit->getId() << " updating move " 
			//	<< "Unit is at " << unit->getPos() << " now moving into " << unit->getNextPos() );
			break;
		case TravelState::BLOCKED:
			unit->setCurrSkill(SkillClass::STOP);
			if (unit->getPath()->isBlocked() && !command->getUnit()) {
				unit->clearPath();
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
		string skillName = n->getChild("stop-skill")->getAttribute("value")->getRestrictedValue();
		m_stopSkillType = static_cast<const StopSkillType*>(unitType->getSkillType(skillName, SkillClass::STOP));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
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

	Command *autoCmd;

	// if we have another command then stop sitting on your ass
	if (unit->getCommands().size() > 1) {
		unit->finishCommand();
		return;
	}
	unit->setCurrSkill(m_stopSkillType);

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
		string skillName = n->getChild("produce-skill")->getAttribute("value")->getRestrictedValue();
		m_produceSkillType = static_cast<const ProduceSkillType*>(unitType->getSkillType(skillName, SkillClass::PRODUCE));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		loadOk = false;
	}

	if (n->getOptionalChild("produced-unit")) {
		try { 
			string producedUnitName = n->getChild("produced-unit")->getAttribute("name")->getRestrictedValue();
			m_producedUnits.push_back(ft->getUnitType(producedUnitName));
		} catch (runtime_error e) {
			g_errorLog.addXmlError(dir, e.what ());
			loadOk = false;
		}
	} else {
		try {
			const XmlNode *un = n->getChild("produced-units");
			for (int i=0; i < un->getChildCount(); ++i) {
				const XmlNode *prodNode = un->getChild("produced-unit", i);
				string name = prodNode->getAttribute("name")->getRestrictedValue();
				m_producedUnits.push_back(ft->getUnitType(name));
				try {
					m_tipKeys[name] = prodNode->getRestrictedAttribute("tip");
				} catch (runtime_error &e) {
					m_tipKeys[name] = "";
				}
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

const ProducibleType* ProduceCommandType::getProduced(int i) const {
	return m_producedUnits[i];
}

void ProduceCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(m_produceSkillType->getName());
	foreach_const (vector<const UnitType*>, it, m_producedUnits) {
		checksum.add((*it)->getName());
	}
}

void ProduceCommandType::getDesc(string &str, const Unit *unit) const {
	m_produceSkillType->getDesc(str, unit);
	if (m_producedUnits.size() == 1) {
		str += "\n" + m_producedUnits[0]->getReqDesc();
	}
}

string ProduceCommandType::getReqDesc() const {
	string res = RequirableType::getReqDesc();
	if (m_producedUnits.size() == 1) {
		res += m_producedUnits[0]->getReqDesc();
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
		unit->setCurrSkill(m_produceSkillType);
		unit->getFaction()->checkAdvanceSubfaction(command->getProdType(), false);
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
				unit->getFaction()->checkAdvanceSubfaction(command->getProdType(), true);
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
					RUNTIME_CHECK(!unit->isCarried());
					g_soundRenderer.playFx(getFinishedSound(), unit->getCurrVector(), 
						g_gameState.getGameCamera()->getPos());
				}
			}
			unit->setCurrSkill(SkillClass::STOP);
		}
	}
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
	GeneratedType *gt = g_world.getMasterTypeFactory().newGeneratedType();
	if (!gt->load(producibleNode, dir, tt, ft)) {
		loadOk = false;
	}
	m_producibles.push_back(gt);
	gt->setCommandType(this);

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
	foreach_const (vector<const GeneratedType*>, it, m_producibles) {
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
		res += m_producibles[0]->getReqDesc();
	}
	return res;
}

void GenerateCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Faction *faction = unit->getFaction();
	
	if (unit->getCurrSkill() != m_produceSkillType) {
		// if not producing
		unit->setCurrSkill(m_produceSkillType);
		faction->checkAdvanceSubfaction(command->getProdType(), false);
	} else {
		unit->update2();
		if (unit->getProgress2() > command->getProdType()->getProductionTime()) {
			faction->addProduct(static_cast<const GeneratedType*>(command->getProdType()));
			faction->checkAdvanceSubfaction(command->getProdType(), true);
			faction->applyStaticProduction(command->getProdType());
			unit->setCurrSkill(SkillClass::STOP);
			unit->finishCommand();
			if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
				RUNTIME_CHECK(!unit->isCarried());
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
		string skillName = n->getChild("upgrade-skill")->getAttribute("value")->getRestrictedValue();
		m_upgradeSkillType = static_cast<const UpgradeSkillType*>(unitType->getSkillType(skillName, SkillClass::UPGRADE));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what ());
		loadOk = false;
	}
	try {
		string producedUpgradeName = n->getChild("produced-upgrade")->getAttribute("name")->getRestrictedValue();
		m_producedUpgrade = ft->getUpgradeType(producedUpgradeName);
	} catch (runtime_error e) {
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

void UpgradeCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(m_upgradeSkillType->getName());
	checksum.add(m_producedUpgrade->getName());
}

string UpgradeCommandType::getReqDesc() const {
	return RequirableType::getReqDesc() + getProducedUpgrade()->getReqDesc();
}

const ProducibleType *UpgradeCommandType::getProduced() const {
	return m_producedUpgrade;
}

void UpgradeCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	Faction *faction = unit->getFaction();
	assert(command->getType() == this);

	if (unit->getCurrSkill() != m_upgradeSkillType) {
		//if not producing
		unit->setCurrSkill(m_upgradeSkillType);
		faction->checkAdvanceSubfaction(m_producedUpgrade, false);
	} else {
		//if producing
		unit->update2();
		if (unit->getProgress2() >= m_producedUpgrade->getProductionTime()) {
			unit->finishCommand();
			unit->setCurrSkill(SkillClass::STOP);
			faction->finishUpgrade(m_producedUpgrade);
			faction->checkAdvanceSubfaction(m_producedUpgrade, true);
			if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
				RUNTIME_CHECK(!unit->isCarried());
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
				const XmlNode *unitNode = un->getChild("morph-unit", i);
				string name = unitNode->getAttribute("name")->getRestrictedValue();
				m_morphUnits.push_back(ft->getUnitType(name));
				try {
					m_tipKeys[name] = unitNode->getRestrictedAttribute("tip");
				} catch (runtime_error &e) {
					m_tipKeys[name] = "";
				}
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

const ProducibleType *MorphCommandType::getProduced(int i) const {
	return m_morphUnits[i];
}

void MorphCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(m_morphSkillType->getName());
	foreach_const (vector<const UnitType*>, it, m_morphUnits) {
		checksum.add((*it)->getName());
	}
	checksum.add(m_discount);
}

void MorphCommandType::getDesc(string &str, const Unit *unit) const {
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
		res += m_morphUnits[0]->getReqDesc();
	}
	return res;
}

void MorphCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	const Map *map = g_world.getMap();

	const UnitType *morphToUnit = static_cast<const UnitType*>(command->getProdType());

	if (unit->getCurrSkill() != m_morphSkillType) {
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
				unit->getFaction()->checkAdvanceSubfaction(morphToUnit, true);
				ScriptManager::onUnitCreated(unit);
				if (mapUpdate) {
					// obstacle added or removed, update annotated maps
					g_world.getCartographer()->updateMapMetrics(unit->getPos(), biggerSize);
				}
				if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
					RUNTIME_CHECK(!unit->isCarried());
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

// =====================================================
// 	class LoadCommandType
// =====================================================

bool LoadCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);
	
	//move
	try { 
		const XmlNode *moveSkillNode = n->getOptionalChild("move-skill");
		if (moveSkillNode) {
			string skillName = moveSkillNode->getAttribute("value")->getRestrictedValue();
			const SkillType *st = unitType->getSkillType(skillName, SkillClass::MOVE);
			moveSkillType= static_cast<const MoveSkillType*>(st);
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}

	//load skill
	try {
		string skillName= n->getChild("load-skill")->getAttribute("value")->getRestrictedValue();
		loadSkillType= static_cast<const LoadSkillType*>(unitType->getSkillType(skillName, SkillClass::LOAD));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		const XmlNode *canLoadNode = n->getChild("units-carried");
		for (int i=0; i < canLoadNode->getChildCount(); ++i) {
			string unitName = canLoadNode->getChild("unit", i)->getStringValue();
			const UnitType *ut = ft->getUnitType(unitName);
			m_canLoadList.push_back(ut);
		}
	} catch (const runtime_error &e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		m_loadCapacity = n->getChild("load-capacity")->getIntValue();
	} catch (const runtime_error &e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	const XmlNode *projNode = n->getOptionalChild("allow-projectiles");
	if (projNode && projNode->getBoolValue()) {
		m_allowProjectiles = true;
		try {
			m_projectileOffsets.x = projNode->getChild("horizontal-offset")->getFloatValue();
			m_projectileOffsets.y = projNode->getChild("vertical-offset")->getFloatValue();
		} catch (runtime_error &e) {
			g_errorLog.addXmlError(dir, e.what());
			loadOk = false;
		}
	}
	return loadOk;
}

void LoadCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(loadSkillType->getName());
}

void LoadCommandType::getDesc(string &str, const Unit *unit) const {
	str+= "\n" + loadSkillType->getName();
}

string LoadCommandType::getReqDesc() const {
	return RequirableType::getReqDesc() /*+ "\n" + getProduced()->getReqDesc()*/;
}

void LoadCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	UnitIdList &unitsToCarry = unit->getUnitsToCarry();

	if (unitsToCarry.empty()) { // if no one to load, finished
		unit->finishCommand();
		unit->setCurrSkill(SkillClass::STOP);
		return;
	}

	Unit *closest = 0; // else find closest
	fixed dist = fixed::max_int();
	foreach (UnitIdList, it, unitsToCarry) {
		Unit *target = g_simInterface->getUnitFactory().getUnit(*it);
		fixed d = fixedDist(target->getCenteredPos(), unit->getCenteredPos());
		if (d < dist) {
			closest = target;
			dist = d;
		}
	}
	assert(closest);
	if (dist < loadSkillType->getMaxRange()) { // if in load range, load 'em
		closest->removeCommands();
		closest->setCurrSkill(SkillClass::STOP);
		closest->setVisible(false);
		g_map.clearUnitCells(closest, closest->getPos());
		closest->setCarried(unit);
		closest->setPos(Vec2i(-1));
		g_userInterface.getSelection()->unSelect(closest);
		unit->getCarriedUnits().push_back(closest->getId());
		unitsToCarry.erase(std::find(unitsToCarry.begin(), unitsToCarry.end(), closest->getId()));
		unit->setCurrSkill(loadSkillType);
		unit->clearPath();
		if (unit->getCarriedCount() == m_loadCapacity && !unitsToCarry.empty()) {
			foreach (UnitIdList, it, unitsToCarry) {
				Unit *unit = g_simInterface->getUnitFactory().getUnit(*it);
				if (unit->getType()->getFirstCtOfClass(CommandClass::MOVE)) {
					assert(unit->getCurrCommand());
					assert(unit->getCurrCommand()->getType()->getClass() == CommandClass::BE_LOADED);
					unit->cancelCommand();
				}
			}
			unitsToCarry.clear();
		}
		return;
	}
	if (!moveSkillType) {
		// can't move, just wait
		unit->setCurrSkill(SkillClass::STOP);
		return;
	}
	Vec2i pos = closest->getCenteredPos(); // else move toward closest
	switch (g_routePlanner.findPath(unit, pos)) {
		case TravelState::MOVING:
			unit->setCurrSkill(moveSkillType);
			unit->face(unit->getNextPos());
			break;
		case TravelState::BLOCKED:
			unit->setCurrSkill(SkillClass::STOP);
			if (unit->getPath()->isBlocked() && !command->getUnit()) {
				unit->clearPath();
				unit->finishCommand();
			}
			break;
		default: // TravelState::ARRIVED or TravelState::IMPOSSIBLE
			unit->setCurrSkill(SkillClass::STOP);
			break;
	}
}

// =====================================================
// 	class UnloadCommandType
// =====================================================

bool UnloadCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	// move
	try { 
		const XmlNode *moveSkillNode = n->getOptionalChild("move-skill");
		if (moveSkillNode) {
			string skillName = moveSkillNode->getAttribute("value")->getRestrictedValue();
			const SkillType *st = unitType->getSkillType(skillName, SkillClass::MOVE);
			moveSkillType= static_cast<const MoveSkillType*>(st);
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	// unload skill
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

void UnloadCommandType::getDesc(string &str, const Unit *unit) const {
	str+= "\n" + unloadSkillType->getName();
}

string UnloadCommandType::getReqDesc() const {
	return RequirableType::getReqDesc() /*+ "\n" + getProduced()->getReqDesc()*/;
}

void UnloadCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);

	if (unit->getUnitsToUnload().empty()) {
		// no more units to deal with
		unit->finishCommand();
		unit->setCurrSkill(SkillClass::STOP);
		return;
	}
	if (command->getPos() != Command::invalidPos) {
		assert(moveSkillType);
		switch (g_routePlanner.findPathToLocation(unit, command->getPos())) {
			case TravelState::MOVING:
				unit->setCurrSkill(moveSkillType);
				unit->face(unit->getNextPos());
				break;
			case TravelState::BLOCKED:
				unit->setCurrSkill(SkillClass::STOP);
				if (unit->getPath()->isBlocked()) {
					unit->clearPath();
					command->setPos(Command::invalidPos);
				}
				break;
			default: // TravelState::ARRIVED or TravelState::IMPOSSIBLE
				unit->setCurrSkill(SkillClass::STOP);
				command->setPos(Command::invalidPos);
				break;
		}
	} else {
		if (unit->getCurrSkill()->getClass() != SkillClass::UNLOAD) {
			unit->setCurrSkill(SkillClass::UNLOAD);
		} else {
			Unit *targetUnit = g_simInterface->getUnitFactory().getUnit(unit->getUnitsToUnload().front());
			int maxRange = unloadSkillType->getMaxRange();
			if (g_world.placeUnit(unit->getCenteredPos(), maxRange, targetUnit)) {
				// pick a free space to put the unit
				g_map.putUnitCells(targetUnit, targetUnit->getPos());
				targetUnit->setVisible(true);
				targetUnit->setCarried(0);
				unit->getUnitsToUnload().pop_front();
				unit->getCarriedUnits().erase(std::find(unit->getCarriedUnits().begin(), unit->getCarriedUnits().end(), targetUnit->getId()));
				// keep unloading, curr skill is ok
			} else {
				// must be crowded, stop unloading
				g_console.addLine("too crowded to unload"); /// @todo change with localised version
				unit->setCurrSkill(SkillClass::STOP);
				// cancel?
				// auto-move if in different field? (ie, air transport would move to find space to unload land units)
			}
		}
	}
}

// =====================================================
// 	class BeLoadedCommandType
// =====================================================

void BeLoadedCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	
	if (!command->getUnit() || command->getUnit()->isDead()) {
		unit->finishCommand();
		return;
	}
	Vec2i targetPos = command->getUnit()->getCenteredPos();
	assert(moveSkillType);
	switch (g_routePlanner.findPathToLocation(unit, targetPos)) {
		case TravelState::MOVING:
			unit->setCurrSkill(moveSkillType);
			unit->face(unit->getNextPos());
			break;
		case TravelState::BLOCKED:
			unit->setCurrSkill(SkillClass::STOP);
			if (unit->getPath()->isBlocked()) {
				unit->clearPath();
				command->setPos(Command::invalidPos);
			}
			break;
		default: // TravelState::ARRIVED or TravelState::IMPOSSIBLE
			unit->setCurrSkill(SkillClass::STOP);
			command->setPos(Command::invalidPos);
			break;
	}
}

// ===============================
//  class GenericCommandType
// ===============================

bool CastSpellCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = CommandType::load(n, dir, tt, ft);

	// cast skill
	try { 
		const XmlNode *genSkillNode = n->getChild("cast-spell-skill");
		m_cycle = genSkillNode->getOptionalBoolValue("cycle");
		string skillName = genSkillNode->getAttribute("value")->getRestrictedValue();
		const SkillType *st = unitType->getSkillType(skillName, SkillClass::CAST_SPELL);
		m_castSpellSkillType = static_cast<const CastSpellSkillType*>(st);
	
		string str = n->getChild("affect")->getRestrictedValue();
		m_affects = SpellAffectNames.match(str.c_str());
		if (m_affects == SpellAffect::INVALID) {
			throw runtime_error("Invalid cast-spell affect '" + str + "'");
		}
		if (m_affects != SpellAffect::SELF) {
			throw runtime_error("In 0.3.2 cast-spell affect must be 'self'");
		}
		XmlAttribute *attrib = n->getChild("affect")->getAttribute("start", false);
		if (attrib) {
			str = attrib->getRestrictedValue();
			m_start = SpellStartNames.match(str.c_str());
			if (m_start == SpellStart::INVALID) {
				throw runtime_error("Invlaid spell start '" + str + "'");
			}
			if (m_start != SpellStart::INSTANT) {
				throw runtime_error("In 0.3.2 spell start must be 'instant'");
			}
		} else {
			m_start = SpellStart::INSTANT;
		}	
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void CastSpellCommandType::update(Unit *unit) const {
	if (unit->getCurrSkill() != m_castSpellSkillType) {
		unit->setCurrSkill(m_castSpellSkillType);
		return;
	}
	if (!m_cycle) {
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
	Vec2i effectivePos;
	fixedVec2 fixedCentre;
	fixed halfSize;
	if (unit->isCarried()) {
		Unit *carrier = g_simInterface->getUnitFactory().getUnit(unit->getCarrier());
		effectivePos = carrier->getCenteredPos();
		fixedCentre = carrier->getFixedCenteredPos();
		halfSize = carrier->getType()->getHalfSize();
	} else {
		effectivePos = unit->getCenteredPos();
		fixedCentre = unit->getFixedCenteredPos();
		halfSize = unit->getType()->getHalfSize();
	}
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
		PosCircularIteratorOrdered pci(g_world.getMap()->getBounds(), effectivePos, 
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

}}//end namespace
