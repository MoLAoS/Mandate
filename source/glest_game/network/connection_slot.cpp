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

using namespace std;
using namespace Shared::Util;

namespace Glest { namespace Game {

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
	if(!socket) {
		socket = serverInterface->getServerSocket()->accept();

		//send intro message when connected
		if(socket) {
			NetworkMessageIntro networkMessageIntro(getNetworkVersionString(), socket->getHostName(),
					Config::getInstance().getNetPlayerName(), playerIndex, resumeSaved);
			send(&networkMessageIntro, true);
		}
	} else {
		NetworkMessage *genericMsg = NULL;
		if(!socket->isConnected()) {
			close();
			return;
		}
				
		while ((genericMsg = nextMsg())) {
			try {
				//process incoming commands
				switch(genericMsg->getType()) {

					//process intro messages
					case nmtIntro: {
						NetworkMessageIntro *msg = (NetworkMessageIntro*)genericMsg;
						setRemoteNames(msg->getHostName(), msg->getPlayerName());
					}
					break;

					//command list
					case nmtCommandList: {
						NetworkMessageCommandList *msg = (NetworkMessageCommandList*)genericMsg;
						for (int i = 0; i < msg->getCommandCount(); ++i){
							serverInterface->requestCommand(msg->getCommand(i));
						}
						msg->clear();
					}
					break;

					case nmtText:
						serverInterface->process(*reinterpret_cast<NetworkMessageText*>(genericMsg), playerIndex);
						break;
					
					case nmtQuit:			// I pwned joo wus!
						break;

					case nmtUpdateRequest:
						serverInterface->process(*reinterpret_cast<NetworkMessageUpdateRequest*>(genericMsg));
						break;

					case nmtReady:			// cute, but I don't care
					case nmtLaunch: 		// launch your heart out
						break;
					
					case nmtFileHeader:		// I don't want your files
					case nmtFileFragment:
					case nmtInvalid:		// invalid, duh!
					case nmtUpdate:			// no thanks, you're not the server
					case nmtPing:			// ping should be intercepted in NetworkInterface::receive()
					case nmtCount:
						throw runtime_error("Unexpected message in connection slot: " + intToStr(genericMsg->getType()));
				}
				flush();
			} catch (runtime_error &e) {
				delete genericMsg;
				throw e;
			}
			delete genericMsg;
		}
		NetworkStatus::update();
	}
}

void ConnectionSlot::close() {
	delete socket;
	socket = NULL;
}

}}//end namespace
