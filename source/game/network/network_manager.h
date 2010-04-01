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

#ifndef _GLEST_GAME_NETWORKMANAGER_H_
#define _GLEST_GAME_NETWORKMANAGER_H_

#include <cassert>

#include "socket.h"
#include "checksum.h"
#include "server_interface.h"
#include "client_interface.h"

using Shared::Util::Checksum;

namespace Glest { namespace Game {

// =====================================================
//	class NetworkManager
// =====================================================

enum NetworkRole{
	nrServer,
	nrClient,
	nrIdle
};

class NetworkManager{
private:
	GameInterface* gameInterface;
	NetworkRole networkRole;

public:
	static NetworkManager &getInstance();

	NetworkManager();
	~NetworkManager();
	void init(NetworkRole networkRole);
	void end();

	void update() {
		if(gameInterface) {
			gameInterface->doUpdate();
		}
	}

	bool isLocal() {
		return !isNetworkGame();
	}

	bool isServer() {
		return networkRole == nrServer;
	}

	bool isNetworkServer() {
		return networkRole == nrServer && getServerInterface()->getConnectedSlotCount() > 0;
	}

	bool isNetworkClient() {
		return networkRole == nrClient;
	}

	bool isNetworkGame() {
		return networkRole == nrClient || getServerInterface()->getConnectedSlotCount() > 0;
	}

	GameInterface* getGameInterface() {
		assert(gameInterface != NULL);
		return gameInterface;
	}

	ServerInterface* getServerInterface() {
		assert(gameInterface != NULL);
		assert(networkRole == nrServer);
		return static_cast<ServerInterface*>(gameInterface);
	}

	ClientInterface* getClientInterface() {
		assert(gameInterface != NULL);
		assert(networkRole == nrClient);
		return static_cast<ClientInterface*>(gameInterface);
	}
};

}}//end namespace

#endif
