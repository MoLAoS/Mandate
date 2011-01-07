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

MEMORY_CHECK_IMPLEMENTATION(Command)

const Vec2i Command::invalidPos = Vec2i(-1);

Command::Command(CreateParamsArch params)//CommandArchetype archetype, CommandFlags flags, const Vec2i &pos, Unit *commandedUnit)
		: m_id(-1)
		, archetype(params.archetype)
		, type(NULL)
		, flags(params.flags)
		, pos(params.pos)
		, pos2(-1)
		, unitRef(-1)
		, unitRef2(-1)
		, prodType(NULL)
		, commandedUnit(params.commandedUnit) {
}

Command::Command(CreateParamsPos params)//const CommandType *type, CommandFlags flags, const Vec2i &pos, Unit *commandedUnit)
		: m_id(-1)
		, archetype(CommandArchetype::GIVE_COMMAND)
		, type(params.type)
		, flags(params.flags)
		, pos(params.pos)
		, pos2(-1)
		, unitRef(-1)
		, unitRef2(-1)
		, prodType(0)
		, commandedUnit(params.commandedUnit) {
}

Command::Command(CreateParamsUnit params)//const CommandType *type, CommandFlags flags, Unit* unit, Unit *commandedUnit)
		: m_id(-1)
		, archetype(CommandArchetype::GIVE_COMMAND)
		, type(params.type)
		, flags(params.flags)
		, pos(-1)
		, pos2(-1)
		, prodType(0)
		, commandedUnit(params.commandedUnit) {
	unitRef = params.unit ? params.unit->getId() : -1;
	unitRef2 = -1;
	
	if (params.unit) {
		pos = params.unit->getCenteredPos();
	}
	if (params.unit && !isAuto() && commandedUnit && commandedUnit->getFaction()->isThisFaction()) {
		params.unit->resetHighlight();
	}
}


Command::Command(CreateParamsProd params)//const CommandType *type, CommandFlags flags, const Vec2i &pos, 
				 //const ProducibleType *prodType, CardinalDir facing, Unit *commandedUnit)
		: m_id(-1)
		, archetype(CommandArchetype::GIVE_COMMAND)
		, type(params.type)
		, flags(params.flags)
		, pos(params.pos)
		, pos2(-1)
		, unitRef(-1)
		, unitRef2(-1)
		, prodType(params.prodType)
		, facing(params.facing)
		, commandedUnit(params.commandedUnit) {
}

Command::Command(CreateParamsLoad params) {// const XmlNode *node, const UnitType *ut, const FactionType *ft) {
	const XmlNode *node = params.node;

	m_id = node->getChildIntValue("id");
	unitRef = node->getOptionalIntValue("unitRef", -1);
	unitRef2 = node->getOptionalIntValue("unitRef2", -1);
	archetype = CommandArchetype(node->getChildIntValue("archetype"));
	type = params.ut->getCommandType(node->getChildStringValue("type"));
	flags.flags = node->getChildIntValue("flags");
	pos = node->getChildVec2iValue("pos");
	pos2 = node->getChildVec2iValue("pos2");
	int prodTypeId = node->getChildIntValue("prodType");
	if (prodTypeId == -1) {
		prodType = 0;
	} else {
		prodType = g_simInterface.getProdType(prodTypeId);
	}
	if (node->getOptionalChild("facing") ) {
		facing = enum_cast<CardinalDir>(node->getChildIntValue("facing"));
	}
}

Unit* Command::getUnit() const {
	return g_world.getUnit(unitRef);
}

Unit* Command::getUnit2() const {
	return g_world.getUnit(unitRef2);
}

void Command::save(XmlNode *node) const {
	node->addChild("id", m_id);
	node->addChild("archetype", archetype);
	node->addChild("type", type->getName());
	node->addChild("flags", (int)flags.flags);
	node->addChild("pos", pos);
	node->addChild("pos2", pos2);
	node->addChild("unitRef", unitRef);
	node->addChild("unitRef2", unitRef2);
	node->addChild("prodType", prodType ? prodType->getId() : -1);
	node->addChild("facing", int(facing));
}

// =============== misc ===============

void Command::swap() {
	std::swap(unitRef, unitRef2);
	std::swap(pos, pos2);
}

}}//end namespace
