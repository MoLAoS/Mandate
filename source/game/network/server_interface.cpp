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
#include "server_interface.h"

#include "platform_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "world.h"
#include "game.h"
#include "network_types.h"

#include "leak_dumper.h"
#include "logger.h"

using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
//	class ServerInterface
// =====================================================

ServerInterface::ServerInterface() {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		slots[i] = NULL;
	}
	try {
		serverSocket.setBlock(false);
		serverSocket.bind(GameConstants::serverPort);
	} catch (runtime_error e) {
		LOG_NETWORK(e.what());
		throw e;
	}
}

ServerInterface::~ServerInterface(){
	quitGame();
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		delete slots[i];
	}
}

void ServerInterface::addSlot(int playerIndex) {
	assert(playerIndex >= 0 && playerIndex < GameConstants::maxPlayers);
	LOG_NETWORK( "Opening slot " + intToStr(playerIndex) );
	delete slots[playerIndex];
	slots[playerIndex] = new ConnectionSlot(this, playerIndex, false);
	updateListen();
}

void ServerInterface::removeSlot(int playerIndex) {
	LOG_NETWORK( "Closing slot " + intToStr(playerIndex) );
	delete slots[playerIndex];
	slots[playerIndex] = NULL;
	updateListen();
}

ConnectionSlot* ServerInterface::getSlot(int playerIndex) {
	return slots[playerIndex];
}

int ServerInterface::getConnectedSlotCount() {
	int connectedSlotCount = 0;
	
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		if(slots[i] && slots[i]->isConnected()) {
			++connectedSlotCount;
		}
	}
	return connectedSlotCount;
}

void ServerInterface::update() {
	//update all slots
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (slots[i]) {
			try {
				slots[i]->update();
			} catch (SocketException e) {
				const string &name = slots[i]->getName();
				removeSlot(i);
				doSendTextMessage("Player " + intToStr(i) + " [" + name + "] diconnected.", -1);
			}
		}
	}
}

void ServerInterface::createSkillCycleTable(const TechTree *techTree) {
	skillCycleTable.create(techTree);
	this->broadcastMessage(&skillCycleTable);
}

void ServerInterface::unitBorn(Unit *unit, int32 cs) {
	//LOG_NETWORK( "Unit born, adding checksum " + intToHex(cs) );
	keyFrame.addChecksum(cs);
}

void ServerInterface::updateUnitCommand(Unit*, int32 cs) {
	keyFrame.addChecksum(cs);
}

void ServerInterface::updateProjectile(Unit*, int, int32 cs) {
	keyFrame.addChecksum(cs);	
}

void ServerInterface::updateAnim(Unit*, int32 cs) {
	keyFrame.addChecksum(cs);
}

void ServerInterface::updateMove(Unit *unit) {
	unit->updateMoveSkillCycle();
	MoveSkillUpdate updt(unit);
	keyFrame.addUpdate(updt);
}

void ServerInterface::updateProjectilePath(Unit *u, Projectile pps, const Vec3f &start, const Vec3f &end) {
	pps->setPath(start, end);
	ProjectileUpdate updt(u, pps);
	keyFrame.addUpdate(updt);
}

void ServerInterface::process(NetworkMessageText &msg, int requestor) {
	broadcastMessage(&msg, requestor);
	GameInterface::processTextMessage(msg);
}

void ServerInterface::updateKeyframe(int frameCount){
	//build command list, remove commands from requested and add to pending
	while(!requestedCommands.empty()){
		keyFrame.add(requestedCommands.back());
		pendingCommands.push_back(requestedCommands.back());
		requestedCommands.pop_back();
	}
	keyFrame.setFrameCount(frameCount);
	//cout << "sending keyframe, frameCount == " << keyFrame.getFrameCount() << endl;
	broadcastMessage(&keyFrame);
	//cout << "keyframe sent." << endl;
	keyFrame.reset();
}

void ServerInterface::waitUntilReady(Checksum &checksum) {
	Chrono chrono;
	chrono.start();
	bool allReady = false;
	LOG_NETWORK( "Waiting for ready messages from all clients" );
	//wait until we get a ready message from all clients
	while(!allReady) {
		allReady = true;
		for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			ConnectionSlot* slot = slots[i];
			if (slot && !slot->isReady()) {
				NetworkMessageType msgType = slot->getNextMessageType();
				NetworkMessageReady readyMsg;
				if (msgType == NetworkMessageType::READY && slot->receiveMessage(&readyMsg)) {
					LOG_NETWORK( "Received ready message, slot " + intToStr(i) );
					slot->setReady();
				} else if (msgType != NetworkMessageType::NO_MSG) {
					throw runtime_error("Unexpected network message: " + intToStr(msgType));
				} else {
					allReady = false;
				}
			}
		}

		//check for timeout
		if (chrono.getMillis() > readyWaitTimeout) {
			throw runtime_error("Timeout waiting for clients");
		}
		sleep(2);
	}

	LOG_NETWORK( "Received all ready messages, sending ready message(s)." );
	
	//send ready message after, so clients start delayed
	for (int i= 0; i < GameConstants::maxPlayers; ++i) {
		NetworkMessageReady readyMsg(checksum.getSum());
		if (slots[i]) {
			slots[i]->send(&readyMsg);
		}
	}
}

void ServerInterface::sendTextMessage(const string &text, int teamIndex){
	NetworkMessageText networkMessageText(text, Config::getInstance().getNetPlayerName(), teamIndex);
	broadcastMessage(&networkMessageText);
}

void ServerInterface::quitGame() {
	LOG_NETWORK( "aborting game" );
	string text = getHostName() + " has ended the game!";
	NetworkMessageText networkMessageText(text,getHostName(),-1);
	broadcastMessage(&networkMessageText, -1);

	NetworkMessageQuit networkMessageQuit;
	broadcastMessage(&networkMessageQuit);
}

string ServerInterface::getStatus() const{
	string str;
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		str += intToStr(i) + ": ";
		if (slots[i] && slots[i]->isConnected()) {
				str += slots[i]->getName(); //str += connectionSlot->getRemotePlayerName() + ": " + connectionSlot->getStatus();
		} else {
			str += theLang.get("NotConnected");
		}
		str += '\n';
	}
	return str;
}

void ServerInterface::syncAiSeeds(int aiCount, int *seeds) {
	assert(aiCount && seeds);
	Random r;
	r.init(Chrono::getCurMillis());
	for (int i=0; i < aiCount; ++i) {
		seeds[i] = r.rand();
	}
	LOG_NETWORK("sending " + intToStr(aiCount) + " Ai random number seeds...");
	NetworkMessageAiSeedSync seedSyncMsg(aiCount, seeds);
	broadcastMessage(&seedSyncMsg);
}

void ServerInterface::launchGame(const GameSettings* gameSettings){
	NetworkMessageLaunch networkMessageLaunch(gameSettings);
	LOG_NETWORK( "Launching game, sending launch message(s)" );
	broadcastMessage(&networkMessageLaunch);	
}

// NETWORK: I'm thinking the file stuff should go somewhere else.
// I agree, /dev/null will do for now ;-) We can resuurect it later.

void ServerInterface::broadcastMessage(const NetworkMessage* networkMessage, int excludeSlot) {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		if(i != excludeSlot && slots[i]) {
			if(slots[i]->isConnected()) {
				slots[i]->send(networkMessage);
			} else {
				Lang &lang = Lang::getInstance();
				string errmsg = slots[i]->getDescription() + " (" + lang.get("Player") + " "
						+ intToStr(slots[i]->getPlayerIndex() + 1) + ") " + lang.get("Disconnected");
				removeSlot(i);
				LOG_NETWORK(errmsg);
				Game::getInstance()->getConsole()->addLine(errmsg);
				//throw SocketException(errmsg);
			}
		}
	}
}

void ServerInterface::updateListen() {
	int openSlotCount = 0;
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		if(slots[i] && !slots[i]->isConnected()) {
			++openSlotCount;
		}
	}
	serverSocket.listen(openSlotCount);
}

#if _RECORD_GAME_STATE_
	void ServerInterface::dumpFrame(int frame) {
		stateLog.logFrame(frame);
	}
#endif

}}//end namespace
