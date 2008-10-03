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

namespace Glest{ namespace Game{

using Shared::Graphics::Vec2i;
using Shared::Platform::int16;

class CommandType;

enum CommandProperties {
	cpQueue,
	cpAuto,
	cpDontReserveResources,

	cpCount
};

typedef Flags<CommandProperties, cpCount, int8> CommandFlags;

// =====================================================
// 	class Command
//
///	A unit command
// =====================================================

class Command {

private:
    const CommandType *commandType;
	CommandFlags flags;
    Vec2i pos;
    Vec2i pos2;					//for patrol command, the position traveling away from.
	UnitReference unitRef;		//target unit, used to move and attack optinally
	UnitReference unitRef2;		//for patrol command, the unit traveling away from.
	const UnitType *unitType;	//used for build

public:
    //constructor
    Command(const CommandType *ct, CommandFlags flags, const Vec2i &pos=Vec2i(0));
    Command(const CommandType *ct, CommandFlags flags, Unit *unit);
    Command(const CommandType *ct, CommandFlags flags, const Vec2i &pos, const UnitType *unitType);
	Command(const XmlNode *node, const UnitType *ut, const FactionType *ft);

    //get
	const CommandType *getCommandType() const	{return commandType;}
	CommandFlags getFlags() const				{return flags;}
	bool isQueue() const						{return flags.get(cpQueue);}
	bool isAuto() const							{return flags.get(cpAuto);}
	bool isReserveResources() const				{return !flags.get(cpDontReserveResources);}
	Vec2i getPos() const						{return pos;}
	Vec2i getPos2() const						{return pos2;}
	Unit* getUnit() const						{return unitRef.getUnit();}
	Unit* getUnit2() const						{return unitRef2.getUnit();}
	const UnitType* getUnitType() const			{return unitType;}

	bool hasPos() const							{return pos.x != -1;}
	bool hasPos2() const						{return pos2.x != -1;}

    //set
	void setCommandType(const CommandType *commandType)	{this->commandType= commandType;}
	void setFlags(CommandFlags flags)					{this->flags = flags;}
	void setQueue(bool queue)							{flags.set(cpQueue, queue);}
	void setAuto(bool _auto)							{flags.set(cpAuto, _auto);}
	void setReserveResources(bool reserveResources)		{flags.set(cpDontReserveResources, !reserveResources);}
	void setPos(const Vec2i &pos)						{this->pos = pos;}
	void setPos2(const Vec2i &pos2)						{this->pos2 = pos2;}

	void setUnit(Unit *unit)							{this->unitRef = unit;}
	void setUnit2(Unit *unit2)							{this->unitRef2 = unit2;}

	//misc
	void swap();
	void popPos()										{pos = pos2; pos2 = Vec2i(-1);}
	void save(XmlNode *node) const;
};

}}//end namespace

#endif
