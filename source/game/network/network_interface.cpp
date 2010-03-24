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
#include "world.h"

#include "leak_dumper.h"
#include "logger.h"
#include "network_util.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;

namespace Glest { namespace Game {

// =====================================================
//	class NetworkInterface
// =====================================================

const int NetworkInterface::readyWaitTimeout= 60000;	//1 minute

void NetworkInterface::send(const NetworkMessage* networkMessage) {
	Socket* socket= getSocket();
	networkMessage->send(socket);
}

//NETWORK: this is (re)moved
NetworkMessageType NetworkInterface::getNextMessageType(){
	Socket* socket = getSocket();
	int8 messageType = NetworkMessageType::NO_MSG;

	//peek message type
	if (socket->getDataToRead() >= sizeof(messageType)) {
		socket->peek(&messageType, sizeof(messageType));
	}

	//sanity check new message type
	if (messageType < 0 || messageType >= NetworkMessageType::COUNT){
		throw runtime_error("Invalid message type: " + intToStr(messageType));
	}
	return NetworkMessageType(messageType);
}

int NetworkInterface::dataAvailable() {
	return getSocket()->getDataToRead();
}

bool NetworkInterface::receiveMessage(NetworkMessage* networkMessage){
	Socket* socket= getSocket();
	return networkMessage->receive(socket);
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

void GameNetworkInterface::requestCommand(Command *command) {
	Unit *unit = command->getCommandedUnit();
	
	if (command->isAuto()) {
		pendingCommands.push_back(command);
	}

	if (command->getArchetype() == CommandArchetype::GIVE_COMMAND) {
		requestedCommands.push_back(NetworkCommand(command));
	} else if (command->getArchetype() == CommandArchetype::CANCEL_COMMAND) {
		requestedCommands.push_back(NetworkCommand(nctCancelCommand, unit, Vec2i(-1)));
	}
}

void GameNetworkInterface::processTextMessage(NetworkMessageText &msg) {
	if (msg.getTeamIndex() == -1 
	|| msg.getTeamIndex() == theWorld.getThisFaction()->getTeam()) {
		chatMessages.push_back(ChatMsg(msg.getText(), msg.getSender()));
	}
}

}}//end namespace
