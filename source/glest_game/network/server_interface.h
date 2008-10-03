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
#include <map>

#include "game_constants.h"
#include "network_interface.h"
#include "connection_slot.h"
#include "socket.h"

using std::vector;
using std::map;
using Shared::Platform::ServerSocket;

namespace Glest{ namespace Game{

// =====================================================
//	class ServerInterface
// =====================================================

class ServerInterface: public GameNetworkInterface{
private:
	enum UnitUpdateType {
		uutNew,
		uutMorph,
		uutFullUpdate,
		uutPartialUpdate
	};
	typedef map<Unit *, UnitUpdateType> UnitUpdateMap;

	ConnectionSlot* slots[GameConstants::maxPlayers];
	ServerSocket serverSocket;
	UnitUpdateMap updateMap;
	bool updateFactionsFlag;

public:
	ServerInterface();
	virtual ~ServerInterface();

	virtual Socket* getSocket()				{return &serverSocket;}
	virtual const Socket* getSocket() const	{return &serverSocket;}

	//message processing
	virtual void update();
	virtual void updateLobby(){};
	virtual void updateKeyframe(int frameCount);
	virtual void waitUntilReady(Checksum &checksum);

	// message sending
	virtual void sendTextMessage(const string &text, int teamIndex);
	virtual void quitGame();

	//misc
	virtual string getNetworkStatus() const;

	ServerSocket* getServerSocket()		{return &serverSocket;}
	void addSlot(int playerIndex);
	void removeSlot(int playerIndex);
	ConnectionSlot* getSlot(int playerIndex);
	int getConnectedSlotCount();

	void launchGame(const GameSettings* gameSettings, const string savedGameFile = "");
	void sendFile(const string path, const string remoteName, bool compress);
	void updateFactions()				{updateFactionsFlag = true;}
	void newUnit(Unit *unit)			{addUnitUpdate(unit, uutNew);}
	void unitMorph(Unit *unit)			{addUnitUpdate(unit, uutMorph);}
	void unitUpdate(Unit *unit)			{addUnitUpdate(unit, uutFullUpdate);}
	void minorUnitUpdate(Unit *unit)	{addUnitUpdate(unit, uutPartialUpdate);}
	void sendUpdates();

private:
	void addUnitUpdate(Unit *unit, UnitUpdateType type);
	void processUpdateRequests(NetworkMessageUpdateRequest &msg);
	void broadcastMessage(const NetworkMessage* networkMessage, int excludeSlot= -1);
	void updateListen();
};

}}//end namespace

#endif
