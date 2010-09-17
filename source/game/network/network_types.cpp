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

#include "leak_dumper.h"

namespace Glest { namespace Net {

// =====================================================
//	class NetworkCommand
// =====================================================

/** Construct from command with archetype == GIVE_COMMAND */
NetworkCommand::NetworkCommand(Command *command) {
	networkCommandType = command->getArchetype();
	unitId = command->getCommandedUnit()->getId();
	commandTypeId = command->getType()->getId();
	positionX = command->getPos().x;
	positionY = command->getPos().y;
	prodTypeId = command->getProdType() ? command->getProdType()->getId() : -1;

	if (command->getType()->getClass() == CommandClass::BUILD) {
		targetId = command->getFacing();
	} else {
		targetId = command->getUnit() ? command->getUnit()->getId() : -1;
	}
	flags = 0;
	if (!command->isReserveResources()) flags |= CmdFlags::NO_RESERVE_RESOURCES;
	if (command->isQueue()) flags |= CmdFlags::QUEUE;
}

/** Construct archetype CANCEL_COMMAND */
NetworkCommand::NetworkCommand(NetworkCommandType type, const Unit *unit, const Vec2i &pos) {
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

	if (networkCommandType == NetworkCommandType::CANCEL_COMMAND) {
		return new Command(CommandArchetype::CANCEL_COMMAND, 0, Vec2i(-1), unit);
	}

	// validate command type
	const CommandType* ct = g_world.getCommandTypeFactory().getType(commandTypeId);
	if (!ct) {
		throw runtime_error("Can not find command type with id: " + intToStr(commandTypeId) + " in unit: " + unit->getType()->getName() + ". Game out of synch.");
	}

	// get target, the target might be dead due to lag, cope with it
	Unit* target = NULL;
	CardinalDir facing = CardinalDir::NORTH;
	if (ct->getClass() != CommandClass::BUILD) {
		if (targetId != GameConstants::invalidId) {
			target = world.findUnitById(targetId);
		}
	} else {
		facing = enum_cast<CardinalDir>(targetId);
	}

	const ProducibleType* prodType = 0;
	if (prodTypeId != -1) {
		MasterTypeFactory &typeFactory = world.getMasterTypeFactory();
		prodType = typeFactory.getType(prodTypeId);

		// sanity check... 
		assert((typeFactory.isGeneratedType(prodType) && ct->getClass() == CommandClass::GENERATE)
			|| (typeFactory.isUpgradeType(prodType) && ct->getClass() == CommandClass::UPGRADE)
			|| (typeFactory.isUnitType(prodType)
				&& (ct->getClass() == CommandClass::PRODUCE
					|| ct->getClass() == CommandClass::MORPH
					|| ct->getClass() == CommandClass::BUILD)));
	}

	// create command
	Command *command= NULL;
	bool queue = flags & CmdFlags::QUEUE;
	bool no_reserve_res = flags & CmdFlags::NO_RESERVE_RESOURCES;
	CommandFlags flags(CommandProperties::QUEUE, queue, CommandProperties::DONT_RESERVE_RESOURCES, no_reserve_res);
	
	if (target) {
		command= new Command(ct, flags, target, unit);
	} else if (prodType) {
		command= new Command(ct, flags, Vec2i(positionX, positionY), prodType, facing, unit);
	} else {
		command= new Command(ct, flags, Vec2i(positionX, positionY), unit);
	}

	return command;
}

MoveSkillUpdate::MoveSkillUpdate(const Unit *unit) {
	assert(unit->getCurrSkill()->getClass() == SkillClass::MOVE);
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
