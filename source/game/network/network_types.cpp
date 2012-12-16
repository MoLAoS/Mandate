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
#include "network_types.h"
#include "command.h"
#include "world.h"
#include "unit.h"
#include "program.h"
#include "sim_interface.h"

#include "leak_dumper.h"

namespace Glest { namespace Net {

using Main::Program;

// =====================================================
//	class NetworkCommand
// =====================================================

MEMORY_CHECK_IMPLEMENTATION(NetworkCommand)

/** Construct from command with archetype == GIVE_COMMAND */
NetworkCommand::NetworkCommand(Command *command) {
	networkCommandType = command->getArchetype();
	unitId = command->getCommandedUnit()->getId();
	commandTypeId = command->getType()->getId();
	positionX = command->getPos().x;
	positionY = command->getPos().y;
	prodTypeId = command->getProdType() ? command->getProdType()->getId() : -1;

	if (command->getType()->getClass() == CmdClass::BUILD || command->getType()->getClass() == CmdClass::CONSTRUCT
	|| command->getType()->getClass() == CmdClass::TRANSFORM) {
		targetId = command->getFacing();
	} else {
		targetId = command->getUnit() ? command->getUnit()->getId() : -1;
	}
	flags = 0;
	if (!command->isReserveResources()) flags |= Flags::NO_RESERVE_RESOURCES;
	if (command->isQueue()) flags |= Flags::QUEUE;
	if (command->isMiscEnabled()) flags |= Flags::MISC_ENABLE;
}

/** Construct archetype SET_AUTO_ [REPAIR|ATTACK|FLEE] */
NetworkCommand::NetworkCommand(NetworkCommandType type, const Unit *unit, bool value) {
	this->networkCommandType = type;
	this->unitId = unit->getId();
	this->commandTypeId = -1;
	this->positionX= -1;
	this->positionY= -1;
	this->prodTypeId = -1;
	this->targetId = -1;
	this->flags = (value ? Flags::MISC_ENABLE : 0);
}

/** Construct archetype CANCEL_COMMAND */
NetworkCommand::NetworkCommand(NetworkCommandType type, const Unit *unit, const Vec2i &pos) {
	RUNTIME_CHECK(type == NetworkCommandType::CANCEL_COMMAND);
	this->networkCommandType = type;
	this->unitId = unit->getId();
	this->commandTypeId = -1;
	this->positionX= pos.x;
	this->positionY= pos.y;
	this->prodTypeId = -1;
	this->targetId = -1;
	this->flags = 0;
}
/*
NetworkCommand::NetworkCommand(int networkCommandType, int unitId, int commandTypeId, const Vec2i &pos, int unitTypeId, int targetId){
	this->networkCommandType= networkCommandType;
	this->unitId= unitId;
	this->commandTypeId= commandTypeId;
	this->positionX= pos.x;
	this->positionY= pos.y;
	this->unitTypeId= unitTypeId;
	this->targetId= targetId;
	this->queue = 0;
}
*/
Command *NetworkCommand::toCommand() const {
	// validate unit
	World &world = World::getInstance();
	Unit* unit= world.findUnitById(unitId);
	if (!unit) {
		throw runtime_error("Can not find unit with id: " + intToStr(unitId) + ". Game out of synch.");
	}

	// handle CmdDirective != GIVE_COMMAND
	if (networkCommandType == NetworkCommandType::CANCEL_COMMAND) {
		return g_world.newCommand(CmdDirective::CANCEL_COMMAND, CmdFlags(), Vec2i(-1), unit);
	}
	if (networkCommandType == NetworkCommandType::SET_AUTO_REPAIR) {
		bool auto_cmd_enable = flags & Flags::MISC_ENABLE;
		return g_world.newCommand(CmdDirective::SET_AUTO_REPAIR,
			CmdFlags(CmdProps::MISC_ENABLE, auto_cmd_enable), Command::invalidPos, unit);
	} else if (networkCommandType == NetworkCommandType::SET_AUTO_ATTACK) {
		bool auto_cmd_enable = flags & Flags::MISC_ENABLE;
		return g_world.newCommand(CmdDirective::SET_AUTO_ATTACK,
			CmdFlags(CmdProps::MISC_ENABLE, auto_cmd_enable), Command::invalidPos, unit);
	} else if (networkCommandType == NetworkCommandType::SET_AUTO_FLEE) {
		bool auto_cmd_enable = flags & Flags::MISC_ENABLE;
		return g_world.newCommand(CmdDirective::SET_AUTO_FLEE,
			CmdFlags(CmdProps::MISC_ENABLE, auto_cmd_enable), Command::invalidPos, unit);
	} else if (networkCommandType == NetworkCommandType::SET_CLOAK) {
		bool enable = flags & Flags::MISC_ENABLE;
		return g_world.newCommand(CmdDirective::SET_CLOAK,
			CmdFlags(CmdProps::MISC_ENABLE, enable), Command::invalidPos, unit);
	}

	// else CmdDirective == GIVE_COMMAND

	// validate command type
	const CommandType* ct = g_prototypeFactory.getCommandType(commandTypeId);
	if (!ct) {
		throw runtime_error("Can not find command type with id: " + intToStr(commandTypeId) + " in unit: " + unit->getType()->getName() + ". Game out of synch.");
	}

	// get target, the target might be dead due to lag, cope with it
	Unit* target = NULL;
	CardinalDir facing = CardinalDir::NORTH;
	if (ct->getClass() == CmdClass::BUILD || ct->getClass() == CmdClass::CONSTRUCT || ct->getClass() == CmdClass::TRANSFORM) {
		facing = enum_cast<CardinalDir>(targetId);
	} else {
		if (targetId != GameConstants::invalidId) {
			target = world.findUnitById(targetId);
		}
	}

	const ProducibleType* prodType = 0;
	if (prodTypeId != -1) {
		prodType = g_prototypeFactory.getProdType(prodTypeId);

		// sanity check...
		assert((g_prototypeFactory.isGeneratedType(prodType) && ct->getClass() == CmdClass::GENERATE)
			|| (g_prototypeFactory.isUpgradeType(prodType) && ct->getClass() == CmdClass::UPGRADE)
			|| (g_prototypeFactory.isItemType(prodType) && ct->getClass() == CmdClass::CREATE_ITEM)
			|| (g_prototypeFactory.isUnitType(prodType)
				&& (ct->getClass() == CmdClass::PRODUCE || ct->getClass() == CmdClass::MORPH
					|| ct->getClass() == CmdClass::BUILD || ct->getClass() == CmdClass::STRUCTURE
                    || ct->getClass() == CmdClass::CONSTRUCT
                    || ct->getClass() == CmdClass::TRANSFORM)));
	}

	// create command
	Command *command= NULL;
	bool queue = flags & Flags::QUEUE;
	bool no_reserve_res = flags & Flags::NO_RESERVE_RESOURCES;
	CmdFlags cmdFlags;
	cmdFlags.set(CmdProps::QUEUE, queue);
	cmdFlags.set(CmdProps::DONT_RESERVE_RESOURCES, no_reserve_res);
	if (target) {
		command = g_world.newCommand(ct, cmdFlags, target, unit);
	} else {
		Vec2i pos(positionX, positionY);
		RUNTIME_CHECK(g_world.getMap()->isInside(pos) || pos == Vec2i(-1));
		if (prodType) {
			command = g_world.newCommand(ct, cmdFlags, Vec2i(positionX, positionY), prodType, facing, unit);
		} else {
			command = g_world.newCommand(ct, cmdFlags, Vec2i(positionX, positionY), unit);
		}
	}
	return command;
}

MoveSkillUpdate::MoveSkillUpdate(const Unit *unit) {
	assert(unit->isMoving());
	Vec2i offset = unit->getNextPos() - unit->getPos();
	if (offset.length() < 1.f || offset.length() > 1.5f) {
		LOG_NETWORK( "ERROR: MoveSKillUpdate being constructed on illegal move." );
	}
	assert(offset.x >= -1 && offset.x <= 1 && offset.y >= -1 && offset.y <= 1);
	assert(offset.x || offset.y);
	assert(unit->getNextCommandUpdate() - g_world.getFrameCount() < 256);

	this->offsetX = offset.x;
	this->offsetY = offset.y;
	this->end_offset = unit->getNextCommandUpdate() - g_world.getFrameCount();
}

ProjectileUpdate::ProjectileUpdate(const Unit *unit, Projectile *pps) {
	assert(pps->getEndFrame() - g_world.getFrameCount() < 256);
	this->end_offset = pps->getEndFrame() - g_world.getFrameCount();
}

}}//end namespace
