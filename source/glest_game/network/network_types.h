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

#ifndef _GLEST_GAME_NETWORKTYPES_H_
#define _GLEST_GAME_NETWORKTYPES_H_

#include <string>
#include <limits.h>

#include "types.h"
#include "vec.h"
#include "command.h"
#include "socket.h"

using std::string;
using Shared::Platform::int8;
using Shared::Platform::int16;
using Shared::Platform::int32;
using Shared::Graphics::Vec2i;

namespace Glest{ namespace Game{

// =====================================================
//	class NetworkString
// =====================================================

template<int S> class NetworkString : public NetworkWriteable {
private:
	uint16 size;	//size excluding null terminator, max of S - 1
	char buffer[S];

public:
	NetworkString() {
		assert(S && S < USHRT_MAX);
		size = 0;
		*buffer = 0;
	}
	string getString() const			{return buffer;}
	void operator=(const string &str) {
		size = (uint16)(str.size() < S ? str.size() : S - 1);
		strncpy(buffer, str.c_str(), size);
		buffer[size] = 0;
	}

	size_t getNetSize() const {
		return sizeof(size) + size;
	}

	size_t getMaxNetSize() const {
		return sizeof(size) + sizeof(buffer);
	}

	void write(NetworkDataBuffer &buf) const {
		buf.write(size);
		buf.write(buffer, size);
	}

	void read(NetworkDataBuffer &buf) {
		buf.read(size);
		assert(size < S);
		buf.read(buffer, size);
		buffer[size] = 0;
	}
};

// =====================================================
//	class NetworkCommand
// =====================================================

enum NetworkCommandType{
	nctGiveCommand,
	nctCancelCommand,
	nctSetMeetingPoint,
	nctSetAutoRepair
};

class NetworkCommand : public NetworkWriteable {
private:
	int16 networkCommandType;
	int16 unitId;
	int16 commandTypeId;
	CommandFlags flags;
	int16 positionX;
	int16 positionY;
	int16 unitTypeId;
	int16 targetId;

public:
	NetworkCommand(){};
	NetworkCommand(
			int networkCommandType,
   			int unitId,
   			int commandTypeId= -1,
			CommandFlags flags = CommandFlags(),
   			const Vec2i &pos= Vec2i(-1),
   			int unitTypeId= -1,
   			int targetId= -1) {
/*
		assert(networkCommandType >= 0			&& networkCommandType <= UCHAR_MAX);
		assert(unitId			  >= SHRT_MIN	&& unitId			  <= SHRT_MAX);
		assert(commandTypeId	  >= SCHAR_MIN	&& commandTypeId	  <= SCHAR_MAX);
		assert(pos.x			  >= SHRT_MIN	&& pos.x			  <= SHRT_MAX);
		assert(pos.y			  >= SHRT_MIN	&& pos.y			  <= SHRT_MAX);
		assert(unitTypeId		  >= 0			&& unitTypeId		  <= UCHAR_MAX);
		assert(targetId			  >= SHRT_MIN	&& targetId			  <= SHRT_MAX);
*/
		this->networkCommandType = networkCommandType;
		this->unitId = unitId;
		this->commandTypeId = commandTypeId;
		this->flags = flags;
		this->positionX = pos.x;
		this->positionY = pos.y;
		this->unitTypeId = unitTypeId;
		this->targetId = targetId;
	}

	NetworkCommandType getNetworkCommandType() const	{return static_cast<NetworkCommandType>(networkCommandType);}
	int getUnitId() const								{return unitId;}
	int getCommandTypeId() const						{return commandTypeId;}
	CommandFlags getFlags() const						{return flags;}
	bool isQueue() const								{return flags.get(cpQueue);}
	bool isAuto() const									{return flags.get(cpAuto);}
	Vec2i getPosition() const							{return Vec2i(positionX, positionY);}
	int getUnitTypeId() const							{return unitTypeId;}
	int getTargetId() const								{return targetId;}

	void setQueue(bool queue)							{flags.set(cpQueue, queue);}
	void setAuto(bool _auto)							{flags.set(cpAuto, _auto);}

	size_t getNetSize() const							{return getMaxNetSize();}
	size_t getMaxNetSize() const {
		return
			  sizeof(networkCommandType)
			+ sizeof(unitId)
			+ sizeof(commandTypeId)
			+ sizeof(flags.flags)
			+ sizeof(positionX)
			+ sizeof(positionY)
			+ sizeof(unitTypeId)
			+ sizeof(targetId);
	}
	void write(NetworkDataBuffer &buf) const;
	void read(NetworkDataBuffer &buf);
};

}}//end namespace

#endif
