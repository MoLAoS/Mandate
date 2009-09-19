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

using Shared::Graphics::Vec2i;
using Shared::Platform::int16;

class CommandType;

enum CommandProperties {
	cpQueue,
	cpAuto,
	cpDontReserveResources,
	cpAutoRepairEnabled,

	cpCount
};

typedef Flags<CommandProperties, cpCount, uint8> CommandFlags;

enum CommandArchetype {
	catGiveCommand,
	catCancelCommand,
//	catSetMeetingPoint,
	catSetAutoRepair
};

// =====================================================
// 	class Command
//
///	A unit command
// =====================================================

class Command : public NetworkWriteable {
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
	Command(NetworkDataBuffer &buf);
//	Command(const Command &);
	// allow default ctor

    //get
	CommandArchetype getArchetype() const		{return archetype;}
	const CommandType *getType() const			{return type;}
	CommandFlags getFlags() const				{return flags;}
	bool isQueue() const						{return flags.get(cpQueue);}
	bool isAuto() const							{return flags.get(cpAuto);}
	bool isReserveResources() const				{return !flags.get(cpDontReserveResources);}
	bool isAutoRepairEnabled() const			{return flags.get(cpAutoRepairEnabled);}
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
	void setQueue(bool queue)							{flags.set(cpQueue, queue);}
	void setAuto(bool _auto)							{flags.set(cpAuto, _auto);}
	void setReserveResources(bool reserveResources)		{flags.set(cpDontReserveResources, !reserveResources);}
	void setAutoRepairEnabled(bool enabled)				{flags.set(cpAutoRepairEnabled, enabled);}
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

	// NetworkWriteable methods
	void write(NetworkDataBuffer &buf) const;
	void read(NetworkDataBuffer &buf);
	size_t getNetSize() const;
	size_t getMaxNetSize() const						{return Command::getStaticMaxNetSize();}
	static size_t getStaticMaxNetSize() {
		return	  sizeof(uint16)		// id of unit command is being issued to
				+ sizeof(uint8)			// archtype + flags
				+ sizeof(uint8)			// fields present
				+ sizeof(uint8)			// command id (unit type-specific)
				+ sizeof(uint16) * 2	// pos
				+ sizeof(uint16) * 2	// pos2
				+ sizeof(uint16)		// target (unit id)
				+ sizeof(uint16)		// target2 (unit id)
				+ sizeof(uint8);		// unitTypeId
	}
};

}}//end namespace

#endif
