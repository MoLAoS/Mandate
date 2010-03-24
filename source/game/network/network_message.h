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

// NETWORK: nearly whole file changed
// - need more description about what role a NetworkMessage plays.
// - Should NetworkMessageXmlDoc really be a NetworkMessage or is it a network type?
// - Perhaps NetworkMessage and NetworkMessageXmlDoc should go into network_types and this renamed to 
// network_messages?

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
	};

private:
	Data data;

public:
	NetworkMessageIntro();
	NetworkMessageIntro(const string &versionString, const string &pName, const string &hName, int playerIndex);

	string getVersionString() const		{return data.versionString.getString();}
	string getPlayerName() const		{return data.playerName.getString();}
	string getHostName() const			{return data.hostName.getString();}
	int getPlayerIndex() const			{return data.playerIndex;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};

// ==============================================================
//	class NetworkMessageAiSeedSync
// ==============================================================
/** Message sent if there are AI players, to seed their Random objects */
class NetworkMessageAiSeedSync : public NetworkMessage {
private:
	static const int maxAiSeeds = 2;

private:
	struct {
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
	};

private:
	Data data;

public:
	NetworkMessageReady();
	NetworkMessageReady(int32 checksum);

	int32 getChecksum() const	{return data.checksum;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
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
	};

private:
	Data data;

public:
	NetworkMessageLaunch();
	NetworkMessageLaunch(const GameSettings *gameSettings);

	void buildGameSettings(GameSettings *gameSettings) const;

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};

// ==============================================================
//	class CommandList
// ==============================================================
/**	Message to issue commands to several units */
#pragma pack(push, 2)
class NetworkMessageCommandList : public NetworkMessage {
private:
	static const int maxCommandCount= 16*4;
	
private:
	static const int dataHeaderSize = 10;
	struct Data{
		int8 messageType;
		int8 commandCount;
		int32 frameCount;
		int32 totalB4this; //DEBUG
		NetworkCommand commands[maxCommandCount];
	};

private:
	Data data;

public:
	NetworkMessageCommandList(int32 frameCount= -1);

	bool addCommand(const NetworkCommand* networkCommand);
	//DEBUG
	void setTotalB4This(int32 total) { 
		assert(total >= 0);
		data.totalB4this = total; 
	}
	int32 getTotalB4This() const { return data.totalB4this; }
	
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
	};

private:
	Data data;

public:
	NetworkMessageText(){}
	NetworkMessageText(const string &text, const string &sender, int teamIndex);

	string getText() const		{return data.text.getString();}
	string getSender() const	{return data.sender.getString();}
	int getTeamIndex() const	{return data.teamIndex;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};

// =====================================================
//	class NetworkMessageQuit
// =====================================================
/** Message sent at the beggining of the game */
class NetworkMessageQuit: public NetworkMessage {
private:
	struct Data{
		int8 messageType;
	};

private:
	Data data;

public:
	NetworkMessageQuit();

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};

}}//end namespace

#endif
