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
#include "network_interface.h"

#include <exception>
#include <cassert>

#include "types.h"
#include "conversion.h"
#include "platform_util.h"
#include "world.h"

#include "leak_dumper.h"
#include "logger.h"
#include "network_util.h"
#include "network_message.h"
#include "script_manager.h"
#include "command.h"

using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
//	class NetworkInterface
// =====================================================

const int NetworkInterface::readyWaitTimeout = 60000;	// 1 minute

void NetworkInterface::send(const NetworkMessage* networkMessage) {
	Socket* socket= getSocket();
	networkMessage->send(socket);
}

//NETWORK: this is (re)moved
NetworkMessageType NetworkInterface::getNextMessageType() {
	Socket* socket = getSocket();
	int8 messageType = NetworkMessageType::NO_MSG;

	//peek message type
	socket->peek(&messageType, sizeof(messageType));
	
	//sanity check new message type
	if (messageType < 0 || messageType >= NetworkMessageType::COUNT){
		throw runtime_error("Invalid message type: " + intToStr(messageType));
	}
	return NetworkMessageType(messageType);
}

int NetworkInterface::dataAvailable() {
	return getSocket()->getDataToRead();
}

bool NetworkInterface::receiveMessage(NetworkMessage* networkMessage) {
	Socket* socket= getSocket();
	return networkMessage->receive(socket);
}

void NetworkInterface::setRemoteNames(const string &hostName, const string &playerName) {
	remoteHostName = hostName;
	remotePlayerName = playerName;

	stringstream str;
	str << remotePlayerName;
	if (!remoteHostName.empty()) {
		str << " (" << remoteHostName << ")";
	}
	description = str.str();
}

// =====================================================
//	class GameInterface
// =====================================================

GameInterface::GameInterface() {
	keyFrame.reset();
	quit = false;
}

void GameInterface::requestCommand(Command *command) {
	Unit *unit = command->getCommandedUnit();
	
	if (command->isAuto()) {
		pendingCommands.push_back(command);
	}

	if (command->getArchetype() == CommandArchetype::GIVE_COMMAND) {
		requestedCommands.push_back(NetworkCommand(command));
	} else if (command->getArchetype() == CommandArchetype::CANCEL_COMMAND) {
		requestedCommands.push_back(NetworkCommand(NetworkCommandType::CANCEL_COMMAND, unit, Vec2i(-1)));
	}
}

void GameInterface::processTextMessage(NetworkMessageText &msg) {
	if (msg.getTeamIndex() == -1 
	|| msg.getTeamIndex() == theWorld.getThisFaction()->getTeam()) {
		chatMessages.push_back(ChatMsg(msg.getText(), msg.getSender()));
	}
}

void GameInterface::doUpdateUnitCommand(Unit *unit) {
	const SkillType *old_st = unit->getCurrSkill();

	// if unit has command process it
	if (unit->anyCommand()) {
		// check if a command being 'watched' has finished
		if (unit->getCommandCallback() && unit->getCommandCallback() != unit->getCurrCommand()) {
			ScriptManager::commandCallback(unit);
		}
		unit->getCurrCommand()->getType()->update(unit);
	}

	// if no commands, add stop (or guard for pets) command
	if (!unit->anyCommand() && unit->isOperative()) {
		const UnitType *ut = unit->getType();
		unit->setCurrSkill(SkillClass::STOP);
		if (unit->getMaster() && ut->hasCommandClass(CommandClass::GUARD)) {
			unit->giveCommand(new Command(ut->getFirstCtOfClass(CommandClass::GUARD), 
							CommandFlags(CommandProperties::AUTO), unit->getMaster()));
		} else {
			if (ut->hasCommandClass(CommandClass::STOP)) {
				unit->giveCommand(new Command(ut->getFirstCtOfClass(CommandClass::STOP), CommandFlags()));
			}
		}
	}
	//if unit is out of EP, it stops
	if (unit->computeEp()) {
		if (unit->getCurrCommand()) {
			unit->cancelCurrCommand();
		}
		unit->setCurrSkill(SkillClass::STOP);
	}

	try {
		// update cycle
		if (unit->getCurrSkill()->getClass() == SkillClass::MOVE) {
			updateMove(unit);
		} else {
			unit->updateSkillCycle(skillCycleTable.lookUp(unit).getSkillFrames());
		}
		if (unit->getCurrSkill() != old_st) {	// if starting new skill
			unit->resetAnim(theWorld.getFrameCount() + 1); // reset animation cycle for next frame
		}

		Checksum cs;
		cs.add(unit->getId());
		cs.add(unit->getFactionIndex());
		cs.add(unit->getCurrSkill()->getId());
		switch (unit->getCurrSkill()->getClass()) {
			case SkillClass::MOVE:
				cs.add(unit->getNextPos());
				break;
			case SkillClass::ATTACK:
			case SkillClass::REPAIR:
			case SkillClass::BUILD:
				if (unit->getTarget()) {
					cs.add(unit->getTarget()->getId());
				}
				break;
		}
		updateUnitCommand(unit, cs.getSum());
	} catch (runtime_error &e) {
#		if _RECORD_GAME_STATE_
			assert(theWorld.getFrameCount());

			UnitStateRecord usr(unit);
			stateLog.addUnitRecord(usr);
			stateLog.logFrame();

			SyncError se(theWorld.getFrameCount());
			send(&se);
#		endif
		throw e;
	}

#	if _RECORD_GAME_STATE_
		UnitStateRecord usr(unit);
		stateLog.addUnitRecord(usr);
#	endif
}

void GameInterface::frameStart(int frame) {
#	if _RECORD_GAME_STATE_
		assert(frame);
		stateLog.newFrame(frame);
#	endif
}

void GameInterface::frameEnd(int frame) {
}

void GameInterface::doUpdateAnim(Unit *unit) {
	if (unit->getCurrSkill()->getClass() == SkillClass::DIE) {
		unit->updateAnimDead();
	} else {
		const CycleInfo &inf = skillCycleTable.lookUp(unit);
		unit->updateAnimCycle(inf.getAnimFrames(), inf.getSoundOffset(), inf.getAttackOffset());
		if (inf.getAttackOffset() > 0) {
			Checksum cs;
			cs.add(unit->getId());
			cs.add(inf.getAttackOffset());
			updateAnim(unit, cs.getSum());
		}
	}
}

void GameInterface::doUnitBorn(Unit *unit) {
	const CycleInfo inf = skillCycleTable.lookUp(unit);
	unit->updateSkillCycle(inf.getSkillFrames());
	unit->updateAnimCycle(inf.getAnimFrames());

	Checksum cs;
	cs.add(unit->getType()->getName());
	cs.add(unit->getId());
	cs.add(unit->getFactionIndex());

	unitBorn(unit, cs.getSum());
}

void GameInterface::doUpdateProjectile(Unit *u, Projectile pps, const Vec3f &start, const Vec3f &end) {
	updateProjectilePath(u, pps, start, end);

	Checksum cs;
	cs.add(u->getId());
	cs.add(u->getCurrSkill()->getName());
	if (u->anyCommand()) {
		if (u->getCurrCommand()->getUnit()) {
			cs.add(u->getCurrCommand()->getUnit()->getId());
		} else {
			cs.add(u->getCurrCommand()->getPos());
		}
	}
	cs.add(pps->getEndFrame());
	updateProjectile(u, pps->getEndFrame(), cs.getSum());
}

}} // end namespace
