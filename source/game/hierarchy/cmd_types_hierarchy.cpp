// ====================================================================
//	This file is part of the Mandate Engine (www.lordofthedawn.com)
//
//	Copyright (C) 2012 Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//	It is released under the terms of the GNU General Public License 3
//
// ====================================================================

#include "pch.h"
#include "cmd_types_hierarchy.h"

#include <algorithm>
#include <cassert>
#include <climits>

#include "command.h"
#include "unit_type.h"
#include "settlement.h"
#include "squad.h"
#include "sound.h"
#include "util.h"
#include "leak_dumper.h"
#include "graphics_interface.h"
#include "renderer.h"
#include "world.h"
#include "route_planner.h"

#include "leak_dumper.h"
#include "logger.h"

namespace Glest { namespace Hierarchy {
using namespace ProtoTypes;

// =====================================================
// 	class FormationCommand
// =====================================================

void FormationCommand::init(Formation newFormation, Clicks cl) {
    formation = newFormation;
    clicks = cl;
    m_tipKey = "Formation: " + formation.title;
    m_tipHeaderKey = "Formation";
}

void FormationCommand::subDesc(Formation formation, FormationDescriptor *callback) const {
    Lang &lang = g_lang;
    Formation *pointer = &formation;
    //callback->addElement("Formation: ");
    //callback->addItem(pointer, pointer->title);
}

void FormationCommand::describe(Formation formation, FormationDescriptor *callback) const {
    Formation *pointer = &formation;
	//callback->setHeader("Formation");
	//callback->setTipText("");
	//callback->addItem(pointer, "");
	subDesc(formation, callback);
}

void FormationCommand::issue(Formation formation, const Squad *squad) {
    const Unit *unit = squad->leader;
    Unit *leader = g_world.getUnit(squad->leader->getId());
    Command *command = g_world.newCommand(leader->getType()->getActions()->getFirstCtOfClass(CmdClass::MOVE), CmdFlags());
    Vec2i position = leader->getPos();
    command->setPos(position);
    leader->giveCommand(command);
    for (int i = 0; i < squad->subordinates.size(); ++i) {
        const Unit *subordinate = squad->subordinates[i];
        Unit *sub = g_world.getUnit(subordinate->getId());
        Command *subCommand = g_world.newCommand(sub->getType()->getActions()->getFirstCtOfClass(CmdClass::MOVE), CmdFlags());
        if (i >= formation.lines[0].line.size()) {
            int newPos = formation.lines[0].line.size() - 1;
            Vec2i newOffset = position + formation.lines[0].line[newPos];
            squad->leaderOffsets.push_back(newOffset);
            subCommand->setPos(newOffset);
        } else {
        Vec2i offset = position + formation.lines[0].line[i];
        squad->leaderOffsets.push_back(formation.lines[0].line[i]);
        subCommand->setPos(offset);
        }
        sub->giveCommand(subCommand);
    }
}

// =====================================================
// 	class CreateSquadCommandType
// =====================================================

bool CreateSquadCommandType::load(const XmlNode *n, const string &dir, const FactionType *ft, const CreatableType *ct) {
	bool loadOk = StopBaseCommandType::load(n, dir, ft, ct);

	string skillName;

	try {
		skillName = n->getChild("set-structure-skill")->getAttribute("value")->getRestrictedValue();
		m_setStructureSkillType = static_cast<const SetStructureSkillType*>(creatableType->getActions()->getSkillType(skillName, SkillClass::SET_STRUCTURE));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	return loadOk;
}

void CreateSquadCommandType::doChecksum(Checksum &checksum) const {
	StopBaseCommandType::doChecksum(checksum);
	checksum.add(m_setStructureSkillType->getName());
}

void CreateSquadCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();
}

void CreateSquadCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	getDesc(msg, unit);
	callback->addElement(msg);
}

void CreateSquadCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	Lang &lang = g_lang;
}

void CreateSquadCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
    unit->getType()->leader.squad.create(unit);
    unit->finishCommand();
}

// =====================================================
// 	class ExpandSquadCommandType
// =====================================================

bool ExpandSquadCommandType::load(const XmlNode *n, const string &dir, const FactionType *ft, const CreatableType *ct) {
	bool loadOk = StopBaseCommandType::load(n, dir, ft, ct);

	string skillName;

	try {
		skillName = n->getChild("set-structure-skill")->getAttribute("value")->getRestrictedValue();
		m_setStructureSkillType = static_cast<const SetStructureSkillType*>(creatableType->getActions()->getSkillType(skillName, SkillClass::SET_STRUCTURE));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	return loadOk;
}

void ExpandSquadCommandType::doChecksum(Checksum &checksum) const {
	StopBaseCommandType::doChecksum(checksum);
	checksum.add(m_setStructureSkillType->getName());
}

void ExpandSquadCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();
}

void ExpandSquadCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	getDesc(msg, unit);
	callback->addElement(msg);
}

void ExpandSquadCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	Lang &lang = g_lang;
}

void ExpandSquadCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Vec2i targetPos;
	Map *map = g_world.getMap();
	Unit *target = command->getUnit();
	if (target) {
        if (target != unit) {
            unit->getType()->leader.squad.subordinates.push_back(target);
        }
	}
    unit->finishCommand();
}

// =====================================================
// 	class SquadMoveCommandType
// =====================================================

bool SquadMoveCommandType::load(const XmlNode *n, const string &dir, const FactionType *ft, const CreatableType *ct) {
	bool loadOk = MoveBaseCommandType::load(n, dir, ft, ct);

	string skillName;

	try {
		skillName = n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
		m_moveSkillType = static_cast<const MoveSkillType*>(creatableType->getActions()->getSkillType(skillName, SkillClass::MOVE));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	return loadOk;
}

void SquadMoveCommandType::doChecksum(Checksum &checksum) const {
	MoveBaseCommandType::doChecksum(checksum);
	checksum.add(m_moveSkillType->getName());
}

void SquadMoveCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();
}

void SquadMoveCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	getDesc(msg, unit);
	callback->addElement(msg);
}

void SquadMoveCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	Lang &lang = g_lang;
}

void SquadMoveCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Map *map = g_world.getMap();
    Unit *leader = g_world.getUnit(unit->getId());
    Command *leaderCommand = g_world.newCommand(leader->getType()->getActions()->getFirstCtOfClass(CmdClass::MOVE), CmdFlags());
    Vec2i position = command->getPos();
    leaderCommand->setPos(position);
    leader->giveCommand(leaderCommand);
    for (int i = 0; i < leader->getType()->leader.squad.subordinates.size(); ++i) {
        const Unit *subordinate = leader->getType()->leader.squad.subordinates[i];
        Unit *sub = g_world.getUnit(subordinate->getId());
        Command *subCommand = g_world.newCommand(sub->getType()->getActions()->getFirstCtOfClass(CmdClass::MOVE), CmdFlags());
        Vec2i offset = position + leader->getType()->leader.squad.leaderOffsets[i];
        subCommand->setPos(offset);
        sub->giveCommand(subCommand);
    }
}

}} // end namespace Glest::Game
