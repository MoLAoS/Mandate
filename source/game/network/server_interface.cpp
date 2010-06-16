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
#include "sim_interface.h"

#include "leak_dumper.h"
#include "logger.h"
#include "profiler.h"

using namespace Shared::Platform;
using namespace Shared::Util;

using Glest::Sim::SimulationInterface;

namespace Glest { namespace Net {

// =====================================================
//	class ServerInterface
// =====================================================

ServerInterface::ServerInterface(Program &prog) 
		: NetworkInterface(prog) {
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

ServerInterface::~ServerInterface() {
	quitGame(QuitSource::LOCAL);
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		delete slots[i];
	}
}

void ServerInterface::addSlot(int playerIndex) {
	assert(playerIndex >= 0 && playerIndex < GameConstants::maxPlayers);
	LOG_NETWORK( "Opening slot " + intToStr(playerIndex) );
	delete slots[playerIndex];
	slots[playerIndex] = new ConnectionSlot(this, playerIndex);
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
	
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		if (slots[i] && slots[i]->isConnected()) {
			++connectedSlotCount;
		}
	}
	return connectedSlotCount;
}

void ServerInterface::update() {
	// chat messages
	while (hasChatMsg()) {
		getGameState()->getConsole()->addLine(getChatSender() + ": " + getChatText(), true);
		popChatMsg();
	}

	//update all slots
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (slots[i]) {
			try {
				slots[i]->update();
			} catch (NetworkError &e) {
				LOG_NETWORK( e.what() );
				const string &name = slots[i]->getName();
				removeSlot(i);
				sendTextMessage("Player " + intToStr(i) + " [" + name + "] diconnected.", -1);
			}
		}
	}
}

void ServerInterface::createSkillCycleTable(const TechTree *techTree) {
	LOG_NETWORK( "Creating and sending SkillCycleTable." );
	SimulationInterface::createSkillCycleTable(techTree);
	this->broadcastMessage(&skillCycleTable);
}

void ServerInterface::checkUnitBorn(Unit *unit, int32 cs) {
	//LOG_NETWORK( __FUNCTION__", adding checksum " + intToHex(cs) );
	keyFrame.addChecksum(cs);
}

void ServerInterface::checkCommandUpdate(Unit*, int32 cs) {
	keyFrame.addChecksum(cs);
}

void ServerInterface::checkProjectileUpdate(Unit*, int, int32 cs) {
	keyFrame.addChecksum(cs);
}

void ServerInterface::checkAnimUpdate(Unit*, int32 cs) {
	keyFrame.addChecksum(cs);
}

void ServerInterface::updateSkillCycle(Unit *unit) {
	SimulationInterface::updateSkillCycle(unit);
	if (unit->getCurrSkill()->getClass() == SkillClass::MOVE) {
		MoveSkillUpdate updt(unit);
		keyFrame.addUpdate(updt);
	}
}

void ServerInterface::updateProjectilePath(Unit *u, Projectile *pps, const Vec3f &start, const Vec3f &end) {
	SimulationInterface::updateProjectilePath(u, pps, start, end);
	ProjectileUpdate updt(u, pps);
	keyFrame.addUpdate(updt);
}

void ServerInterface::process(TextMessage &msg, int requestor) {
	broadcastMessage(&msg, requestor);
	NetworkInterface::processTextMessage(msg);
}

void ServerInterface::updateKeyframe(int frameCount) {
	// build command list, remove commands from requested and add to pending
	while (!requestedCommands.empty()) {
		keyFrame.add(requestedCommands.back());
		pendingCommands.push_back(requestedCommands.back());
		requestedCommands.pop_back();
	}
	keyFrame.setFrameCount(frameCount);
	broadcastMessage(&keyFrame);
	keyFrame.reset();
}

void ServerInterface::waitUntilReady(Checksum *checksums) {
	Chrono chrono;
	chrono.start();
	bool allReady = false;
	const TechTree *tt = world->getTechTree();
	const int &n = tt->getFactionTypeCount();
	string factionChecks;
	for (int i=0; i < n; ++i) {
		factionChecks += "Faction " + tt->getFactionType(i)->getName() + " = " 
			+ intToHex(checksums[4+i].getSum()) + "\n";
	}
	NETWORK_LOG( 
		"Waiting for ready messages from all clients. My Chcecksums:\n"
		<< "Tileset = " << intToHex(checksums[0].getSum()) << endl
		<< "Map = " << intToHex(checksums[1].getSum()) << endl
		<< "Damage Multipliers = " << intToHex(checksums[2].getSum()) << endl
		<< "Resources = " << intToHex(checksums[3].getSum()) << endl
		<< factionChecks;
	);	
	// wait until we get a ready message from all clients
	while (!allReady) {
		allReady = true;
		for (int i = 0; i < GameConstants::maxPlayers; ++i) {
			ConnectionSlot* slot = slots[i];
			if (slot && !slot->isReady()) {
				slot->receiveMessages();
				if (!slot->hasMessage()) {
					allReady = false;
					continue;
				}
				RawMessage raw = slot->getNextMessage();
				if (raw.type != MessageType::READY) {
					throw InvalidMessage(MessageType::READY, raw.type);
				}
				LOG_NETWORK( "Received ready message, slot " + intToStr(i) );
				ReadyMessage readyMsg(raw);
				slot->setReady();
			}
		}
		// check for timeout
		if (chrono.getMillis() > readyWaitTimeout) {
			LOG_NETWORK( "Timed Out." );
			for (int i = 0; i < GameConstants::maxPlayers; ++i) {
				if (slots[i]) {
					Socket *sock = slots[i]->getSocket();
					int n = sock->getDataToRead();
					NETWORK_LOG( "Slot[" << i << "] : data to read: " << n );
					if (n >= 4) {
						MsgHeader hdr;
						sock->receive(&hdr, 4);
						NETWORK_LOG( "Message type: " << MessageTypeNames[MessageType(hdr.messageType)]
							<< ", message size: " << hdr.messageSize );
					}
				}
			}
			throw TimeOut(NetSource::CLIENT);
		}
		sleep(2);
	}
	LOG_NETWORK( "Received all ready messages, sending ready message(s)." );
	// send ready message after, so clients start delayed
	ReadyMessage readyMsg(checksums);
	for (int i= 0; i < GameConstants::maxPlayers; ++i) {
		if (slots[i]) {
			slots[i]->send(&readyMsg);
		}
	}
}

void ServerInterface::sendTextMessage(const string &text, int teamIndex){
	TextMessage txtMsg(text, Config::getInstance().getNetPlayerName(), teamIndex);
	broadcastMessage(&txtMsg);
	NetworkInterface::processTextMessage(txtMsg);
}

void ServerInterface::quitGame(QuitSource source) {
	LOG_NETWORK( "aborting game" );
	string text = getHostName() + " has ended the game!";
	TextMessage networkMessageText(text,getHostName(),-1);
	broadcastMessage(&networkMessageText, -1);

	QuitMessage networkMessageQuit;
	broadcastMessage(&networkMessageQuit);
	if (game && !program.isTerminating()) {
		game->quitGame();
	}
}

string ServerInterface::getStatus() const {
	string str;
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		str += intToStr(i) + ": ";
		if (slots[i] && slots[i]->isConnected()) {
				str += slots[i]->getName(); //str += connectionSlot->getRemotePlayerName() + ": " + connectionSlot->getStatus();
		} else {
			str += g_lang.get("NotConnected");
		}
		str += '\n';
	}
	return str;
}

void ServerInterface::syncAiSeeds(int aiCount, int *seeds) {
	assert(aiCount && seeds);
	SimulationInterface::syncAiSeeds(aiCount, seeds);
	LOG_NETWORK("sending " + intToStr(aiCount) + " Ai random number seeds...");
	AiSeedSyncMessage seedSyncMsg(aiCount, seeds);
	broadcastMessage(&seedSyncMsg);
}

void ServerInterface::doLaunchBroadcast() {
	LaunchMessage networkMessageLaunch(&gameSettings);
	LOG_NETWORK( "Launching game, sending launch message(s)" );
	broadcastMessage(&networkMessageLaunch);	
}

void ServerInterface::broadcastMessage(const Message* networkMessage, int excludeSlot) {
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		if (i != excludeSlot && slots[i]) {
			if (slots[i]->isConnected()) {
				slots[i]->send(networkMessage);
			} else {
				Lang &lang = Lang::getInstance();
				string errmsg = slots[i]->getDescription() + " (" + lang.get("Player") + " "
					+ intToStr(slots[i]->getPlayerIndex() + 1) + ") " + lang.get("Disconnected");
				removeSlot(i);
				LOG_NETWORK(errmsg);
				if (World::isConstructed()) {
					g_gameState.getConsole()->addLine(errmsg);
				}
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

#if _GAE_DEBUG_EDITION_
	void ServerInterface::dumpFrame(int frame) {
		worldLog.logFrame(frame);
		throw GameSyncError();
	}
#endif

}}//end namespace
