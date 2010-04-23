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
#include "sim_interface.h"
#include "program.h"

using Shared::Util::Checksum;

namespace Glest { namespace Net {

// =====================================================
//	class NetworkManager
// =====================================================
/*
enum GameRole{
	nrServer,
	nrClient,
	nrIdle
};*/
/*
class NetworkManager{
private:
	//NetworkInterface* netInterface;
	//GameRole networkRole;

public:
	static NetworkManager &getInstance();

	NetworkManager();
	~NetworkManager();
	//void init(GameRole networkRole);
	//void end();

	//REFACTOR: get this into Program ?
	void update() {
		NetworkInterface *netInterface = theSimInterface->asNetworkInterface();
		if (netInterface) {
			netInterface->update();
		}
	}

	bool isLocal() {
		return !isNetworkGame();
	}

	bool isServer() {
		return theSimInterface->getNetworkRole() == GameRole::SERVER;
	}

	bool isNetworkServer() {
		return theSimInterface->getNetworkRole() == GameRole::SERVER 
			&& getServerInterface()->getConnectedSlotCount() > 0;
	}

	bool isNetworkClient() {
		return theSimInterface->getNetworkRole() == GameRole::NONE;
	}

	bool isNetworkGame() {
		return theSimInterface->getNetworkRole() == GameRole::NONE 
			|| getServerInterface()->getConnectedSlotCount() > 0;
	}

	NetworkInterface* getNetworkInterface() {
		if (theSimInterface->getNetworkRole() != GameRole::NONE) {
			return static_cast<NetworkInterface*>(theSimInterface);
		}
		return 0;
	}

	ServerInterface* getServerInterface() {
		if (theSimInterface->getNetworkRole() == GameRole::SERVER) {
			return static_cast<ServerInterface*>(theSimInterface);
		}
		return 0;
	}

	ClientInterface* getClientInterface() {
		if (theSimInterface->getNetworkRole() == GameRole::NONE) {
			return static_cast<ClientInterface*>(theSimInterface);
		}
		return 0;
	}
};*/

}}// namespace Game::Net

#endif
