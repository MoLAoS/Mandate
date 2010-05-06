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
#include "client_interface.h"

#include <stdexcept>
#include <cassert>

#include "platform_util.h"
#include "game_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "game.h"
#include "world.h"

#include "random.h"
#include "sim_interface.h"

#include "leak_dumper.h"
#include "logger.h"
#include "profiler.h"
#include "timer.h"

using namespace Glest::Sim;

using namespace Shared::Platform;
using namespace Shared::Util;
using Shared::Platform::Chrono;

namespace Glest { namespace Net {

// =====================================================
//	class ClientInterface
// =====================================================

const int ClientInterface::messageWaitTimeout = 10000;	//10 seconds
const int ClientInterface::waitSleepTime = 5; // changed from 50, no obvious effect

ClientInterface::ClientInterface(Program &prog)
		: NetworkInterface(prog) {
	_TRACE_FUNCTION();
	clientSocket = NULL;
	launchGame = false;
	introDone = false;
	playerIndex = -1;
}

ClientInterface::~ClientInterface() {
	_TRACE_FUNCTION();
	quitGame(QuitSource::LOCAL);
	delete clientSocket;
	clientSocket = NULL;
}

void ClientInterface::connect(const Ip &ip, int port) {
	_TRACE_FUNCTION();
	delete clientSocket;
	clientSocket = new ClientSocket();
	clientSocket->connect(ip, port);
	clientSocket->setBlock(false);
	LOG_NETWORK( "connecting to " + ip.getString() + ":" + intToStr(port) );
}

void ClientInterface::reset() {
	_TRACE_FUNCTION();
	delete clientSocket;
	clientSocket = NULL;
}

void ClientInterface::createSkillCycleTable(const TechTree *) {
	_TRACE_FUNCTION();
	Chrono chrono;
	chrono.start();
	LOG_NETWORK( "waiting for server to send Skill Cycle Table." );
	while (true) {
		MessageType msgType = getNextMessageType();
		if (msgType == MessageType::SKILL_CYCLE_TABLE) {
			if (receiveMessage(&skillCycleTable)) {
				LOG_NETWORK( "Received Skill Cycle Table." );
				break;
			}
		} else if (msgType == MessageType::NO_MSG) {
			if (chrono.getMillis() > readyWaitTimeout) {
				throw TimeOut(NetSource::SERVER);
			}
		} else {
			throw InvalidMessage(MessageType::SKILL_CYCLE_TABLE, msgType);
		}
		sleep(2);
	}
}


void ClientInterface::updateLobby() {
	// NETWORK: this method is very different
	MessageType msgType = getNextMessageType();

	if (msgType == MessageType::INTRO) {
		IntroMessage introMsg;
		if (receiveMessage(&introMsg)) {
			//check consistency
			if(theConfig.getNetConsistencyChecks() && introMsg.getVersionString() != getNetworkVersionString()) {
				throw VersionMismatch(NetSource::CLIENT, getNetworkVersionString(), introMsg.getVersionString());
			}
			LOG_NETWORK( "Received intro message, sending intro message reply." );
			playerIndex = introMsg.getPlayerIndex();
			serverName = introMsg.getHostName();

			setRemoteNames(introMsg.getHostName(), introMsg.getPlayerName());

			if (playerIndex < 0 || playerIndex >= GameConstants::maxPlayers) {
				throw GarbledMessage(MessageType::INTRO, NetSource::SERVER);
			}

			//send reply
			IntroMessage replyMsg(
				getNetworkVersionString(), theConfig.getNetPlayerName(), getHostName(), -1);

			send(&replyMsg);
			introDone= true;
		}
	} else if (msgType == MessageType::LAUNCH) {
		LaunchMessage launchMsg;
		if (receiveMessage(&launchMsg)) {
			GameSettings &gameSettings = theSimInterface->getGameSettings();
			gameSettings.clear();
			launchMsg.buildGameSettings(&gameSettings);
			//replace server player by network
			for (int i= 0; i < gameSettings.getFactionCount(); ++i) {
				//replace by network
				if (gameSettings.getFactionControl(i) == ControlType::HUMAN) {
					gameSettings.setFactionControl(i, ControlType::NETWORK);
				}
				//set the faction index
				if (gameSettings.getStartLocationIndex(i)==playerIndex) {
					gameSettings.setThisFactionIndex(i);
					gameSettings.setFactionControl(i, ControlType::HUMAN);
				}
			}
			launchGame= true;
			LOG_NETWORK( "Received launch message." );
		}
	} else if (msgType != MessageType::NO_MSG) {
		LOG_NETWORK( "Received bad message type : " + intToStr(msgType) + ". Resetting connection." );
		reset();
	}
}

void ClientInterface::syncAiSeeds(int aiCount, int *seeds) {
	_TRACE_FUNCTION();
	assert(aiCount && seeds);
	AiSeedSyncMessage seedSyncMsg;
	Chrono chrono;
	chrono.start();
	LOG_NETWORK( "Ready, waiting for server to send Ai random number seeds." );
	//wait until we get an ai seed sync message from the server
	while (true) {
		MessageType msgType = getNextMessageType();
		switch (msgType) {
			case MessageType::AI_SYNC:
				if (receiveMessage(&seedSyncMsg)) {
					LOG_NETWORK( "Received AI random number seed message." );
					assert(seedSyncMsg.getSeedCount());
					for (int i=0; i < seedSyncMsg.getSeedCount(); ++i) {
						seeds[i] = seedSyncMsg.getSeed(i);
					}
					return;
				}
				break;
			case MessageType::NO_MSG:
				if (chrono.getMillis() > readyWaitTimeout) {
					throw TimeOut(NetSource::SERVER);
				}
			default:
				throw InvalidMessage(MessageType::AI_SYNC, msgType);
		}
		sleep(2);
	}
}

void ClientInterface::waitUntilReady(Checksum &checksum) {
	_TRACE_FUNCTION();
	// NETWORK: this method is very different
	ReadyMessage readyMsg;
	Chrono chrono;
	chrono.start();

	//send ready message
	send(&readyMsg);
	LOG_NETWORK( "Ready, waiting for server ready message." );

	bool ready = false;
	//wait until we get a ready message from the server
	while (!ready) {
		MessageType msgType = getNextMessageType();
		switch (msgType) {
			case MessageType::READY:
				if (receiveMessage(&readyMsg)) {
					LOG_NETWORK( "Received ready message." );
					ready = true;
				}
				break;
			case MessageType::NO_MSG:
				if (chrono.getMillis() > readyWaitTimeout) {
					throw TimeOut(NetSource::SERVER);
				}
				break;
			default:
				throw runtime_error("Unexpected network message: " + intToStr(msgType) );
		}
		sleep(waitSleepTime);
	}
	//check checksum
	if (theConfig.getNetConsistencyChecks() && readyMsg.getChecksum() != checksum.getSum()) {
		///@todo send a message to the server explaining what happened ...
		throw DataSyncError(NetSource::CLIENT);
	}
	//delay the start a bit, so clients have nore room to get messages
	sleep(GameConstants::networkExtraLatency);
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex) {
	TextMessage textMsg(text, theConfig.getNetPlayerName(), teamIndex);
	send(&textMsg);
}

string ClientInterface::getStatus() const {
	return theLang.get("Server") + ": " + serverName;
	//	return getRemotePlayerName() + ": " + NetworkStatus::getStatus();
}

void ClientInterface::waitForMessage() {
	Chrono chrono;
	chrono.start();
	MessageType nextType;
	while ((nextType = getNextMessageType()) == MessageType::NO_MSG) {
		if (!isConnected()) {
			throw Disconnect();
		}
		if (chrono.getMillis() > messageWaitTimeout) {
			throw TimeOut(NetSource::SERVER);
		}
		sleep(waitSleepTime);
	}
}

void ClientInterface::quitGame(QuitSource source) {
	LOG_NETWORK( "Quitting" );
	if (clientSocket && clientSocket->isConnected() && source != QuitSource::SERVER) {
		QuitMessage networkMessageQuit;
		send(&networkMessageQuit);
		LOG_NETWORK( "Sent quit message." );
	}
	delete clientSocket;
	clientSocket = NULL;

	if (game) {
		if (source == QuitSource::SERVER) {
			throw Net::Disconnect();
		}
		///@todo ... if the server terminated the game, keep it running and show a message box...
		//if (source == QuitSource::SERVER) {
		//	//do nice stuff here
		//} else {
			game->quitGame();
		//}
	}
}

void ClientInterface::startGame() {
	_TRACE_FUNCTION();
	updateKeyframe(0);
}

void ClientInterface::update() {
	CommandListMessage cmdList;

	//send as many commands as we can
	while (!requestedCommands.empty() && cmdList.addCommand(&requestedCommands.back())) {
		requestedCommands.pop_back();
	}
	if (cmdList.getCommandCount() > 0) {
		send(&cmdList);
	}
}

void ClientInterface::updateKeyframe(int frameCount) {
	_TRACE_FUNCTION();
	// give all commands from last KeyFrame
	for (size_t i=0; i < keyFrame.getCmdCount(); ++i) {
		pendingCommands.push_back(*keyFrame.getCmd(i));
	}
	keyFrame.reset();
	while (true) {
		waitForMessage();
		MessageType msgType = getNextMessageType();
		switch (msgType) {
			case MessageType::KEY_FRAME:
				// make sure we read the message
				{	Chrono chrono;
					chrono.start();
					while (!receiveMessage(&keyFrame)) {
						sleep(waitSleepTime);
						if (chrono.getMillis() > messageWaitTimeout) {
							throw TimeOut(NetSource::SERVER);
						}
					}
				}
				// check that we are in the right frame
				if (keyFrame.getFrameCount() != frameCount + GameConstants::networkFramePeriod) {
					throw GameSyncError();
				}
				return;
			case MessageType::QUIT: {
					QuitMessage quitMsg;
					receiveMessage(&quitMsg);
					quitGame(QuitSource::SERVER);
				}
				return;
			case MessageType::TEXT: {
					TextMessage textMsg;
					receiveMessage(&textMsg);
					NetworkInterface::processTextMessage(textMsg);
				}
				break;
			default:
				throw InvalidMessage(msgType);
		}
	}
}


void ClientInterface::updateSkillCycle(Unit *unit) {
	if (unit->getCurrSkill()->getClass() == SkillClass::MOVE) {
		updateMove(unit);
	} else {
		unit->updateSkillCycle(skillCycleTable.lookUp(unit).getSkillFrames());
	}
}

void ClientInterface::updateMove(Unit *unit) {
	MoveSkillUpdate updt = keyFrame.getMoveUpdate();
	if (updt.offsetX < -1 || updt.offsetX > 1 || updt.offsetY < - 1 || updt.offsetY > 1
	|| (!updt.offsetX && !updt.offsetY)) {
		LOG_NETWORK(
			"Bad server update, pos offset out of range, x="
			+ intToStr(updt.offsetX) + ", y=" + intToStr(updt.offsetY)
		);
		throw GameSyncError(); // msgBox and then graceful exit to Menu please...
	}
	unit->setNextPos(unit->getPos() + Vec2i(updt.offsetX, updt.offsetY));
	unit->updateSkillCycle(updt.end_offset);
}

void ClientInterface::updateProjectilePath(Unit *u, Projectile *pps, const Vec3f &start, const Vec3f &end) {
	ProjectileUpdate updt = keyFrame.getProjUpdate();
	pps->setPath(start, end, updt.end_offset);
}

void ClientInterface::checkUnitBorn(Unit *unit, int32 cs) {
	int32 server_cs = keyFrame.getNextChecksum();
	if (cs != server_cs) {
		stringstream ss;
		ss << "Sync Error: " << __FUNCTION__ << " , unit type: " << unit->getType()->getName()
			<< " unit id: " << unit->getId() << " faction: " << unit->getFactionIndex();
		LOG_NETWORK( ss.str() );
		LOG_NETWORK( "\tserver checksum " + Conversion::toHex(server_cs) + " my checksum " + Conversion::toHex(cs) );
		throw GameSyncError();
	}
}

void ClientInterface::checkCommandUpdate(Unit *unit, int32 cs) {
	if (cs != keyFrame.getNextChecksum()) {
		stringstream ss;
		ss << "Sync Error: " << __FUNCTION__ << " , unit type: " << unit->getType()->getName()
			<< ", skill class: " << SkillClassNames[unit->getCurrSkill()->getClass()];
		LOG_NETWORK( ss.str() );
		IF_DEBUG_EDITION(
			assert(theWorld.getFrameCount());
			worldLog.logFrame();		// dump frame log
			SyncErrorMsg se(theWorld.getFrameCount());
			send(&se); // ask server to also dump a frame log.
		)
		throw GameSyncError(); // nice message & graceful exit to Menu please...
	}
}

void ClientInterface::checkProjectileUpdate(Unit *unit, int endFrame, int32 cs) {
	if (cs != keyFrame.getNextChecksum()) {
		stringstream ss;
		ss << "Sync Error: " << __FUNCTION__ << ", unit id: " << unit->getId() << " skill: "
			<< unit->getCurrSkill()->getName();
		if (unit->getCurrCommand()->getUnit()) {
			ss << " target id: " << unit->getCurrCommand()->getUnit()->getId();
		} else {
			ss << " target pos: " << unit->getCurrCommand()->getPos();
		}
		ss << " end frame: " << endFrame;
		LOG_NETWORK( ss.str() );
		throw GameSyncError(); // graceful exit to Menu please...
	}
}

void ClientInterface::checkAnimUpdate(Unit *unit, int32 cs) {
	if (cs != keyFrame.getNextChecksum()) {
		const CycleInfo &inf = skillCycleTable.lookUp(unit);
		stringstream ss;
		ss << "Sync Error: " << __FUNCTION__ << " unit id: " << unit->getId()
			<< " attack offset: " << inf.getAttackOffset();
		LOG_NETWORK( ss.str() );
		throw GameSyncError(); // graceful exit to Menu please...
	}
}


}}//end namespace
