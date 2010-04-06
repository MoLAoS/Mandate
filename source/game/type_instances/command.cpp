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

#include "leak_dumper.h"

namespace Glest{ namespace Game{


// =====================================================
// 	class Command
// =====================================================

const Vec2i Command::invalidPos = Vec2i(-1);

Command::Command(CommandArchetype archetype, CommandFlags flags, const Vec2i &pos, Unit *commandedUnit)
		: archetype(archetype)
		, type(NULL)
		, flags(flags)
		, pos(pos)
		, pos2(-1)
		, unitRef()
		, unitRef2()
		, unitType(NULL)
		, commandedUnit(commandedUnit) {
}

Command::Command(const CommandType *type, CommandFlags flags, const Vec2i &pos, Unit *commandedUnit)
		: archetype(CommandArchetype::GIVE_COMMAND)
		, type(type)
		, flags(flags)
		, pos(pos)
		, pos2(-1)
		, unitRef()
		, unitRef2()
		, unitType(NULL)
		, commandedUnit(commandedUnit) {
}

Command::Command(const CommandType *type, CommandFlags flags, Unit* unit, Unit *commandedUnit)
		: archetype(CommandArchetype::GIVE_COMMAND)
		, type(type)
		, flags(flags)
		, pos(-1)
		, pos2(-1)
		, unitRef(unit)
		, unitRef2()
		, unitType(NULL)
		, commandedUnit(commandedUnit) {
	if (unit) {
		pos = unit->getCenteredPos();
	}
	if (unit && !isAuto() && unit->getFaction()->isThisFaction()) {
		unit->resetHighlight();
		//pos = unit->getCellPos();
	}
}


Command::Command(const CommandType *type, CommandFlags flags, const Vec2i &pos, const UnitType *unitType, Unit *commandedUnit)
		: archetype(CommandArchetype::GIVE_COMMAND)
		, type(type)
		, flags(flags)
		, pos(pos)
		, pos2(-1)
		, unitRef()
		, unitRef2()
		, unitType(unitType)
		, commandedUnit(commandedUnit) {
}

Command::Command(const XmlNode *node, const UnitType *ut, const FactionType *ft)
		: unitRef(node->getChild("unitRef"))
		, unitRef2(node->getChild("unitRef2")) {
	archetype = CommandArchetype(node->getChildIntValue("archetype"));
	type = ut->getCommandType(node->getChildStringValue("type"));
	flags.flags = node->getChildIntValue("flags");
	pos = node->getChildVec2iValue("pos");
	pos2 = node->getChildVec2iValue("pos2");

	string unitTypeName = node->getChildStringValue("unitType");
	unitType = unitTypeName == "none" ? NULL : ft->getUnitType(unitTypeName);
}

void Command::save(XmlNode *node) const {
	node->addChild("archetype", archetype);
	node->addChild("type", type->getName());
	node->addChild("flags", (int)flags.flags);
	node->addChild("pos", pos);
	node->addChild("pos2", pos2);
	unitRef.save(node->addChild("unitRef"));
	unitRef2.save(node->addChild("unitRef2"));
	node->addChild("unitType", unitType ? unitType->getName() : "none");
}

// =============== misc ===============

void Command::swap() {
	UnitReference tmpUnitRef = unitRef;
	unitRef = unitRef2;
	unitRef2 = tmpUnitRef;

	Vec2i tmpPos = pos;
	pos = pos2;
	pos2 = tmpPos;
}

}}//end namespace
