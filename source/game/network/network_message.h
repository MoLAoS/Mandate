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

#ifndef _GLEST_GAME_NETWORKMESSAGE_H_
#define _GLEST_GAME_NETWORKMESSAGE_H_

#include "socket.h"
#include "game_constants.h"
#include "network_types.h"

using Shared::Platform::Socket;
using Shared::Platform::int8;
using Shared::Platform::int16;

namespace Glest { namespace Game {

class GameSettings;
class Command;

class TechTree;
class FactionType;
class UnitType;
class SkillType;
class Unit;
class NetworkInterface;

// ==============================================================
//	class NetworkMessage
// ==============================================================
/** Abstract base class for network messages, requires concrete subclasses 
  * to implement receive(Socket*)/send(Socket*), and provides send/receive methods
  * for them to use to accomplish this. */
class NetworkMessage {
public:
	virtual ~NetworkMessage(){}
	virtual bool receive(Socket* socket)= 0;
	virtual void send(Socket* socket) const = 0;

protected:
	bool receive(Socket* socket, void* data, int dataSize);
	bool peek(Socket* socket, void *data, int dataSize);
	void send(Socket* socket, const void* data, int dataSize) const;
};

// ==============================================================
//	class NetworkMessageIntro
// ==============================================================
/**	Message sent from the server to the client
  *	when the client connects and vice versa */
class NetworkMessageIntro : public NetworkMessage {
private:
	static const int maxVersionStringSize= 64;
	static const int maxNameSize= 16;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxVersionStringSize> versionString; // change to uint32 ?
		NetworkString<maxNameSize> playerName;
		NetworkString<maxNameSize> hostName;
		int16 playerIndex;
	} data;

public:
	NetworkMessageIntro();
	NetworkMessageIntro(const string &versionString, const string &pName, const string &hName, int playerIndex);

	string getVersionString() const		{return data.versionString.getString();}
	string getPlayerName() const		{return data.playerName.getString();}
	string getHostName() const			{return data.hostName.getString();}
	int getPlayerIndex() const			{return data.playerIndex;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
	static size_t getSize() { return sizeof(Data); }
};

// ==============================================================
//	class NetworkMessageAiSeedSync
// ==============================================================
/** Message sent if there are AI players, to seed their Random objects */
class NetworkMessageAiSeedSync : public NetworkMessage {
private:
	static const int maxAiSeeds = 2;

private:
	struct Data {
		int8 msgType;
		int8 seedCount;
		int32 seeds[maxAiSeeds];
	} data;

public:
	NetworkMessageAiSeedSync();
	NetworkMessageAiSeedSync(int count, int32 *seeds);

	int getSeedCount() const { return data.seedCount; }
	int32 getSeed(int i) const { return data.seeds[i]; }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
	static size_t getSize() { return sizeof(Data); }
};

// ==============================================================
//	class NetworkMessageReady
// ==============================================================
/**	Message sent at the beggining of the game */
class NetworkMessageReady : public NetworkMessage {
private:
	struct Data{
		int8 messageType;
		int32 checksum;
	} data;

public:
	NetworkMessageReady();
	NetworkMessageReady(int32 checksum);

	int32 getChecksum() const	{return data.checksum;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
	static size_t getSize() { return sizeof(Data); }
};

// ==============================================================
//	class NetworkMessageLaunch
// ==============================================================
/**	Message sent from the server to the client to launch the game */
class NetworkMessageLaunch : public NetworkMessage {
private:
	static const int maxStringSize= 256;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxStringSize> description;
		NetworkString<maxStringSize> map;
		NetworkString<maxStringSize> tileset;
		NetworkString<maxStringSize> tech;
		NetworkString<maxStringSize> factionTypeNames[GameConstants::maxPlayers]; //faction names

		int8 factionControls[GameConstants::maxPlayers];
		float resourceMultipliers[GameConstants::maxPlayers];

		int8 thisFactionIndex;
		int8 factionCount;
		int8 teams[GameConstants::maxPlayers];
		int8 startLocationIndex[GameConstants::maxPlayers];
		int8 defaultResources;
		int8 defaultUnits;
		int8 defaultVictoryConditions;
		int8 fogOfWar;
	} data;

public:
	NetworkMessageLaunch();
	NetworkMessageLaunch(const GameSettings *gameSettings);

	void buildGameSettings(GameSettings *gameSettings) const;

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
	static size_t getSize() { return sizeof(Data); }
};

// ==============================================================
//	class CommandList
// ==============================================================
/**	Message to issue commands to several units */
#pragma pack(push, 2)
class NetworkMessageCommandList : public NetworkMessage {
	friend class NetworkInterface;
private:
	static const int maxCommandCount= 16*4;
	
private:
	static const int dataHeaderSize = 6;
	struct Data{
		int8 messageType;
		int8 commandCount;
		int32 frameCount;
		NetworkCommand commands[maxCommandCount];
	} data;

public:
	NetworkMessageCommandList(int32 frameCount= -1);

	bool addCommand(const NetworkCommand* networkCommand);
	void clear()									{data.commandCount= 0;}
	int getCommandCount() const						{return data.commandCount;}
	int getFrameCount() const						{return data.frameCount;}
	const NetworkCommand* getCommand(int i) const	{return &data.commands[i];}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};
#pragma pack(pop)

// ==============================================================
//	class NetworkMessageText
// ==============================================================
/**	Chat text message */
class NetworkMessageText : public NetworkMessage {
private:
	static const int maxStringSize= 64;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxStringSize> text;
		NetworkString<maxStringSize> sender;
		int8 teamIndex;
	} data;

public:
	NetworkMessageText(){}
	NetworkMessageText(const string &text, const string &sender, int teamIndex);

	string getText() const		{return data.text.getString();}
	string getSender() const	{return data.sender.getString();}
	int getTeamIndex() const	{return data.teamIndex;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
	static size_t getSize() { return sizeof(Data); }
};

// =====================================================
//	class NetworkMessageQuit
// =====================================================
/** Message sent by clients to quit nicely, or by the server to terminate the game */
class NetworkMessageQuit: public NetworkMessage {
private:
	struct Data{
		int8 messageType;
	} data;

public:
	NetworkMessageQuit();

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
	static size_t getSize() { return sizeof(Data); }
};

class SkillIdTriple {
	int factionTypeId;
	int unitTypeId;
	int skillTypeId;

public:
	SkillIdTriple(int ftId, int utId, int stId) 
			: factionTypeId(ftId)
			, unitTypeId(utId)
			, skillTypeId(stId) {
	}

	int getFactionTypeId() const { return factionTypeId; }
	int getUnitTypeId() const { return unitTypeId; }
	int getSkillTypeId() const { return skillTypeId; }

	bool operator==(const SkillIdTriple &other) const {
		return memcmp(this, &other, sizeof(SkillIdTriple));
	}

	bool operator<(const SkillIdTriple &other) const {
		if (factionTypeId < other.getFactionTypeId()) return true;
		if (factionTypeId > other.getFactionTypeId()) return false;

		if (unitTypeId < other.getUnitTypeId()) return true;
		if (unitTypeId > other.getUnitTypeId()) return false;

		if (skillTypeId < other.getSkillTypeId()) return true;
		// (skillTypeId >= other.getSkillTypeId())
		return false;
	}
};

class CycleInfo {
	int skillFrames, animFrames;
	int soundOffset, attackOffset;

public:
	CycleInfo()
			: skillFrames(-1)
			, animFrames(-1)
			, soundOffset(-1)
			, attackOffset(-1) {
	}

	CycleInfo(int sFrames, int aFrames, int sOffset = -1, int aOffset = -1) 
			: skillFrames(sFrames)
			, animFrames(aFrames)
			, soundOffset(sOffset)
			, attackOffset(aOffset) {
	}

	int getSkillFrames() const	{ return skillFrames;	}
	int getAnimFrames() const	{ return animFrames;	}
	int getSoundOffset() const	{ return soundOffset;	}
	int getAttackOffset() const { return attackOffset;	}

};

class SkillCycleTable : public NetworkMessage {
private:
	typedef std::map<SkillIdTriple, CycleInfo> CycleMap;
	CycleMap cycleTable;

public:
	SkillCycleTable() {}

	void create(const TechTree *techTree);

	const CycleInfo& lookUp(SkillIdTriple id) {
		return cycleTable[id];
	}
	const CycleInfo& lookUp(int ftId, int utId, int stId) {
		return cycleTable[SkillIdTriple(ftId, utId, stId)];
	}
	const CycleInfo& lookUp(const Unit *unit);

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};

class KeyFrame : public NetworkMessage {
private:
	static const int buffer_size = 1024 * 4;
	static const int max_cmds = 512;
	static const int max_checksums = 2048;
	typedef char* byte_ptr;

	int32	frame;

	int32	checksums[max_checksums];
	int32	checksumCount;
	uint32	checksumCounter;
	
	char	 updateBuffer[buffer_size];
	size_t	 updateSize;
	uint32	 projUpdateCount;
	uint32	 moveUpdateCount;
	byte_ptr writePtr;
	byte_ptr readPtr;

	NetworkCommand commands[max_cmds];
	size_t cmdCount;


public:
	KeyFrame()		{ reset(); }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;

	void setFrameCount(int fc) { frame = fc; }
	int getFrameCount() const { return frame; }

	const size_t& getCmdCount() const	{ return cmdCount; }
	const NetworkCommand* getCmd(size_t ndx) const { return &commands[ndx]; }

	int32 getNextChecksum();
	void addChecksum(int32 cs);
	void add(NetworkCommand &nc);
	void reset();
	void addUpdate(MoveSkillUpdate updt);
	void addUpdate(ProjectileUpdate updt);
	MoveSkillUpdate getMoveUpdate();
	ProjectileUpdate getProjUpdate();
};

#if _RECORD_GAME_STATE_

class SyncError : public NetworkMessage {
	struct Data{
		int32	messageType	:  8;
		uint32	frameCount	: 24;
	} data;

public:
	SyncError(int frame) {
		data.messageType = NetworkMessageType::SYNC_ERROR;
		data.frameCount = frame;
	}
	SyncError() {}

	int getFrame() const { return data.frameCount; }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};

#endif


}}//end namespace

#endif
