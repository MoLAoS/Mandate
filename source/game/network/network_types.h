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

#include "game_constants.h"
#include "types.h"
#include "vec.h"

using std::string;
using Shared::Math::Vec2i;
using namespace Shared::Platform;

namespace Glest { namespace Game {

class Unit;
class Command;
class ProjectileParticleSystem;

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

WRAPPED_ENUM( NetworkCommandType,
	GIVE_COMMAND,
	CANCEL_COMMAND,
	SET_MEETING_POINT
);

#pragma pack(push, 1)

	class NetworkCommand{
	private:
		uint32 networkCommandType	:  8;
		int32 unitId				: 24; // 32
		int32 commandTypeId			:  8;
		int32 targetId				: 24; // 32
		int32 positionX				: 16; 
		int32 positionY				: 16; // 32
		int32 unitTypeId			: 15;
		uint32 queue				:  1; // 16
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

#pragma pack(push, 1)
	struct MoveSkillUpdate {
		int8	offsetX		:  2;
		int8	offsetY		:  2;
		int16	end_offset	: 12;

		MoveSkillUpdate(const Unit *unit);
		MoveSkillUpdate(const char *ptr) { *this = *((MoveSkillUpdate*)ptr); }
		Vec2i posOffset() const { return Vec2i(offsetX, offsetY); }
	};

	struct ProjectileUpdate {
		uint8 end_offset	:  8;
		ProjectileUpdate(const Unit *unit, ProjectileParticleSystem *pps);
		ProjectileUpdate(const char *ptr) { *this = *((ProjectileUpdate*)ptr); }
	}; // 2 bytes
#pragma pack(pop)


#define _RECORD_GAME_STATE_ 1

#if _RECORD_GAME_STATE_
struct UnitStateRecord {
	uint32	unit_id		: 24;
	int32	cmd_class	:  8;
	uint32	skill_cl	:  8;
	int32	curr_pos_x	: 12;
	int32	curr_pos_y	: 12;
	int32	next_pos_x	: 12;
	int32	next_pos_y	: 12;
	int32	targ_pos_x	: 12;
	int32	targ_pos_y	: 12;
	int32	target_id	: 24;

	UnitStateRecord(Unit *unit);
	UnitStateRecord() {}
};					//	: 20 bytes

ostream& operator<<(ostream &lhs, const UnitStateRecord&);

struct FrameRecord : public vector<UnitStateRecord> {
	int32	frame;
};

ostream& operator<<(ostream &lhs, const FrameRecord&);

class GameStateLog {
	FrameRecord currFrame;

	void writeFrame();

public:
	GameStateLog();

	int getCurrFrame() const { return currFrame.frame; }

	void addUnitRecord(UnitStateRecord &usr) {
		currFrame.push_back(usr);
	}

	void newFrame(int frame) {
		if (currFrame.frame) {
			writeFrame();
		}
		currFrame.clear();
		assert(frame == currFrame.frame + 1);
		currFrame.frame = frame;
	}

	void logFrame(int frame = -1);
};

#endif

}}//end namespace

#endif
