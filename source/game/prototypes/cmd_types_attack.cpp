// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//                2008-2009 Daniel Santos
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

#include "command.h"
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
#include "route_planner.h"

#include "leak_dumper.h"
#include "logger.h"

namespace Glest { namespace ProtoTypes {

// ===============================
// 	class AttackCommandTypeBase
// ===============================

bool AttackCommandTypeBase::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType *unitType) {
	const AttackSkillType *ast;
	string skillName;
	const XmlNode *attackSkillNode = n->getChild("attack-skill", 0, false);
	bool loadOk = true;
	//single attack skill
	if(attackSkillNode) {
		try {
			skillName = attackSkillNode->getAttribute("value")->getRestrictedValue();
			ast = static_cast<const AttackSkillType*>(unitType->getSkillType(skillName, SkillClass::ATTACK));
			attackSkillTypes.push_back(ast, AttackSkillPreferences());
		} catch (runtime_error e) {
			g_logger.logXmlError(dir, e.what ());
			loadOk = false;
		}
	} else { //multiple attack skills
		try {
			const XmlNode *flagsNode;
			const XmlNode *attackSkillsNode;

			attackSkillsNode = n->getChild("attack-skills", 0, false);
			if(!attackSkillsNode)
				throw runtime_error("Must specify either a single <attack-skill> node or an <attack-skills> node with nested <attack-skill>s.");
			int count = attackSkillsNode->getChildCount();

			for(int i = 0; i < count; ++i) {
				try {
					AttackSkillPreferences prefs;
					attackSkillNode = attackSkillsNode->getChild("attack-skill", i);
					skillName = attackSkillNode->getAttribute("value")->getRestrictedValue();
					ast = static_cast<const AttackSkillType*>(unitType->getSkillType(skillName, SkillClass::ATTACK));
					flagsNode = attackSkillNode->getChild("flags", 0, false);
					if(flagsNode) {
						prefs.load(flagsNode, dir, tt, ft);
					}
					attackSkillTypes.push_back(ast, prefs);
				}
				catch (runtime_error e) {
					g_logger.logXmlError(dir, e.what ());
					loadOk = false;
				}
			}
		} catch (runtime_error e) {
			g_logger.logXmlError(dir, e.what ());
			loadOk = false;
		}
	}
	if (loadOk) {
		attackSkillTypes.init();
	}
	return loadOk;
}

/** Returns an attack skill for the given field if one exists. */ /*
const AttackSkillType * AttackCommandTypeBase::getAttackSkillType(Field field) const {
	for(AttackSkillTypes::const_iterator i = attackSkillTypes.begin(); i != attackSkillTypes.end(); i++) {
		if(i->first->getField(field)) {
			return i->first;
		}
	}
	return NULL;
}
 */

// =====================================================
// 	class AttackCommandType
// =====================================================

/** Update helper for attack based commands that includes a move skill (sub classes of AttackCommandType).
  * @returns true when completed */
bool AttackCommandType::updateGeneric(Unit *unit, Command *command, const AttackCommandType *act, 
									  Unit* target, const Vec2i &targetPos) const {	
	if (target && target->getHp() <= 0) {
		// the target is dead, finish command so the unit doesn't 
		// wander to the target pos
		unit->setCurrSkill(SkillClass::STOP);
		return true;
	}

	const AttackSkillType *ast = NULL;

	if (target && !attackSkillTypes.getZone(target->getCurrZone())) { // if have target but can't attack it
		unit->finishCommand();
		return true;
	}
	if (attackableInRange(unit, &target, &attackSkillTypes, &ast)) { // found a target in range
		assert(ast);
		if (unit->getEp() >= ast->getEpCost()) { // enough ep for skill?
			unit->setCurrSkill(ast);
			unit->setTarget(target);
		} else {
			unit->setCurrSkill(SkillClass::STOP); ///@todo check other attack skills for a cheaper one??
		}
		return false;
	}
	if (unit->isCarried()) { // if housed, dont try to wander off!
		unit->setCurrSkill(SkillClass::STOP);
		return true;
	}

	///@todo We've already searched out to attack-range and found nothing, rather than
	/// starting again to check visible-range, we should just keep using the same search

	// couldn't attack anyone, look for someone to smite nearby, compute target pos
	Vec2i pos;
	if (attackableInSight(unit, &target, &attackSkillTypes, NULL)) { // got a target
		pos = target->getNearestOccupiedCell(unit->getPos());
		if (pos != unit->getTargetPos()) {
			unit->setTargetPos(pos);
			unit->clearPath();
		}
	} else { // if no more targets and on auto command, then turn around
		if (command->isAuto() && command->hasPos2()) {
			if (g_config.getGsAutoReturnEnabled()) {
				command->popPos();
				pos = command->getPos();
				RUNTIME_CHECK(g_world.getMap()->isInside(pos));
				unit->clearPath();
			} else {
				unit->setCurrSkill(SkillClass::STOP);
				return true;
			}
		} else { // no targets & not auto-command, use command target pos
			pos = targetPos;
			RUNTIME_CHECK(g_world.getMap()->isInside(pos));
		}
	}

	return unit->travel(pos, act->getMoveSkillType()) == TravelState::ARRIVED;
}

void AttackCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Unit *target = command->getUnit();

	if (updateGeneric(unit, command, this, target, command->getPos())) {
		unit->finishCommand();
	}
}

bool AttackCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool ok = MoveBaseCommandType::load(n, dir, tt, ft);
	return AttackCommandTypeBase::load(n, dir, tt, ft, unitType) && ok;
}

void AttackCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg = g_lang.get("WalkSpeed") + ": " + intToStr(unit->getSpeed(m_moveSkillType)) + "\n";
	attackSkillTypes.getDesc(msg, unit);
	callback->addElement(msg);
}


Command *AttackCommandType::doAutoAttack(Unit *unit) const {
	if (!unit->isAutoCmdEnabled(AutoCmdFlag::ATTACK)) {
		return 0;
	}
	// look for someone to smite
	Unit *sighted = NULL;
	if (!unit->getFaction()->isAvailable(this)
	|| !attackableInSight(unit, &sighted, &attackSkillTypes, NULL)) {
		return 0;
	}
	Command *newCommand = g_world.newCommand(this, CmdFlags(CmdProps::AUTO, true), sighted->getPos());
	newCommand->setPos2(unit->getPos());
	assert(newCommand->isAuto());
	return newCommand;
}

// =====================================================
// 	class AttackStoppedCommandType
// =====================================================

void AttackStoppedCommandType::update(Unit *unit) const {
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Unit *enemy = NULL;
	const AttackSkillType *ast = NULL;
	if (attackableInRange(unit, &enemy, &attackSkillTypes, &ast)) {
		assert(ast);
		unit->setCurrSkill(ast);
		unit->setTarget(enemy, true, true);
	} else {
		unit->setCurrSkill(m_stopSkillType);
	}
}

bool AttackStoppedCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool ok = StopBaseCommandType::load(n, dir, tt, ft);
	return AttackCommandTypeBase::load(n, dir, tt, ft, unitType) && ok;
}

Command *AttackStoppedCommandType::doAutoAttack(Unit *unit) const {
	// look for someone to smite
	Unit *sighted = NULL;
	if (!unit->getFaction()->isAvailable(this) || !attackableInRange(unit, &sighted, &attackSkillTypes, NULL)) {
		return 0;
	}
	Command *newCommand = g_world.newCommand(this, CmdFlags(CmdProps::AUTO, true), sighted->getPos());
	return newCommand;
}

void AttackStoppedCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	attackSkillTypes.getDesc(msg, unit);	
	callback->addElement(msg);
}

// =====================================================
// 	class PatrolCommandType
// =====================================================

void PatrolCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Unit *target = command->getUnit();
	Unit *target2 = command->getUnit2();
	Vec2i pos;

	if (target) {	// patrolling toward a target unit
 		if (target->isDead()) { // check target
			pos = target->getCenteredPos();
			command->setUnit(NULL);
			command->setPos(pos);
		} else { // if target not dead calc nearest pos with patrol range 
			pos = Map::getNearestPos(unit->getPos(), target->getPos(), 1, getMaxDistance());
		}
	} else { // no target unit, use command pos
		pos = command->getPos();
	}
	if (target2 && target2->isDead()) { // patrolling away from a unit, check target
		command->setUnit2(NULL);
		command->setPos2(target2->getCenteredPos());
	}
	// If destination reached or blocked, turn around on next update.
	if (updateGeneric(unit, command, this, NULL, pos)) {
		unit->clearPath();
		command->swap();
	}
}

void PatrolCommandType::tick(const Unit *unit, Command &command) const {
	replaceDeadReferences(command);
}

void PatrolCommandType::finish(Unit *unit, Command &command) const {
	// remember where we started from
	command.setPos2(unit->getPos());
}

void PatrolCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	m_moveSkillType->getDesc(msg, unit);
	// max-range ...
	attackSkillTypes.getDesc(msg, unit);
	callback->addElement(msg);
}

// =====================================================
// 	class GuardCommandType
// =====================================================

void GuardCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Unit *target = command->getUnit();
	Vec2i pos;

	if (target && target->isDead()) {
		//if you suck ass as a body guard then you have to hang out where your client died.
		command->setUnit(NULL);
		command->setPos(target->getPos());
		target = NULL;
	}
	// calculate target pos
	if (target) {
		pos = Map::getNearestPos(unit->getPos(), target, 1, getMaxDistance());
	} else {
		pos = Map::getNearestPos(unit->getPos(), command->getPos(), 1, getMaxDistance());
	}
	// if within 'guard range' and no bad guys to attack
	if (updateGeneric(unit, command, this, NULL, pos)) { 
		unit->clearPath();
		unit->setCurrSkill(SkillClass::STOP);  // just hang-ten
	}
}

void GuardCommandType::tick(const Unit *unit, Command &command) const {
	replaceDeadReferences(command);
}

bool GuardCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = AttackCommandType::load(n, dir, tt, ft);

	//distance
	try { m_maxDistance = n->getChild("max-distance")->getAttribute("value")->getIntValue(); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void GuardCommandType::doChecksum(Checksum &checksum) const {
	AttackCommandType::doChecksum(checksum);
	checksum.add<int>(m_maxDistance);
}

void GuardCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	m_moveSkillType->getDesc(msg, unit);
	// max-range ...
	attackSkillTypes.getDesc(msg, unit);
	callback->addElement(msg);
}

// update helper, move somewhere sensible
// =====================================================
// 	class Targets
// =====================================================

void Targets::record(Unit *target, fixed dist) {
	if (find(target) == end()) {
		insert(std::make_pair(target, dist));
	}
	if (dist < distance) {
		nearest = target;
		distance = dist;
	}
}

Unit* Targets::getNearestSkillClass(SkillClass sc) {
	foreach(Targets, it, *this) {
		if (it->first->getType()->hasSkillClass(sc)) {
			return it->first;
		}
	}
	return NULL;
}

Unit* Targets::getNearestHpRatio(fixed hpRatio) {
	foreach(Targets, it, *this) {
		if (it->first->getHpRatioFixed() < hpRatio) {
			return it->first;
		}
	}
	return NULL;
}

}} // end namespace Glest::Game
