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

#ifndef _GLEST_GAME_NETWORKINTERFACE_H_
#define _GLEST_GAME_NETWORKINTERFACE_H_

#include <string>
#include <vector>
//#include <deque>

#include "checksum.h"
#include "network_message.h"
#include "network_types.h"
//#include "network_status.h"

#include "logger.h"

using std::string;
using std::vector;
//using std::deque;
using Shared::Util::Checksum;
//using Shared::Platform::int64;

class Command;

namespace Glest { namespace Game {

// =====================================================
//	class NetworkInterface
// =====================================================
//NETWORK: a lot different with this class
class NetworkInterface {
protected:
	static const int readyWaitTimeout;

private:
	string remoteHostName;
	string remotePlayerName;
	string description;

public:
	virtual ~NetworkInterface() {}

	virtual Socket* getSocket() = 0;
	virtual const Socket* getSocket() const = 0;

	string getIp() const				{return getSocket()->getIp();}
	string getHostName() const			{return getSocket()->getHostName();}
	string getRemoteHostName() const	{return remoteHostName;}
	string getRemotePlayerName() const	{return remotePlayerName;}
	string getDescription() const		{return description;}

	void setRemoteNames(const string &hostName, const string &playerName);

	int dataAvailable();

	void send(const NetworkMessage* networkMessage);
	NetworkMessageType getNextMessageType();
	bool receiveMessage(NetworkMessage* networkMessage);

	bool isConnected() {
		return getSocket() && getSocket()->isConnected();
	}
};

// =====================================================
//	class GameNetworkInterface
//
// Adds functions common to servers and clients
// but not connection slots
// =====================================================

class GameNetworkInterface: public NetworkInterface {
private:
	typedef vector<NetworkCommand> Commands; //NETWORK: uses command instead

protected:
	Commands requestedCommands;	//commands requested by the user
	Commands pendingCommands;	//commands ready to be given
	bool quit;

	struct ChatMsg {
		string text;
		string sender;

		ChatMsg(const string &txt, const string &sndr)
				: text(txt), sender(sndr) {}
	};
	std::vector<ChatMsg> chatMessages;

	// All network accessing virtuals were made protected so the exception handling
	// can be done in the same place for clients and servers.
	//
	// For users of GameNetworkInterface, non virtual wrappers are used, see below.

	//message processimg
	virtual void update() = 0;
	virtual void updateLobby() = 0;
	virtual void updateKeyframe(int frameCount) = 0;
	virtual void waitUntilReady(Checksum &checksum) = 0;
	virtual void syncAiSeeds(int aiCount, int *seeds) = 0;
	//virtual void logUnit(int id) = 0;

	//message sending
	virtual void sendTextMessage(const string &text, int teamIndex) = 0;
	virtual void quitGame() = 0;
	
	//misc
	virtual string getStatus() const = 0;

public:
	GameNetworkInterface();

	void doUpdate() {
		try {
			update();
		} catch (runtime_error e) {
			LOG_NETWORK(e.what());
			throw e;
		}
	}

	void doUpdateLobby() {
		try {
			updateLobby();
		} catch (runtime_error e) {
			LOG_NETWORK(e.what());
			throw e;
		}
	}

	void doUpdateKeyframe(int frameCount) {
		try {
			updateKeyframe(frameCount);
		} catch (runtime_error e) {
			LOG_NETWORK(e.what());
			throw e;
		}
	}

	void doWaitUntilReady(Checksum &checksum) {
		try {
			waitUntilReady(checksum);
		} catch (runtime_error e) {
			LOG_NETWORK(e.what());
			throw e;
		}
	}

	void doSyncAiSeeds(int aiCount, int *seeds) {
		try {
			syncAiSeeds(aiCount, seeds);
		} catch (runtime_error e) {
			LOG_NETWORK(e.what());
			throw e;
		}
	}

	void doSendTextMessage(const string &text, int teamIndex) {
		try {
			sendTextMessage(text, teamIndex);
		} catch (runtime_error e) {
			LOG_NETWORK(e.what());
			throw e;
		}
	}

	void doQuitGame() {
		try {
			quitGame();
		} catch (runtime_error e) {
			LOG_NETWORK(e.what());
			throw e;
		}
	}

	string doGetStatus() const {
		string res;
		try {
			res = getStatus();
		} catch (runtime_error e) {
			LOG_NETWORK(e.what());
			throw e;
		}
	}

	//access functions

	virtual void requestCommand(const NetworkCommand *networkCommand) {
		requestedCommands.push_back(*networkCommand);
	} 
	
	virtual void requestCommand(Command *command);
	
	int getPendingCommandCount() const				{return pendingCommands.size();}
	Command *getPendingCommand(int i)				{return pendingCommands[i].toCommand();}
	const NetworkCommand *getPendingNetworkCommand(int i) const			{return &pendingCommands[i];}
	void clearPendingCommands()						{pendingCommands.clear();}
	bool getQuit() const							{return quit;}
	
	bool hasChatMsg() const				{ return !chatMessages.empty(); }
	void popChatMsg() { chatMessages.pop_back();}
	void processTextMessage(NetworkMessageText &msg);

	const string getChatText() const				{return chatMessages.back().text;}
	const string getChatSender() const				{return chatMessages.back().sender;}
};

}}//end namespace

#endif
