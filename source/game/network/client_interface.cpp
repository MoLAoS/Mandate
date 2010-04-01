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
#include "leak_dumper.h"
#include "logger.h"
#include "timer.h"

#include "network_util.h"

#include "game.h"
#include "world.h"

#include "random.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using Shared::Platform::Chrono;

namespace Glest { namespace Game {

// =====================================================
//	class ClientInterface
// =====================================================

const int ClientInterface::messageWaitTimeout = 10000;	//10 seconds
const int ClientInterface::waitSleepTime = 5; // changed from 50, no obvious effect

ClientInterface::ClientInterface() {
	clientSocket = NULL;
	launchGame = false;
	introDone = false;
	playerIndex = -1;
}

ClientInterface::~ClientInterface() {
	quitGame();
	delete clientSocket;
	clientSocket = NULL;
}

void ClientInterface::connect(const Ip &ip, int port) {
	delete clientSocket;
	clientSocket = new ClientSocket();
	clientSocket->connect(ip, port);
	clientSocket->setBlock(false);
	LOG_NETWORK( "connecting to " + ip.getString() + ":" + intToStr(port) );
}

void ClientInterface::reset() {
	quitGame();
	delete clientSocket;
	clientSocket = NULL;
}

void ClientInterface::update() {
	NetworkMessageCommandList cmdList;

	//send as many commands as we can
	while (!requestedCommands.empty() && cmdList.addCommand(&requestedCommands.back())) {
		requestedCommands.pop_back();
	}
	if (cmdList.getCommandCount() > 0) {
		send(&cmdList);
	}
}

void ClientInterface::createSkillCycleTable(const TechTree *) {
	Chrono chrono;
	chrono.start();
	LOG_NETWORK( "waiting for server to send Skill Cycle Table." );
	while (true) {
		NetworkMessageType msgType = getNextMessageType();
		if (msgType == NetworkMessageType::SKILL_CYCLE_TABLE) {
			if (receiveMessage(&skillCycleTable)) {
				LOG_NETWORK( "Received Skill Cycle Table." );
				break;
			}
		} else if (msgType == NetworkMessageType::NO_MSG) {
			if (chrono.getMillis() > readyWaitTimeout) {
				throw runtime_error("Timeout waiting for server");
			}
		} else {
			throw runtime_error("Unexpected network message: " + intToStr(msgType) );
		}
		sleep(2);
	}
}

void ClientInterface::unitBorn(Unit *unit, int32 cs) {
	int32 server_cs = keyFrame.getNextChecksum();
	if (cs != server_cs) {
		stringstream ss;
		ss << "Sync Error: unitBorn() , unit type: " << unit->getType()->getName()
			<< " unit id: " << unit->getId() << " faction: " << unit->getFactionIndex();
		LOG_NETWORK( ss.str() );
		LOG_NETWORK( "\tserver checksum " + Conversion::toHex(server_cs) + " my checksum " + Conversion::toHex(cs) );
		throw runtime_error("Sync error: ClientInterface::unitBorn() checksum mismatch.");
	}
}

void ClientInterface::updateUnitCommand(Unit *unit, int32 cs) {
	if (cs != keyFrame.getNextChecksum()) {
		stringstream ss;
		ss << "Sync Error: updateUnitCommand() , unit type: " << unit->getType()->getName()
			<< ", skill class: " << SkillClassNames[unit->getCurrSkill()->getClass()];
		LOG_NETWORK( ss.str() );
		throw runtime_error("Sync error: ClientInterface::updateUnitCommand() checksum mismatch.");
	}
}

void ClientInterface::updateProjectile(Unit *unit, int endFrame, int32 cs) {
	if (cs != keyFrame.getNextChecksum()) {
		stringstream ss;
		ss << "Sync Error: updateProjectile(), unit id: " << unit->getId() << " skill: "
			<< unit->getCurrSkill()->getName();
		if (unit->getCurrCommand()->getUnit()) {
			ss << " target id: " << unit->getCurrCommand()->getUnit()->getId();
		} else {
			ss << " target pos: " << unit->getCurrCommand()->getPos();
		}
		ss << " end frame: " << endFrame;
		LOG_NETWORK( ss.str() );
		throw runtime_error("Sync error: ClientInterface::updateProjectile() checksum mismatch.");
	}
}

void ClientInterface::updateAnim(Unit *unit, int32 cs) {
	if (cs != keyFrame.getNextChecksum()) {
		const CycleInfo &inf = skillCycleTable.lookUp(unit);
		stringstream ss;
		ss << "updateAnim() unit id: " << unit->getId() << " attack offset: " << inf.getAttackOffset();
		LOG_NETWORK( ss.str() );
		throw runtime_error("Sync error: ClientInterface::updateAnim() checksum mismatch.");
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
		// no throw, we want to let it go so it throws on the checksum mismatch and game state is dumped
	}
	unit->setNextPos(unit->getPos() + Vec2i(updt.offsetX, updt.offsetY));
	unit->updateSkillCycle(updt.end_offset);
}

void ClientInterface::updateProjectilePath(Unit *u, Projectile pps, const Vec3f &start, const Vec3f &end) {
	ProjectileUpdate updt = keyFrame.getProjUpdate();
	pps->setPath(start, end, updt.end_offset);
}

void ClientInterface::updateLobby() {
	// NETWORK: this method is very different
	NetworkMessageType msgType = getNextMessageType();

	if (msgType == NetworkMessageType::INTRO) {
		NetworkMessageIntro introMsg;
		if (receiveMessage(&introMsg)) {
			//check consistency
			if(theConfig.getNetConsistencyChecks() && introMsg.getVersionString() != getNetworkVersionString()) {
				throw runtime_error("Server and client versions do not match ("
							+ introMsg.getVersionString() + "). You have to use the same binaries.");
			}
			LOG_NETWORK( "Received intro message, sending intro message reply." );
			playerIndex = introMsg.getPlayerIndex();
			serverName = introMsg.getHostName();

			setRemoteNames(introMsg.getHostName(), introMsg.getPlayerName());

			if (playerIndex < 0 || playerIndex >= GameConstants::maxPlayers) {
				throw runtime_error("Intro message from server contains bad data, are you using the same version?");
			}

			//send reply
			NetworkMessageIntro replyMsg(
				getNetworkVersionString(), theConfig.getNetPlayerName(), getHostName(), -1);

			send(&replyMsg);
			introDone= true;
		}
	} else if (msgType == NetworkMessageType::LAUNCH) {
		NetworkMessageLaunch launchMsg;
		if (receiveMessage(&launchMsg)) {
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
	} else if (msgType != NetworkMessageType::NO_MSG) {
		LOG_NETWORK( "Received bad message type : " + intToStr(msgType) + ". Resetting connection." );
		reset();
		//throw runtime_error("Unexpected network message: " + intToStr(msgType));
	}
}

void ClientInterface::updateKeyframe(int frameCount) {
	// give all commands from last KeyFrame
	for (size_t i=0; i < keyFrame.getCmdCount(); ++i) {
		pendingCommands.push_back(*keyFrame.getCmd(i));
	}

	keyFrame.reset();

	while (true) {
		waitForMessage();
		NetworkMessageType msgType = getNextMessageType();
		if (msgType == NetworkMessageType::KEY_FRAME) {
			// make sure we read the message
			{	Chrono chrono;
				chrono.start();
				while (!receiveMessage(&keyFrame)) {
					sleep(waitSleepTime);
					if (chrono.getMillis() > messageWaitTimeout) {
						cout << "timout.\n";
						throw runtime_error("Timeout waiting for key frame.");
					}
				}
			}
			//check that we are in the right frame
			if (keyFrame.getFrameCount() != frameCount + GameConstants::networkFramePeriod) {
				stringstream ss;
				ss << "Network synchronization error, frame count mismatch";
				LOG_NETWORK( ss.str() );
				throw runtime_error(ss.str());
			}
			return;
		} else if (msgType == NetworkMessageType::QUIT) {
			NetworkMessageQuit quitMsg;
			receiveMessage(&quitMsg);
			quit = true;
			quitGame();
			return;
		} else if (msgType == NetworkMessageType::TEXT) {
			NetworkMessageText textMsg;
			receiveMessage(&textMsg);
			GameInterface::processTextMessage(textMsg);
		} else {
			throw runtime_error("Unexpected message in client interface: " + intToStr(msgType));
		}
	}
}

void ClientInterface::syncAiSeeds(int aiCount, int *seeds) {
	assert(aiCount && seeds);
	NetworkMessageAiSeedSync seedSyncMsg;
	Chrono chrono;
	chrono.start();
	LOG_NETWORK( "Ready, waiting for server to send Ai random number seeds." );
	//wait until we get an ai seed sync message from the server
	while (true) {
		NetworkMessageType msgType = getNextMessageType();
		if (msgType == NetworkMessageType::AI_SYNC) {
			if (receiveMessage(&seedSyncMsg)) {
				LOG_NETWORK( "Received AI random number seed message." );
				assert(seedSyncMsg.getSeedCount());
				for (int i=0; i < seedSyncMsg.getSeedCount(); ++i) {
					seeds[i] = seedSyncMsg.getSeed(i);
				}
				break;
			}
		} else if (msgType == NetworkMessageType::NO_MSG) {
			if (chrono.getMillis() > readyWaitTimeout) {
				throw runtime_error("Timeout waiting for server");
			}
		} else {
			throw runtime_error("Unexpected network message: " + intToStr(msgType) );
		}
		sleep(2);
	}
}

void ClientInterface::waitUntilReady(Checksum &checksum) {
	// NETWORK: this method is very different
	NetworkMessageReady readyMsg;
	Chrono chrono;
	chrono.start();

	//send ready message
	send(&readyMsg);
	LOG_NETWORK( "Ready, waiting for server ready message." );

	//wait until we get a ready message from the server
	while (true) {
		NetworkMessageType msgType = getNextMessageType();
		if (msgType == NetworkMessageType::READY) {
			if (receiveMessage(&readyMsg)) {
				LOG_NETWORK( "Received ready message." );
				break;
			}
		} else if (msgType == NetworkMessageType::NO_MSG) {
			if (chrono.getMillis() > readyWaitTimeout) {
				throw runtime_error("Timeout waiting for server");
			}
		} else {
			throw runtime_error("Unexpected network message: " + intToStr(msgType) );
		}
		// sleep a bit
		sleep(waitSleepTime);
	}
	//check checksum
	if (theConfig.getNetConsistencyChecks() && readyMsg.getChecksum() != checksum.getSum()) {
		throw runtime_error("Checksum error, you don't have the same data as the server");
	}
	//delay the start a bit, so clients have nore room to get messages
	sleep(GameConstants::networkExtraLatency);
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex){
	NetworkMessageText textMsg(text, theConfig.getNetPlayerName(), teamIndex);
	send(&textMsg);
}

string ClientInterface::getStatus() const{
	return theLang.get("Server") + ": " + serverName;
	//	return getRemotePlayerName() + ": " + NetworkStatus::getStatus();
}

void ClientInterface::waitForMessage() {
	Chrono chrono;
	chrono.start();
	NetworkMessageType nextType;
	while ((nextType = getNextMessageType()) == NetworkMessageType::NO_MSG) {
		if (!isConnected()) {
			throw runtime_error("Disconnected");
		}
		if (chrono.getMillis() > messageWaitTimeout) {
			throw runtime_error("Timeout waiting for message");
		}
		sleep(waitSleepTime);
	}

	//DEBUG .. induced delay
	/*Shared::Util::Random rand(Chrono::getCurTicks());
	if (rand.randRange(1, 100) <= 10) {
		sleep(rand.randRange(50,100));
	}*/

}

void ClientInterface::quitGame() {
	LOG_NETWORK( "Quitting" );
	if(!quit && clientSocket && clientSocket->isConnected()) {
		//string text = getHostName() + " has left the game!";
		//sendTextMessage(text, -1);
		//LOG_NETWORK( "Sent goodbye text messsage." );
		NetworkMessageQuit networkMessageQuit;
		send(&networkMessageQuit);
		LOG_NETWORK( "Sent quit message." );
	}
	delete clientSocket;
	clientSocket = NULL;
}

}}//end namespace
