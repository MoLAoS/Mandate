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

CommandType::CommandType(const char* name, /*CommandClass cc,*/ Clicks clicks, bool queuable)
		: RequirableType(-1, name, NULL)
		//, cc(cc)
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
	if (rct && (autoCmd = rct->doAutoRepair(unit))) {
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
		Logger::getErrorLog().addXmlError ( dir, e.what() );
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

	switch (theRoutePlanner.findPath(unit, pos)) {
		case TravelState::MOVING:
			unit->setCurrSkill(moveSkillType);
			unit->face(unit->getNextPos());
			//MOVE_LOG( theWorld.getFrameCount() << "::Unit:" << unit->getId() << " updating move " 
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
		Logger::getErrorLog().addXmlError ( dir, e.what() );
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
		UNIT_LOG( theWorld.getFrameCount() << "::Unit:" << unit->getId() << " cancelling stop" );
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
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	try { 
		string producedUnitName= n->getChild("produced-unit")->getAttribute("name")->getRestrictedValue();
		producedUnit= ft->getUnitType(producedUnitName);
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void ProduceCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(produceSkillType->getName());
	checksum.add(producedUnit->getName());
}

void ProduceCommandType::getDesc(string &str, const Unit *unit) const {
	produceSkillType->getDesc(str, unit);
	str+= "\n" + getProducedUnit()->getReqDesc();
}

string ProduceCommandType::getReqDesc() const{
	return RequirableType::getReqDesc()+"\n"+getProducedUnit()->getReqDesc();
}

const ProducibleType *ProduceCommandType::getProduced() const{
	return producedUnit;
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

		if (unit->getProgress2() > producedUnit->getProductionTime()) {
			Unit *produced = theSimInterface->getUnitFactory().newInstance(Vec2i(0), producedUnit, unit->getFaction(), theWorld.getMap());
				//new Unit(theWorld.getNextUnitId(), Vec2i(0), producedUnit, unit->getFaction(), theWorld.getMap());
			if (!theWorld.placeUnit(unit->getCenteredPos(), 10, produced)) {
				unit->cancelCurrCommand();
				theSimInterface->getUnitFactory().deleteUnit(unit);
			} else {
				produced->create();
				produced->born();
				ScriptManager::onUnitCreated(produced);
				theSimInterface->getStats()->produce(unit->getFactionIndex());
				const CommandType *ct = produced->computeCommandType(unit->getMeetingPos());

				if (ct) {
					produced->giveCommand(new Command(ct, CommandFlags(), unit->getMeetingPos()));
				}
				unit->finishCommand();
			}
			unit->setCurrSkill(SkillClass::STOP);
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
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	try {
		string producedUpgradeName= n->getChild("produced-upgrade")->getAttribute("name")->getRestrictedValue();
		producedUpgrade= ft->getUpgradeType(producedUpgradeName);
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
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
		}
	}
}

// =====================================================
// 	class MorphCommandType
// =====================================================

bool MorphCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);
	//morph skill
	try {
		string skillName= n->getChild("morph-skill")->getAttribute("value")->getRestrictedValue();
		morphSkillType= static_cast<const MorphSkillType*>(unitType->getSkillType(skillName, SkillClass::MORPH));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	//morph unit
	try {
		string morphUnitName= n->getChild("morph-unit")->getAttribute("name")->getRestrictedValue();
		morphUnit= ft->getUnitType(morphUnitName);
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	//discount
	try { discount= n->getChild("discount")->getAttribute("value")->getIntValue(); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void MorphCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(morphSkillType->getName());
	checksum.add(morphUnit->getName());
	checksum.add<int>(discount);
}

void MorphCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	morphSkillType->getDesc(str, unit);

	//discount
	if(discount!=0){
		str+= lang.get("Discount")+": "+intToStr(discount)+"%\n";
	}

	str+= "\n"+getProduced()->getReqDesc();
}

string MorphCommandType::getReqDesc() const{
	return RequirableType::getReqDesc() + "\n" + getProduced()->getReqDesc();
}

const ProducibleType *MorphCommandType::getProduced() const{
	return morphUnit;
}

void MorphCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	const Map *map = theWorld.getMap();

	if (unit->getCurrSkill()->getClass() != SkillClass::MORPH) {
		//if not morphing, check space
		Field mf = morphUnit->getField();
		if (map->areFreeCellsOrHasUnit(unit->getPos(), morphUnit->getSize(), mf, unit)) {
			unit->setCurrSkill(morphSkillType);
			unit->getFaction()->checkAdvanceSubfaction(morphUnit, false);
		} else {
			if (unit->getFactionIndex() == theWorld.getThisFactionIndex()) {
				theConsole.addStdMessage("InvalidPosition");
			}
			unit->cancelCurrCommand();
		}
	} else {
		unit->update2();
		if (unit->getProgress2() > morphUnit->getProductionTime()) {
			bool mapUpdate = unit->isMobile() != morphUnit->isMobile();
			int biggerSize = std::max(unit->getSize(), morphUnit->getSize());
			//finish the command
			if (unit->morph(this)) {
				unit->finishCommand();
				if (theGui.isSelected(unit)) {
					theGui.onSelectionChanged();
				}
				ScriptManager::onUnitCreated(unit);
				if (mapUpdate) {
					// obstacle added or removed, update annotated maps
					theWorld.getCartographer()->updateMapMetrics(unit->getPos(), biggerSize);
				}
			} else {
				unit->cancelCurrCommand();
				if (unit->getFactionIndex() == theWorld.getThisFactionIndex()) {
					theConsole.addStdMessage("InvalidPosition");
				}
			}
			unit->setCurrSkill(SkillClass::STOP);
		}
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
		PosCircularIteratorOrdered pci(theWorld.getMap()->getBounds(), unit->getPos(), 
			theWorld.getPosIteratorFactory().getInsideOutIterator(1, range + halfSize.intp()));

		Map *map = theWorld.getMap();
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
		: idCounter(0) {
	registerClass<StopCommandType>("stop");
	registerClass<MoveCommandType>("move");
	registerClass<AttackCommandType>("attack");
	registerClass<AttackStoppedCommandType>("attack_stopped");
	registerClass<BuildCommandType>("build");
	registerClass<HarvestCommandType>("harvest");
	registerClass<RepairCommandType>("repair");
	registerClass<ProduceCommandType>("produce");
	registerClass<UpgradeCommandType>("upgrade");
	registerClass<MorphCommandType>("morph");
	registerClass<GuardCommandType>("guard");
	registerClass<PatrolCommandType>("patrol");
	registerClass<SetMeetingPointCommandType>("set-meeting-point");
}

}}//end namespace
