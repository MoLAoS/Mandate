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
#include "network_message.h"

#include <cassert>
#include <stdexcept>

#include "types.h"
#include "util.h"
#include "game_settings.h"

#include "leak_dumper.h"
#include "logger.h"

#include "tech_tree.h"
#include "unit.h"
#include "world.h"

using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest { namespace Net {

// =====================================================
//	class Message
// =====================================================

bool Message::receive(Socket* socket, void* data, int dataSize){
	if (socket->getDataToRead() >= dataSize) {
		if (socket->receive(data, dataSize) == dataSize) {
			return true;
		}
		if (socket && socket->isConnected() /*&&socket->getSocketId() > 0*/) {
			throw runtime_error("Error receiving Message");
		} else {
			LOG_NETWORK( "socket disconnected, trying to read message." );
		}
	}
	return false;
}

bool Message::peek(Socket *socket, void *data, int dataSize) {
	if (socket->getDataToRead() >= dataSize) {
		if (socket->peek(data, dataSize) == dataSize) {
			return true;
		}
		if (socket && socket->isConnected() /*&&socket->getSocketId() > 0*/) {
			throw runtime_error("Error receiving Message");
		} else {
			LOG_NETWORK( "socket disconnected, trying to read message." );
		}
	}
	return false;
}

void Message::send(Socket* socket, const void* data, int dataSize) const {
	if (socket->send(data, dataSize)!=dataSize) {
		if (socket /*&& socket->getSocketId() > 0*/) {
			throw runtime_error("Error sending Message");
		} else {
			LOG_NETWORK( "socket disconnected, trying to send message.." );
		}
	}
}

// =====================================================
//	class IntroMessage
// =====================================================

IntroMessage::IntroMessage(){
	data.messageType= -1; 
	data.playerIndex= -1;
}

IntroMessage::IntroMessage(const string &versionString, const string &pName, const string &hName, int playerIndex){
	data.messageType = MessageType::INTRO; 
	data.versionString = versionString;
	data.playerName = pName;
	data.hostName = hName;
	data.playerIndex= static_cast<int16>(playerIndex);
}

bool IntroMessage::receive(Socket* socket){
	return Message::receive(socket, &data, sizeof(data));
}

void IntroMessage::send(Socket* socket) const{
	assert(data.messageType==MessageType::INTRO);
	Message::send(socket, &data, sizeof(data));
}

// =====================================================
//	class ReadyMessage
// =====================================================

ReadyMessage::ReadyMessage(){
	data.messageType= MessageType::READY; 
}

ReadyMessage::ReadyMessage(int32 checksum){
	data.messageType= MessageType::READY; 
	data.checksum= checksum;
}

bool ReadyMessage::receive(Socket* socket){
	return Message::receive(socket, &data, sizeof(data));
}

void ReadyMessage::send(Socket* socket) const{
	assert(data.messageType==MessageType::READY);
	Message::send(socket, &data, sizeof(data));
}


// =====================================================
//	class AiSeedSyncMessage
// =====================================================

AiSeedSyncMessage::AiSeedSyncMessage(){
	data.msgType = -1; 
	data.seedCount = -1;
}

AiSeedSyncMessage::AiSeedSyncMessage(int count, int32 *seeds) {
	assert(count > 0 && count <= maxAiSeeds);
	data.msgType = MessageType::AI_SYNC; 
	data.seedCount = count;
	for (int i=0; i < count; ++i) {
		data.seeds[i] = seeds[i];
	}
}

bool AiSeedSyncMessage::receive(Socket* socket){
	return Message::receive(socket, &data, sizeof(data));
}

void AiSeedSyncMessage::send(Socket* socket) const{
	assert(data.msgType == MessageType::AI_SYNC);
	Message::send(socket, &data, sizeof(data));
}


// =====================================================
//	class LaunchMessage
// =====================================================

/** Construct empty message (for clients to receive gamesettings into) */
LaunchMessage::LaunchMessage(){
	data.messageType=-1; 
}

/** Construct from GameSettings (for the server to send out) */
LaunchMessage::LaunchMessage(const GameSettings *gameSettings){
	data.messageType=MessageType::LAUNCH;

	data.description= gameSettings->getDescription();
	data.map= gameSettings->getMapPath();
	data.tileset= gameSettings->getTilesetPath();
	data.tech= gameSettings->getTechPath();
	data.factionCount= gameSettings->getFactionCount();
	data.thisFactionIndex= gameSettings->getThisFactionIndex();
	data.defaultResources= gameSettings->getDefaultResources();
	data.defaultUnits= gameSettings->getDefaultUnits();
	data.defaultVictoryConditions= gameSettings->getDefaultVictoryConditions();
	data.fogOfWar = gameSettings->getFogOfWar();

	for(int i= 0; i<data.factionCount; ++i){
		data.factionTypeNames[i]= gameSettings->getFactionTypeName(i);
		data.factionControls[i]= gameSettings->getFactionControl(i);
		data.teams[i]= gameSettings->getTeam(i);
		data.startLocationIndex[i]= gameSettings->getStartLocationIndex(i);
		data.resourceMultipliers[i] = gameSettings->getResourceMultilpier(i);
	}
}

/** Fills in a GameSettings object from this message (for clients) */
void LaunchMessage::buildGameSettings(GameSettings *gameSettings) const{
	gameSettings->setDescription(data.description.getString());
	gameSettings->setMapPath(data.map.getString());
	gameSettings->setTilesetPath(data.tileset.getString());
	gameSettings->setTechPath(data.tech.getString());
	gameSettings->setFactionCount(data.factionCount);
	gameSettings->setThisFactionIndex(data.thisFactionIndex);
	gameSettings->setDefaultResources(data.defaultResources);
	gameSettings->setDefaultUnits(data.defaultUnits);
	gameSettings->setDefaultVictoryConditions(data.defaultVictoryConditions);
	gameSettings->setFogOfWar(data.fogOfWar);

	for(int i= 0; i<data.factionCount; ++i){
		gameSettings->setFactionTypeName(i, data.factionTypeNames[i].getString());
		gameSettings->setFactionControl(i, static_cast<ControlType>(data.factionControls[i]));
		gameSettings->setTeam(i, data.teams[i]);
		gameSettings->setStartLocationIndex(i, data.startLocationIndex[i]);
		gameSettings->setResourceMultiplier(i, data.resourceMultipliers[i]);
	}
}

bool LaunchMessage::receive(Socket* socket){
	return Message::receive(socket, &data, sizeof(data));
}

void LaunchMessage::send(Socket* socket) const{
	assert(data.messageType==MessageType::LAUNCH);
	Message::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkCommandList
// =====================================================

CommandListMessage::CommandListMessage(int32 frameCount){
	data.messageType= MessageType::COMMAND_LIST;
	data.frameCount= frameCount;
	data.commandCount= 0;
}

bool CommandListMessage::addCommand(const NetworkCommand* networkCommand){
	if (data.commandCount < maxCommandCount) {
		data.commands[data.commandCount++]= *networkCommand;
		return true;
	}
	return false;
}

bool CommandListMessage::receive(Socket* socket) {
	// peek type, commandCount & frame num first.
	if (!Message::peek(socket, &data, dataHeaderSize)) {
		return false;
	}
	// read 'header' (we only had a 'peek' above) + data.commandCount commands.
	return Message::receive(socket, &data, dataHeaderSize + sizeof(NetworkCommand) * data.commandCount);
}

void CommandListMessage::send(Socket* socket) const {
	assert(data.messageType == MessageType::COMMAND_LIST);
	Message::send(socket, &data, dataHeaderSize + sizeof(NetworkCommand) * data.commandCount);
}

// =====================================================
//	class TextMessage
// =====================================================

TextMessage::TextMessage(const string &text, const string &sender, int teamIndex){
	data.messageType= MessageType::TEXT; 
	data.text= text;
	data.sender= sender;
	data.teamIndex= teamIndex;
}

bool TextMessage::receive(Socket* socket){
	return Message::receive(socket, &data, sizeof(data));
}

void TextMessage::send(Socket* socket) const{
	assert(data.messageType==MessageType::TEXT);
	Message::send(socket, &data, sizeof(data));
}

// =====================================================
//	class QuitMessage
// =====================================================

QuitMessage::QuitMessage(){
	data.messageType= MessageType::QUIT; 
}

bool QuitMessage::receive(Socket* socket){
	return Message::receive(socket, &data, sizeof(data));
}

void QuitMessage::send(Socket* socket) const{
	assert(data.messageType == MessageType::QUIT);
	Message::send(socket, &data, sizeof(data));
}

// =====================================================
//	class KeyFrame
// =====================================================

struct KeyFrameMsgHeader {
	int8	type;
	uint8	cmdCount;
	uint16	checksumCount;
	uint16 moveUpdateCount;
	uint16 projUpdateCount;
	uint16 updateSize;
	int32 frame;

};

bool KeyFrame::receive(Socket* socket) {
	KeyFrameMsgHeader  header;
	if (socket->peek(&header, sizeof(KeyFrameMsgHeader ))) {
		assert(header.type == MessageType::KEY_FRAME);
		this->frame = header.frame;
		this->checksumCount = header.checksumCount;
		
		this->moveUpdateCount = header.moveUpdateCount;
		this->projUpdateCount = header.projUpdateCount;
		this->updateSize = header.updateSize;
		
		this->cmdCount = header.cmdCount;
		size_t headerSize = sizeof(KeyFrameMsgHeader );
		size_t commandsSize = header.cmdCount * sizeof(NetworkCommand);
		size_t totalSize = headerSize + checksumCount * sizeof(int32) + commandsSize + updateSize;

		if (socket->getDataToRead() >= totalSize) {
			socket->receive(&header, headerSize);
			if (checksumCount) {
				socket->receive(checksums, checksumCount * sizeof(int32));
			}
			if (updateSize) {
				socket->receive(updateBuffer, updateSize);
			}
			if (commandsSize) {
				socket->receive(commands, commandsSize);
			}
			return true;
		} else {
			return false;
		}
	}
	return false;
}

void KeyFrame::send(Socket* socket) const {
	KeyFrameMsgHeader header;
	header.type = MessageType::KEY_FRAME;
	header.frame = this->frame;
	header.cmdCount = this->cmdCount;
	header.checksumCount = this->checksumCount;

	header.moveUpdateCount = this->moveUpdateCount;
	header.projUpdateCount = this->projUpdateCount;
	header.updateSize = this->updateSize;

	size_t commandsSize = header.cmdCount * sizeof(NetworkCommand);
	size_t headerSize = sizeof(KeyFrameMsgHeader);

	size_t totalSize = sizeof(KeyFrameMsgHeader) + checksumCount * sizeof(int32) + updateSize + commandsSize;
	char *buf = new char[totalSize];
	char *ptr = buf;
	memcpy(ptr, &header, sizeof(KeyFrameMsgHeader));
	ptr += sizeof(KeyFrameMsgHeader);
	if (checksumCount) {
		memcpy(ptr, checksums, checksumCount * sizeof(int32));
		ptr += checksumCount * sizeof(int32);
	}
	if (updateSize) {
		memcpy(ptr, updateBuffer, updateSize);
		ptr += updateSize;
	}
	if (commandsSize) {
		memcpy(ptr, commands, commandsSize);
	}
	try {
		Message::send(socket, buf, totalSize);
	} catch (...) {
		delete [] buf;
		throw;
	}
	delete [] buf;
}

int32 KeyFrame::getNextChecksum() { 
	assert(checksumCounter < checksumCount);
	if (checksumCounter >= checksumCount) {
		throw runtime_error("sync error: insufficient checksums in keyframe.");
	}
	return checksums[checksumCounter++];
}

void KeyFrame::addChecksum(int32 cs) {
	assert(checksumCount < max_checksums);
	if (checksumCount >= max_checksums) {
		throw runtime_error("Error: insufficient room for checksums in keyframe, increase KeyFrame::max_checksums.");
	}
	checksums[checksumCount++] = cs;
}

void KeyFrame::add(NetworkCommand &nc) {
	assert(cmdCount < max_cmds);
	memcpy(&commands[cmdCount++], &nc, sizeof(NetworkCommand));		
}

void KeyFrame::reset() {
	checksumCounter = checksumCount = 0;
	projUpdateCount = moveUpdateCount = cmdCount = 0;
	updateSize = 0;
	writePtr = updateBuffer;
	readPtr = updateBuffer;
}

void KeyFrame::addUpdate(MoveSkillUpdate updt) {
	assert(writePtr < &updateBuffer[buffer_size - 2]);
	memcpy(writePtr, &updt, sizeof(MoveSkillUpdate));
	writePtr += sizeof(MoveSkillUpdate);
	updateSize += sizeof(MoveSkillUpdate);
	++moveUpdateCount;
}

void KeyFrame::addUpdate(ProjectileUpdate updt) {
	assert(writePtr < &updateBuffer[buffer_size - 1]);
	memcpy(writePtr, &updt, sizeof(ProjectileUpdate));
	writePtr += sizeof(ProjectileUpdate);
	updateSize += sizeof(ProjectileUpdate);
	++projUpdateCount;
}

MoveSkillUpdate KeyFrame::getMoveUpdate() {
	assert(readPtr < updateBuffer + updateSize - 1);
	if (!moveUpdateCount) {
		throw runtime_error("Sync error: Insufficient move skill updates in keyframe");
	}
	MoveSkillUpdate res = *((MoveSkillUpdate*)readPtr);
	readPtr += sizeof(MoveSkillUpdate);
	--moveUpdateCount;
	return res;
}

ProjectileUpdate KeyFrame::getProjUpdate() {
	assert(readPtr < updateBuffer + updateSize);
	if (!projUpdateCount) {
		throw runtime_error("Sync error: Insufficient projectile updates in keyframe");
	}
	ProjectileUpdate res = *((ProjectileUpdate*)readPtr);
	readPtr += sizeof(ProjectileUpdate);
	--projUpdateCount;
	return res;
}

#if _GAE_DEBUG_EDITION_

bool SyncErrorMsg::receive(Socket* socket){
	return Message::receive(socket, &data, sizeof(data));
}

void SyncErrorMsg::send(Socket* socket) const{
	assert(data.messageType == MessageType::SYNC_ERROR);
	Message::send(socket, &data, sizeof(data));
}

#endif

}}//end namespace
