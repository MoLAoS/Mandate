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

#include "pch.h"
#include "network_manager.h"

#include "leak_dumper.h"

namespace Glest { namespace Game {

// =====================================================
//	class NetworkManager
// =====================================================

NetworkManager &NetworkManager::getInstance() {
	static NetworkManager networkManager;
	return networkManager;
}

NetworkManager::NetworkManager() {
	gameNetworkInterface = NULL;
	networkRole = nrIdle;
}

void NetworkManager::init(NetworkRole networkRole) {
	assert(gameNetworkInterface == NULL);

	this->networkRole = networkRole;

	if(networkRole == nrServer) {
		gameNetworkInterface = new ServerInterface();
	} else {
		gameNetworkInterface = new ClientInterface();
	}
}

NetworkManager::~NetworkManager() {
	delete gameNetworkInterface;
}

void NetworkManager::end() {
	delete gameNetworkInterface;
	gameNetworkInterface = NULL;
	networkRole = nrIdle;
}

}}//end namespace
