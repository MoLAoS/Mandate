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

ConnectionSlot::ConnectionSlot(ServerInterface* serverInterface, int playerIndex) {
	this->serverInterface = serverInterface;
	this->playerIndex = playerIndex;
	socket = NULL;
	ready = false;
}

ConnectionSlot::~ConnectionSlot() {
	close();
}

void ConnectionSlot::update() {
	if (!socket) {
		socket = serverInterface->getServerSocket()->accept();

		// send intro message when connected
		if (socket) {
			NETWORK_LOG( "Connection established, slot " << playerIndex << " sending intro message." );
			socket->setBlock(false);
			socket->setNoDelay();
			IntroMessage networkMessageIntro(getNetworkVersionString(), 
				g_config.getNetPlayerName(), socket->getHostName(), playerIndex);
			send(&networkMessageIntro);
		}
		return;
	}
	if (!socket->isConnected()) {
		NETWORK_LOG( "Slot " << playerIndex << " disconnected, [" << getRemotePlayerName() << "]" );
		throw Disconnect();
	}
	// process incoming commands
	try {
		receiveMessages();
	} catch (SocketException &e) {
		NETWORK_LOG( "Slot " << playerIndex << " [" << getRemotePlayerName() << "]" << " : " << e.what() );
		string msg = getRemotePlayerName() + " [" + getRemoteHostName() + "] has disconnected.";
		serverInterface->sendTextMessage(msg, -1);
		throw Disconnect();
	}
	while (hasMessage()) {
		MessageType type = peekNextMsg();
		if (type == MessageType::DATA_SYNC) {
			if (!serverInterface->syncReady()) {
				return;
			}
		} else if (type == MessageType::READY && !serverInterface->syncReady()) {
			return;
		}
		RawMessage raw = getNextMessage();
		if (raw.type == MessageType::TEXT) {
			NETWORK_LOG( "Received text message on slot " << playerIndex );
			TextMessage textMsg(raw);
			serverInterface->process(textMsg, playerIndex);
		} else if (raw.type == MessageType::INTRO) {
			IntroMessage msg(raw);
			NETWORK_LOG( "Received intro message on slot " << playerIndex << ", host name = "
				<< msg.getHostName() << ", player name = " << msg.getPlayerName() );
			setRemoteNames(msg.getHostName(), msg.getPlayerName());
		} else if (raw.type == MessageType::COMMAND_LIST) {
			NETWORK_LOG( "Received command list message on slot " << playerIndex );
			CommandListMessage cmdList(raw);
			for (int i=0; i < cmdList.getCommandCount(); ++i) {
				serverInterface->requestCommand(cmdList.getCommand(i));
			}
		} else if (raw.type == MessageType::QUIT) {
			QuitMessage quitMsg(raw);
			NETWORK_LOG( "Received quit message on slot " << playerIndex );
			string msg = getRemotePlayerName() + " [" + getRemoteHostName() + "] has quit the game!";
			serverInterface->sendTextMessage(msg, -1);
			throw Disconnect();
#		if MAD_SYNC_CHECKING
		} else if (raw.type == MessageType::SYNC_ERROR) {
			SyncErrorMsg e;
			receiveMessage(&e);
			int frame = e.getFrame();
			serverInterface->dumpFrame(frame);
			throw GameSyncError("Client detected sync error");
#		endif
		} else if (raw.type == MessageType::DATA_SYNC) {
			DataSyncMessage msg(raw);
			serverInterface->dataSync(playerIndex, msg);
		} else {
			NETWORK_LOG( "Unexpected message type: " << raw.type << " on slot: " << playerIndex );
			stringstream ss;
			ss << "Player " << playerIndex << " [" << getName()
				<< "] was disconnected because they sent the server bad data.";
			serverInterface->sendTextMessage(ss.str(), -1);
			throw InvalidMessage((int8)raw.type);
		}
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
