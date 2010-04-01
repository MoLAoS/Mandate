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

namespace Glest{ namespace Game{

// =====================================================
//	class ClientInterface
// =====================================================
class ClientInterface: public GameInterface{
private:
	static const int messageWaitTimeout;
	static const int waitSleepTime;

	ClientSocket *clientSocket;
	GameSettings gameSettings;
	string serverName;
	bool introDone;
	bool launchGame;
	int playerIndex;

public:
	ClientInterface();
	virtual ~ClientInterface();

	virtual Socket* getSocket()					{return clientSocket;}
	virtual const Socket* getSocket() const		{return clientSocket;}

protected:
	//message processing
	virtual void update();
	virtual void updateLobby();
	virtual void updateKeyframe(int frameCount);
	virtual void waitUntilReady(Checksum &checksum);
	virtual void syncAiSeeds(int aiCount, int *seeds);
	virtual void createSkillCycleTable(const TechTree *);

	// message sending
	virtual void sendTextMessage(const string &text, int teamIndex);
	virtual void quitGame();

	// unit/projectile updates
	virtual void updateUnitCommand(Unit *unit, int32);
	virtual void unitBorn(Unit *unit, int32);
	virtual void updateProjectile(Unit *unit, int endFrame, int32);
	virtual void updateAnim(Unit *unit, int32);

	virtual void updateMove(Unit *unit);
	virtual void updateProjectilePath(Unit *u, Projectile pps, const Vec3f &start, const Vec3f &end);

	//misc
	virtual string getStatus() const;

public:
	//accessors
	string getServerName() const			{return serverName;}
	bool getLaunchGame() const				{return launchGame;}
	bool getIntroDone() const				{return introDone;}
	int getPlayerIndex() const				{return playerIndex;}
	const GameSettings *getGameSettings()	{return &gameSettings;}

	void connect(const Ip &ip, int port);
	void reset();

private:
	void waitForMessage();
};

}}//end namespace

#endif
