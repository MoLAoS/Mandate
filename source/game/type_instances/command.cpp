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

Command::Command(CommandArchetype archetype, CommandFlags flags, const Vec2i &pos, Unit *commandedUnit) :
		archetype(archetype),
		type(NULL),
		flags(flags),
		pos(pos),
		pos2(-1),
		unitRef(),
		unitRef2(),
		unitType(NULL),
		commandedUnit(commandedUnit) {
}

Command::Command(const CommandType *type, CommandFlags flags, const Vec2i &pos, Unit *commandedUnit) :
		archetype(catGiveCommand),
		type(type),
		flags(flags),
		pos(pos),
		pos2(-1),
		unitRef(),
		unitRef2(),
		unitType(NULL),
		commandedUnit(commandedUnit) {
}

Command::Command(const CommandType *type, CommandFlags flags, Unit* unit, Unit *commandedUnit) :
		archetype(catGiveCommand),
		type(type),
		flags(flags),
		pos(-1),
		pos2(-1),
		unitRef(unit),
		unitRef2(),
		unitType(NULL),
		commandedUnit(commandedUnit) {
	if(unit) {
		pos = unit->getCenteredPos();
	}

	if(unit && !isAuto() && unit->getFaction()->isThisFaction()) {
		unit->resetHighlight();
		//pos = unit->getCellPos();
	}
}


Command::Command(const CommandType *type, CommandFlags flags, const Vec2i &pos, const UnitType *unitType, Unit *commandedUnit) :
		archetype(catGiveCommand),
		type(type),
		flags(flags),
		pos(pos),
		pos2(-1),
		unitRef(),
		unitRef2(),
		unitType(unitType),
		commandedUnit(commandedUnit) {
}

Command::Command(const XmlNode *node, const UnitType *ut, const FactionType *ft) :
		unitRef(node->getChild("unitRef")),
		unitRef2(node->getChild("unitRef2")) {
	archetype = static_cast<CommandArchetype>(node->getChildIntValue("archetype"));
	type = ut->getCommandType(node->getChildStringValue("type"));
	flags.flags = node->getChildIntValue("flags");
	pos = node->getChildVec2iValue("pos");
	pos2 = node->getChildVec2iValue("pos2");

	string unitTypeName = node->getChildStringValue("unitType");
	unitType = unitTypeName == "none" ? NULL : ft->getUnitType(unitTypeName);
}

Command::Command(NetworkDataBuffer &buf) :
		archetype(catGiveCommand),
		type(NULL),
		flags(0),
		pos(-1),
		pos2(-1),
		unitRef(),
		unitRef2(),
		unitType(NULL),
		commandedUnit(NULL) {
	read(buf);
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

// =============== NetworkWriteable ===============

size_t Command::getNetSize() const {
	return	  sizeof(uint16)		// id of unit command is being issued to
			+ sizeof(uint8)			// archtype + flags
			+ sizeof(uint8)			// fields present
			+ (type					? sizeof(uint8) : 0) 		// command id
			+ (pos != invalidPos 	? sizeof(uint16) : 0) * 2	// pos
			+ (pos2 != invalidPos	? sizeof(uint16) : 0) * 2	// pos2
			+ (unitRef.getUnit()	? sizeof(uint16) : 0)		// target (unit id)
			+ (unitRef2.getUnit()	? sizeof(uint16) : 0)		// target2 (unit id)
			+ (unitType				? sizeof(uint8) : 0);		// unitTypeId
}

void Command::write(NetworkDataBuffer &buf) const {
	uint8 fieldsPresent;
	assert(commandedUnit);

	buf.write(static_cast<uint16>(commandedUnit->getId()));
	buf.write(static_cast<uint8>((archetype & 0x0f) | ((flags.flags & 0x0f) << 4)));
	fieldsPresent = 
			  (type					? 0x01 : 0)
			| (pos != invalidPos 	? 0x02 : 0)
			| (pos2 != invalidPos	? 0x04 : 0)
			| (unitRef.getUnit()	? 0x08 : 0)
			| (unitRef2.getUnit()	? 0x10 : 0)
			| (unitType				? 0x20 : 0);
	buf.write(fieldsPresent);

	if(fieldsPresent & 0x01) {
		buf.write(static_cast<uint8>(type->getUnitTypeIndex()));
	}

	if(fieldsPresent & 0x02) {
		buf.write(static_cast<uint16>(pos.x));
		buf.write(static_cast<uint16>(pos.y));
	}

	if(fieldsPresent & 0x04) {
		buf.write(static_cast<uint16>(pos2.x));
		buf.write(static_cast<uint16>(pos2.y));
	}

	if(fieldsPresent & 0x08) {
		buf.write(static_cast<uint16>(unitRef.getUnitId()));
	}

	if(fieldsPresent & 0x10) {
		buf.write(static_cast<uint16>(unitRef2.getUnitId()));
	}

	if(fieldsPresent & 0x20) {
		buf.write(static_cast<uint8>(unitType->getId()));
	}
}

void Command::read(NetworkDataBuffer &buf) {
	uint16 commandedUnitId;
	uint8 archetypeAndFlags;
	uint8 fieldsPresent;
	World * world = World::getCurrWorld();

	// commandedUnit
	buf.read(commandedUnitId);
	commandedUnit = world->findUnitById(commandedUnitId);

	// archtype and flags
	buf.read(archetypeAndFlags);
	archetype = static_cast<CommandArchetype>(archetypeAndFlags & 0x0f);
	flags.flags = (archetypeAndFlags & 0xf0) >> 4;

	// fields present
	buf.read(fieldsPresent);

	// commandType
	if(fieldsPresent & 0x01) {
		uint8 commandTypeIndex;
		buf.read(commandTypeIndex);
		type = commandedUnit->getType()->getCommandType(commandTypeIndex);
	}

	// pos
	if(fieldsPresent & 0x02) {
		uint16 x, y;
		buf.read(x);
		buf.read(y);
		pos = Vec2i(x, y);
	}

	// pos2
	if(fieldsPresent & 0x04) {
		uint16 x, y;
		buf.read(x);
		buf.read(y);
		pos2 = Vec2i(x, y);
	}

	// unitRef
	if(fieldsPresent & 0x08) {
		uint16 unitId;
		buf.read(unitId);
		unitRef = world->findUnitById(unitId);
	}

	// unitRef2
	if(fieldsPresent & 0x10) {
		uint16 unitId;
		buf.read(unitId);
		unitRef2 = world->findUnitById(unitId);
	}

	// unitType
	if(fieldsPresent & 0x20) {
		uint8 unitTypeId;
		buf.read(unitTypeId);
		unitType = commandedUnit->getFaction()->getType()->getUnitType(unitTypeId);
	}
}

}}//end namespace
