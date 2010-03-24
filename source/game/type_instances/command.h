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

#ifndef _GLEST_GAME_COMMAND_H_
#define _GLEST_GAME_COMMAND_H_

#include <cstdlib>

#include "unit.h"
#include "vec.h"
#include "types.h"
#include "flags.h"
#include "socket.h"

namespace Glest{ namespace Game{

using Shared::Math::Vec2i;
using Shared::Platform::int16;

class CommandType;

typedef Flags<CommandProperties, CommandProperties::COUNT, uint8> CommandFlags;

// =====================================================
// 	class Command
//
///	A unit command
// =====================================================

class Command {
public:

	static const Vec2i invalidPos;

private:
	CommandArchetype archetype;
	const CommandType *type;
	CommandFlags flags;
	Vec2i pos;
	Vec2i pos2;					//for patrol command, the position traveling away from.
	UnitReference unitRef;		//target unit, used to move and attack optinally
	UnitReference unitRef2;		//for patrol command, the unit traveling away from.
	const UnitType *unitType;	//used for build
	Unit *commandedUnit;

public:
	//constructor
	Command(CommandArchetype archetype, CommandFlags flags, const Vec2i &pos = invalidPos, Unit *commandedUnit = NULL);
	Command(const CommandType *type, CommandFlags flags, const Vec2i &pos = invalidPos, Unit *commandedUnit = NULL);
	Command(const CommandType *type, CommandFlags flags, Unit *unit, Unit *commandedUnit = NULL);
	Command(const CommandType *type, CommandFlags flags, const Vec2i &pos, const UnitType *unitType, Unit *commandedUnit = NULL);
	Command(const XmlNode *node, const UnitType *ut, const FactionType *ft);

	// allow default ctor

	//get
	CommandArchetype getArchetype() const		{return archetype;}
	const CommandType *getType() const			{return type;}
	CommandFlags getFlags() const				{return flags;}
	bool isQueue() const						{return flags.get(CommandProperties::QUEUE);}
	bool isAuto() const							{return flags.get(CommandProperties::AUTO);}
	bool isReserveResources() const				{return !flags.get(CommandProperties::DONT_RESERVE_RESOURCES);}
	bool isAutoRepairEnabled() const			{return flags.get(CommandProperties::AUTO_REPAIR_ENABLED);}
	Vec2i getPos() const						{return pos;}
	Vec2i getPos2() const						{return pos2;}
	Unit* getUnit() const						{return unitRef.getUnit();}
	Unit* getUnit2() const						{return unitRef2.getUnit();}
	const UnitType* getUnitType() const			{return unitType;}
	Unit *getCommandedUnit() const				{return commandedUnit;}

	bool hasPos() const							{return pos.x != -1;}
	bool hasPos2() const						{return pos2.x != -1;}

	//set
	void setType(const CommandType *type)				{this->type= type;}
	void setFlags(CommandFlags flags)					{this->flags = flags;}
	void setQueue(bool queue)							{flags.set(CommandProperties::QUEUE, queue);}
	void setAuto(bool _auto)							{flags.set(CommandProperties::AUTO, _auto);}
	void setReserveResources(bool reserveResources)		{flags.set(CommandProperties::DONT_RESERVE_RESOURCES, !reserveResources);}
	void setAutoRepairEnabled(bool enabled)				{flags.set(CommandProperties::AUTO_REPAIR_ENABLED, enabled);}
	void setPos(const Vec2i &pos)						{this->pos = pos;}
	void setPos2(const Vec2i &pos2)						{this->pos2 = pos2;}

	void setUnit(Unit *unit)							{this->unitRef = unit;}
	void setUnit2(Unit *unit2)							{this->unitRef2 = unit2;}	
	void setUnitType(const UnitType* unitType)			{this->unitType = unitType;}
	void setCommandedUnit(Unit *commandedUnit)			{this->commandedUnit = commandedUnit;}

	//misc
	void swap();
	void popPos()										{pos = pos2; pos2 = invalidPos;}
	void save(XmlNode *node) const;
};

}}//end namespace

#endif
