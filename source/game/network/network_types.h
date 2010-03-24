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
#include <stdexcept>

#include "types.h"
#include "vec.h"
#include "unit.h"

using std::string;
using Shared::Platform::int8;
using Shared::Platform::int16;
using Shared::Platform::int32;
using Shared::Math::Vec2i;

namespace Glest { namespace Game {

class Command;

// =====================================================
//	class NetworkException
// =====================================================

class NetworkException : public std::runtime_error {
public:
	NetworkException(const string &msg) : runtime_error(msg) {}
	NetworkException(const char *msg) : runtime_error(msg) {}
};

// =====================================================
//	class NetworkString
// =====================================================

template<int S>
class NetworkString{
private:
	char buffer[S];

public:
#if 0
	NetworkString() /*: s() */{
		assert(S && S < USHRT_MAX);
		size = 0;
		*buffer = 0;
	}
	NetworkString(const string &str) {
		assert(S && S < USHRT_MAX);
		(*this) = str;
	}
#endif
	NetworkString()						{memset(buffer, 0, S);}
	void operator=(const string& str)	{strncpy(buffer, str.c_str(), S-1);}
	string getString() const			{return buffer;}

#if 0
	void operator=(const string &str) {
		/*
		s = str;
		if(s.size() > S) {
			s.resize(S);
		}
		*/
		size = (uint16)(str.size() < S ? str.size() : S - 1);
		strncpy(buffer, str.c_str(), size);
		buffer[size] = 0;
	}

	size_t getNetSize() const {
		//return s.size() + sizeof(uint16);
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
#endif
};

// =====================================================
//	class NetworkCommand
// =====================================================

enum NetworkCommandType{
	nctGiveCommand,
	nctCancelCommand,
	nctSetMeetingPoint
};

#pragma pack(push, 2)

class NetworkCommand{
private:
	int16 networkCommandType;
	int16 unitId;
	int16 commandTypeId;
	int16 positionX;
	int16 positionY;
	int16 unitTypeId;
	int16 targetId;

public:
	NetworkCommand(){};
	NetworkCommand(Command *command);
	NetworkCommand(NetworkCommandType type, const Unit *unit, const Vec2i &pos);
	NetworkCommand(int networkCommandType, int unitId, int commandTypeId= -1, const Vec2i &pos= Vec2i(0), int unitTypeId= -1, int targetId= -1);

	Command *toCommand() const;
	NetworkCommandType getNetworkCommandType() const	{return static_cast<NetworkCommandType>(networkCommandType);}
	int getUnitId() const								{return unitId;}
	int getCommandTypeId() const						{return commandTypeId;}
	Vec2i getPosition() const							{return Vec2i(positionX, positionY);}
	int getUnitTypeId() const							{return unitTypeId;}
	int getTargetId() const								{return targetId;}
};

#pragma pack(pop)

}}//end namespace

#endif
