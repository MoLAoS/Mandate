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

#include "type_factories.h"

using namespace Shared::Platform;
using namespace Shared::Util;

using Glest::Sim::SimulationInterface;

namespace Glest { namespace Net {

// =====================================================
//	class ServerInterface
// =====================================================

ServerInterface::ServerInterface(Program &prog) 
		: NetworkInterface(prog)
		, m_waitingForPlayers(false)
		, m_dataSync(0)
		, m_syncCounter(0)
		, m_dataSyncDone(false){
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		slots[i] = NULL;
	}
	try {
		serverSocket.setBlock(false);
		serverSocket.bind(GameConstants::serverPort);
	} catch (runtime_error &e) {
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
	NETWORK_LOG( __FUNCTION__ << " Opening slot " << playerIndex );
	delete slots[playerIndex];
	slots[playerIndex] = new ConnectionSlot(this, playerIndex);
	updateListen();
}

void ServerInterface::removeSlot(int playerIndex) {
	NETWORK_LOG( __FUNCTION__ << " Closing slot " << playerIndex );
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
		Console *c = g_userInterface.getDialogConsole();
		c->addDialog(getChatSender() + ": ", factionColours[getChatColourIndex()],
			getChatText(), true);
		popChatMsg();
	}

	// update all slots
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (slots[i]) {
			try {
				slots[i]->update();
			} catch (DataSyncError &e) {
				LOG_NETWORK( e.what() );
				throw runtime_error("DataSync Fail : " + slots[i]->getName()
					+ "\n" + e.what());
			} catch (GameSyncError &e) {
				LOG_NETWORK( e.what() );
				removeSlot(i);
				throw runtime_error(e.what());
			} catch (NetworkError &e) {
				LOG_NETWORK( e.what() );
				string playerName = slots[i]->getName();
				removeSlot(i);
				sendTextMessage("Player " + intToStr(i) + " [" + playerName + "] diconnected.", -1);
			}
		}
	}
}

void ServerInterface::doDataSync() {
	NETWORK_LOG( __FUNCTION__ );
	m_dataSync = new DataSyncMessage(g_world);
	m_syncCounter = getConnectedSlotCount();

	while (!m_dataSyncDone) {
		// update all slots
		for (int i=0; i < GameConstants::maxPlayers; ++i) {
			if (slots[i]) {
				try {
					slots[i]->update();
				} catch (NetworkError &e) {
					LOG_NETWORK( e.what() );
					string playerName = slots[i]->getName();
					removeSlot(i);
					sendTextMessage("Player " + intToStr(i) + " [" + playerName + "] diconnected.", -1);
				}
			}

		}
		sleep(10);
	}
	//broadcastMessage(m_dataSync);
}

void ServerInterface::dataSync(int playerNdx, DataSyncMessage &msg) {
	NETWORK_LOG( __FUNCTION__ );
	assert(m_dataSync);
	--m_syncCounter;
	if (!m_syncCounter) {
		m_dataSyncDone = true;
	}
	bool ok = true;
	if (m_dataSync->getChecksumCount() != msg.getChecksumCount()) {
		NETWORK_LOG( "DataSync Fail: Client has sent " << msg.getChecksumCount() 
			<< " total checksums, I have " << m_dataSync->getChecksumCount() )
		ok = false;
	}
	if (m_dataSync->getCmdTypeCount() != msg.getCmdTypeCount()) {
		NETWORK_LOG( "DataSync Fail: Client has sent " << msg.getCmdTypeCount() 
			<< " CommandType checksums, I have " << m_dataSync->getCmdTypeCount() )
		ok = false;
	}
	if (m_dataSync->getSkillTypeCount() != msg.getSkillTypeCount()) {
		NETWORK_LOG( "DataSync Fail: Client has sent " << msg.getSkillTypeCount() 
			<< " SkillType checksums, I have " << m_dataSync->getSkillTypeCount() )
		ok = false;
	}
	if (m_dataSync->getProdTypeCount() != msg.getProdTypeCount()) {
		NETWORK_LOG( "DataSync Fail: Client has sent " << msg.getProdTypeCount() 
			<< " ProducibleType checksums, I have " << m_dataSync->getProdTypeCount() )
		ok = false;
	}
	if (m_dataSync->getCloakTypeCount() != msg.getCloakTypeCount()) {
		NETWORK_LOG( "DataSync Fail: Client has sent " << msg.getProdTypeCount() 
			<< " CloakType checksums, I have " << m_dataSync->getProdTypeCount() )
		ok = false;
	}

	if (!ok) {
		throw DataSyncError(NetSource::SERVER);
	}

	int cmdOffset = 4;
	int skllOffset = cmdOffset + m_commandTypeFactory.getTypeCount();
	int prodOffset = skllOffset + m_skillTypeFactory.getTypeCount();
	int cloakOffset = prodOffset + m_prodTypeFactory.getTypeCount();

	const int n = m_dataSync->getChecksumCount();
	for (int i=0; i < n; ++i) {
		if (m_dataSync->getChecksum(i) != msg.getChecksum(i)) {
			ok = false;
			if (i < cmdOffset) {
				string badBit;
				switch (i) {
					case 0: badBit = "Tileset"; break;
					case 1: badBit = "Map"; break;
					case 2: badBit = "Damage multiplier"; break;
					case 3: badBit = "Resource"; break;
				}
				NETWORK_LOG( "DataSync Fail: " << badBit << " data does not match." )
			} else if (i < skllOffset) {
				CommandType *ct = m_commandTypeFactory.getType(i - cmdOffset);
				NETWORK_LOG(
					"DataSync Fail: CommandType '" << ct->getName() << "' of UnitType '"
					<< ct->getUnitType()->getName() << "' of FactionType '"
					<< ct->getUnitType()->getFactionType()->getName() << "'";
				)
			} else if (i < prodOffset) {
				SkillType *skillType = m_skillTypeFactory.getType(i - skllOffset);
				NETWORK_LOG(
					"DataSync Fail: SkillType '" << skillType->getName() << "' of UnitType '"
					<< skillType->getUnitType()->getName() << "' of FactionType '"
					<< skillType->getUnitType()->getFactionType()->getName() << "'";
				)
			} else if (i < cloakOffset) {
				ProducibleType *pt = m_prodTypeFactory.getType(i - prodOffset);
				if (isUnitType(pt)) {
					UnitType *ut = static_cast<UnitType*>(pt);
					NETWORK_LOG( "DataSync Fail: UnitType " << i << ": " << ut->getName() 
						<< " of FactionType: " << ut->getFactionType()->getName() );
				} else if (isUpgradeType(pt)) {
					UpgradeType *ut = static_cast<UpgradeType*>(pt);
					NETWORK_LOG( "DataSync Fail: UpgradeType " << i << ": " << ut->getName() 
						<< " of FactionType: " << ut->getFactionType()->getName() );
				} else if (isGeneratedType(pt)) {
					GeneratedType *gt = static_cast<GeneratedType*>(pt);
					NETWORK_LOG( "DataSync Fail: GeneratedType " << i << ": " << gt->getName() << " of CommandType: " 
						<< gt->getCommandType()->getName() << " of UnitType: " 
						<< gt->getCommandType()->getUnitType()->getName() );
				} else {
					throw runtime_error(string("Unknown producible class for type: ") + pt->getName());
				}
			} else {
				CloakType *ct = m_cloakTypeFactory.getType(i - cloakOffset);
				NETWORK_LOG(
					"DataSync Fail: CloakType '" << ct->getName() << "' of UnitType '"
					<< ct->getUnitType()->getName() << "' of FactionType '"
					<< ct->getUnitType()->getFactionType()->getName() << "'";
				)
			}
		}
	}
	if (!ok) {
		throw DataSyncError(NetSource::SERVER);
	}
}

void ServerInterface::createSkillCycleTable(const TechTree *techTree) {
	NETWORK_LOG( __FUNCTION__ << " Creating and sending SkillCycleTable." );
	SimulationInterface::createSkillCycleTable(techTree);
	broadcastMessage(skillCycleTable);
}

#if MAD_SYNC_CHECKING

void ServerInterface::checkUnitBorn(Unit *unit, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ << " Adding checksum " << intToHex(cs) );
	keyFrame.addChecksum(cs);
}

void ServerInterface::checkCommandUpdate(Unit*, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ << " Adding checksum " << intToHex(cs) );
	keyFrame.addChecksum(cs);
}

void ServerInterface::checkProjectileUpdate(Unit*, int, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ << " Adding checksum " << intToHex(cs) );
	keyFrame.addChecksum(cs);
}

void ServerInterface::checkAnimUpdate(Unit*, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ << " Adding checksum " << intToHex(cs) );
	keyFrame.addChecksum(cs);
}

#endif

void ServerInterface::updateSkillCycle(Unit *unit) {
	SimulationInterface::updateSkillCycle(unit);
	if (unit->isMoving()) {
//		NETWORK_LOG( __FUNCTION__ << " Adding move update" );
		MoveSkillUpdate updt(unit);
		keyFrame.addUpdate(updt);
	}
}

void ServerInterface::updateProjectilePath(Unit *u, Projectile *pps, const Vec3f &start, const Vec3f &end) {
//	NETWORK_LOG( __FUNCTION__ << " Adding projectile update" );
	SimulationInterface::updateProjectilePath(u, pps, start, end);
	ProjectileUpdate updt(u, pps);
	keyFrame.addUpdate(updt);
}

void ServerInterface::process(TextMessage &msg, int requestor) {
	broadcastMessage(&msg, requestor);
	NetworkInterface::processTextMessage(msg);
}

void ServerInterface::updateKeyframe(int frameCount) {
	NETWORK_LOG( __FUNCTION__ << " building & sending keyframe " 
		<< (frameCount / GameConstants::networkFramePeriod) << " @ frame " << frameCount);
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

void ServerInterface::waitUntilReady() {
	NETWORK_LOG( __FUNCTION__ );
	Chrono chrono;
	chrono.start();
	bool allReady = false;

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
				NETWORK_LOG( __FUNCTION__ << " Received ready message, slot " << i );
				slot->setReady();
			}
		}
		// check for timeout
		if (chrono.getMillis() > readyWaitTimeout) {
			NETWORK_LOG( __FUNCTION__ << "Timed out." );
			for (int i = 0; i < GameConstants::maxPlayers; ++i) {
				if (slots[i] && !slots[i]->isReady()) {
					Socket *sock = slots[i]->getSocket();
					int n = sock->getDataToRead();
					NETWORK_LOG( "\tSlot[" << i << "] : data to read: " << n );
					if (n >= 4) {
						MsgHeader hdr;
						sock->receive(&hdr, 4);
						NETWORK_LOG( "\tMessage type: " << MessageTypeNames[MessageType(hdr.messageType)]
							<< ", message size: " << hdr.messageSize );
					}
				}
			}
			throw TimeOut(NetSource::CLIENT);
		}
		sleep(2);
	}
	NETWORK_LOG( __FUNCTION__ << " Received all ready messages, sending ready message(s)." );
	ReadyMessage readyMsg;
	for (int i= 0; i < GameConstants::maxPlayers; ++i) {
		if (slots[i]) {
			slots[i]->send(&readyMsg);
		}
	}
}

void ServerInterface::sendTextMessage(const string &text, int teamIndex){
	NETWORK_LOG( __FUNCTION__ );
	int ci = g_world.getThisFaction()->getColourIndex();
	TextMessage txtMsg(text, Config::getInstance().getNetPlayerName(), teamIndex, ci);
	broadcastMessage(&txtMsg);
	NetworkInterface::processTextMessage(txtMsg);
}

void ServerInterface::quitGame(QuitSource source) {
	NETWORK_LOG( __FUNCTION__ << " aborting game." );
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
	NETWORK_LOG( __FUNCTION__ << " sending " << aiCount << " Ai random number seeds..." );
	assert(aiCount && seeds);
	SimulationInterface::syncAiSeeds(aiCount, seeds);
	AiSeedSyncMessage seedSyncMsg(aiCount, seeds);
	broadcastMessage(&seedSyncMsg);
}

void ServerInterface::doLaunchBroadcast() {
	NETWORK_LOG( __FUNCTION__ << " Launching game." );
	LaunchMessage networkMessageLaunch(&gameSettings);
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
					g_userInterface.getRegularConsole()->addLine(errmsg);
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

IF_MAD_SYNC_CHECKS(
	void ServerInterface::dumpFrame(int frame) {
		if (frame > 5) {
			stringstream ss;
			for (int i = 5; i >= 0; --i) {
				worldLog->logFrame(ss, frame - i);
			}
			NETWORK_LOG( ss.str() );
		}
	}
)

}}//end namespace
