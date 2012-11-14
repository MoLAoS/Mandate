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

CmdResult Commander::tryUnloadCommand(Unit *unit, CmdFlags flags, const Vec2i &pos, Unit *targetUnit) const {
    if (const CommandType *ct = unit->getFirstAvailableCt(CmdClass::UNLOAD)) {
	    if (!ct) {
		    assert(false);
		    return CmdResult::FAIL_UNDEFINED;
	    }
	    return pushCommand(g_world.newCommand(ct, CmdFlags(), targetUnit, unit));
    } else {
        assert(false);
        return CmdResult::FAIL_UNDEFINED;
    }
}

CmdResult Commander::tryDegarrisonCommand(Unit *unit, CmdFlags flags, const Vec2i &pos, Unit *targetUnit) const {
    if (const CommandType *ct = unit->getFirstAvailableCt(CmdClass::DEGARRISON)) {
	    if (!ct) {
		    assert(false);
		    return CmdResult::FAIL_UNDEFINED;
	    }
	    return pushCommand(g_world.newCommand(ct, CmdFlags(), targetUnit, unit));
    } else {
        assert(false);
        return CmdResult::FAIL_UNDEFINED;
    }
}

CmdResult Commander::tryGiveCommand(const Selection *selection, CmdFlags flags,
		const CommandType *ct, CmdClass cc, const Vec2i &pos, Unit *targetUnit,
		const ProducibleType* prodType, CardinalDir facing) const {
	//COMMAND_LOG(__FUNCTION__ << "() " << selection.getUnits().size() << " units selected.");
	if (selection->isEmpty()) {
		//COMMAND_LOG(__FUNCTION__ << "() No units selected!");
		return CmdResult::FAIL_UNDEFINED;
	}
	assert(!(prodType && targetUnit));

	Vec2i refPos = computeRefPos(selection);
	CmdResults results;

	// give orders to all selected units
	const UnitVector &units = selection->getUnits();
	CmdResult result;

	foreach_const (UnitVector, i, units) {
		const CommandType *effectiveCt;
		if (ct) {
			effectiveCt = ct;
			//COMMAND_LOG(__FUNCTION__ << "() " << **i << " trying command " << ct->getName() );
		} else if (cc != CmdClass::NULL_COMMAND) {
			effectiveCt = (*i)->getFirstAvailableCt(cc);
			//COMMAND_LOG(__FUNCTION__ << "() " << **i << " trying first command of class " << CmdClassNames[cc] );
		} else {
			effectiveCt = (*i)->computeCommandType(pos, targetUnit);
			//COMMAND_LOG(__FUNCTION__ << "() " << **i << " trying default, with pos " << pos << " and target " << (targetUnit ? targetUnit->getId() : -1));
			//if (effectiveCt) {
			//	COMMAND_LOG(__FUNCTION__ << "() " << **i << " computed command = " << effectiveCt->getName());
			//} else {
			//	COMMAND_LOG(__FUNCTION__ << "() " << **i << " no defualt command could be computed.");
			//}
		}
		if(effectiveCt) {
			if (prodType) { // production command
				if (effectiveCt->getClass() == CmdClass::BUILD) {
					//COMMAND_LOG(__FUNCTION__ << "() build command, setting DONT_RESERVE_RESOURCES flag = "
					//	<< (i != units.begin() ? "true." : "false."));
					if (!flags.get(CmdProps::DONT_RESERVE_RESOURCES) && i != units.begin()) {
						flags.set(CmdProps::DONT_RESERVE_RESOURCES, true);
					}
				}
				if (effectiveCt->getClass() == CmdClass::STRUCTURE) {
					if (!flags.get(CmdProps::DONT_RESERVE_RESOURCES) && i != units.begin()) {
						flags.set(CmdProps::DONT_RESERVE_RESOURCES, true);
					}
				}
				if (effectiveCt->getClass() == CmdClass::CONSTRUCT) {
					if (!flags.get(CmdProps::DONT_RESERVE_RESOURCES) && i != units.begin()) {
						flags.set(CmdProps::DONT_RESERVE_RESOURCES, true);
					}
				}
				result = pushCommand(g_world.newCommand(effectiveCt, flags, pos, prodType, facing, *i));
			} else if (targetUnit) { // 'target' based command
				if (effectiveCt->getClass() == CmdClass::LOAD) {
					if (*i != targetUnit) {
						// give *i a command to load targetUnit
						result = pushCommand(g_world.newCommand(effectiveCt, flags, targetUnit, *i));
						if (result == CmdResult::SUCCESS) {
							// if load is ok, give targetUnit a command to be-loaded by *i
							effectiveCt = targetUnit->getFirstAvailableCt(CmdClass::BE_LOADED);
							pushCommand(g_world.newCommand(effectiveCt, CmdFlags(), *i, targetUnit));
						}
					}
				} else if (effectiveCt->getClass() == CmdClass::FACTIONLOAD) {
					if (*i != targetUnit) {
						// give *i a command to load targetUnit
						result = pushCommand(g_world.newCommand(effectiveCt, flags, targetUnit, *i));
						if (result == CmdResult::SUCCESS) {
							// if load is ok, give targetUnit a command to be-loaded by *i
							effectiveCt = targetUnit->getFirstAvailableCt(CmdClass::BE_LOADED);
							pushCommand(g_world.newCommand(effectiveCt, CmdFlags(), *i, targetUnit));
						}
					}
				} else if (effectiveCt->getClass() == CmdClass::GARRISON) {
					if (*i != targetUnit) {
						// give *i a command to load targetUnit
						result = pushCommand(g_world.newCommand(effectiveCt, flags, targetUnit, *i));
						if (result == CmdResult::SUCCESS) {
							// if load is ok, give targetUnit a command to be-loaded by *i
							effectiveCt = targetUnit->getFirstAvailableCt(CmdClass::BE_LOADED);
							pushCommand(g_world.newCommand(effectiveCt, CmdFlags(), *i, targetUnit));
						}
					}
				} else if (effectiveCt->getClass() == CmdClass::BE_LOADED) {
					// a carrier unit was right clicked
					result = pushCommand(g_world.newCommand(targetUnit->getFirstAvailableCt(CmdClass::LOAD), CmdFlags(), *i, targetUnit));
					if (result == CmdResult::SUCCESS) {
						// give *i a be-loaded command with targetUnit
						pushCommand(g_world.newCommand(effectiveCt, flags, targetUnit, *i));
					}
				} else {
					result = pushCommand(g_world.newCommand(effectiveCt, flags, targetUnit, *i));
				}
			} else if (effectiveCt->getClass() == CmdClass::LOAD ||
              effectiveCt->getClass() == CmdClass::FACTIONLOAD ||
              effectiveCt->getClass() == CmdClass::GARRISON) {
				// the player has tried to load nothing, it shouldn't get here if it has
				// a targetUnit
				result = CmdResult::FAIL_UNDEFINED;
			} else if(pos != Command::invalidPos) { // 'position' based command
				//every unit is ordered to a different pos
				Vec2i currPos = computeDestPos(refPos, (*i)->getPos(), pos);
				result = pushCommand(g_world.newCommand(effectiveCt, flags, currPos, *i));
			} else {
				result = pushCommand(g_world.newCommand(effectiveCt, flags, Command::invalidPos, *i));
			}
			results.push_back(result);
		} else {
			results.push_back(CmdResult::FAIL_UNDEFINED);
		}
	}

	return computeResult(results);
}

CmdResult Commander::tryCancelCommand(const Selection *selection) const{
	const UnitVector &units = selection->getUnits();
	for(Selection::UnitIterator i = units.begin(); i != units.end(); ++i) {
		//COMMAND_LOG(__FUNCTION__ << "() " << *i << " trying cancel command.");
		pushCommand(g_world.newCommand(CmdDirective::CANCEL_COMMAND, CmdFlags(), Command::invalidPos, *i));
	}

	return CmdResult::SUCCESS;
}

void Commander::trySetAutoCommandEnabled(const Selection *selection, AutoCmdFlag flag, bool enabled) const {
	CmdDirective archetype;
	CmdFlags cmdFlags = CmdFlags(CmdProps::MISC_ENABLE, enabled);
	switch (flag) {
		case AutoCmdFlag::REPAIR:
			archetype = CmdDirective::SET_AUTO_REPAIR;
			break;
		case AutoCmdFlag::ATTACK:
			archetype = CmdDirective::SET_AUTO_ATTACK;
			break;
		case AutoCmdFlag::FLEE:
			archetype = CmdDirective::SET_AUTO_FLEE;
			break;
	}
	if (iSim->isNetworkInterface()) {
		g_console.addLine(g_lang.get("NotAvailable"));
	} else {
		const UnitVector &units = selection->getUnits();
		foreach_const (UnitVector, i, units) {
			Command *c = g_world.newCommand(archetype, cmdFlags, Command::invalidPos, *i);
			pushCommand(c);
		}
	}
}

void Commander::trySetCloak(const Selection *selection, bool enabled) const {
	CmdFlags flags(CmdProps::MISC_ENABLE, enabled);
	foreach_const (UnitVector, it, selection->getUnits()) {
		if ((*it)->getType()->getCloakClass() == CloakClass::ENERGY) {
			Command *c = g_world.newCommand(CmdDirective::SET_CLOAK, flags, Command::invalidPos, *it);
			pushCommand(c);
		}
	}
}

// ==================== PRIVATE ====================

Vec2i Commander::computeRefPos(const Selection *selection) const{
	Vec2i total = Vec2i(0);
	for (int i = 0; i < selection->getCount(); ++i) {
		total = total + selection->getUnit(i)->getPos();
	}

	return Vec2i(total.x / selection->getCount(), total.y / selection->getCount());
}

Vec2i Commander::computeDestPos(const Vec2i &refPos, const Vec2i &unitPos, const Vec2i &cmdPos) const {
	Vec2i pos;
	Vec2i posDiff = unitPos - refPos;

	if (abs(posDiff.x) >= 3) {
		posDiff.x = posDiff.x % 3;
	}

	if (abs(posDiff.y) >= 3) {
		posDiff.y = posDiff.y % 3;
	}

	pos = cmdPos + posDiff;
	world->getMap()->clampPos(pos);
	return pos;
}

CmdResult Commander::computeResult(const CmdResults &results) const {
	switch (results.size()) {
	case 0:
		return CmdResult::FAIL_UNDEFINED;
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
				if (*i == CmdResult::SUCCESS) {
					anySucceed = true;
				} else {
					anyFail = true;
				}
			}
			if (anySucceed) {
				return anyFail ? CmdResult::SOME_FAILED : CmdResult::SUCCESS;
			} else {
				return unique ? results.front() : CmdResult(CmdResult::FAIL_UNDEFINED);
			}
		}
	}
}

CmdResult Commander::pushCommand(Command *command) const {
	RUNTIME_CHECK(command);
	RUNTIME_CHECK(command->getCommandedUnit());
	CmdResult result = command->getCommandedUnit()->checkCommand(*command);
	//COMMAND_LOG( __FUNCTION__ << "(): " << *command->getCommandedUnit() << ", " << *command
	//	<< ", Result=" << CmdResultNames[result] );
	if (result == CmdResult::SUCCESS) {
		iSim->requestCommand(command);
	} else {
		g_world.deleteCommand(command);
	}
	return result;
}

void Commander::giveCommand(Command *command) const {
	//COMMAND_LOG( __FUNCTION__ << "(): " << *command->getCommandedUnit() << ", " << *command );
	Unit* unit = command->getCommandedUnit();

	//execute command, if unit is still alive and non-deleted
	if (unit && unit->isAlive()) {
		switch (command->getArchetype()) {
			case CmdDirective::GIVE_COMMAND:
				assert(command->getType());
				unit->giveCommand(command);
				break;
			case CmdDirective::CANCEL_COMMAND:
				unit->cancelCommand();
				g_world.deleteCommand(command);
				break;
			case CmdDirective::SET_AUTO_REPAIR:
				unit->setAutoCmdEnable(AutoCmdFlag::REPAIR, command->isMiscEnabled());
				g_world.deleteCommand(command);
				break;
			case CmdDirective::SET_AUTO_ATTACK:
				unit->setAutoCmdEnable(AutoCmdFlag::ATTACK, command->isMiscEnabled());
				g_world.deleteCommand(command);
				break;
			case CmdDirective::SET_AUTO_FLEE:
				unit->setAutoCmdEnable(AutoCmdFlag::FLEE, command->isMiscEnabled());
				g_world.deleteCommand(command);
				break;
			case CmdDirective::SET_CLOAK: {
				bool cloak = command->isMiscEnabled();
				if (cloak && !unit->isCloaked()) {
					if (unit->getFaction()->reqsOk(unit->getType()->getCloakType())) {
						unit->cloak();
					}
				} else if (!cloak && unit->isCloaked()) {
					unit->deCloak();
				}
				break;
			}
			default:
				assert(false);
		}
	} else {
		g_world.deleteCommand(command);
	}
}

}}//end namespace
