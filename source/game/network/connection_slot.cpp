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
#include "connection_slot.h"

#include <stdexcept>

#include "conversion.h"
#include "game_util.h"
#include "config.h"
#include "server_interface.h"
#include "network_message.h"

#include "leak_dumper.h"
#include "logger.h"
#include "world.h"
#include "program.h"
#include "sim_interface.h"

using namespace Glest::Sim;
using namespace Shared::Util;

namespace Glest { namespace Net {

// =====================================================
//	class ClientConnection
// =====================================================

ConnectionSlot::ConnectionSlot(ServerInterface* serverInterface, int playerIndex, bool resumeSaved) {
	this->serverInterface = serverInterface;
	this->playerIndex = playerIndex;
	this->resumeSaved = resumeSaved;
	socket = NULL;
	ready = false;
}

ConnectionSlot::~ConnectionSlot() {
	close();
}

void ConnectionSlot::update() {
	if (!socket) {
		socket = serverInterface->getServerSocket()->accept();

		//send intro message when connected
		if (socket) {
			socket->setBlock(false);
			socket->setNoDelay();
			IntroMessage networkMessageIntro(
				getNetworkVersionString(), theConfig.getNetPlayerName(), socket->getHostName(), playerIndex);
			send(&networkMessageIntro);
			LOG_NETWORK( "Connection established, slot " + intToStr(playerIndex) +  " sending intro message." );
		}
		return;
	}
	if (!socket->isConnected()) {
		LOG_NETWORK("Slot " + intToStr(playerIndex) + " disconnected, [" + getRemotePlayerName() + "]");
		close();
		return;
	}
	MessageType networkMessageType = getNextMessageType();
	//process incoming commands
	switch (networkMessageType) {
		case MessageType::NO_MSG:
			break;
		case MessageType::TEXT: {
				TextMessage textMsg;
				if (receiveMessage(&textMsg)) {
					serverInterface->process(textMsg, playerIndex);
					LOG_NETWORK( "Received text message on slot " + intToStr(playerIndex) + " : " + textMsg.getText() );
				}
			}
			break;
		case MessageType::COMMAND_LIST: {
				CommandListMessage cmdList;
				if(receiveMessage(&cmdList)){
					for (int i=0; i < cmdList.getCommandCount(); ++i) {
						theSimInterface->requestCommand(cmdList.getCommand(i));
					}
				}
			}
			break;
		case MessageType::INTRO: {
				IntroMessage msg;
				if (receiveMessage(&msg)) {
					LOG_NETWORK(
						"Received intro message on slot " + intToStr(playerIndex) + ", host name = "
						+ msg.getHostName() + ", player name = " + msg.getPlayerName()
					);
					setRemoteNames(msg.getHostName(), msg.getPlayerName());
				}
			}
			break;

		case MessageType::QUIT: {
				LOG_NETWORK( "Received quit message in slot " + intToStr(playerIndex) );
				string msg = getRemotePlayerName() + " [" + getRemoteHostName() + "] has quit the game!";
				close();
				serverInterface->sendTextMessage(msg, -1);
			}
			break;
#		if _GAE_EDBUG_EDITION_
		case MessageType::SYNC_ERROR: {
				SyncErrorMsg e;
				receiveMessage(&e);
				int frame = e.getFrame();
				serverInterface->dumpFrame(frame);
				throw runtime_error("Sync error. GameState records dumped.");
			}
			break;
#		endif
		default: {
				stringstream ss;
				ss << "Unexpected message type: " << networkMessageType << " on slot: " << playerIndex;
				LOG_NETWORK( ss.str() );
				ss.clear();
				ss << "Player " << playerIndex << " [" << getName()
					<< "] was disconnected because they sent the server bad data.";
				serverInterface->sendTextMessage(ss.str(), -1);
				close();
			}
			return;
	}
}

void ConnectionSlot::close() {
	if (socket) {
		socket->close();
	}
	delete socket;
	socket = NULL;
}

}}//end namespace
