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
			LOG_NETWORK( "Connection established, slot " + intToStr(playerIndex) +  " sending intro message." );
			socket->setBlock(false);
			socket->setNoDelay();
			IntroMessage networkMessageIntro(getNetworkVersionString(), 
				g_config.getNetPlayerName(), socket->getHostName(), playerIndex);
			send(&networkMessageIntro);
		}
		return;
	}
	if (!socket->isConnected()) {
		LOG_NETWORK("Slot " + intToStr(playerIndex) + " disconnected, [" + getRemotePlayerName() + "]");
		close();
		return;
	}
	//process incoming commands
	try {
		receiveMessages();
	} catch (SocketException &e) {
		NETWORK_LOG(
			"Slot " << playerIndex << " [" << getRemotePlayerName() << "]"
			<< " : " << e.what()
		);
		string msg = getRemotePlayerName() + " [" + getRemoteHostName() + "] has disconnected.";
		ServerInterface *si = serverInterface;
		serverInterface->removeSlot(playerIndex);
		si->sendTextMessage(msg, -1);
		return;
	}
	while (hasMessage()) {
		RawMessage raw = getNextMessage();
		if (raw.type == MessageType::TEXT) {
			LOG_NETWORK( "Received text message on slot " + intToStr(playerIndex) );
			TextMessage textMsg(raw);
			serverInterface->process(textMsg, playerIndex);
		} else if (raw.type == MessageType::INTRO) {
			IntroMessage msg(raw);
			LOG_NETWORK(
				"Received intro message on slot " + intToStr(playerIndex) + ", host name = "
				+ msg.getHostName() + ", player name = " + msg.getPlayerName()
			);
			setRemoteNames(msg.getHostName(), msg.getPlayerName());
		} else if (raw.type == MessageType::COMMAND_LIST) {
			LOG_NETWORK( "Received command list message on slot " + intToStr(playerIndex) );
			CommandListMessage cmdList(raw);
			for (int i=0; i < cmdList.getCommandCount(); ++i) {
				serverInterface->requestCommand(cmdList.getCommand(i));
			}
		} else if (raw.type == MessageType::QUIT) {
			QuitMessage quitMsg(raw);
			LOG_NETWORK( "Received quit message on slot " + intToStr(playerIndex) );
			string msg = getRemotePlayerName() + " [" + getRemoteHostName() + "] has quit the game!";
			ServerInterface *si = serverInterface;
			serverInterface->removeSlot(playerIndex);
			si->sendTextMessage(msg, -1);
			return;
#		if _GAE_EDBUG_EDITION_
		} else if (raw.type == MessageType::SYNC_ERROR) {
			SyncErrorMsg e;
			receiveMessage(&e);
			int frame = e.getFrame();
			serverInterface->dumpFrame(frame);
			throw GameSyncError();
#		endif
		} else {
			stringstream ss;
			ss << "Unexpected message type: " << raw.type << " on slot: " << playerIndex;
			LOG_NETWORK( ss.str() );
			ss.clear();
			ss << "Player " << playerIndex << " [" << getName()
				<< "] was disconnected because they sent the server bad data.";
			ServerInterface *si = serverInterface;
			serverInterface->removeSlot(playerIndex);
			si->sendTextMessage(ss.str(), -1);
			return;
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
