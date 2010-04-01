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
#include "commander.h"

#include "world.h"
#include "unit.h"
#include "conversion.h"
#include "upgrade.h"
#include "command.h"
#include "command_type.h"
#include "network_manager.h"
#include "console.h"
#include "config.h"
#include "platform_util.h"

#include "leak_dumper.h"


using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest{ namespace Game{

// =====================================================
// 	class Commander
// =====================================================

// ===================== PUBLIC ========================

CommandResult Commander::tryGiveCommand(
		const Selection &selection,
		CommandFlags flags,
		const CommandType *ct,
		CommandClass cc,
		const Vec2i &pos,
		Unit *targetUnit,
		const UnitType* unitType) const {
	if (selection.isEmpty()) {
		return CommandResult::FAIL_UNDEFINED;
	}
	// 'build' related asserts, if unitType is non null, there mustn't be a target unit, 
	// must be a pos, and command class must be build
	assert(!(unitType && targetUnit));
	assert(!unitType || pos != Command::invalidPos);
	assert(!unitType || ((ct && ct->getClass() == CommandClass::BUILD) || cc == CommandClass::BUILD));

	Vec2i refPos = computeRefPos(selection);
	CommandResultContainer results;

	//give orders to all selected units
	const Selection::UnitContainer &units = selection.getUnits();

	foreach_const (Selection::UnitContainer, i, units) {
		const CommandType *effectiveCt;
		if(ct) {
			effectiveCt = ct;
		} else if(cc != CommandClass::NULL_COMMAND) {
			effectiveCt = (*i)->getFirstAvailableCt(cc);
		} else {
			effectiveCt = (*i)->computeCommandType(pos, targetUnit);
		}
		if(effectiveCt) {
			CommandResult result;
			if(unitType) { // build command
				result = pushCommand(new Command(effectiveCt, flags, pos, unitType, *i));
			} else if(targetUnit) { // 'target' based command
				result = pushCommand(new Command(effectiveCt, flags, targetUnit, *i));
			} else if(pos != Command::invalidPos) { // 'position' based command
				//every unit is ordered to a different pos
				Vec2i currPos = computeDestPos(refPos, (*i)->getPos(), pos);
				result = pushCommand(new Command(effectiveCt, flags, currPos, *i));
			} else {
				result = pushCommand(new Command(effectiveCt, flags, Command::invalidPos, *i));
			}
			results.push_back(result);
		} else {
			results.push_back(CommandResult::FAIL_UNDEFINED);
		}
	}
	return computeResult(results);

}

CommandResult Commander::tryCancelCommand(const Selection *selection) const{
	const Selection::UnitContainer &units = selection->getUnits();
	for(Selection::UnitIterator i = units.begin(); i != units.end(); ++i) {
		pushCommand(new Command(CommandArchetype::CANCEL_COMMAND, CommandFlags(), Command::invalidPos, *i));
	}

	return CommandResult::SUCCESS;
}

void Commander::trySetAutoRepairEnabled(const Selection &selection, CommandFlags flags, bool enabled) const{
	/*
	const Selection::UnitContainer &units = selection.getUnits();
	for(Selection::UnitIterator i = units.begin(); i != units.end(); ++i) {
		pushCommand(new Command(CommandArchetype::SET_AUTO_REPAIR, CommandFlags(CommandProperties::AUTO_REPAIR_ENABLED, enabled),
				Command::invalidPos, *i));
	}
	*/
}

// ==================== PRIVATE ====================

Vec2i Commander::computeRefPos(const Selection &selection) const{
	Vec2i total = Vec2i(0);
	for (int i = 0; i < selection.getCount(); ++i) {
		total = total + selection.getUnit(i)->getPos();
	}

	return Vec2i(total.x / selection.getCount(), total.y / selection.getCount());
}

Vec2i Commander::computeDestPos(const Vec2i &refUnitPos, const Vec2i &unitPos, const Vec2i &commandPos) const {
	Vec2i pos;
	Vec2i posDiff = unitPos - refUnitPos;

	if (abs(posDiff.x) >= 3) {
		posDiff.x = posDiff.x % 3;
	}

	if (abs(posDiff.y) >= 3) {
		posDiff.y = posDiff.y % 3;
	}

	pos = commandPos + posDiff;
	world->getMap()->clampPos(pos);
	return pos;
}

CommandResult Commander::computeResult(const CommandResultContainer &results) const {
	switch (results.size()) {
	case 0:
		return CommandResult::FAIL_UNDEFINED;
	case 1:
		return results.front();
	default: {
			bool anySucceed = false;
			bool anyFail = false;
			bool unique = true;
			for (CRIterator i = results.begin(); i != results.end(); ++i) {
				if (*i != results.front()) {
					unique = false;
				}
				if (*i == CommandResult::SUCCESS) {
					anySucceed = true;
				} else {
					anyFail = true;
				}
			}
			if (anySucceed) {
				return anyFail ? CommandResult::SOME_FAILED : CommandResult::SUCCESS;
			} else {
				return unique ? results.front() : CommandResult(CommandResult::FAIL_UNDEFINED);
			}
		}
	}
}

CommandResult Commander::pushCommand(Command *command) const {
	assert(command->getCommandedUnit());
	GameInterface *gameInterface = NetworkManager::getInstance().getGameInterface();
	CommandResult result = command->getCommandedUnit()->checkCommand(*command);
	gameInterface->requestCommand(command);
	return result;
}

void Commander::updateNetwork() {
	NetworkManager &networkManager = NetworkManager::getInstance();
	GameInterface *gni = NetworkManager::getInstance().getGameInterface();

	if (!networkManager.isNetworkGame() || world->getFrameCount() % GameConstants::networkFramePeriod == 0) {
		//update the keyframe
		gni->doUpdateKeyframe(world->getFrameCount());
		//give pending commands
		for (int i = 0; i < gni->getPendingCommandCount(); ++i) {
			Command *cmd = gni->getPendingCommand(i);
			giveCommand(cmd);
		}
		gni->clearPendingCommands();
	}
}

void Commander::giveCommand(Command *command) const {
	Unit* unit = command->getCommandedUnit();

	//execute command, if unit is still alive and non-deleted
	if(unit && unit->isAlive()) {
		switch(command->getArchetype()) {
			case CommandArchetype::GIVE_COMMAND:
				assert(command->getType());
				unit->giveCommand(command);
				break;
			case CommandArchetype::CANCEL_COMMAND:
				unit->cancelCommand();
				delete command;
				break;
			/*
			case CommandArchetype::SET_AUTO_REPAIR:
				unit->setAutoRepairEnabled(command->isAutoRepairEnabled());
				delete command;
				break;
			*/
			default:
				assert(false);
		}
	}
}

}}//end namespace
