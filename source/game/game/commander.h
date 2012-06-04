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

typedef const CommandType*     CmdType;
typedef const ProducibleType*  ProdType;

// =====================================================
// 	class Commander
//
///	Gives commands to the units
// =====================================================

class Commander {
private:
	typedef vector<CmdResult> CmdResults;
	typedef CmdResults::const_iterator CRIterator;
	World *world;
	SimulationInterface *iSim;

public:
	Commander(SimulationInterface *iSim) : world(0), iSim(iSim) { }
    void init(World *world)		{this->world = world;}
	void updateNetwork();

	CmdResult tryUnloadCommand(Unit *unit, CmdFlags flags, const Vec2i &pos, Unit *targetUnit) const;
	CmdResult tryDegarrisonCommand(Unit *unit, CmdFlags flags, const Vec2i &pos, Unit *targetUnit) const;

	// give command(s) to a target pos (move, attack, patrol, guard, harvest, repair ..?)
	// CmdResult tryPosition(UnitVector &units, CmdFlags flags, CmdType commandType, const Vec2i &pos);
	// CmdResult tryPosition(UnitVector &units, CmdFlags flags, CmdClass commandClass, const Vec2i &pos);

	// give command(s) to a target unit (move, attack, patrol, guard, repair, load, ..?)
	// CmdResult tryTarget(UnitVector &units, CmdFlags flags, CmdType commandType, Unit *target);
	// CmdResult tryTarget(UnitVector &units, CmdFlags flags, CmdClass commandClass, Unit *target);

	// give command(s) with producible type (produce, morph, upgrade or generate command)
	// CmdResult tryProduce(UnitVector &units, CmdFlags flags, CmdType commandType, ProdType prodType);
	// CmdResult tryProduce(UnitVector &units, CmdFlags flags, CmdClass commandClass, ProdType prodType);

	// give command(s) with producible type and position (build or transform command)
	// CmdResult tryBuild(UnitVector &units, CmdFlags flags, CmdType commandType, ProdType prodType, Vec2i &pos);
	// CmdResult tryBuild(UnitVector &units, CmdFlags flags, CmdClass commandClass, ProdType prodType, Vec2i &pos);

	//START_DELETE
	CmdResult tryGiveCommand(const Selection *selection, CmdFlags flags,
		const CommandType *commandType = NULL, CmdClass commandClass = CmdClass::NULL_COMMAND,
		const Vec2i &pos = Command::invalidPos, Unit *targetUnit = NULL,
		const ProducibleType* prodType = NULL, CardinalDir facing = CardinalDir::NORTH) const;
	//END_DELETE

	CmdResult tryCancelCommand(const Selection *selection) const;
	void trySetAutoCommandEnabled(const Selection *selection, AutoCmdFlag flag, bool enabled) const;
	void trySetCloak(const Selection *selection, bool enabled) const;

	void giveCommand(Command *command) const;

	CmdResult pushCommand(Command *command) const;
private:
    Vec2i computeRefPos(const Selection *selection) const;
    Vec2i computeDestPos(const Vec2i &refUnitPos, const Vec2i &unitPos, const Vec2i &commandPos) const;
    CmdResult computeResult(const CmdResults &results) const;
};

}} //end namespace

#endif
