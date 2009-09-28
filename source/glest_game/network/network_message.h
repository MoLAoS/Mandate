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
#include "logger.h"
#include "platform_util.h"
#ifdef DEBUG_NETWORK
#include "random.h"
#endif

using Shared::Platform::Socket;
using Shared::Platform::int8;
using Shared::Platform::uint8;
using Shared::Platform::int16;
using Shared::Platform::uint16;
using Shared::Platform::Chrono;

namespace Glest{ namespace Game{

class GameSettings;

enum NetworkMessageType {
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
	nmtUpdate,
	nmtUpdateRequest,

	nmtCount
};


// =====================================================
//	class NetworkMessage
// =====================================================

class NetworkMessage : public NetworkWriteable {
	uint8 type;
#ifdef DEBUG_NETWORK
	int simRxTime;
	Shared::Util::Random rand;
#endif

public:
	static const char* msgTypeName[nmtCount];

	NetworkMessage(NetworkMessageType type) : type(type) {
#ifdef DEBUG_NETWORK
		simRxTime = Chrono::getCurMillis();
		rand.init(simRxTime);
		simRxTime += rand.randRange(-DEBUG_NETWORK_DELAY_VAR, DEBUG_NETWORK_DELAY_VAR)
				+ DEBUG_NETWORK_DELAY;
#endif
	}

	virtual ~NetworkMessage(){}
	NetworkMessageType getType() const {return (NetworkMessageType)type;}

	static NetworkMessage *readMsg(NetworkDataBuffer &buf);
	virtual void writeMsg(NetworkDataBuffer &buf) const;
	virtual void read(NetworkDataBuffer &buf) = 0;
#ifdef DEBUG_NETWORK
	int getSimRxTime() const			{return simRxTime;}
#endif
};

// =====================================================
//	class NetworkMessageIntro
//
///	Message sent from the server to the client
///	when the client connects and vice versa
// =====================================================

class NetworkMessageIntro: public NetworkMessage{
private:
	static const int maxVersionStringSize = 64;
	static const int maxHostNameSize = 32;
	static const int maxPlayerNameSize = 32;

	NetworkString<maxVersionStringSize> versionString;
	NetworkString<maxHostNameSize> hostName;
	NetworkString<maxPlayerNameSize> playerName;
	int8 playerIndex;
	int8 resumeSaved;

public:
	NetworkMessageIntro(NetworkDataBuffer &buf) : NetworkMessage(nmtIntro) {read(buf);}
	NetworkMessageIntro(const string &versionString, const string &hostName, const string &playerName, int playerIndex, bool resumeSaved);

	string getVersionString() const		{return versionString.getString();}
	string getHostName() const			{return hostName.getString();}
	string getPlayerName() const		{return playerName.getString();}
	int getPlayerIndex() const			{return playerIndex;}
	bool isResumeSaved() const			{return (bool)resumeSaved;}

	virtual size_t getNetSize() const;
	virtual size_t getMaxNetSize() const;
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;
};

// =====================================================
//	class NetworkMessagePing
// =====================================================
/**
 * A ping
 */
//TODO: Exchange time at handshake, convert this to milliseconds and use 32 bit values instead that
//      are an offset from the local time of each machine, maybe even 16 bit values?
class NetworkMessagePing : public NetworkMessage {
	int64 time;
	int64 timeRcvd;
	int8 pong;

public:
	NetworkMessagePing(NetworkDataBuffer &buf) :
			NetworkMessage(nmtPing),
			timeRcvd(Chrono::getCurMicros()) {
		read(buf);
	}

	NetworkMessagePing() :
			NetworkMessage(nmtPing),
			time(Chrono::getCurMicros()),
			timeRcvd(0),
			pong(false) {
	}

	int64 getTime() const								{return time;}
	int64 getTimeRcvd() const							{return timeRcvd;}
	bool isPong() const									{return pong;}
	void setPong()										{pong = true;}

	virtual size_t getNetSize() const					{return getMaxNetSize();}
	virtual size_t getMaxNetSize() const {
		return sizeof(time) + sizeof(timeRcvd) + sizeof(pong);
	}

	virtual void read(NetworkDataBuffer &buf) {
		buf.read(time);
		buf.read(timeRcvd);
		buf.read(pong);
	}

	virtual void write(NetworkDataBuffer &buf) const {
		buf.write(time);
		buf.write(timeRcvd);
		buf.write(pong);
	}
};

// =====================================================
//	class NetworkMessageReady
//
///	Message sent at the beggining of the game
// =====================================================

class NetworkMessageReady: public NetworkMessage {
private:
	int32 checksum;

public:
	NetworkMessageReady(NetworkDataBuffer &buf) : NetworkMessage(nmtReady) {read(buf);}
	NetworkMessageReady(int32 checksum);

	int32 getChecksum() const	{return checksum;}

	virtual size_t getNetSize() const			{return getMaxNetSize();}
	virtual size_t getMaxNetSize() const		{return sizeof(checksum);}
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;
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

	NetworkString<maxStringSize> description;
	NetworkString<maxStringSize> mapPath;
	NetworkString<maxStringSize> tilesetPath;
	NetworkString<maxStringSize> techPath;
	int8 thisFactionIndex;
	int8 factionCount;

	NetworkString<maxStringSize> factionTypeNames[GameConstants::maxPlayers]; //faction names
	int8 factionControls[GameConstants::maxPlayers];
	int8 teams[GameConstants::maxPlayers];
	int8 startLocationIndex[GameConstants::maxPlayers];

public:
	NetworkMessageLaunch(NetworkDataBuffer &buf): NetworkMessage(nmtLaunch) {read(buf);}
	NetworkMessageLaunch(const GameSettings *gameSettings);

	void buildGameSettings(GameSettings *gameSettings) const;

	virtual size_t getNetSize() const;
	virtual size_t getMaxNetSize() const;
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;
};

// =====================================================
//	class CommandList
//
///	Message to order a commands to several units
// =====================================================

class NetworkMessageCommandList: public NetworkMessage {
private:
	typedef vector<Command *> Commands;
	static const uint8 maxCommandCount = 128;

	uint32 frameCount;
	Commands commands;

public:
	NetworkMessageCommandList(NetworkDataBuffer &buf) : NetworkMessage(nmtCommandList) {read(buf);}
	NetworkMessageCommandList(int32 frameCount);
	virtual ~NetworkMessageCommandList() {
		while(!commands.empty()) {
			delete commands.back();
			commands.pop_back();
		}
	}

	bool addCommand(Command *command) {
		if(isFull()) {
			return false;
		}
		commands.push_back(command);
		return true;
	}

	size_t getCommandCount() const			{return commands.size();}
	size_t getFrameCount() const			{return frameCount;}
	Command *getCommand(int i) const		{return commands[i];}
	bool isFull() const						{return commands.size() >= maxCommandCount;}

	/**
	 * Clears the command list, but does not delete them, you should only call this if you have
	 * taken ownership of all Command objects and intend to delete them when you are finished with
	 * them.
	 */
	void clear()							{commands.clear();}

	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;
	virtual size_t getNetSize() const;
	virtual size_t getMaxNetSize() const {
		return sizeof(uint8) + sizeof(frameCount)
				+ Command::getStaticMaxNetSize() * maxCommandCount;
	}
};

// =====================================================
//	class NetworkMessageText
//
///	Chat text message
// =====================================================

class NetworkMessageText: public NetworkMessage{
private:
	static const int maxStringSize= 64;

	NetworkString<maxStringSize> text;
	NetworkString<maxStringSize> sender;
	int8 teamIndex;

public:
	NetworkMessageText(NetworkDataBuffer &buf) : NetworkMessage(nmtText) {read(buf);}
	NetworkMessageText(const string &text, const string &sender, int teamIndex);

	string getText() const		{return text.getString();}
	string getSender() const	{return sender.getString();}
	int getTeamIndex() const	{return teamIndex;}

	virtual size_t getNetSize() const;
	virtual size_t getMaxNetSize() const;
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;
};

// =====================================================
//	class NetworkMessageQuit
//
///	Message sent at the beggining of the game
// =====================================================

class NetworkMessageQuit: public NetworkMessage{
public:
	NetworkMessageQuit(NetworkDataBuffer &buf) : NetworkMessage(nmtQuit) {}
	NetworkMessageQuit() : NetworkMessage(nmtQuit) {}

	virtual size_t getNetSize() const				{return 0;}
	virtual size_t getMaxNetSize() const			{return 0;}
	virtual void read(NetworkDataBuffer &buf)		{}
	virtual void write(NetworkDataBuffer &buf) const{}
};

// =====================================================
//	class NetworkMessageFileHeader
//
///	Message to initiate sending a file
// =====================================================

class NetworkMessageFileHeader: public NetworkMessage{
private:
	static const int maxNameSize= 128;

	NetworkString<maxNameSize> name;
	int8 compressed;

public:
	NetworkMessageFileHeader(NetworkDataBuffer &buf) : NetworkMessage(nmtFileHeader) {read(buf);}
	NetworkMessageFileHeader(const string &str, bool compressed);

	string getName() const		{return name.getString();}
	bool isCompressed() const	{return compressed;}

	virtual size_t getNetSize() const;
	virtual size_t getMaxNetSize() const;
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;
};


// =====================================================
//	class NetworkMessageFileFragment
//
///	Message to send part of a file
// =====================================================

class NetworkMessageFileFragment: public NetworkMessage {
public:
	static const int bufSize= 1024;

	uint16 size;
	char data[bufSize];
	uint32 seq;
	uint8 last;

public:
	NetworkMessageFileFragment(NetworkDataBuffer &buf) : NetworkMessage(nmtFileFragment) {read(buf);}
	NetworkMessageFileFragment(char *data, size_t size, int seq, bool last);

	size_t getDataSize() const	{return size;}
	const char *getData() const	{return data;}
	int getSeq() const			{return seq;}
	bool isLast() const			{return last;}

	virtual size_t getNetSize() const;
	virtual size_t getMaxNetSize() const;
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;
};

// =====================================================
//	class NetworkMessageXmlDoc
//
///	Message containing a compressed XML file
/// TODO: Optimize using a zlib dictionary
// =====================================================

class NetworkMessageXmlDoc : public NetworkMessage {
public:
//	static const int maxSize = USHRT_MAX - 0x10;
	static const int maxCompressedSize = 0x4000;	// 16k max, and that's probably too big

protected:
	uint32 size;
	uint32 compressedSize;
	char *data;
	XmlNode *rootNode;
	bool cleanupNode;
	bool domAllocated;
	bool compressed;

public:
	NetworkMessageXmlDoc(NetworkDataBuffer &buf, NetworkMessageType type) :
			NetworkMessage(type),
			size(0),
			compressedSize(0),
			data(NULL),
			rootNode(NULL),
			cleanupNode(false),
			domAllocated(false),
			compressed(false) {
		read(buf);
	}

	NetworkMessageXmlDoc(XmlNode *rootNode, bool adoptNode, NetworkMessageType type) :
			NetworkMessage(type),
			size(0),
			compressedSize(0),
			data(NULL),
			rootNode(rootNode),
			cleanupNode(adoptNode),
			domAllocated(false),
			compressed(false) {
	}

	virtual ~NetworkMessageXmlDoc() {freeData(); freeNode();}

	XmlNode *getRootNode()			{return rootNode;}
	char *getData()					{return data;}
	size_t getDataSize() const		{return size;}
	bool isCompressed() const		{return compressed;}
	bool isParsed() const			{return rootNode;}
	bool isReadyForXmit() const		{return data && compressed;}
	void uncompress();
	void parse();
	void writeXml();
	void compress();
	void log(Logger &logger);

	virtual size_t getNetSize() const					{return compressedSize + 8;}
	virtual size_t getMaxNetSize() const				{return maxCompressedSize + 8;}
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;

private:
	void freeData();
	void freeNode();
};

// =====================================================
//	class EffectReference
//
///
// =====================================================
/*
class EffectReference : public NetworkWriteable {
	UnitReference source;
	int32 typeId;
	float strength;
	int32 duration;
	int8 recourse;

public:
	EffectReference() : typeId(-1){}

	size_t getNetSize() const				{return getMaxNetSize();}
	size_t getMaxNetSize() const {
		return
			source.getNetSize()
			+ sizeof(typeId)
			+ sizeof(strength)
			+ sizeof(duration)
			+ sizeof(recourse);
	}
	void read(NetworkDataBuffer &buf, World *world);
	void write(NetworkDataBuffer &buf) const;

	friend Effect;
};

// =====================================================
//	class UnitState
//
///
// =====================================================

class UnitState : public NetworkWriteable {
public:
	typedef vector<EffectReference> Effects;

private:
	int32 id;
	int32 hp;
	int32 ep;
	int32 loadCount;
	int32 deadCount;
	float progress;			//between 0 and 1
	float lastAnimProgress;	//between 0 and 1
	float animProgress;		//between 0 and 1
	int progress2;
	int kills;

	UnitReference targetRef;

	Field currField;
	Field targetField;

	Vec2<int32> pos;
	Vec2<int32> lastPos;
	Vec2<int32> targetPos;		//absolute target pos
	Vec3f targetVec;			//IEEE 745 format on most processors now
	Vec2<int32> meetingPos;

	float lastRotation;			//in degrees
	float targetRotation;
	float rotation;

	int type;					//const UnitType *
	int loadType;				//const ResourceType *
	string currSkill;			//const SkillType *

	bool toBeUndertaken;
	bool alive;
	bool autoRepairEnabled;

	Effects effects;
	Effects effectsCreated;

	int8 faction;
	bool fire;

	vector<int> commands;
	Unit::Pets pets;
	UnitReference master;

public:
	friend Unit;
};
*/

// =====================================================
//	class NetworkMessageUnitUpdate
//
/// Sent by the server to update a client
// =====================================================

class NetworkMessageUpdate : public NetworkMessageXmlDoc {
protected:
	XmlNode *newUnits;
	XmlNode *unitUpdates;
	XmlNode *minorUnitUpdates;
	XmlNode *factions;

public:
	NetworkMessageUpdate(NetworkDataBuffer &buf) : NetworkMessageXmlDoc(buf, nmtUpdate) {}
	NetworkMessageUpdate() : NetworkMessageXmlDoc(new XmlNode("update"), true, nmtUpdate) {
		newUnits = NULL;
		unitUpdates = NULL;
		minorUnitUpdates = NULL;
		factions = NULL;
	}

	void newUnit(Unit *unit) {
		XmlNode *n = getNewUnits()->addChild("unit");
		UnitReference(unit).save(n);
		unit->save(n);
	}

	void unitMorph(Unit *unit) {
		XmlNode *n = getUnitUpdates()->addChild("unit");
		UnitReference(unit).save(n);
		unit->save(n, true);
	}

	void unitUpdate(Unit *unit) {
		XmlNode *n = getUnitUpdates()->addChild("unit");
		UnitReference(unit).save(n);
		unit->save(n, false);
	}

	void minorUnitUpdate(Unit *unit) {
		XmlNode *n = getMinorUnitUpdates()->addChild("unit");
		UnitReference(unit).save(n);
		unit->writeMinorUpdate(n);
	}

	void updateFaction(Faction *faction) {
		faction->writeUpdate(getFactions()->addChild("faction"));
	}

	bool hasUpdates() {
		return newUnits || unitUpdates || minorUnitUpdates || factions;
	}

private:
	XmlNode *getNewUnits() {
		if(!newUnits) {
			newUnits = rootNode->addChild("new-units");
		}
		return newUnits;
	}

	XmlNode *getUnitUpdates() {
		if(!unitUpdates) {
			unitUpdates = rootNode->addChild("unit-updates");
		}
		return unitUpdates;
	}

	XmlNode *getMinorUnitUpdates() {
		if(!minorUnitUpdates) {
			minorUnitUpdates = rootNode->addChild("minor-unit-updates");
		}
		return minorUnitUpdates;
	}

	XmlNode *getFactions() {
		if(!factions) {
			factions = rootNode->addChild("factions");
		}
		return factions;
	}
};


// =====================================================
//	class NetworkMessageUpdateRequest
//
/// Sent by a client to request updates for units that are known to be in an
/// invalid state, usually because their position conflicts with the position of
/// a unit that an update was recieved for.
// =====================================================

class NetworkMessageUpdateRequest : public NetworkMessageXmlDoc {
public:
	NetworkMessageUpdateRequest(NetworkDataBuffer &buf) : NetworkMessageXmlDoc(buf, nmtUpdateRequest) {}
	NetworkMessageUpdateRequest() : NetworkMessageXmlDoc(new XmlNode("requests"), true, nmtUpdateRequest) {}

	void addUnit(UnitReference ur, bool full) {
		XmlNode *n = rootNode->addChild("unit");
		ur.save(n);
		n->addAttribute("full", full);
	}
};

}}//end namespace

#endif
