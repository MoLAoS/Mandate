// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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

namespace Glest{ namespace Game{

class GameSettings;

enum NetworkMessageType{
	nmtInvalid,
	nmtIntro,
	nmtPing,
	nmtReady,
	nmtLaunch,
	nmtCommandList,
	nmtText,
	nmtQuit,
	nmtFileHeader,
	nmtFileFragment,

	nmtCount
};

// =====================================================
//	class NetworkMessage
// =====================================================

class NetworkMessage{
public:
	virtual ~NetworkMessage(){}
	virtual bool receive(Socket* socket)= 0;
	virtual void send(Socket* socket) const = 0;

protected:
	bool receive(Socket* socket, void* data, int dataSize);
	void send(Socket* socket, const void* data, int dataSize) const;
};

// =====================================================
//	class NetworkMessageIntro
//
///	Message sent from the server to the client
///	when the client connects and vice versa
// =====================================================

class NetworkMessageIntro: public NetworkMessage{
private:
	static const int maxVersionStringSize= 64;
	static const int maxNameSize= 16;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxVersionStringSize> versionString;
		NetworkString<maxNameSize> name;
		int8 playerIndex;
		int8 resumeSaved;
	};

private:
	Data data;

public:
	NetworkMessageIntro();
	NetworkMessageIntro(const string &versionString, const string &name, int playerIndex, bool resumeSaved);

	string getVersionString() const		{return data.versionString.getString();}
	string getName() const				{return data.name.getString();}
	int getPlayerIndex() const			{return data.playerIndex;}
	bool isResumeSaved() const			{return (bool)data.resumeSaved;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};

// =====================================================
//	class NetworkMessageReady
//
///	Message sent at the beggining of the game
// =====================================================

class NetworkMessageReady: public NetworkMessage{
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

// =====================================================
//	class NetworkMessageLaunch
//
///	Message sent from the server to the client
///	to launch the game
// =====================================================

class NetworkMessageLaunch: public NetworkMessage{
private:
	static const int maxStringSize= 256;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxStringSize> description;
		NetworkString<maxStringSize> mapPath;
		NetworkString<maxStringSize> tilesetPath;
		NetworkString<maxStringSize> techPath;
		NetworkString<maxStringSize> factionTypeNames[GameConstants::maxPlayers]; //faction names

		int8 factionControls[GameConstants::maxPlayers];

		int8 thisFactionIndex;
		int8 factionCount;
		int8 teams[GameConstants::maxPlayers];
		int8 startLocationIndex[GameConstants::maxPlayers];
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

// =====================================================
//	class CommandList
//
///	Message to order a commands to several units
// =====================================================

class NetworkMessageCommandList: public NetworkMessage{
private:
	static const int maxCommandCount= 16*4;

private:
	struct Data{
		int8 messageType;
		int8 commandCount;
		int32 frameCount;
		NetworkCommand commands[maxCommandCount];
	};

private:
	Data data;

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

// =====================================================
//	class NetworkMessageText
//
///	Chat text message
// =====================================================

class NetworkMessageText: public NetworkMessage{
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
//
///	Message sent at the beggining of the game
// =====================================================

class NetworkMessageQuit: public NetworkMessage{
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

// =====================================================
//	class NetworkMessageFileHeader
//
///	Message to initiate sending a file
// =====================================================

class NetworkMessageFileHeader: public NetworkMessage{
private:
	static const int maxNameSize= 128;

	struct Data{
		int8 messageType;
		NetworkString<maxNameSize> name;
		int8 compressed;
	};

private:
	Data data;

public:
	NetworkMessageFileHeader(){}
	NetworkMessageFileHeader(const string &str, bool compressed);

	string getName() const		{return data.name.getString();}
	bool isCompressed() const	{return data.compressed;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};


// =====================================================
//	class NetworkMessageFileFragment
//
///	Message to send part of a file
// =====================================================

class NetworkMessageFileFragment: public NetworkMessage{
public:
	static const int bufSize= 1024;

private:
	struct Data{
		int8 messageType;
		char data[bufSize];
		uint16 size;
		uint32 seq;
		uint8 last;
	};

	Data data;

public:
	NetworkMessageFileFragment(){}
	NetworkMessageFileFragment(char *data, size_t size, int seq, bool last);

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;

	const char *getData() const	{return data.data;}
	size_t getDataSize() const	{return data.size;}
	int getSeq() const			{return data.seq;}
	bool isLast() const			{return data.last;}
};

}}//end namespace

#endif
