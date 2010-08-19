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

#ifndef _GLEST_GAME_SERVERINTERFACE_H_
#define _GLEST_GAME_SERVERINTERFACE_H_

#include <vector>

#include "game_constants.h"
#include "network_interface.h"
#include "connection_slot.h"
#include "socket.h"

using std::vector;
using Shared::Platform::ServerSocket;

namespace Glest { namespace Net {

// =====================================================
//	class ServerInterface
// =====================================================

/** A concrete SimulationInterface for network servers */
class ServerInterface: public NetworkInterface {
private:
	struct LostPlayerInfo {
		string playerName;
		string hostName;
		int64 timeDropped;
	};

private:
	ConnectionSlot* slots[GameConstants::maxPlayers];
	ServerSocket serverSocket;
	vector<LostPlayerInfo> m_lostPlayers;
	bool m_waitingForPlayers;
	DataSyncMessage *m_dataSync;
	int m_syncCounter;
	bool m_dataSyncDone;

public:
	ServerInterface(Program &prog);
	virtual ~ServerInterface();

	virtual Socket* getSocket()				{return &serverSocket;}
	virtual const Socket* getSocket() const	{return &serverSocket;}

	virtual GameRole getNetworkRole() const { return GameRole::SERVER; }
	bool syncReady() const { return m_dataSync; }

#	if _GAE_DEBUG_EDITION_
		void dumpFrame(int frame);
#	endif

protected:
	// SimulationInterface and NetworkInterface virtuals
	// See documentation in sim_interface.h

	//message processing
	virtual void doDataSync();
	virtual void update();
	virtual void updateKeyframe(int frameCount);
	virtual void waitUntilReady();
	virtual void syncAiSeeds(int aiCount, int *seeds);
	virtual void createSkillCycleTable(const TechTree *techTree);

	// unit/projectile updates
	virtual void checkCommandUpdate(Unit *unit, int32);
	virtual void checkUnitBorn(Unit *unit, int32);
	virtual void checkProjectileUpdate(Unit *unit, int, int32);
	virtual void checkAnimUpdate(Unit *unit, int32);

	virtual void updateSkillCycle(Unit *unit);

	//virtual void updateMove(Unit *unit);
	virtual void updateProjectilePath(Unit *u, Projectile *pps, const Vec3f &start, const Vec3f &end);

	//misc
	virtual string getStatus() const;

public:
	// used to listen for new connections
	ServerSocket* getServerSocket()		{return &serverSocket;}

	// ConnectionSlot management
	void addSlot(int playerIndex);
	void removeSlot(int playerIndex);
	ConnectionSlot* getSlot(int playerIndex);
	int getConnectedSlotCount();
	
	void dataSync(int playerNdx, DataSyncMessage &msg);
	void doLaunchBroadcast();
	void process(TextMessage &msg, int requestor);

	// message sending
	virtual void sendTextMessage(const string &text, int teamIndex);
	virtual void quitGame(QuitSource);

private:
	void broadcastMessage(const Message* networkMessage, int excludeSlot= -1);
	void updateListen();
};

}}//end namespace

#endif
