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

namespace Glest { namespace Game {

// =====================================================
//	class NetworkMessage
// =====================================================

bool NetworkMessage::receive(Socket* socket, void* data, int dataSize){
	if (socket->getDataToRead() >= dataSize) {
		if (socket->receive(data, dataSize) == dataSize) {
			return true;
		}
		if (socket && socket->isConnected() /*&&socket->getSocketId() > 0*/) {
			throw runtime_error("Error receiving NetworkMessage");
		} else {
			LOG_NETWORK( "socket disconnected, trying to read message." );
		}
	}
	return false;
}

bool NetworkMessage::peek(Socket *socket, void *data, int dataSize) {
	if (socket->getDataToRead() >= dataSize) {
		if (socket->peek(data, dataSize) == dataSize) {
			return true;
		}
		if (socket && socket->isConnected() /*&&socket->getSocketId() > 0*/) {
			throw runtime_error("Error receiving NetworkMessage");
		} else {
			LOG_NETWORK( "socket disconnected, trying to read message." );
		}
	}
	return false;
}

void NetworkMessage::send(Socket* socket, const void* data, int dataSize) const {
	if (socket->send(data, dataSize)!=dataSize) {
		if (socket /*&& socket->getSocketId() > 0*/) {
			throw runtime_error("Error sending NetworkMessage");
		} else {
			LOG_NETWORK( "socket disconnected, trying to send message.." );
		}
	}
}

// =====================================================
//	class NetworkMessageIntro
// =====================================================

NetworkMessageIntro::NetworkMessageIntro(){
	data.messageType= -1; 
	data.playerIndex= -1;
}

NetworkMessageIntro::NetworkMessageIntro(const string &versionString, const string &pName, const string &hName, int playerIndex){
	data.messageType = NetworkMessageType::INTRO; 
	data.versionString = versionString;
	data.playerName = pName;
	data.hostName = hName;
	data.playerIndex= static_cast<int16>(playerIndex);
}

bool NetworkMessageIntro::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageIntro::send(Socket* socket) const{
	assert(data.messageType==NetworkMessageType::INTRO);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageReady
// =====================================================

NetworkMessageReady::NetworkMessageReady(){
	data.messageType= NetworkMessageType::READY; 
}

NetworkMessageReady::NetworkMessageReady(int32 checksum){
	data.messageType= NetworkMessageType::READY; 
	data.checksum= checksum;
}

bool NetworkMessageReady::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageReady::send(Socket* socket) const{
	assert(data.messageType==NetworkMessageType::READY);
	NetworkMessage::send(socket, &data, sizeof(data));
}


// =====================================================
//	class NetworkMessageAiSeedSync
// =====================================================

NetworkMessageAiSeedSync::NetworkMessageAiSeedSync(){
	data.msgType = -1; 
	data.seedCount = -1;
}

NetworkMessageAiSeedSync::NetworkMessageAiSeedSync(int count, int32 *seeds) {
	assert(count > 0 && count <= maxAiSeeds);
	data.msgType = NetworkMessageType::AI_SYNC; 
	data.seedCount = count;
	for (int i=0; i < count; ++i) {
		data.seeds[i] = seeds[i];
	}
}

bool NetworkMessageAiSeedSync::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageAiSeedSync::send(Socket* socket) const{
	assert(data.msgType == NetworkMessageType::AI_SYNC);
	NetworkMessage::send(socket, &data, sizeof(data));
}


// =====================================================
//	class NetworkMessageLaunch
// =====================================================

/** Construct empty message (for clients to receive gamesettings into) */
NetworkMessageLaunch::NetworkMessageLaunch(){
	data.messageType=-1; 
}

/** Construct from GameSettings (for the server to send out) */
NetworkMessageLaunch::NetworkMessageLaunch(const GameSettings *gameSettings){
	data.messageType=NetworkMessageType::LAUNCH;

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
void NetworkMessageLaunch::buildGameSettings(GameSettings *gameSettings) const{
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

bool NetworkMessageLaunch::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageLaunch::send(Socket* socket) const{
	assert(data.messageType==NetworkMessageType::LAUNCH);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkCommandList
// =====================================================

NetworkMessageCommandList::NetworkMessageCommandList(int32 frameCount){
	data.messageType= NetworkMessageType::COMMAND_LIST;
	data.frameCount= frameCount;
	data.commandCount= 0;
}

bool NetworkMessageCommandList::addCommand(const NetworkCommand* networkCommand){
	if (data.commandCount < maxCommandCount) {
		data.commands[data.commandCount++]= *networkCommand;
		return true;
	}
	return false;
}

bool NetworkMessageCommandList::receive(Socket* socket) {
	// peek type, commandCount & frame num first.
	if (!NetworkMessage::peek(socket, &data, dataHeaderSize)) {
		return false;
	}
	// read 'header' (we only had a 'peek' above) + data.commandCount commands.
	return NetworkMessage::receive(socket, &data, dataHeaderSize + sizeof(NetworkCommand) * data.commandCount);
}

void NetworkMessageCommandList::send(Socket* socket) const {
	assert(data.messageType == NetworkMessageType::COMMAND_LIST);
	NetworkMessage::send(socket, &data, dataHeaderSize + sizeof(NetworkCommand) * data.commandCount);
}

// =====================================================
//	class NetworkMessageText
// =====================================================

NetworkMessageText::NetworkMessageText(const string &text, const string &sender, int teamIndex){
	data.messageType= NetworkMessageType::TEXT; 
	data.text= text;
	data.sender= sender;
	data.teamIndex= teamIndex;
}

bool NetworkMessageText::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageText::send(Socket* socket) const{
	assert(data.messageType==NetworkMessageType::TEXT);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageQuit
// =====================================================

NetworkMessageQuit::NetworkMessageQuit(){
	data.messageType= NetworkMessageType::QUIT; 
}

bool NetworkMessageQuit::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageQuit::send(Socket* socket) const{
	assert(data.messageType == NetworkMessageType::QUIT);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class SkillCycleTable
// =====================================================

void SkillCycleTable::create(const TechTree *techTree) {
	const FactionType *ft;
	const UnitType *ut;
	const SkillType *st;
	for (int i=0; i < techTree->getFactionTypeCount(); ++i) {
		ft = techTree->getFactionType(i);
		for (int j=0; j < ft->getUnitTypeCount(); ++j) {
			ut = ft->getUnitType(j);
			for (int k=0; k < ut->getSkillTypeCount(); ++k) {
				st = ut->getSkillType(k);
				SkillIdTriple id(ft->getId(), ut->getId(), st->getId());
				cycleTable[id] = st->calculateCycleTime();
			}
		}
	}
}

struct SkillCycleTableMsgHeader {
	int32  type		:  8;
	uint32 dataSize : 24;
};

void SkillCycleTable::send(Socket *socket) const {
	size_t numEntries = cycleTable.size();
	size_t dataSize = numEntries * (sizeof(SkillIdTriple) + sizeof(CycleInfo));
	size_t totalSize = sizeof(SkillCycleTableMsgHeader) + dataSize;
	char* buffer = new char[totalSize];
	char *ptr = buffer;

	assert(dataSize < 16777216);
	assert( (1<<24) == 16777216);
	SkillCycleTableMsgHeader* hdr = (SkillCycleTableMsgHeader*)buffer;
	hdr->type = NetworkMessageType::SKILL_CYCLE_TABLE;
	hdr->dataSize = dataSize;
	ptr += sizeof(SkillCycleTableMsgHeader);

	foreach_const (CycleMap, it, cycleTable) {
		memcpy(ptr, &it->first, sizeof(SkillIdTriple));
		ptr += sizeof(SkillIdTriple);
		memcpy(ptr, &it->second, sizeof(CycleInfo));
		ptr += sizeof(CycleInfo);
	}
	assert(ptr == buffer + totalSize);
	try {
		cout << "sending SkillCycleTable " << totalSize << " bytes.\n";
		NetworkMessage::send(socket, buffer, totalSize);
	} catch (...) {
		delete [] buffer;
		throw;
	}
	delete [] buffer;
}

struct SkillIdCycleInfoPair {
	SkillIdTriple id;
	CycleInfo info;
};

bool SkillCycleTable::receive(Socket *socket) {
	SkillCycleTableMsgHeader hdr;
	const size_t &hdrSize = sizeof(SkillCycleTableMsgHeader);
	if (!socket->peek(&hdr, hdrSize)) {
		return false;
	}
	size_t totalSize = hdrSize + hdr.dataSize;
	char *buffer = new char[totalSize];
	try {
		if (socket->receive(buffer, totalSize) == totalSize) {
			cout << "received SkillCycleTable " << totalSize << " bytes.\n";
			SkillIdCycleInfoPair *data = (SkillIdCycleInfoPair*)(buffer + hdrSize);
			const size_t items = hdr.dataSize / sizeof(SkillIdCycleInfoPair);
			assert(hdr.dataSize % sizeof(SkillIdCycleInfoPair) == 0);
			for (size_t i=0; i < items; ++i) {
				cycleTable.insert(std::make_pair(data[i].id, data[i].info));
			}
			delete [] buffer;
			return true;
		} else {
			delete [] buffer;
			return false;
		}
	} catch(...) {
		delete [] buffer;
		throw;
	}
}

const CycleInfo& SkillCycleTable::lookUp(const Unit *unit) {
	SkillIdTriple id(
		unit->getFaction()->getType()->getId(),
		unit->getType()->getId(),
		unit->getCurrSkill()->getId()
	);
	return cycleTable[id];
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
		assert(header.type == NetworkMessageType::KEY_FRAME);
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
	header.type = NetworkMessageType::KEY_FRAME;
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
	NetworkMessage::send(socket, buf, totalSize);
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

#if _RECORD_GAME_STATE_

bool SyncError::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void SyncError::send(Socket* socket) const{
	assert(data.messageType == NetworkMessageType::SYNC_ERROR);
	NetworkMessage::send(socket, &data, sizeof(data));
}

#endif

}}//end namespace
