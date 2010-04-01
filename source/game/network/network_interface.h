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
using Shared::Math::Vec3f;

namespace Glest { namespace Game {

class Unit;
class ProjectileParticleSystem;
class UnitUpdater;

// =====================================================
//	class NetworkInterface
// =====================================================

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
//	class GameInterface
//
// Adds functions common to servers and clients
// but not connection slots
// =====================================================

class GameInterface: public NetworkInterface {
private:
	typedef vector<NetworkCommand> Commands;

protected:
	typedef ProjectileParticleSystem* Projectile;

	Commands requestedCommands;	//commands requested by the user
	Commands pendingCommands;	//commands ready to be given
	bool quit;

	KeyFrame keyFrame;
	SkillCycleTable skillCycleTable;
#	if _RECORD_GAME_STATE_
		GameStateLog stateLog;
#	endif

	struct ChatMsg {
		string text;
		string sender;
		ChatMsg(const string &txt, const string &sndr) : text(txt), sender(sndr) {}
	};
	std::vector<ChatMsg> chatMessages;

	// All network accessing virtuals were made protected so the exception handling
	// can be done in the same place for clients and servers.
	//
	// For users of GameInterface, non virtual wrappers are used, see below.

	//message processimg
	virtual void update() = 0;
	virtual void updateLobby() = 0;
	virtual void updateKeyframe(int frameCount) = 0;
	virtual void waitUntilReady(Checksum &checksum) = 0;
	virtual void syncAiSeeds(int aiCount, int *seeds) = 0;
	virtual void createSkillCycleTable(const TechTree *techTree) = 0;

	// unit/projectile updates
	virtual void updateUnitCommand(Unit *unit, int32 checksum) = 0;
	virtual void updateProjectile(Unit *unit, int endFrame, int32 checksum) = 0;
	virtual void updateAnim(Unit *unit, int32 checksum) = 0;
	virtual void unitBorn(Unit *unit, int32 checksum) = 0;

	virtual void updateMove(Unit *unit) = 0;
	virtual void updateProjectilePath(Unit *u, Projectile pps, const Vec3f &start, const Vec3f &end) = 0;

	//message sending
	virtual void sendTextMessage(const string &text, int teamIndex) = 0;
	virtual void quitGame() = 0;
	
	//misc
	virtual string getStatus() const = 0;

public:
	GameInterface();

	KeyFrame& getKeyFrame() { return keyFrame; }

	void frameStart(int frame);
	void frameEnd(int frame);

#	define TRY_CATCH_LOG_THROW(methodCall)	\
		try { methodCall; }					\
		catch (runtime_error &e) {			\
			LOG_NETWORK(e.what());			\
			throw e;						\
		}

	// wrappers for the pure virtuals
	void doUpdate()										{ TRY_CATCH_LOG_THROW(update())						}
	void doUpdateLobby()								{ TRY_CATCH_LOG_THROW(updateLobby())				}
	void doUpdateKeyframe(int frameCount)				{ TRY_CATCH_LOG_THROW(updateKeyframe(frameCount))	}
	void doWaitUntilReady(Checksum &checksum)			{ TRY_CATCH_LOG_THROW(waitUntilReady(checksum))		}
	void doSyncAiSeeds(int aiCount, int *seeds)			{ TRY_CATCH_LOG_THROW(syncAiSeeds(aiCount, seeds))	}
	void doSendTextMessage(const string &text, int team){ TRY_CATCH_LOG_THROW(sendTextMessage(text, team))	}
	void doCreateSkillCycleTable(const TechTree *tt)	{ TRY_CATCH_LOG_THROW(createSkillCycleTable(tt))	}
	void doQuitGame()									{ TRY_CATCH_LOG_THROW(quitGame())					}

	void doUnitBorn(Unit *unit);
	void doUpdateProjectile(Unit *u, Projectile pps, const Vec3f &start, const Vec3f &end);
	void doUpdateUnitCommand(UnitUpdater *uu, Unit *unit);
	void doUpdateAnim(Unit *unit);

	// commands
	virtual void requestCommand(const NetworkCommand *networkCommand) {
		requestedCommands.push_back(*networkCommand);
	} 
	virtual void requestCommand(Command *command);
	
	int getPendingCommandCount() const				{return pendingCommands.size();}
	Command *getPendingCommand(int i)				{return pendingCommands[i].toCommand();}
	const NetworkCommand *getPendingNetworkCommand(int i) const			{return &pendingCommands[i];}
	void clearPendingCommands()						{pendingCommands.clear();}
	bool getQuit() const							{return quit;}

	// chat message
	bool hasChatMsg() const							{ return !chatMessages.empty(); }
	void popChatMsg()								{ chatMessages.pop_back();}
	void processTextMessage(NetworkMessageText &msg);
	const string getChatText() const				{return chatMessages.back().text;}
	const string getChatSender() const				{return chatMessages.back().sender;}
};

}}//end namespace

#endif
