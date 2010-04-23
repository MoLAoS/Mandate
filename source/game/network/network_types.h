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
#include "FSFactory.hpp"


using std::string;
using Shared::Math::Vec2i;
using namespace Shared::Platform;

namespace Glest { namespace Game {
	class Unit;
	class Command;
	class ProjectileParticleSystem;
}}
using namespace Glest::Game;

namespace Glest { namespace Net {

WRAPPED_ENUM( NetSource, CLIENT, SERVER )

// =====================================================
//	class NetworkError & Derivitives
// =====================================================

class NetworkError : public std::exception {};

class TimeOut : public NetworkError {
public:
	TimeOut(NetSource waitingFor) : source(waitingFor) {}

	NetSource getSource() const { return source; }

	const char* what() const {
		if (source == NetSource::SERVER) {
			return "Time out waiting for server.";
		} else {
			return "Time out waiting for client(s).";
		}
	}

private:
	NetSource source;
};

class InvalidMessage : public NetworkError {
public:
	InvalidMessage(MessageType expected, MessageType got) 
		: expected(expected), received(got) {}

	InvalidMessage(MessageType got) 
		: expected(MessageType::INVALID), received(got) {}

	const char* what() const {
		static char msgBuf[512];
		if (expected != MessageType::INVALID) {
			sprintf(msgBuf, "Expected message type %d(%s), got type %d(%s).",
				int(expected), MessageTypeNames[expected], int(received), MessageTypeNames[received]);
		} else {
			sprintf(msgBuf, "Unexpected message type received: %d(%s).",
				int(received), MessageTypeNames[received]);
		}
		return msgBuf;
	}

private:
	MessageType expected, received;
};

class GarbledMessage : public NetworkError {
public:
	GarbledMessage(MessageType type, NetSource sender) : type(type), sender(sender) {}

	const char* what() const {
		static char msgBuf[512];
		sprintf(msgBuf, "While processing %s message from %s, encountered invalid data.",
			MessageTypeNames[type], (sender == NetSource::SERVER ? "server" : "client"));
		return msgBuf;
	}

private:
	MessageType type;
	NetSource sender;
};

class DataSyncError : public NetworkError {
public:
	DataSyncError(NetSource role) : role(role) {}

	const char* what() const {
		if (role == NetSource::CLIENT) {
			return "Your data does not match the server's.";
		} else {
			return "Client data does not match yours.";
		}
	}

private:
	NetSource role;
};

class GameSyncError : public NetworkError {
public:
	const char* what() const {
		return "A network synchronisation error has occured.";
	}
};

class VersionMismatch : public NetworkError {
public:
	VersionMismatch(NetSource role, const string &v1, const string &v2)
			: role(role), v1(v1), v2(v2) {}

	const char* what() const {
		static char msgBuf[512];
		sprintf(msgBuf, "Version mismatch, your version: %s, %s version: %s",
			v1.c_str(), (role == NetSource::SERVER ? "Client" : "Server"), v2.c_str());
		return msgBuf;
	}

private:
	NetSource role;
	string v1, v2;
};

class Disconnect : public NetworkError {
public:
	const char* what() const {
		return "The network connection was severed.";
	}
};

// =====================================================
//	class NetworkString
// =====================================================

template<int S>
class NetworkString {
private:
	char buffer[S];

public:
	NetworkString()						{memset(buffer, 0, S);}
	void operator=(const string& str)	{strncpy(buffer, str.c_str(), S-1);}
	string getString() const			{return buffer;}
};

// =====================================================
//	class NetworkCommand
// =====================================================

WRAPPED_ENUM( NetworkCommandType,
	GIVE_COMMAND,
	CANCEL_COMMAND,
	SET_MEETING_POINT
);

#pragma pack(push, 4)
	class NetworkCommand {
	private:
		struct CmdFlags { enum { QUEUE = 1, NO_RESERVE_RESOURCES = 2 }; };
		uint32 networkCommandType	:  8;
		int32 unitId				: 24;
			//OSX: first int32 == netCmdType & unitId

		int32 commandTypeId			: 16;
		int32 unitTypeId			: 16;
			//OSX: second int32 == commandTypeId & unitTypeId

		int32 targetId				: 24;
		uint32 flags				:  8;
			//OSX: third int32 == targetId & commandFlags

		int32 positionX				: 16;
		int32 positionY				: 16;
			//OSX: fourth int32 == posX & posY

									// 128 bits (16 bytes)
	public:
		NetworkCommand(){};
		NetworkCommand(Command *command);
		NetworkCommand(NetworkCommandType type, const Unit *unit, const Vec2i &pos);
		//NetworkCommand(int networkCommandType, int unitId, int commandTypeId= -1, const Vec2i &pos= Vec2i(0), int unitTypeId= -1, int targetId= -1);

		Command *toCommand() const;
		NetworkCommandType getNetworkCommandType() const	{return static_cast<NetworkCommandType>(networkCommandType);}
		int getUnitId() const								{return unitId;}
		int getCommandTypeId() const						{return commandTypeId;}
		Vec2i getPosition() const							{return Vec2i(positionX, positionY);}
		int getUnitTypeId() const							{return unitTypeId;}
		int getTargetId() const								{return targetId;}
	};
#pragma pack(pop)

#pragma pack(push, 2)
	struct MoveSkillUpdate {
		int8	offsetX		:  2;
		int8	offsetY		:  2;
		int16	end_offset	: 12;

		MoveSkillUpdate(const Unit *unit);
		MoveSkillUpdate(const char *ptr) { *this = *((MoveSkillUpdate*)ptr); }
		Vec2i posOffset() const { return Vec2i(offsetX, offsetY); }
	};
#pragma pack(pop)

#pragma pack(push, 1)
	struct ProjectileUpdate {
		uint8 end_offset	:  8;
		ProjectileUpdate(const Unit *unit, ProjectileParticleSystem *pps);
		ProjectileUpdate(const char *ptr) { *this = *((ProjectileUpdate*)ptr); }
	}; // 2 bytes
#pragma pack(pop)

}}//end namespace

#endif
