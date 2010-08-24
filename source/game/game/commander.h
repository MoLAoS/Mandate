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

#ifndef _GLEST_GAME_COMMANDER_H_
#define _GLEST_GAME_COMMANDER_H_

#include <vector>

#include "vec.h"
#include "selection.h"
#include "command.h"
#include "command_type.h"

using std::vector;
using Shared::Math::Vec2i;

namespace Glest { namespace Sim {
	class SimulationInterface;
}}
using Glest::Sim::SimulationInterface;

namespace Glest { namespace Sim {
using namespace Gui;

// =====================================================
// 	class Commander
//
///	Gives commands to the units
// =====================================================

class Commander {
private:
	typedef vector<CommandResult> CommandResultContainer;
	typedef CommandResultContainer::const_iterator CRIterator;
	World *world;
	SimulationInterface *iSim;

public:
	Commander(SimulationInterface *iSim) : world(0), iSim(iSim) { }
    void init(World *world)		{this->world = world;}
	void updateNetwork();

	CommandResult tryGiveCommand(const Selection &selection, CommandFlags flags,
		const CommandType *commandType = NULL, CommandClass commandClass = CommandClass::NULL_COMMAND,
		const Vec2i &pos = Command::invalidPos, Unit *targetUnit = NULL,
		const UnitType* unitType = NULL, CardinalDir facing = CardinalDir::NORTH) const;

	CommandResult tryCancelCommand(const Selection *selection) const;
	
	void trySetAutoRepairEnabled(const Selection &selection, CommandFlags flags, bool enabled) const;

	void giveCommand(Command *command) const;

private:
	CommandResult pushCommand(Command *command) const;
    Vec2i computeRefPos(const Selection &selection) const;
    Vec2i computeDestPos(const Vec2i &refUnitPos, const Vec2i &unitPos, const Vec2i &commandPos) const;
    CommandResult computeResult(const CommandResultContainer &results) const;
};

}} //end namespace

#endif
