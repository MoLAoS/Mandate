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

#include "game.h"
#include "world.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;
using Shared::Platform::Chrono;

namespace Glest { namespace Game {

// =====================================================
//	class ClientInterface
// =====================================================

const int ClientInterface::messageWaitTimeout = 10000;	//10 seconds
const int ClientInterface::waitSleepTime = 5; // changed from 50, no obvious effect

ClientInterface::ClientInterface(){
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
		LOG_NETWORK( "Received bad message type : " + intToStr(msgType) );
		reset();
		//throw runtime_error("Unexpected network message: " + intToStr(msgType));
	}
}

void ClientInterface::updateKeyframe(int frameCount) {
	//DEBUG
	static int commandsReceived = 0;

	// NETWORK: this method is very different
	while (true) {
		//wait for the next message
		waitForMessage();

		//check we have an expected message
		NetworkMessageType msgType = getNextMessageType();

		if (msgType == NetworkMessageType::COMMAND_LIST) {
			NetworkMessageCommandList cmdList;
			//make sure we read the message
			while (!receiveMessage(&cmdList)) {
				sleep(waitSleepTime);
			}
			
			if (cmdList.getTotalB4This() != commandsReceived) {
				stringstream ss;
				ss << "ERROR: Server claims to have sent " << cmdList.getTotalB4This() << " commands in total, "
					<< "but I have received " << commandsReceived;
				LOG_NETWORK( ss.str() );
			}

			//check that we are in the right frame
			if (cmdList.getFrameCount() != frameCount) {
				stringstream ss;
				ss << "Network synchronization error, my frame count == " << frameCount
					<< ", server frame count == " << cmdList.getFrameCount();
				LOG_NETWORK( ss.str() );
				throw runtime_error(ss.str());
			}

			// give all commands
			if (cmdList.getCommandCount()) {
				/*LOG_NETWORK( 
					"Keyframe update: " + intToStr(frameCount) + " received "
					+ intToStr(cmdList.getCommandCount()) + " commands. " 
					+ intToStr(dataAvailable()) + " bytes waiting to be read."
				);*/
				for (int i= 0; i < cmdList.getCommandCount(); ++i) {
					pendingCommands.push_back(*cmdList.getCommand(i));
					++commandsReceived;
					/*const NetworkCommand * const &cmd = cmdList.getCommand(i);
					const Unit * const &unit = theWorld.findUnitById(cmd->getUnitId());
					LOG_NETWORK( 
						"\tUnit: " + intToStr(unit->getId()) + " [" + unit->getType()->getName() + "] " 
						+ unit->getType()->findCommandTypeById(cmd->getCommandTypeId())->getName() + "."
					);*/
				}
			}
			return;
		} else if (msgType == NetworkMessageType::QUIT) {
			NetworkMessageQuit quitMsg;
			if (receiveMessage(&quitMsg)) {
				quit = true;
				quitGame();
			}
			return;
		} else if (msgType == NetworkMessageType::TEXT) {
			NetworkMessageText textMsg;
			if (receiveMessage(&textMsg)) {
				GameNetworkInterface::processTextMessage(textMsg);
			}
		} else {
			throw runtime_error("Unexpected message in client interface: " + intToStr(msgType));
		}
	}
}

void ClientInterface::syncAiSeeds(int aiCount, int *seeds) {
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
	while (getNextMessageType() == NetworkMessageType::NO_MSG) {
		if (!isConnected()) {
			throw runtime_error("Disconnected");
		}
		if (chrono.getMillis() > messageWaitTimeout) {
			throw runtime_error("Timeout waiting for message");
		}
		sleep(waitSleepTime);
	}
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
