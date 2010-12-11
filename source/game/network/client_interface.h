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

#ifndef _GLEST_GAME_CLIENTINTERFACE_H_
#define _GLEST_GAME_CLIENTINTERFACE_H_

#include <vector>
#include <fstream>

#include "network_interface.h"
#include "game_settings.h"

#include "socket.h"

using Shared::Platform::Ip;
using Shared::Platform::ClientSocket;
using std::vector; //using std::deque;

namespace Glest { namespace Net {

// =====================================================
//	class ClientInterface
// =====================================================
/** A concrete SimulationInterface for network clients */
class ClientInterface: public NetworkInterface {
private:
	static const int messageWaitTimeout = 10000; // 10 seconds
	static const int waitSleepTime = 5; // 5 milli-seconds

	ClientSocket *clientSocket;
	string serverName;
	bool introDone;
	bool launchGame;
	int playerIndex;

public:
	ClientInterface(Program &prog);
	virtual ~ClientInterface();

	virtual Socket* getSocket()					{return clientSocket;}
	virtual const Socket* getSocket() const		{return clientSocket;}

	virtual GameRole getNetworkRole() const { return GameRole::CLIENT; }

	/** Called instead of update() while in MenuStateJoinGame */
	virtual void updateLobby();

	// unit-skill updates
	virtual void updateSkillCycle(Unit *unit);

protected:
	// SimulationInterface and NetworkInterface virtuals
	// See documentation in sim_interface.h & net_interface.h

	// Game launch hooks, SimulationInterface virtuals
	virtual void waitUntilReady();
	virtual void syncAiSeeds(int aiCount, int *seeds);
	virtual void doDataSync();
	virtual void createSkillCycleTable(const TechTree *);
	virtual void startGame();

	// Game progress, NetworkInterface virtuals
	virtual void update();
	virtual void updateKeyframe(int frameCount);

	// projectile updates, SimulationInterface virtuals
	virtual void updateProjectilePath(Unit *u, Projectile *pps, const Vec3f &start, const Vec3f &end);

	// move skill needs special handling, NetworkInterface virtual
	virtual void updateMove(Unit *unit);

#	if MAD_SYNC_CHECKING
		void handleSyncError();

		// unit/projectile checks, NetworkInterface virtuals
		virtual void checkCommandUpdate(Unit *unit, int32);
		virtual void checkUnitBorn(Unit *unit, int32);
		virtual void checkProjectileUpdate(Unit *unit, int, int32);
		virtual void checkAnimUpdate(Unit *unit, int32);
#	endif

	//misc
	virtual string getStatus() const;

public:
	//accessors
	string getServerName() const			{return serverName;}
	bool getLaunchGame() const				{return launchGame;}
	bool getIntroDone() const				{return introDone;}
	int getPlayerIndex() const				{return playerIndex;}

	void connect(const Ip &ip, int port);
	void reset();

	// message sending
	virtual void sendTextMessage(const string &text, int teamIndex);
	virtual void quitGame(QuitSource);

private:
	void waitForMessage(int timeout = messageWaitTimeout);

	void doIntroMessage();
	void doLaunchMessage();
};

}}//end namespace

#endif
