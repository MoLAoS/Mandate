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
#include "network_message.h"
#include "script_manager.h"
#include "command.h"

using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest { namespace Net {

// =====================================================
//	class NetworkConnection
// =====================================================

void NetworkConnection::send(const Message* networkMessage) {
	networkMessage->send(this);
}

/*MessageType NetworkConnection::getNextMessageType() {
	Socket* socket = getSocket();
	int8 messageType;
	//peek message type
	if (socket->peek(&messageType, sizeof(messageType))) {
		//sanity check new message type
		if (messageType < 0 || messageType >= MessageType::COUNT){
			throw InvalidMessage(messageType);
		}
		return MessageType(messageType);
	}
	return MessageType::NO_MSG;
}*/

int NetworkConnection::dataAvailable() {
	return getSocket()->getDataToRead();
}

/*bool NetworkConnection::receiveMessage(Message* networkMessage) {
	return networkMessage->receive(this);
}*/

void NetworkConnection::receiveMessages() {
	Socket *socket = getSocket();
	if (!socket->isConnected()) {
		return;
	}
	size_t n = socket->getDataToRead();
	while (n >= MsgHeader::headerSize) {
		MsgHeader header;
		socket->peek(&header, MsgHeader::headerSize);
		if (n >= MsgHeader::headerSize + header.messageSize) {
			RawMessage rawMsg;
			rawMsg.type = header.messageType;
			rawMsg.size = header.messageSize;
			rawMsg.data = new uint8[header.messageSize];
			socket->skip(MsgHeader::headerSize);
			if (header.messageSize) {
				socket->receive(rawMsg.data, header.messageSize);
			} else {
				rawMsg.data = 0;
			}
			messageQueue.push_back(rawMsg);
			n = socket->getDataToRead();
		} else {
			return;
		}
	}
}

RawMessage NetworkConnection::getNextMessage() {
	assert(hasMessage());
	RawMessage res = messageQueue.front();
	messageQueue.pop_front();
	return res;
}


void NetworkConnection::setRemoteNames(const string &hostName, const string &playerName) {
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
//	class NetworkInterface
// =====================================================

NetworkInterface::NetworkInterface(Program &prog) 
		: SimulationInterface(prog) {
	keyFrame.reset();
}

void NetworkInterface::processTextMessage(TextMessage &msg) {
	if (msg.getTeamIndex() == -1 
	|| msg.getTeamIndex() == theWorld.getThisFaction()->getTeam()) {
		chatMessages.push_back(ChatMsg(msg.getText(), msg.getSender()));
	}
}

void NetworkInterface::frameProccessed() {
	if (world->getFrameCount() % GameConstants::networkFramePeriod == 0) {
		updateKeyframe(world->getFrameCount());
	}
}

void NetworkInterface::postCommandUpdate(Unit *unit) {
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
				cs.add(unit->getTarget());
			}
			break;
	}
	checkCommandUpdate(unit, cs.getSum());
}

void NetworkInterface::postProjectileUpdate(Unit *u, int endFrame) {
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
	cs.add(endFrame);
	checkProjectileUpdate(u, endFrame, cs.getSum());
}

void NetworkInterface::postAnimUpdate(Unit *unit) {
	if (unit->getNextAttackFrame() != -1) {
		Checksum cs;
		cs.add(unit->getId());
		cs.add(unit->getNextAttackFrame());
		checkAnimUpdate(unit, cs.getSum());
	}
}

void NetworkInterface::postUnitBorn(Unit *unit) {
	Checksum cs;
	cs.add(unit->getType()->getName());
	cs.add(unit->getId());
	cs.add(unit->getFactionIndex());
	checkUnitBorn(unit, cs.getSum());
}


}} // end namespace
