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

ClientInterface::ClientInterface(Program &prog)
		: NetworkInterface(prog) {
	clientSocket = NULL;
	launchGame = false;
	introDone = false;
	playerIndex = -1;
}

ClientInterface::~ClientInterface() {
	quitGame(QuitSource::LOCAL);
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
	delete clientSocket;
	clientSocket = NULL;
}

void ClientInterface::doIntroMessage() {
	RawMessage msg = getNextMessage();
	if (msg.type != MessageType::INTRO) {
		throw InvalidMessage(MessageType::INTRO, msg.type);
	}
	IntroMessage introMsg(msg);
	if (introMsg.getVersionString() != getNetworkVersionString()) {
		throw VersionMismatch(NetSource::CLIENT, getNetworkVersionString(), introMsg.getVersionString());
	}
	playerIndex = introMsg.getPlayerIndex();
	if (playerIndex < 0 || playerIndex >= GameConstants::maxPlayers) {
		throw GarbledMessage(MessageType::INTRO, NetSource::SERVER);
	}
	serverName = introMsg.getHostName();

	setRemoteNames(introMsg.getHostName(), introMsg.getPlayerName());
	//send reply
	IntroMessage replyMsg(getNetworkVersionString(), g_config.getNetPlayerName(), getHostName(), -1);
	send(&replyMsg);
	introDone= true;
}

void ClientInterface::doLaunchMessage() {
	RawMessage msg = getNextMessage();
	if (msg.type != MessageType::LAUNCH) {
		throw InvalidMessage(MessageType::LAUNCH, msg.type);
	}
	LaunchMessage launchMsg(msg);
	GameSettings &gameSettings = g_simInterface->getGameSettings();
	gameSettings.clear();
	launchMsg.buildGameSettings(&gameSettings);
	// replace server player by network
	for (int i=0; i < gameSettings.getFactionCount(); ++i) {
		// replace by network
		if (gameSettings.getFactionControl(i) == ControlType::HUMAN) {
			gameSettings.setFactionControl(i, ControlType::NETWORK);
		}
		// set the faction index
		if (gameSettings.getStartLocationIndex(i) == playerIndex) {
			gameSettings.setThisFactionIndex(i);
			gameSettings.setFactionControl(i, ControlType::HUMAN);
		}
	}
	launchGame = true;
	LOG_NETWORK( "Received launch message." );
}

void ClientInterface::createSkillCycleTable(const TechTree *) {
	int skillCount = g_world.getSkillTypeFactory()->getSkillTypeCount();
	int expectedSize = skillCount * sizeof(CycleInfo);
	LOG_NETWORK( "waiting for server to send Skill Cycle Table." );
	waitForMessage(readyWaitTimeout);
	RawMessage raw = getNextMessage();
	if (raw.type != MessageType::SKILL_CYCLE_TABLE) {
		throw InvalidMessage(MessageType::SKILL_CYCLE_TABLE, raw.type);
	}
	if (raw.size != expectedSize) {
		throw GarbledMessage(MessageType::SKILL_CYCLE_TABLE, NetSource::SERVER);
	}
	skillCycleTable = SkillCycleTable(raw);
	LOG_NETWORK( "Received Skill Cycle Table." );
}

void ClientInterface::updateLobby() {
	receiveMessages();
	if (!hasMessage()) {
		return;
	}
	if (!introDone) {
		doIntroMessage();
	} else {
		doLaunchMessage();
	}
}

void ClientInterface::syncAiSeeds(int aiCount, int *seeds) {
	waitForMessage(readyWaitTimeout);
	RawMessage raw = getNextMessage();
	if (raw.type != MessageType::AI_SYNC) {
		throw InvalidMessage(MessageType::AI_SYNC, raw.type);
	}
	AiSeedSyncMessage seedSyncMsg(raw);
	assert(aiCount && seeds);
	assert(seedSyncMsg.getSeedCount());
	for (int i=0; i < seedSyncMsg.getSeedCount(); ++i) {
		seeds[i] = seedSyncMsg.getSeed(i);
	}
}

void ClientInterface::waitUntilReady(Checksum *checksums) {
	// Should send checksums, then the server will know if something is screwed up...
	ReadyMessage readyMsg;
	send(&readyMsg);

	const TechTree *tt = world->getTechTree();
	const int &n = tt->getFactionTypeCount();
	string factionChecks;
	for (int i=0; i < n; ++i) {
		factionChecks += "Faction " + tt->getFactionType(i)->getName() + " = " 
			+ intToHex(checksums[4+i].getSum()) + "\n";
	}
	NETWORK_LOG( 
		"Ready, waiting for server ready message. My checksums:\n"
		<< "Tileset = " << intToHex(checksums[0].getSum()) << endl
		<< "Map = " << intToHex(checksums[1].getSum()) << endl
		<< "Damage Multipliers = " << intToHex(checksums[2].getSum()) << endl
		<< "Resources = " << intToHex(checksums[3].getSum()) << endl
		<< factionChecks;
	);
	waitForMessage(readyWaitTimeout);
	RawMessage raw = getNextMessage();
	if (raw.type != MessageType::READY) {
		throw InvalidMessage(MessageType::READY, raw.type);
	}
	readyMsg = ReadyMessage(raw);
	//check checksums
	bool checks[4 + GameConstants::maxPlayers];
	bool anyFalse = false;
	for (int i=0; i < 4 + n; ++i) {
		if (readyMsg.getChecksum(i) == checksums[i].getSum()) {
			checks[i] = true;
		} else {
			checks[i] = false;
			anyFalse = true;
		}
	}
	if (anyFalse) {
		throw DataSyncError(checks, n, NetSource::CLIENT);
	}
	//delay the start a bit, so clients have more room to get messages
	sleep(GameConstants::networkExtraLatency);
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex) {
	TextMessage textMsg(text, g_config.getNetPlayerName(), teamIndex);
	send(&textMsg);
}

string ClientInterface::getStatus() const {
	return g_lang.get("Server") + ": " + serverName;
	//	return getRemotePlayerName() + ": " + NetworkStatus::getStatus();
}

void ClientInterface::waitForMessage(int timeout) {
	Chrono chrono;
	chrono.start();
	receiveMessages();
	while (true) {
		if (hasMessage()) {
			return;
		}
		if (chrono.getMillis() > timeout) {
			throw TimeOut(NetSource::SERVER);
		} else {
			sleep(2);
			receiveMessages();
			continue;
		}
	}
}

void ClientInterface::quitGame(QuitSource source) {
	LOG_NETWORK( "Quitting" );
	if (clientSocket && clientSocket->isConnected() && source != QuitSource::SERVER) {
		QuitMessage networkMessageQuit;
		send(&networkMessageQuit);
		LOG_NETWORK( "Sent quit message." );
	}

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
	updateKeyframe(0);
}

void ClientInterface::update() {
	// chat messages
	while (hasChatMsg()) {
		getGameState()->getConsole()->addLine(getChatSender() + ": " + getChatText(), true);
		popChatMsg();
	}

	if (requestedCommands.empty()) {
		return;
	}
	// send as many commands as we can
	CommandListMessage cmdList;
	while (!requestedCommands.empty() && cmdList.addCommand(&requestedCommands.back())) {
		requestedCommands.pop_back();
	}
	send(&cmdList);
}

void ClientInterface::updateKeyframe(int frameCount) {
	// give all commands from last KeyFrame
	for (size_t i=0; i < keyFrame.getCmdCount(); ++i) {
		pendingCommands.push_back(*keyFrame.getCmd(i));
	}
	while (true) {
		waitForMessage();
		RawMessage raw = getNextMessage();
		if (raw.type == MessageType::KEY_FRAME) {
			keyFrame = KeyFrame(raw);
			if (keyFrame.getFrameCount() != frameCount + GameConstants::networkFramePeriod) {
				throw GameSyncError();
			}
			return;
		} else if (raw.type == MessageType::TEXT) {
			TextMessage textMsg(raw);
			NetworkInterface::processTextMessage(textMsg);
		} else if (raw.type == MessageType::QUIT) {
			QuitMessage quitMsg(raw);
			quitGame(QuitSource::SERVER);
			return;
		} else {
			throw InvalidMessage(MessageType::KEY_FRAME, raw.type);
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
			assert(g_world.getFrameCount());
			worldLog.logFrame(); // dump frame log
			SyncErrorMsg se(g_world.getFrameCount());
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
