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

#include "command_type.h"
#include "faction_type.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{


// =====================================================
// 	class Command
// =====================================================

Command::Command(const CommandType *ct, CommandFlags flags, const Vec2i &pos) :
		commandType(ct),
		flags(flags),
		pos(pos),
		pos2(-1),
		unitRef(),
		unitRef2(),
		unitType(NULL) {
}

Command::Command(const CommandType *ct, CommandFlags flags, Unit* unit) :
		commandType(ct),
		flags(flags),
		pos(-1),
		pos2(-1),
		unitRef(unit),
		unitRef2(),
		unitType(NULL) {

	if(unit) {
		unit->resetHighlight();
		if(!isAuto()) {
			pos = unit->getCellPos();
		}
	}
}

Command::Command(const CommandType *ct, CommandFlags flags, const Vec2i &pos, const UnitType *unitType) :
		commandType(ct),
		flags(flags),
		pos(pos),
		pos2(-1),
		unitRef(),
		unitRef2(),
		unitType(unitType) {
}

Command::Command(const XmlNode *node, const UnitType *ut, const FactionType *ft) :
		unitRef(node->getChild("unitRef")),
		unitRef2(node->getChild("unitRef2")) {
	commandType = ut->getCommandType(node->getChildStringValue("commandType"));
	flags.flags = node->getChildIntValue("flags");
	pos = node->getChildVec2iValue("pos");
	pos2 = node->getChildVec2iValue("pos2");

	string unitTypeName = node->getChildStringValue("unitType");
	unitType = unitTypeName == "none" ? NULL : ft->getUnitType(unitTypeName);
}

void Command::save(XmlNode *node) const {
	node->addChild("commandType", commandType->getName());
	node->addChild("flags", flags.flags);
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
