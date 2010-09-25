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
#include "command.h"

#include "world.h"
#include "command_type.h"
#include "faction_type.h"
#include "program.h"
#include "sim_interface.h"

#include "leak_dumper.h"

namespace Glest { namespace Entities {

// =====================================================
// 	class Command
// =====================================================

const Vec2i Command::invalidPos = Vec2i(-1);
int Command::lastId = -1;

Command::Command(CommandArchetype archetype, CommandFlags flags, const Vec2i &pos, Unit *commandedUnit)
		: archetype(archetype)
		, type(NULL)
		, flags(flags)
		, pos(pos)
		, pos2(-1)
		, unitRef(-1)
		, unitRef2(-1)
		, prodType(NULL)
		, commandedUnit(commandedUnit) {
	lastId++;
	id = lastId;
}

Command::Command(const CommandType *type, CommandFlags flags, const Vec2i &pos, Unit *commandedUnit)
		: archetype(CommandArchetype::GIVE_COMMAND)
		, type(type)
		, flags(flags)
		, pos(pos)
		, pos2(-1)
		, unitRef(-1)
		, unitRef2(-1)
		, prodType(0)
		, commandedUnit(commandedUnit) {
	lastId++;
	id = lastId;
}

Command::Command(const CommandType *type, CommandFlags flags, Unit* unit, Unit *commandedUnit)
		: archetype(CommandArchetype::GIVE_COMMAND)
		, type(type)
		, flags(flags)
		, pos(-1)
		, pos2(-1)
		, prodType(0)
		, commandedUnit(commandedUnit) {
	lastId++;
	id = lastId;

	unitRef = unit ? unit->getId() : -1;
	unitRef2 = -1;
	
	if (unit) {
		pos = unit->getCenteredPos();
	}
	if (unit && !isAuto() && commandedUnit && commandedUnit->getFaction()->isThisFaction()) {
		unit->resetHighlight();
		//pos = unit->getCellPos();
	}
}


Command::Command(const CommandType *type, CommandFlags flags, const Vec2i &pos, 
				 const ProducibleType *prodType, CardinalDir facing, Unit *commandedUnit)
		: archetype(CommandArchetype::GIVE_COMMAND)
		, type(type)
		, flags(flags)
		, pos(pos)
		, pos2(-1)
		, unitRef(-1)
		, unitRef2(-1)
		, prodType(prodType)
		, facing(facing)
		, commandedUnit(commandedUnit) {
	lastId++;
	id = lastId;
}

Command::Command(const XmlNode *node, const UnitType *ut, const FactionType *ft) {
	unitRef = node->getOptionalIntValue("unitRef", -1);
	unitRef2 = node->getOptionalIntValue("unitRef2", -1);
	archetype = CommandArchetype(node->getChildIntValue("archetype"));
	type = ut->getCommandType(node->getChildStringValue("type"));
	flags.flags = node->getChildIntValue("flags");
	pos = node->getChildVec2iValue("pos");
	pos2 = node->getChildVec2iValue("pos2");

	string prodTypeName = node->getChildStringValue("prodType");
	if (prodTypeName == "none") {
		prodType = 0;
	} else {
		const ProducibleType *pt = g_world.getMasterTypeFactory().getType(prodTypeName);
		prodType = pt;
	}
	if (node->getOptionalChild("facing") ) {
		facing = enum_cast<CardinalDir>(node->getChildIntValue("facing"));
	}
}

Unit* Command::getUnit() const {
	return g_simInterface->getUnitFactory().getUnit(unitRef);
}

Unit* Command::getUnit2() const {
	return g_simInterface->getUnitFactory().getUnit(unitRef2);
}

void Command::save(XmlNode *node) const {
	node->addChild("archetype", archetype);
	node->addChild("type", type->getName());
	node->addChild("flags", (int)flags.flags);
	node->addChild("pos", pos);
	node->addChild("pos2", pos2);
	node->addChild("unitRef", unitRef);
	node->addChild("unitRef2", unitRef2);
	node->addChild("prodType", prodType ? prodType->getName() : "none");
	node->addChild("facing", int(facing));
}

// =============== misc ===============

void Command::swap() {
	std::swap(unitRef, unitRef2);
	std::swap(pos, pos2);
}

}}//end namespace
