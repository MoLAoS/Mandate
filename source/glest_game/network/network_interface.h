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
#include <deque>

#include "checksum.h"
#include "network_message.h"
#include "network_types.h"
#include "network_status.h"

using std::string;
using std::vector;
using std::deque;
using Shared::Util::Checksum;
using Shared::Platform::int64;

namespace Glest { namespace Game {

// =====================================================
//	class NetworkInterface
// =====================================================

class NetworkInterface : public NetworkStatus {
protected:
	typedef deque<NetworkMessage*> MsgQueue;

	static const int readyWaitTimeout;
	NetworkDataBuffer txbuf;
	NetworkDataBuffer rxbuf;
	MsgQueue q;

private:
	string remoteHostName;
	string remotePlayerName;
	string description;

public:
	NetworkInterface() : NetworkStatus(), txbuf(16384), rxbuf(16384) {}
	virtual ~NetworkInterface() {}
	virtual Socket* getSocket() = 0;
	virtual const Socket* getSocket() const = 0;

	string getIp() const				{return getSocket()->getIp();}
	string getHostName() const			{return getSocket()->getHostName();}
	string getRemoteHostName() const	{return remoteHostName;}
	string getRemotePlayerName() const	{return remotePlayerName;}
	string getDescription() const		{return description;}
	
	void setRemoteNames(const string &hostName, const string &playerName);
	void pop()							{if(q.empty()) throw runtime_error("queue empty"); q.pop_front();}

	void send(const NetworkMessage* msg, bool flush = true);
	bool flush();
	void receive();

#ifdef DEBUG_NETWORK
	std::deque<NetworkMessagePing*> pingQ;
	
	NetworkMessage *peek() {
		receive();
		
		int now = Chrono::getCurMillis();

		while(!pingQ.empty()) {
			NetworkMessagePing *ping = pingQ.front();
			if(ping->getSimRxTime() < now) {
				processPing(ping);
				pingQ.pop_front();
			} else {
				break;
			} 
		}

		if(!q.empty()) {
			NetworkMessage *msg = q.front();
			return msg->getSimRxTime() < now ? msg : NULL;
		} else {
			return NULL;
		}
	}
#else
	NetworkMessage *peek() {
		// more processor, more accurate ping times
		receive();
		return q.empty() ? NULL : q.front();
		
		// less processor, less accurate ping times
		/*
		if(!q.empty()) {
			return q.front();
		} else {
			receive();
			return q.empty() ? NULL : q.front();
		}*/
	}
#endif

	NetworkMessage *nextMsg() {
		NetworkMessage *m = peek();
		if(m) {
			q.pop_front();
		}
		return m;
	}

	virtual bool isConnected() {
		return getSocket() && getSocket()->isConnected();
	}

protected:
	virtual void ping() {
		NetworkMessagePing msg;
		send(&msg);
	}
	
	void processPing(NetworkMessagePing *ping) {
		if(ping->isPong()) {
			pong(ping->getTime(), ping->getTimeRcvd());
		} else {
			ping->setPong();
			send(ping);
			flush();
		}
		delete ping;
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
	typedef vector<Command *> Commands;

protected:
	Commands requestedCommands;	//commands requested by the user
	Commands pendingCommands;	//commands ready to be given
	bool quit;
	string chatText;
	string chatSender;
	int chatTeamIndex;

public:
	GameNetworkInterface();
	//message processimg
//	virtual void update()= 0;
	virtual void updateLobby() = 0;
	virtual void updateKeyframe(int frameCount) = 0;
	virtual void waitUntilReady(Checksum &checksum) = 0;

	//message sending
	virtual void sendTextMessage(const string &text, int teamIndex) = 0;
	virtual void quitGame() = 0;

	//access functions
	virtual void requestCommand(Command *command)	{requestedCommands.push_back(command);}
	int getPendingCommandCount() const				{return pendingCommands.size();}
	Command *getPendingCommand(int i) const			{return pendingCommands[i];}
	void clearPendingCommands()						{pendingCommands.clear();}
	bool getQuit() const							{return quit;}
	const string getChatText() const				{return chatText;}
	const string getChatSender() const				{return chatSender;}
	int getChatTeamIndex() const					{return chatTeamIndex;}
};

}}//end namespace

#endif
