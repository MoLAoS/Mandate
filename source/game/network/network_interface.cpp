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
#include "network_interface.h"

#include <exception>
#include <cassert>

#include "types.h"
#include "conversion.h"
#include "platform_util.h"

#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;

namespace Glest{ namespace Game{

// =====================================================
//	class NetworkInterface
// =====================================================

const int NetworkInterface::readyWaitTimeout= 60000;	//1 minute


void NetworkInterface::send(const NetworkMessage* msg, bool flush) {
	size_t startBufSize = txbuf.size();
	msg->writeMsg(txbuf);
	addDataSent(txbuf.size() - startBufSize);
	if(flush) {
		NetworkInterface::flush();
	}
}

/** returns false if there is still data to be written */
bool NetworkInterface::flush() {
	if(txbuf.size()) {
		txbuf.pop(getSocket()->send(txbuf.data(), txbuf.size()));
		return !txbuf.size();
	}
	return true;
}

void NetworkInterface::receive() {
	int bytesReceived;
	NetworkMessage *m;
	Socket* socket= getSocket();
	
	rxbuf.ensureRoom(32768);
	while((bytesReceived = socket->receive(rxbuf.data(), rxbuf.room())) > 0) {
		addDataRecieved(bytesReceived);
		rxbuf.resize(rxbuf.size() + bytesReceived);
		while((m = NetworkMessage::readMsg(rxbuf))) {

			// respond immediately to pings
			if(m->getType() == nmtPing) {
#ifdef DEBUG_NETWORK
				pingQ.push_back((NetworkMessagePing*)m);
#else
				processPing((NetworkMessagePing*)m);
#endif
			} else {
				q.push_back(m);
			}
		}
		rxbuf.ensureRoom(32768);
	}
}

void NetworkInterface::setRemoteNames(const string &hostName, const string &playerName) {
	remoteHostName = hostName;
	remotePlayerName = playerName;

	stringstream str;
	str << remotePlayerName;
	if (!remoteHostName.empty()) {
		str << " (" << remoteHostName << ")";
	}
	description = str.str();
}
// =====================================================
//	class GameNetworkInterface
// =====================================================

GameNetworkInterface::GameNetworkInterface() {
	quit = false;
}


}}//end namespace
