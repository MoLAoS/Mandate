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

#include "forward_decs.h"

using std::string;
using Shared::Math::Vec2i;
using namespace Shared::Platform;
using namespace Glest::Entities;
using namespace Glest::Sim;
using namespace Glest::ProtoTypes;

namespace Glest { namespace Net {

WRAPPED_ENUM( NetSource, CLIENT, SERVER )

// =====================================================
//	class NetworkError & Derivitives
// =====================================================

class NetworkError : public std::exception {};

class TimeOut : public NetworkError {
public:
	TimeOut(NetSource waitingFor) : source(waitingFor) {}
	~TimeOut() throw() {}

	NetSource getSource() const { return source; }

	const char* what() const throw() {
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
	InvalidMessage(MessageType expected, int8 got)
		: expected(expected), received(got) {}

	InvalidMessage(int8 got)
		: expected(MessageType::INVALID), received(got) {}

	~InvalidMessage() throw() {}

	const char* what() const throw() {
		static char msgBuf[512];
		if (expected != MessageType::INVALID) {
			sprintf(msgBuf, "Expected message type %d(%s), got type %d(%s).",
				int(expected), MessageTypeNames[expected], int(received), 
				MessageTypeNames[MessageType(received)]);
		} else {
			sprintf(msgBuf, "Unexpected message type received: %d(%s).",
				int(received), MessageTypeNames[MessageType(received)]);
		}
		return msgBuf;
	}

private:
	MessageType expected;
	int8 received;
};

class GarbledMessage : public NetworkError {
public:
	GarbledMessage(MessageType type, NetSource sender) : type(type), sender(sender) {}
	GarbledMessage(MessageType type) : type(type), sender(NetSource::INVALID) {}
	~GarbledMessage() throw() {}

	const char* what() const throw() {
		static char msgBuf[512];
		if (sender != NetSource::INVALID) {
			sprintf(msgBuf, "While processing %s message from %s, encountered invalid data.",
				MessageTypeNames[type], (sender == NetSource::SERVER ? "server" : "client"));
		} else {
			sprintf(msgBuf, "While processing %s message, encountered invalid data.", MessageTypeNames[type]);
		}
		return msgBuf;
	}

private:
	MessageType type;
	NetSource sender;
};

class DataSyncError : public NetworkError {
public:
	DataSyncError(NetSource role)
			: role(role) {
	}
	~DataSyncError() throw() {}

	const char* what() const throw() {
		return (role == NetSource::CLIENT 
			? " Your data does not match the servers, see glestadv-client.log for details."
			: " Your data does not match the clients, see glestadv-server.log for details.");
	}

private:
	NetSource role;
};

class GameSyncError : public NetworkError {
private:
	string msg;
public:
	GameSyncError(const string &err) {
		msg = "A game synchronisation error has occured.\n" + err;
	}
	~GameSyncError() throw() {}

	const char* what() const throw() {
		return msg.c_str();
	}
};

class VersionMismatch : public NetworkError {
public:
	VersionMismatch(NetSource role, const string &v1, const string &v2)
			: role(role), v1(v1), v2(v2) {}
	~VersionMismatch() throw() {}

	const char* what() const throw() {
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
private:
	string msg;

public:
	Disconnect() {}
	Disconnect(const string &msg) : msg(msg) {}
	~Disconnect() throw() {}

	const char* what() const throw() {
		if (msg.empty()) {
			return "The network connection was severed.";
		} else {
			return msg.c_str();
		}
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
		int32 commandTypeId			: 16;
		int32 unitTypeId			: 16;
		int32 targetId				: 24;
		uint32 flags				:  8;
		int32 positionX				: 16;
		int32 positionY				: 16;
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
		ProjectileUpdate(const Unit *unit, Projectile *pps);
		ProjectileUpdate(const char *ptr) { *this = *((ProjectileUpdate*)ptr); }
	}; // 2 bytes
#pragma pack(pop)

}}//end namespace

#endif
