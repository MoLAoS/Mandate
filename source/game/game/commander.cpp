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
#include "console.h"
#include "config.h"
#include "platform_util.h"
#include "sim_interface.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest { namespace Sim {

// =====================================================
// 	class Commander
// =====================================================

// ===================== PUBLIC ========================

CommandResult Commander::tryUnloadCommand(Unit *unit, CommandFlags flags, const Vec2i &pos, Unit *targetUnit) const {
	const CommandType *ct = unit->getFirstAvailableCt(CommandClass::UNLOAD);
	if (!ct) {
		assert(false);
		return CommandResult::FAIL_UNDEFINED;
	}
	return pushCommand(new Command(ct, CommandFlags(), targetUnit, unit));
}


CommandResult Commander::tryGiveCommand(const Selection &selection, CommandFlags flags,
		const CommandType *ct, CommandClass cc, const Vec2i &pos, Unit *targetUnit,
		const ProducibleType* prodType, CardinalDir facing) const {
	COMMAND_LOG(__FUNCTION__ << "() " << selection.getUnits().size() << " units selected.");
	if (selection.isEmpty()) {
		COMMAND_LOG(__FUNCTION__ << "() No units selected!");
		return CommandResult::FAIL_UNDEFINED;
	}
	assert(!(prodType && targetUnit));

	Vec2i refPos = computeRefPos(selection);
	CommandResultContainer results;

	// give orders to all selected units
	const UnitVector &units = selection.getUnits();
	CommandResult result;

	foreach_const (UnitVector, i, units) {
		const CommandType *effectiveCt;
		if (ct) {
			effectiveCt = ct;
			COMMAND_LOG(__FUNCTION__ << "() " << **i << " trying command " << ct->getName() );
		} else if (cc != CommandClass::NULL_COMMAND) {
			effectiveCt = (*i)->getFirstAvailableCt(cc);
			COMMAND_LOG(__FUNCTION__ << "() " << **i << " trying first command of class " << CommandClassNames[cc] );
		} else {
			effectiveCt = (*i)->computeCommandType(pos, targetUnit);
			COMMAND_LOG(__FUNCTION__ << "() " << **i << " trying default, with pos " << pos << " and target " << (targetUnit ? targetUnit->getId() : -1));
			if (effectiveCt) {
				COMMAND_LOG(__FUNCTION__ << "() " << **i << " computed command = " << effectiveCt->getName());
			} else {
				COMMAND_LOG(__FUNCTION__ << "() " << **i << " no defualt command could be computed.");
			}
		}
		if(effectiveCt) {
			if (prodType) { // production command
				if (effectiveCt->getClass() == CommandClass::BUILD) {
					COMMAND_LOG(__FUNCTION__ << "() build command, setting DONT_RESERVE_RESOURCES flag = "
						<< (i != units.begin() ? "true." : "false."));
					flags.set(CommandProperties::DONT_RESERVE_RESOURCES, i != units.begin());
				}
				result = pushCommand(new Command(effectiveCt, flags, pos, prodType, facing, *i));
			} else if (targetUnit) { // 'target' based command
				if (effectiveCt->getClass() == CommandClass::LOAD) {
					if (*i != targetUnit) {
						// give *i a command to load targetUnit
						result = pushCommand(new Command(effectiveCt, flags, targetUnit, *i));
						if (result == CommandResult::SUCCESS) {
							// if load is ok, give targetUnit a command to be-loaded by *i
							effectiveCt = targetUnit->getFirstAvailableCt(CommandClass::BE_LOADED);
							pushCommand(new Command(effectiveCt, CommandFlags(), *i, targetUnit));
						}
					}
				} else if (effectiveCt->getClass() == CommandClass::BE_LOADED) {
					// a carrier unit was right clicked
					result = pushCommand(new Command(targetUnit->getFirstAvailableCt(CommandClass::LOAD), CommandFlags(), *i, targetUnit));
					if (result == CommandResult::SUCCESS) {
						// give *i a be-loaded command with targetUnit
						pushCommand(new Command(effectiveCt, flags, targetUnit, *i));
					}
				} else {
					result = pushCommand(new Command(effectiveCt, flags, targetUnit, *i));
				}
			} else if (effectiveCt->getClass() == CommandClass::LOAD) {
				// the player has tried to load nothing, it shouldn't get here if it has 
				// a targetUnit
				result = CommandResult::FAIL_UNDEFINED;
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
	const UnitVector &units = selection->getUnits();
	for(Selection::UnitIterator i = units.begin(); i != units.end(); ++i) {
		COMMAND_LOG(__FUNCTION__ << "() " << *i << " trying cancel command.");
		pushCommand(new Command(CommandArchetype::CANCEL_COMMAND, CommandFlags(), Command::invalidPos, *i));
	}

	return CommandResult::SUCCESS;
}

void Commander::trySetAutoRepairEnabled(const Selection &selection, CommandFlags flags, bool enabled) const {
	if (iSim->isNetworkInterface()) {
		g_console.addLine(g_lang.get("NotAvailable"));
	} else {
		const UnitVector &units = selection.getUnits();
		foreach_const (UnitVector, i, units) {
			Command *c = new Command(CommandArchetype::SET_AUTO_REPAIR, CommandFlags(CommandProperties::AUTO_REPAIR_ENABLED, enabled), 
				Command::invalidPos, *i);
			pushCommand(c);
		}
	}
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
	RUNTIME_CHECK(command);
	RUNTIME_CHECK(command->getCommandedUnit());
	CommandResult result = command->getCommandedUnit()->checkCommand(*command);
	COMMAND_LOG( __FUNCTION__ << "(): " << *command->getCommandedUnit() << ", " << *command << ", Result=" << CommandResultNames[result] );
	if (result == CommandResult::SUCCESS) {
		iSim->requestCommand(command);
	} else {
		delete command;
	}
	return result;
}

void Commander::giveCommand(Command *command) const {
	COMMAND_LOG( __FUNCTION__ << "(): " << *command->getCommandedUnit() << ", " << *command );
	Unit* unit = command->getCommandedUnit();

	//execute command, if unit is still alive and non-deleted
	if (unit && unit->isAlive()) {
		switch (command->getArchetype()) {
			case CommandArchetype::GIVE_COMMAND:
				assert(command->getType());
				unit->giveCommand(command);
				break;
			case CommandArchetype::CANCEL_COMMAND:
				unit->cancelCommand();
				delete command;
				break;
			case CommandArchetype::SET_AUTO_REPAIR:
				unit->setAutoRepairEnabled(command->isAutoRepairEnabled());
				delete command;
				break;
			default:
				assert(false);
		}
	} else {
		delete command;
	}
}

}}//end namespace
