// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_NETWORKINTERFACE_H_
#define _GLEST_GAME_NETWORKINTERFACE_H_

#include <string>
#include <vector>

#include "checksum.h"
#include "network_message.h"
#include "network_types.h"
#include "sim_interface.h"
#include "logger.h"

using std::string;
using std::vector;
using Shared::Util::Checksum;
using Shared::Math::Vec3f;

namespace Glest { namespace Net {

WRAPPED_ENUM( NetworkState, LOBBY, GAME )

// =====================================================
//	class NetworkConnection
// =====================================================

class NetworkConnection {
protected:
	typedef deque<RawMessage> MessageQueue;
	static const int readyWaitTimeout = 60000; // 60 sec

private:
	string remoteHostName;
	string remotePlayerName;
	string description;

	// received but not processed messages
	MessageQueue messageQueue;

public:
	virtual ~NetworkConnection() {}

	virtual Socket* getSocket() = 0;
	virtual const Socket* getSocket() const = 0;

	string getIp() const				{return getSocket()->getIp();}
	string getHostName() const			{return getSocket()->getHostName();}
	string getRemoteHostName() const	{return remoteHostName;}
	string getRemotePlayerName() const	{return remotePlayerName;}
	string getDescription() const		{return description;}

	void setRemoteNames(const string &hostName, const string &playerName);
	int dataAvailable();
	void send(const Message* networkMessage);

	// message retrieval
	void receiveMessages();
	bool hasMessage()					{ return !messageQueue.empty(); }
	RawMessage getNextMessage();
	MessageType peekNextMsg() const;
	void pushMessage(RawMessage raw)	{ messageQueue.push_back(raw); }

	bool isConnected()					{ return getSocket() && getSocket()->isConnected(); }
};

// =====================================================
//	class NetworkInterface
//
// Adds functions common to servers and clients
// but not connection slots
// =====================================================

/** An abstract SimulationInterface for network games */
class NetworkInterface: public NetworkConnection, public SimulationInterface {
public:
	typedef vector<NetworkCommand> Commands;

protected:
	KeyFrame keyFrame;

	// chat messages
	struct ChatMsg {
		string text;
		string sender;
		ChatMsg(const string &txt, const string &sndr) : text(txt), sender(sndr) {}
	};
	std::vector<ChatMsg> chatMessages;

	/** Called after each frame is processed, SimulationInterface virtual */
	virtual void frameProccessed();

	/** send/receive key-frame, issue queued commands */
	virtual void updateKeyframe(int frameCount) = 0;

	// misc
	virtual string getStatus() const = 0;

#if MAD_SYNC_CHECKING
	/** 'Interesting event' handlers, for insane checksum comparisons */
	virtual void checkCommandUpdate(Unit *unit, int32 checksum) = 0;
	virtual void checkProjectileUpdate(Unit *unit, int endFrame, int32 checksum) = 0;
	virtual void checkAnimUpdate(Unit *unit, int32 checksum) = 0;
	virtual void checkUnitBorn(Unit *unit, int32 checksum) = 0;

	/** SimulationInterface post 'interesting event' virtuals (calc checksum and pass to checkXxxXxx()) */
	virtual void postCommandUpdate(Unit *unit);
	virtual void postProjectileUpdate(Unit *unit, int endFrame);
	virtual void postAnimUpdate(Unit *unit);
	virtual void postUnitBorn(Unit *unit);
#endif

public:
	NetworkInterface(Program &prog);

	KeyFrame& getKeyFrame() { return keyFrame; }

	/** Called frequently to check for and/or send new network messages */
	virtual void update() = 0;

	// send chat message
	virtual void sendTextMessage(const string &text, int teamIndex) = 0;

	// place message on chat queue
	void processTextMessage(TextMessage &msg);
	
	// chat message queue
	bool hasChatMsg() const							{ return !chatMessages.empty(); }
	void popChatMsg()								{ chatMessages.pop_back();}
	const string& getChatText() const				{return chatMessages.back().text;}
	const string& getChatSender() const				{return chatMessages.back().sender;}
};

}}//end namespace

#endif
