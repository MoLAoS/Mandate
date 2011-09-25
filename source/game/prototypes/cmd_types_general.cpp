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
		, unitType(NULL)
		, energyCost(0)
		, m_display(true) {
}

bool CommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	const XmlNode *nameNode = n->getChild("name");
	m_name = nameNode->getRestrictedValue();
	XmlAttribute *tipAttrib = nameNode->getAttribute("tip", false);
	if (tipAttrib) {
		m_tipKey = tipAttrib->getRestrictedValue();
	} else {
		m_tipKey = "";
	}
	XmlAttribute *tipHeaderAttrib = nameNode->getAttribute("sub-header", false);
	if (tipHeaderAttrib) {
		m_tipHeaderKey = tipHeaderAttrib->getRestrictedValue();
	} else {
		m_tipHeaderKey = "";
	}
	const XmlNode *energyNode = n->getOptionalChild("ep-cost");
	if (energyNode) {
		energyCost = energyNode->getIntValue();
	}
	const XmlNode *displayNode = n->getOptionalChild("display");
	if (displayNode) {
		m_display = displayNode->getBoolValue();
	}

	bool ok = DisplayableType::load(n, dir);
	return RequirableType::load(n, dir, tt, ft) && ok;
}

void CommandType::doChecksum(Checksum &checksum) const {
	RequirableType::doChecksum(checksum);
	checksum.add<CmdClass>(getClass());
}

typedef const AttackStoppedCommandType* AttackStoppedCmd;
typedef const MoveBaseCommandType* MoveBaseCmdType;
typedef const RepairCommandType* RepairCmd;

///@todo fixme
Command* CommandType::doAutoCommand(Unit *unit) const {
	Command *autoCmd;
	const UnitType *ut = unit->getType();
	if (unit->isCarried()) {
		Unit *carrier = g_world.getUnit(unit->getCarrier());
		const LoadCommandType *lct = 
			static_cast<const LoadCommandType *>(carrier->getType()->getFirstCtOfClass(CmdClass::LOAD));
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
	AttackStoppedCmd asct = static_cast<AttackStoppedCmd>(ut->getFirstCtOfClass(CmdClass::ATTACK_STOPPED));
	if (asct && (autoCmd = asct->doAutoAttack(unit))) {
		return autoCmd;
	}
	if (unit->isCarried()) {
		return 0;
	}
	// can we repair any ally ? ///@todo check all repair commands
	RepairCmd rct = static_cast<RepairCmd>(ut->getFirstCtOfClass(CmdClass::REPAIR));
	if (!unit->getFaction()->getCpuControl() && rct && (autoCmd = rct->doAutoRepair(unit))) {
		//REMOVE
		if (autoCmd->getUnit()) {
			REPAIR_LOG( unit, "Auto-Repair command generated, target unit: "
				<< *autoCmd->getUnit() << ", command pos:" << autoCmd->getPos()
			);
		} else {
			REPAIR_LOG( unit, "Auto-Repair command generated, no target unit, command pos:" << autoCmd->getPos() );
		}
		return autoCmd;
	}
	// can we see an enemy we cant attack ? can we run ?
	MoveBaseCmdType mct = static_cast<MoveBaseCmdType>(ut->getFirstCtOfClass(CmdClass::MOVE));
	if (mct && (autoCmd = mct->doAutoFlee(unit))) {
		return autoCmd;
	}
	return 0;
}

/** 
 * Determines command arrow details and if it should be drawn
 * @param out_arrowTarget output for arrowTarget
 * @param out_arrowColor output for arrowColor
 * @return whether to render arrow
 */
bool CommandType::getArrowDetails(const Command *cmd, Vec3f &out_arrowTarget, Vec3f &out_arrowColor) const {
	if (getClicks() != Clicks::ONE) {
		// arrow color
		out_arrowColor = getArrowColor();

		// arrow target
		if (cmd->getUnit() != NULL) {
			if (!cmd->getUnit()->isCarried()) {
				RUNTIME_CHECK(cmd->getUnit()->getPos().x >= 0 && cmd->getUnit()->getPos().y >= 0);
				out_arrowTarget = cmd->getUnit()->getCurrVectorFlat();
				return true;
			}
		} else {
			Vec2i pos = cmd->getPos();
			if (pos != Command::invalidPos) {
				out_arrowTarget = Vec3f(float(pos.x), g_map.getCell(pos)->getHeight(), float(pos.y));
				return true;
			}
		}
	}

	return false;
}

void CommandType::apply(Faction *faction, const Command &command) const {
	ProdTypePtr produced = command.getProdType();
	if (produced && !command.getFlags().get(CmdProps::DONT_RESERVE_RESOURCES)) {
		if (command.getType()->getClass() == CmdClass::MORPH) {
			int discount = static_cast<const MorphCommandType*>(command.getType())->getDiscount();
			faction->applyCosts(produced, discount);
		} else {
			faction->applyCosts(produced);
		}
	}
}

void CommandType::undo(Unit *unit, const Command &command) const {
	//return cost
	ProdTypePtr produced = command.getProdType();
	if (produced) {
		unit->getFaction()->deApplyCosts(produced);
	}
}

/** 
 * replace references to dead units with their dying position prior to their
 * deletion for some commands 
 * @param command the command to modify
 */
void CommandType::replaceDeadReferences(Command &command) const {
	const Unit* unit1 = command.getUnit();
	if (unit1 && unit1->isDead()) {
		command.setUnit(NULL);
		command.setPos(unit1->getPos());
	}
	const Unit* unit2 = command.getUnit2();
	if (unit2 && unit2->isDead()) {
		command.setUnit2(NULL);
		command.setPos2(unit2->getPos());
	}
}

string CommandType::toString() const {
	return g_lang.lookUp(m_name, unitType->getFactionType()->getName());
}

void CommandType::describe(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	Lang lang = g_lang;
	string factionName = unit->getFaction()->getType()->getName();
	string commandName, header, tip;
	if (!lang.lookUp(getName(), factionName, commandName)) {
		commandName = formatString(getName());
	}
	if (pt == 0) { // no producible, header is command name, tip is command tip
		header = commandName;
		if (!lang.lookUp(getTipKey(), factionName, tip)) {
			tip = "";
		}
	} else { // producible
		// header is composed from command 'sub-header', with producible name replacing "%s"
		// tip is from command tip for this producible (not the 'tip' in command the name node
		// but the tip in the 'producible' node.
		assert(getProducedCount() > 0);
		if (!lang.lookUp(getSubHeader(), factionName, pt->getName(), header)) { // header
			if (!lang.lookUp(pt->getName(), factionName, header)) {
				header = formatString(pt->getName());
			}
		}
		if (!lang.lookUp(getTipKey(pt->getName()), factionName, tip)) { // tip
			tip = "";
		}
	}
	callback->setHeader(header);
	callback->setTipText(tip);

	if (!pt && getProducedCount() == 1 && getClicks() == Clicks::ONE) {
		// if no producible was supplied but this is a single click production command, set producible
		pt = getProduced(0);
	}

	descSkills(unit, callback, pt);

	CommandCheckResult cmdCheckResult;
	unit->getFaction()->reportReqsAndCosts(this, pt, cmdCheckResult);

	if (cmdCheckResult.m_upgradedAlready) {
		callback->addItem(pt, g_lang.get("AlreadyUpgraded"));
	} else if (cmdCheckResult.m_upgradingAlready) {
		callback->addItem(pt, g_lang.get("Upgrading"));
	} else if (!cmdCheckResult.m_availableInSubFaction) {
		callback->addItem(this, g_lang.get("NotAvailableInSubfaction"));
	} else {
		vector<UnitReqResult> &unitReqs = cmdCheckResult.m_unitReqResults;
		vector<UpgradeReqResult> &upgradeReqs = cmdCheckResult.m_upgradeReqResults;
		if (!unitReqs.empty() || !upgradeReqs.empty()) {
			callback->addElement(g_lang.get("Reqs") + ":");
			if (!unitReqs.empty()) {
				foreach (vector<UnitReqResult>, it, cmdCheckResult.m_unitReqResults) {
					string name = g_lang.getTranslatedFactionName(factionName, it->getUnitType()->getName());
					callback->addReq(it->getUnitType(), it->isRequirementMet(), name);
				}
			}
			if (!upgradeReqs.empty()) {
				foreach (vector<UpgradeReqResult>, it, cmdCheckResult.m_upgradeReqResults) {
					string name = g_lang.getTranslatedFactionName(factionName, it->getUpgradeType()->getName());
					callback->addReq(it->getUpgradeType(), it->isRequirementMet(), name);
				}
			}
		}
		if (pt) {
			if (!cmdCheckResult.m_resourceCostResults.empty()) {
				callback->addElement(g_lang.get("Costs") + ":");
				foreach (ResourceCostResults, it, cmdCheckResult.m_resourceCostResults) {
					string name = g_lang.getTranslatedTechName(it->getResourceType()->getName());
					string msg = name + " (" + intToStr(it->getCost()) + ")";
					if (!it->isCostMet()) {
						msg += " [-" + intToStr(it->getDifference()) + "]";
					}
					callback->addReq(it->getResourceType(), it->isCostMet(), msg);
				}
			}
			if (!cmdCheckResult.m_resourceMadeResults.empty()) {
				callback->addElement(g_lang.get("Generated") + ":");
				foreach (ResourceMadeResults, it, cmdCheckResult.m_resourceMadeResults) {
					string name = g_lang.getTranslatedTechName(it->getResourceType()->getName());
					string msg = name + " (" + intToStr(it->getAmount()) + ")";
					callback->addItem(it->getResourceType(), msg);
				}
			}
		}
	}
	subDesc(unit, callback, pt);
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
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

Command *MoveBaseCommandType::doAutoFlee(Unit *unit) const {
	if (!unit->isAutoCmdEnabled(AutoCmdFlag::FLEE)) {
		return 0;
	}
	Unit *sighted = NULL;
	if (attackerInSight(unit, &sighted)) {
		Vec2i escapePos = unit->getPos() * 2 - sighted->getPos();
		return g_world.newCommand(this, CmdFlags(CmdProps::AUTO, true), escapePos);
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

	if (unit->travel(pos, m_moveSkillType) == TravelState::ARRIVED) {
		unit->finishCommand();
	}

	// if we're doing an auto command, let's make sure we still want to do it
	Command *autoCmd;
	if (command->isAuto() && (autoCmd = doAutoCommand(unit))) {
		unit->giveCommand(autoCmd);
	}
}

void MoveCommandType::tick(const Unit *unit, Command &command) const {
	replaceDeadReferences(command);
}

void MoveCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	m_moveSkillType->getDesc(msg, unit);
	callback->addElement(msg);
}

// =====================================================
// 	class TeleportCommandType
// =====================================================

CmdResult TeleportCommandType::check(const Unit *unit, const Command &command) const {
	if (m_moveSkillType->getVisibleOnly()) {
		if (g_map.getTileFromCellPos(command.getPos())->isVisible(unit->getTeam())) {
			return CmdResult::SUCCESS;
		} else {
			return CmdResult::FAIL_UNDEFINED;
		}
	}
	return CmdResult::SUCCESS;
}

void TeleportCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);

	Vec2i pos = command->getPos();
	if (command->getUnit()) {
		pos = command->getUnit()->getCenteredPos();
		if (!command->getUnit()->isAlive()) {
			command->setPos(pos);
			command->setUnit(NULL);
		}
	}

	// some back bending to get the unit to face the direction of travel
	if (unit->getPos() == pos) {
		// finish command
		unit->setCurrSkill(SkillClass::STOP);
		unit->finishCommand();
		return;
	} else if (g_map.areFreeCellsOrHasUnit(pos, unit->getSize(), unit->getType()->getField(), unit)) {
		// set-up for the move, the actual moving will be done in World::updateUnits(),
		// after SimInterface::doUpdateUnitCommand() checks EP
		unit->face(pos);
		unit->setCurrSkill(m_moveSkillType);
		unit->setNextPos(pos);
	} else {
		if (unit->getFaction()->isThisFaction()) {
			g_console.addLine(g_lang.get("InvalidPosition"));
		}
		// finish command
		unit->setCurrSkill(SkillClass::STOP);
		unit->finishCommand();
	}
}

void TeleportCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg = "\n";
	if (m_moveSkillType->getVisibleOnly()) {
		msg += g_lang.get("ExploredTargetOnly");
	} else {
		msg += g_lang.get("UnexploredTargetOk");
	}
	callback->addElement(msg);
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
		g_logger.logXmlError(dir, e.what());
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
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	if (n->getOptionalChild("produced-unit")) {
		try { 
			string producedUnitName = n->getChild("produced-unit")->getAttribute("name")->getRestrictedValue();
			m_producedUnits.push_back(ft->getUnitType(producedUnitName));
			if (const XmlAttribute *attrib = n->getChild("produced-unit")->getAttribute("number", false)) {
				m_producedNumbers.push_back(attrib->getIntValue());
			} else {
				m_producedNumbers.push_back(1);
			}
		} catch (runtime_error e) {
			g_logger.logXmlError(dir, e.what ());
			loadOk = false;
		}
		clicks = Clicks::ONE;
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
				if (const XmlAttribute *attrib = prodNode->getAttribute("number", false)) {
					m_producedNumbers.push_back(attrib->getIntValue());
				} else {
					m_producedNumbers.push_back(1);
				}
			}
		} catch (runtime_error e) {
			g_logger.logXmlError(dir, e.what ());
			loadOk = false;
		}
		clicks = Clicks::TWO;
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
		g_logger.logXmlError(dir, e.what ());
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
	string msg;
	m_produceSkillType->getDesc(msg, unit);
	if (m_producedUnits.size() == 1) {
		str += "\n" + m_producedUnits[0]->getReqDesc(unit->getFaction());
	}
}

string ProduceCommandType::getReqDesc(const Faction *f) const {
	string res = RequirableType::getReqDesc(f);
	if (m_producedUnits.size() == 1) {
		res += m_producedUnits[0]->getReqDesc(f);
	}
	return res;
}

void ProduceCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	if (!pt) {
		m_produceSkillType->getDesc(msg, unit);
	} else {
		// do the time-to-produce calc... 
		int framesPerCycle = int(floorf((1.f / (float(unit->getSpeed(m_produceSkillType)) / 4000.f)) + 1.f));
		int timeToBuild = pt->getProductionTime() * framesPerCycle / 40;
		msg = g_lang.get("TimeToBuild") + ": " + intToStr(timeToBuild);
	}
	callback->addElement(msg);
}

void ProduceCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	if (!pt) {
		Lang &lang = g_lang;
		const string factionName = unit->getFaction()->getType()->getName();
		callback->addElement(g_lang.get("Produced") + ":");
		foreach_const (vector<const UnitType*>, it, m_producedUnits) {
			callback->addItem(*it, lang.getTranslatedFactionName(factionName, (*it)->getName()));
		}
	}
}

int ProduceCommandType::getProducedNumber(const UnitType *ut) const {
	for (int i=0; i < m_producedUnits.size(); ++i) {
		if (m_producedUnits[i] == ut) {
			return m_producedNumbers[i];
		}
	}
	throw runtime_error("UnitType not in produce command.");
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
			for (int i=0; i < getProducedNumber(prodType); ++i) {
				Unit *produced = g_world.newUnit(Vec2i(0), prodType, unit->getFaction(),
													g_world.getMap(), CardinalDir::NORTH);
				if (!g_world.placeUnit(unit->getCenteredPos(), 10, produced)) {
					unit->cancelCurrCommand();
					g_world.getUnitFactory().deleteUnit(unit);
					return;
				} else {
					unit->getFaction()->checkAdvanceSubfaction(command->getProdType(), true);
					produced->create();
					produced->born();
					ScriptManager::onUnitCreated(produced);
					g_simInterface.getStats()->produce(unit->getFactionIndex());
					const CommandType *ct = produced->computeCommandType(unit->getMeetingPos());
					if (ct) {
						produced->giveCommand(g_world.newCommand(ct, CmdFlags(), unit->getMeetingPos()));
					}
				}
			}
			// all the units have been produced, safe to finish command now
			unit->finishCommand();
			if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
				RUNTIME_CHECK(!unit->isCarried());
				g_soundRenderer.playFx(getFinishedSound(), unit->getCurrVector(), 
					g_gameState.getGameCamera()->getPos());
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
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	// Producible(s)
	const XmlNode *producibleNode = 0;
	try {
		producibleNode = n->getChild("produced");
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		return false;
	}
	GeneratedType *gt = g_prototypeFactory.newGeneratedType();
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
		g_logger.logXmlError(dir, e.what ());
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
		str += "\n" + m_producibles[0]->getReqDesc(unit->getFaction());
	}
}

void GenerateCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	if (!pt) {
		m_produceSkillType->getDesc(msg, unit);
	} else {
		// do the time-to-produce calc... 
		int framesPerCycle = int(floorf((1.f / (float(unit->getSpeed(m_produceSkillType)) / 4000.f)) + 1.f));
		int timeToBuild = pt->getProductionTime() * framesPerCycle / 40;
		msg = "\n" + g_lang.get("TimeToBuild") + ": " + intToStr(timeToBuild);
	}
	callback->addElement(msg);
}

void GenerateCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	if (!pt) {
		Lang &lang = g_lang;
		const string factionName = unit->getFaction()->getType()->getName();
		callback->addElement(g_lang.get("Produced") + ":");
		foreach_const (vector<const GeneratedType*>, it, m_producibles) {
			callback->addItem(*it, lang.getTranslatedFactionName(factionName, (*it)->getName()));
		}
	}
}

string GenerateCommandType::getReqDesc(const Faction *f) const {
	string res = RequirableType::getReqDesc(f);
	if (m_producibles.size() == 1) {
		res += m_producibles[0]->getReqDesc(f);
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

	// upgrade skill
	try {
		string skillName = n->getChild("upgrade-skill")->getAttribute("value")->getRestrictedValue();
		m_upgradeSkillType = static_cast<const UpgradeSkillType*>(unitType->getSkillType(skillName, SkillClass::UPGRADE));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}
	// upgrade(s)
	try {
		const XmlNode *node = n->getChild("produced-upgrade", 0, false);
		if (node) {
			string producedUpgradeName = node->getAttribute("name")->getRestrictedValue();
			m_producedUpgrades.push_back(ft->getUpgradeType(producedUpgradeName));
			clicks = Clicks::ONE;
		} else {
			node = n->getChild("produced-upgrades");
			for (int i=0; i < node->getChildCount(); ++i) {
				const XmlNode *upgradeNode = node->getChild(i);
				if (upgradeNode->getName() != "upgrade") {
					continue;
				}
				string name = upgradeNode->getRestrictedAttribute("name");
				m_producedUpgrades.push_back(ft->getUpgradeType(name));
				try {
					m_tipKeys[name] = upgradeNode->getRestrictedAttribute("tip");
				} catch (runtime_error &e) {
					m_tipKeys[name] = "";
				}
			}
			clicks = Clicks::TWO;
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
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
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void UpgradeCommandType::getDesc(string &str, const Unit *unit) const {
	m_upgradeSkillType->getDesc(str, unit);
	if (clicks == Clicks::ONE) {
		str += "\n" + getProducedUpgrade(0)->getDesc(unit->getFaction());
	}
}

void UpgradeCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(m_upgradeSkillType->getName());
	foreach_const (vector<const UpgradeType*>, it, m_producedUpgrades) {
		checksum.add((*it)->getName());
	}
}

string UpgradeCommandType::getReqDesc(const Faction *f) const {
	string res = RequirableType::getReqDesc(f);
	if (clicks == Clicks::ONE) {
		res += "\n" + getProducedUpgrade(0)->getReqDesc(f);
	}
	return res;
}

void UpgradeCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	msg = "";
	if (!pt) {
		m_upgradeSkillType->getDesc(msg, unit);
	} else {
		// do the time-to-produce calc... 
		int framesPerCycle = int(floorf((1.f / (float(unit->getSpeed(m_upgradeSkillType)) / 4000.f)) + 1.f));
		int timeToUpgrade = pt->getProductionTime() * framesPerCycle / 40;
		msg = g_lang.get("TimeToResearch") + ": " + intToStr(timeToUpgrade);
	}
	callback->addElement(msg);
}

void UpgradeCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	if (!pt) {
		Lang &lang = g_lang;
		const string factionName = unit->getFaction()->getType()->getName();
		callback->addElement(g_lang.get("Upgrades") + ":");
		foreach_const (vector<const UpgradeType*>, it, m_producedUpgrades) {
			callback->addItem(*it, lang.getTranslatedFactionName(factionName, (*it)->getName()));
		}
	} else {
		///@todo pass descriptor on, to describe effects nicely ...
		string upgradeDesc = static_cast<const UpgradeType*>(pt)->getDesc(unit->getFaction());
		callback->addElement(upgradeDesc);
	}
}

void UpgradeCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	const UpgradeType *upgrade = static_cast<const UpgradeType*>(command->getProdType());
	Faction *faction = unit->getFaction();
	assert(command->getType() == this);
	assert(upgrade);

	if (unit->getCurrSkill() != m_upgradeSkillType) {
		//if not producing
		unit->setCurrSkill(m_upgradeSkillType);
		faction->checkAdvanceSubfaction(upgrade, false);
	} else {
		//if producing
		unit->update2();
		if (unit->getProgress2() >= upgrade->getProductionTime()) {
			unit->finishCommand();
			unit->setCurrSkill(SkillClass::STOP);
			faction->finishUpgrade(upgrade);
			faction->checkAdvanceSubfaction(upgrade, true);
			if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
				RUNTIME_CHECK(!unit->isCarried());
				g_soundRenderer.playFx(getFinishedSound(), unit->getCurrVector(), 
					g_gameState.getGameCamera()->getPos());
			}
		}
	}
}

void UpgradeCommandType::start(Unit *unit, Command &command) const {

	// ???
	//command.setProdType(getProducedUpgrade());

}

void UpgradeCommandType::apply(Faction *faction, const Command &command) const {
	CommandType::apply(faction, command);

	faction->startUpgrade(static_cast<const UpgradeType*>(command.getProdType()));
}

void UpgradeCommandType::undo(Unit *unit, const Command &command) const {
	CommandType::undo(unit, command);
	// upgrade command cancel from list
	unit->getFaction()->cancelUpgrade(static_cast<const UpgradeType*>(command.getProdType()));
}

CmdResult UpgradeCommandType::check(const Unit *unit, const Command &command) const {
	const UpgradeType *upgrade = static_cast<const UpgradeType*>(command.getProdType());
	if (unit->getFaction()->getUpgradeManager()->isUpgradingOrUpgraded(upgrade)) {
		return CmdResult::FAIL_UNDEFINED;
	}
	return CmdResult::SUCCESS;
}

// =====================================================
// 	class MorphCommandType
// =====================================================

MorphCommandType::MorphCommandType()
		: CommandType("Morph", Clicks::ONE, true)
		, m_morphSkillType(0)
		, m_discount(0), m_refund(0) {
}

MorphCommandType::MorphCommandType(const char* name)
		: CommandType(name, Clicks::ONE, true)
		, m_morphSkillType(0)
		, m_discount(0), m_refund(0) {
}

bool MorphCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);
	try { // morph skill
		string skillName = n->getChild("morph-skill")->getAttribute("value")->getRestrictedValue();
		m_morphSkillType = static_cast<const MorphSkillType*>(unitType->getSkillType(skillName, SkillClass::MORPH));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	if (n->getOptionalChild("morph-unit")) { // morph unit(s)
		try {
			string morphUnitName = n->getChild("morph-unit")->getAttribute("name")->getRestrictedValue();
			m_morphUnits.push_back(ft->getUnitType(morphUnitName));
		} catch (runtime_error e) {
			g_logger.logXmlError(dir, e.what ());
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
			g_logger.logXmlError(dir, e.what ());
			loadOk = false;
		}
	}
	try { // discount/refund
		const XmlNode *cmNode = n->getOptionalChild("cost-modifier");
		if (cmNode) {
			const XmlAttribute *a = cmNode->getAttribute("discount", false);
			if (a) {
				m_discount = a->getIntValue();
			} 
			a = cmNode->getAttribute("refund", false);
			if (a) {
				m_refund = a->getIntValue();
			}
		} else {
			const XmlNode *dn = n->getOptionalChild("discount");
			if (dn) {
				m_refund = dn->getAttribute("value")->getIntValue(); // not an error.
				g_logger.logError(dir, "Warning: node 'discount' of morph command is deprecated,"
					" use 'cost-modifier' instead");
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
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
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

ProdTypePtr MorphCommandType::getProduced(int i) const {
	return m_morphUnits[i];
}

void MorphCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(m_morphSkillType->getName());
	foreach_const (vector<const UnitType*>, it, m_morphUnits) {
		checksum.add((*it)->getName());
	}
	checksum.add(m_discount);
	checksum.add(m_refund);
}

void MorphCommandType::getDesc(string &str, const Unit *unit) const {
	Lang &lang= Lang::getInstance();
	m_morphSkillType->getDesc(str, unit);
	if (m_discount != 0) { // discount
		str += lang.get("Discount") + ": " + intToStr(m_discount) + "%\n";
	}
	if (m_refund != 0) {
		str += lang.get("Refund") + ": " + intToStr(m_refund) + "%\n";
	}
	if (m_morphUnits.size() == 1) {
		str += "\n" + m_morphUnits[0]->getReqDesc(unit->getFaction());
	}
}

void MorphCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	if (!pt) {
		m_morphSkillType->getDesc(msg, unit);
	} else {
		// do the time-to-produce calc... 
		int framesPerCycle = int(floorf((1.f / (float(unit->getSpeed(m_morphSkillType)) / 4000.f)) + 1.f));
		int timeToBuild = pt->getProductionTime() * framesPerCycle / 40;
		msg = g_lang.get("TimeToMorph") + ": " + intToStr(timeToBuild);
		if (m_discount != 0) { // discount
			msg += "\n" + g_lang.get("Discount") + ": " + intToStr(m_discount) + "%";
		}
		if (m_refund != 0) {
			msg += "\n" + g_lang.get("Refund") + ": " + intToStr(m_refund) + "%";
		}
	}
	callback->addElement(msg);
}

void MorphCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	if (!pt) {
		Lang &lang = g_lang;
		const string factionName = unit->getFaction()->getType()->getName();
		callback->addElement(g_lang.get("MorphUnits") + ":");
		foreach_const (vector<const UnitType*>, it, m_morphUnits) {
			callback->addItem(*it, lang.getTranslatedFactionName(factionName, (*it)->getName()));
		}
	}
}

string MorphCommandType::getReqDesc(const Faction *f) const{
	string res = RequirableType::getReqDesc(f);
	if (m_morphUnits.size() == 1) {
		res += m_morphUnits[0]->getReqDesc(f);
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

// ===============================
//  class TransformCommandType
// ===============================

TransformCommandType::TransformCommandType()
		: MorphCommandType("Transform")
		, m_moveSkillType(0)
		, m_position(-1)
		, m_rotation(0.f) {
}

bool TransformCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = MorphCommandType::load(n, dir, tt, ft);
	try {
		const XmlNode *skillNode = n->getChild("move-skill");
		string skillName = skillNode->getAttribute("value")->getRestrictedValue();
		m_moveSkillType = static_cast<const MoveSkillType*>(unitType->getSkillType(skillName, SkillClass::MOVE));
		m_position = n->getChildVec2iValue("position");
		m_rotation = n->getOptionalFloatValue("rotation");
		string policy = n->getChildRestrictedValue("hp-policy");
		m_hpPolicy = HpPolicyNames.match(policy);
		if (m_hpPolicy == HpPolicy::INVALID) {
			string msg = "invalid value '" + policy + "' for node hp-policy"
				+ "\n\tMust be one of 'set-to-one', 'reset', or 'maintain'";
			g_logger.logXmlError(dir, msg.c_str());
			loadOk = false;
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void TransformCommandType::doChecksum(Checksum &checksum) const {
	MorphCommandType::doChecksum(checksum);
	checksum.add(m_moveSkillType->getName()); // name or id ?
	checksum.add(m_position);
}

void TransformCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	///@todo

	//callback->addElement(msg);
}

void TransformCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Map *map = g_world.getMap();
	const UnitType *morphToUnit = static_cast<const UnitType*>(command->getProdType());
	RUNTIME_CHECK(m_moveSkillType != 0);

	Vec2i offset = rotateCellOffset(m_position, morphToUnit->getSize(), command->getFacing());
	Vec2i targetPos = command->getPos() + offset; // pos we need to get to

	if (unit->getCurrSkill() == m_morphSkillType) { // if completed one morph skill cycle
		// check space
		Field mf = morphToUnit->getField();
		if (map->areFreeCellsOrHasUnit(command->getPos(), morphToUnit->getSize(), mf, unit)) {
			int biggerSize = std::max(unit->getSize(), morphToUnit->getSize());
			// transform() will morph and then add the new build-self command
			if (unit->transform(this, morphToUnit, command->getPos(), command->getFacing())) { // do it!
				map->prepareTerrain(unit);
				if (g_userInterface.isSelected(unit)) {
					g_userInterface.onSelectionChanged();
				}
				unit->getFaction()->checkAdvanceSubfaction(morphToUnit, true);
				// obstacle added, update annotated maps
				g_world.getCartographer()->updateMapMetrics(unit->getPos(), biggerSize);
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
				unit->setCurrSkill(SkillClass::STOP);
			}
		} else {
			if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
				g_console.addStdMessage("InvalidPosition");
			}
			unit->cancelCurrCommand();
		}

	} else {
		Vec2i offset = rotateCellOffset(m_position, morphToUnit->getSize(), command->getFacing());
		Vec2i targetPos = command->getPos() + offset;
		if (unit->travel(targetPos, m_moveSkillType) == TravelState::ARRIVED) {
			if (unit->getPos() != targetPos) {
				// blocked ?
				unit->setCurrSkill(SkillClass::STOP);
			} else {
				unit->setCurrSkill(m_morphSkillType); // set morph for one cycle
				float rot = m_rotation + command->getFacing() * 90.f;
				while (rot > 360.f) rot -= 360.f;
				unit->face(rot);
			}
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
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	//load skill
	try {
		string skillName= n->getChild("load-skill")->getAttribute("value")->getRestrictedValue();
		loadSkillType= static_cast<const LoadSkillType*>(unitType->getSkillType(skillName, SkillClass::LOAD));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
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
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		m_loadCapacity = n->getChild("load-capacity")->getIntValue();
	} catch (const runtime_error &e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	const XmlNode *projNode = n->getOptionalChild("allow-projectiles");
	if (projNode && projNode->getBoolValue()) {
		m_allowProjectiles = true;
		try {
			m_projectileOffsets.x = projNode->getChild("horizontal-offset")->getFloatValue();
			m_projectileOffsets.y = projNode->getChild("vertical-offset")->getFloatValue();
		} catch (runtime_error &e) {
			g_logger.logXmlError(dir, e.what());
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

string LoadCommandType::getReqDesc(const Faction *f) const {
	return RequirableType::getReqDesc(f);
}

void LoadCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	if (moveSkillType) {
		moveSkillType->getDesc(msg, unit);
	}
	loadSkillType->getDesc(msg, unit);
	callback->addElement(msg);
}

void LoadCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	Lang &lang = g_lang;
	const string factionName = unit->getFaction()->getType()->getName();
	callback->addElement(g_lang.get("CanLoad") + ":");
	foreach_const (vector<const UnitType*>, it, m_canLoadList) {
		callback->addItem(*it, lang.getTranslatedFactionName(factionName, (*it)->getName()));
	}
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
		Unit *target = g_world.getUnit(*it);
		fixed d = fixedDist(target->getCenteredPos(), unit->getCenteredPos());
		if (d < dist) {
			closest = target;
			dist = d;
		}
	}
	assert(closest);
	if (dist < loadSkillType->getMaxRange() + unit->getSize()) { // if in load range, load 'em
		closest->removeCommands();
		closest->setCurrSkill(SkillClass::STOP);
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
				Unit *unit = g_world.getUnit(*it);
				if (unit->getType()->getFirstCtOfClass(CmdClass::MOVE)) {
					assert(unit->getCurrCommand());
					assert(unit->getCurrCommand()->getType()->getClass() == CmdClass::BE_LOADED);
					unit->cancelCommand();
				}
			}
			unitsToCarry.clear();
		}
		unit->StateChanged(unit);
		return;
	}
	if (!moveSkillType) {
		// can't move, just wait
		unit->setCurrSkill(SkillClass::STOP);
		return;
	}

	Vec2i pos = closest->getCenteredPos(); // else move toward closest
	if (unit->travel(pos, moveSkillType) == TravelState::ARRIVED) {
		//unit->finishCommand(); was in blocked which might be better to use since it should be within 
		// load distance by the time it gets to arrived state anyway? - hailstone 21Dec2010
		if (unit->getPath()->isBlocked() && !command->getUnit()) {
			unit->finishCommand();
		} else {
			unit->setCurrSkill(SkillClass::STOP);
		}
	}
}

void LoadCommandType::start(Unit *unit, Command *command) const {
	unit->loadUnitInit(command);
}

CmdResult LoadCommandType::check(const Unit *unit, const Command &command) const {
	if (getLoadCapacity() > unit->getCarriedCount()) {
		if (canCarry(command.getUnit()->getType())
		&& command.getUnit()->getFactionIndex() == unit->getFactionIndex()) {
			return CmdResult::SUCCESS;
		}
		return CmdResult::FAIL_INVALID_LOAD;
	} else {
		return CmdResult::FAIL_LOAD_LIMIT;
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
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	// unload skill
	try {
		string skillName= n->getChild("unload-skill")->getAttribute("value")->getRestrictedValue();
		unloadSkillType= static_cast<const UnloadSkillType*>(unitType->getSkillType(skillName, SkillClass::UNLOAD));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
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

string UnloadCommandType::getReqDesc(const Faction *f) const {
	return RequirableType::getReqDesc(f) /*+ "\n" + getProduced()->getReqDesc()*/;
}

void UnloadCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	if (moveSkillType) {
		moveSkillType->getDesc(msg, unit);
	}
	unloadSkillType->getDesc(msg, unit);
	callback->addElement(msg);
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
	if (command->getPos() != Command::invalidPos && moveSkillType) {
		if (unit->travel(command->getPos(), moveSkillType) == TravelState::ARRIVED) {
			command->setPos(Command::invalidPos);
		}
	} else {
		if (unit->getCurrSkill()->getClass() != SkillClass::UNLOAD) {
			unit->setCurrSkill(SkillClass::UNLOAD);
		} else {
			Unit *targetUnit = g_world.getUnit(unit->getUnitsToUnload().front());
			int maxRange = unloadSkillType->getMaxRange() + unit->getSize();
			if (g_world.placeUnit(unit->getCenteredPos(), maxRange, targetUnit)) {
				// pick a free space to put the unit
				g_map.putUnitCells(targetUnit, targetUnit->getPos());
				targetUnit->setCarried(0);
				unit->getUnitsToUnload().pop_front();
				unit->getCarriedUnits().erase(std::find(unit->getCarriedUnits().begin(), unit->getCarriedUnits().end(), targetUnit->getId()));
				unit->StateChanged(unit);
				// keep unloading, curr skill is ok
			} else {
				// must be crowded, stop unloading
				g_console.addLine("too crowded to unload"); /// @todo change with localised version
				unit->setCurrSkill(SkillClass::STOP);
				// cancel?
				// auto-move if in different field? (ie, air transport would move to find space to unload land units)
				// should check it has moveskill first - hailstone 3Feb2011
			}
		}
	}
}

void UnloadCommandType::start(Unit *unit, Command *command) const {
	unit->unloadUnitInit(command);
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
	if (unit->travel(targetPos, moveSkillType) == TravelState::ARRIVED) {
		command->setPos(Command::invalidPos);
	}
}

// ===============================
//  class CastSpellCommandType
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
		if (m_affects == SpellAffect::AREA) {
			throw runtime_error("In 0.4 cast-spell affect may not be 'area'");
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void CastSpellCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	m_castSpellSkillType->getDesc(msg, unit);
	callback->addElement(msg);
}

void CastSpellCommandType::update(Unit *unit) const {
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);

	if (unit->getCurrSkill() != m_castSpellSkillType) {
		if (m_affects == SpellAffect::SELF) {
			unit->setTarget(unit);
			unit->setCurrSkill(m_castSpellSkillType);
		} else if (!command->getUnit() && unit->getFaction()->isThisFaction()) {
			g_console.addLine(g_lang.get("InvalidTarget"));
			unit->finishCommand();
			unit->setCurrSkill(SkillClass::STOP);
		} else {
			unit->setTarget(command->getUnit());
			unit->setCurrSkill(m_castSpellSkillType);
		}
		return;
	}
	if (!m_cycle) {
		unit->finishCommand();
		unit->setCurrSkill(SkillClass::STOP);
	}
}

// ===============================
//  class BuildSelfCommandType
// ===============================


bool BuildSelfCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool ok = CommandType::load(n, dir, tt, ft);
	try {
		string bsSkillName = n->getChildRestrictedValue("build-self-skill");
		const SkillType *st = unitType->getSkillType(bsSkillName, SkillClass::BUILD_SELF);
		m_buildSelfSkill = static_cast<const BuildSelfSkillType*>(st);
		const XmlNode *repairFlagNode = n->getOptionalChild("allow-repair");
		if (!repairFlagNode || repairFlagNode->getBoolValue()) {
			m_allowRepair = true;
		} else {
			m_allowRepair = false;
		}
	} catch (runtime_error &e) {
		g_logger.logXmlError(dir, e.what());
		return false;
	}
	return ok;
}

void BuildSelfCommandType::update(Unit *unit) const {
	if (unit->getCurrSkill() == m_buildSelfSkill) {
		// add hp
		if (unit->repair()) {
			ScriptManager::onUnitCreated(unit);
			unit->finishCommand();
			unit->setCurrSkill(SkillClass::STOP);
		}
	} else {
		unit->setCurrSkill(m_buildSelfSkill);
	}
}

void BuildSelfCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	///@todo

	//callback->addElement(msg);
}

// ===============================
//  class SetMeetingPointCommandType
// ===============================

CmdResult SetMeetingPointCommandType::check(const Unit *unit, const Command &command) const {
	return unit->getType()->hasMeetingPoint() ? CmdResult::SUCCESS : CmdResult::FAIL_UNDEFINED;
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
		Unit *carrier = g_world.getUnit(unit->getCarrier());
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

	// if called with a target unit, check that is still alive/visible
	if (*rangedPtr) {
		if ((*rangedPtr)->isDead() || !asts->getZone((*rangedPtr)->getCurrZone())) {
			*rangedPtr = 0;
		}
		if (*rangedPtr && (*rangedPtr)->isCloaked()) {
			Vec2i tpos = Map::toTileCoords((*rangedPtr)->getCenteredPos());
			int cloakGroup = (*rangedPtr)->getType()->getCloakType()->getCloakGroup();
			if (!g_cartographer.canDetect(unit->getTeam(), cloakGroup, tpos)) {
				*rangedPtr = 0;
			}
		}
	}
	if (range == 0) { // blind unit
		if (*rangedPtr) {
			*rangedPtr = NULL;
		}
		return false;
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
						if (possibleEnemy->isCloaked()) {
							int cloakGroup = possibleEnemy->getCloakGroup();
							Vec2i tpos = Map::toTileCoords(possibleEnemy->getCenteredPos());
							if (!g_cartographer.canDetect(unit->getTeam(), cloakGroup, tpos)) {
								continue;
							}
						}
						// If bad guy has an attack command we can short circut this loop now
						if (possibleEnemy->getType()->hasCommandClass(CmdClass::ATTACK)) {
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
