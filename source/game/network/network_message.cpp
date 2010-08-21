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
#include "network_interface.h"
#include "profiler.h"

using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest { namespace Net {

// =====================================================
//	class Message
// =====================================================

bool Message::receive(NetworkConnection* connection, void* data, int dataSize) {
	Socket *socket = connection->getSocket();
	if (socket->getDataToRead() >= dataSize) {
		if (socket->receive(data, dataSize)) {
			return true;
		}
		LOG_NETWORK( "connection severed, trying to read message." );
		throw Disconnect();
	}
	return false;
}

bool Message::peek(NetworkConnection* connection, void *data, int dataSize) {
	Socket *socket = connection->getSocket();
	if (socket->getDataToRead() >= dataSize) {
		if (socket->peek(data, dataSize)) {
			return true;
		}
		LOG_NETWORK( "connection severed, trying to read message." );
		throw Disconnect();
	}
	return false;
}

//REMOVE
void Message::send(NetworkConnection* connection, const void* data, int dataSize) const {
	Socket *socket = connection->getSocket();
	send(socket, data, dataSize);
}

void Message::send(Socket* socket, const void* data, int dataSize) const {
	if (socket->send(data, dataSize) != dataSize) {
		LOG_NETWORK( "connection severed, trying to send message.." );
		throw Disconnect();
	}
}

// =====================================================
//	class IntroMessage
// =====================================================

IntroMessage::IntroMessage(){
	data.messageType = MessageType::INVALID_MSG;
	data.playerIndex = -1;
}

IntroMessage::IntroMessage(const string &versionString, const string &pName, const string &hName, int playerIndex){
	data.messageType = MessageType::INTRO;
	data.messageSize = sizeof(Data) - 4;
	data.versionString = versionString;
	data.playerName = pName;
	data.hostName = hName;
	data.playerIndex = static_cast<int16>(playerIndex);
	cout << "IntroMessage constructed, Version string: " << versionString << endl;
}

IntroMessage::IntroMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	memcpy(&data.versionString, raw.data, raw.size);
	delete raw.data;
	cout << "IntroMessage received, Version string: " << data.versionString.getString() << endl;
}

bool IntroMessage::receive(NetworkConnection* connection) {
	throw runtime_error("Boo!");
	/*bool ok = Message::receive(connection, &data, sizeof(Data));
	if (ok) {
		NETWORK_LOG(
			__FUNCTION__ << "(): received message, type: " << MessageTypeNames[MessageType(data.messageType)]
			<< ", messageSize: " << data.messageSize << ", player name: " << data.playerName.getString()
			<< ", host name: " << data.hostName.getString() << ", player index: " << data.playerIndex
		);
	}
	return ok;
	*/
	return false;
}

void IntroMessage::send(NetworkConnection* connection) const{
	assert(data.messageType == MessageType::INTRO);
	NETWORK_LOG( __FUNCTION__ << "(): message sent, type: " << MessageTypeNames[MessageType(data.messageType)]
		<< ", messageSize: " << data.messageSize << ", player name: " << data.playerName.getString()
		<< ", host name: " << data.hostName.getString() << ", player index: " << data.playerIndex
	);
	Message::send(connection, &data, sizeof(Data));
}

// =====================================================
//	class ReadyMessage
// =====================================================

ReadyMessage::ReadyMessage() {
	data.messageType = MessageType::READY;
	data.messageSize = 0;
}

ReadyMessage::ReadyMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	if (raw.size || raw.data) {
		throw GarbledMessage(MessageType::READY);
	}
}

bool ReadyMessage::receive(NetworkConnection* connection){
	throw runtime_error(string(__FUNCTION__) + "() called");
	return false;
}

void ReadyMessage::send(NetworkConnection* connection) const{
	assert(data.messageType == MessageType::READY);
	Message::send(connection, &data, sizeof(data));
}

// =====================================================
//	class AiSeedSyncMessage
// =====================================================

AiSeedSyncMessage::AiSeedSyncMessage(){
	data.messageType = MessageType::INVALID_MSG;
	data.messageSize = 0;
	data.seedCount = -1;
}

AiSeedSyncMessage::AiSeedSyncMessage(int count, int32 *seeds) {
	assert(count > 0 && count <= maxAiSeeds);
	data.messageType = MessageType::AI_SYNC;
	data.messageSize = sizeof(Data) - 4;
	data.seedCount = count;
	for (int i=0; i < count; ++i) {
		data.seeds[i] = seeds[i];
	}
}

AiSeedSyncMessage::AiSeedSyncMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	memcpy(&data.seedCount, raw.data, raw.size);
	delete raw.data;
}

bool AiSeedSyncMessage::receive(NetworkConnection* connection){
	bool ok = Message::receive(connection, &data, sizeof(Data));
	if (ok) {
		NETWORK_LOG(
			__FUNCTION__ << "(): message received, type: " << MessageTypeNames[MessageType(data.messageType)]
			<< ", messageSize: " << data.messageSize
		);
	}
	return ok;
}

void AiSeedSyncMessage::send(NetworkConnection* connection) const{
	assert(data.messageType == MessageType::AI_SYNC);
	NETWORK_LOG( __FUNCTION__ << "(): message sent, type: " << MessageTypeNames[MessageType(data.messageType)]
		<< ", messageSize: " << data.messageSize
	);
	Message::send(connection, &data, sizeof(Data));
}

// =====================================================
//	class LaunchMessage
// =====================================================

/** Construct empty message (for clients to receive gamesettings into) */
LaunchMessage::LaunchMessage(){
	data.messageType = MessageType::INVALID_MSG;
}

/** Construct from GameSettings (for the server to send out) */
LaunchMessage::LaunchMessage(const GameSettings *gameSettings){
	data.messageType = MessageType::LAUNCH;
	data.messageSize = sizeof(Data) - 4;

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
		data.colourIndices[i] = gameSettings->getColourIndex(i);
		data.resourceMultipliers[i] = gameSettings->getResourceMultilpier(i);
	}
}

LaunchMessage::LaunchMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	memcpy(&data.description, raw.data, raw.size);
	delete raw.data;
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
		gameSettings->setColourIndex(i, data.colourIndices[i]);
		gameSettings->setResourceMultiplier(i, data.resourceMultipliers[i]);
	}
}

bool LaunchMessage::receive(NetworkConnection* connection) {
	bool ok = Message::receive(connection, &data, sizeof(Data));
	if (ok) {
		NETWORK_LOG(
			__FUNCTION__ << "(): message received, type: " << MessageTypeNames[MessageType(data.messageType)]
			<< ", messageSize: " << data.messageSize
		);
	}
	return ok;
}

void LaunchMessage::send(NetworkConnection* connection) const {
	assert(data.messageType==MessageType::LAUNCH);
	NETWORK_LOG( __FUNCTION__ << "(): message sent, type: " << MessageTypeNames[MessageType(data.messageType)]
		<< ", messageSize: " << data.messageSize
	);
	Message::send(connection, &data, sizeof(Data));
}

DataSyncMessage::DataSyncMessage(RawMessage raw)
		: m_data(0), fromRaw(true) {
	if (raw.size % sizeof(int32) != 0) {
		throw GarbledMessage(MessageType::DATA_SYNC, NetSource::SERVER);
	}
	m_unitTypeCount  = reinterpret_cast<int32*>(raw.data)[0];
	m_cmdTypeCount	 = reinterpret_cast<int32*>(raw.data)[1];
	m_skillTypeCount = reinterpret_cast<int32*>(raw.data)[2];
	m_upgrdTypeCount = reinterpret_cast<int32*>(raw.data)[3];

	if (getChecksumCount()) {
		m_data = reinterpret_cast<int32*>(raw.data) + 4;
	}
}

DataSyncMessage::DataSyncMessage(World &world) : m_data(0), fromRaw(false) {
	Checksum checksums[4];
	world.getTileset()->doChecksum(checksums[0]);
	world.getMap()->doChecksum(checksums[1]);
	const TechTree *tt = world.getTechTree();
	tt->doChecksumDamageMult(checksums[2]);
	tt->doChecksumResources(checksums[3]);

	UnitTypeFactory		&unitTFactory	= world.getUnitTypeFactory();
	CommandTypeFactory	&cmdTFactory	= world.getCommandTypeFactory();
	SkillTypeFactory	&sklTFactory	= world.getSkillTypeFactory();
	UpgradeTypeFactory	&upgrdTFactory	= world.getUpgradeTypeFactory();

	m_unitTypeCount  = unitTFactory.getTypeCount();
	m_cmdTypeCount	 = cmdTFactory.getTypeCount();
	m_skillTypeCount = sklTFactory.getTypeCount();
	m_upgrdTypeCount = upgrdTFactory.getTypeCount();

	m_data = new int32[getChecksumCount()];
	int n = -1;
	for (int i=0; i < 4; ++i) {
		m_data[++n] = checksums[i].getSum();
	}

	if (getChecksumCount() - 4 > 0) {
		for (int i=0; i < unitTFactory.getTypeCount(); ++i) {
			m_data[++n] = unitTFactory.getChecksum(unitTFactory.getType(i));
		}
		for (int i=0; i < cmdTFactory.getTypeCount(); ++i) {
			m_data[++n] = cmdTFactory.getChecksum(cmdTFactory.getType(i));
		}
		for (int i=0; i < sklTFactory.getTypeCount(); ++i) {
			m_data[++n] = sklTFactory.getChecksum(sklTFactory.getType(i));
		}
		for (int i=0; i < upgrdTFactory.getTypeCount(); ++i) {
			m_data[++n] = upgrdTFactory.getChecksum(upgrdTFactory.getType(i));
		}
	}
}

DataSyncMessage::~DataSyncMessage() {
	if (fromRaw) {
		delete [] (m_data - 4); // hacky...
	} else {
		delete [] m_data;
	}
}

void DataSyncMessage::send(NetworkConnection* connection) const {
	MsgHeader header;
	header.messageType = MessageType::DATA_SYNC;
	header.messageSize = sizeof(int32) * (getChecksumCount() + 4);
	Message::send(connection, &header, sizeof(MsgHeader));
	Message::send(connection, &m_unitTypeCount, sizeof(int32) * 4);
	Message::send(connection, m_data, header.messageSize - sizeof(int32) * 4);
	NETWORK_LOG( __FUNCTION__ << "(): message sent, type: " << MessageTypeNames[MessageType(header.messageType)]
		<< ", messageSize: " << header.messageSize
	);
}

bool DataSyncMessage::receive(NetworkConnection* connection) {
	throw runtime_error(string(__FUNCTION__) + "() was called");
	return false;
}

// =====================================================
//	class GameSpeedMessage
// =====================================================

GameSpeedMessage::GameSpeedMessage(int frame, GameSpeed setting) {
	data.messageType = MessageType::GAME_SPEED;
	data.messageSize = sizeof(Data) - sizeof(MsgHeader);
	data.frame = frame;
	data.setting = setting;
}

GameSpeedMessage::GameSpeedMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	memcpy(&data + sizeof(MsgHeader), raw.data, raw.size);
	delete raw.data;
}

bool GameSpeedMessage::receive(NetworkConnection* connection) {
	return false;
}

void GameSpeedMessage::send(NetworkConnection* connection) const {
}


// =====================================================
//	class NetworkCommandList
// =====================================================

CommandListMessage::CommandListMessage(int32 frameCount) {
	data.messageType = MessageType::COMMAND_LIST;
	data.messageSize = 4;
	data.frameCount = frameCount;
	data.commandCount = 0;
}

bool CommandListMessage::addCommand(const NetworkCommand* networkCommand){
	if (data.commandCount < maxCommandCount) {
		data.commands[data.commandCount++]= *networkCommand;
		data.messageSize += sizeof(NetworkCommand);
		return true;
	}
	return false;
}

CommandListMessage::CommandListMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	uint8* ptr = reinterpret_cast<uint8*>(&data);
	ptr += 4;
	memcpy(ptr, raw.data, raw.size);
	delete raw.data;
}

bool CommandListMessage::receive(NetworkConnection* connection) {
	MsgHeader header; // peek Message Header first
	if (!Message::peek(connection, &header, sizeof(MsgHeader))) {
		return false;
	}
	bool ok = Message::receive(connection, &data, sizeof(MsgHeader) + header.messageSize);
	if (ok) {
		NETWORK_LOG(
			__FUNCTION__ << "(): message received, type: " << MessageTypeNames[MessageType(data.messageType)]
			<< ", messageSize: " << data.messageSize << ", number of commands: " << data.commandCount
		);
	}
	return ok;
}

void CommandListMessage::send(NetworkConnection* connection) const {
	assert(data.messageType == MessageType::COMMAND_LIST);
	NETWORK_LOG(
		__FUNCTION__ << "(): message sent, type: " << MessageTypeNames[MessageType(data.messageType)]
		<< ", messageSize: " << data.messageSize << ", number of commands: " << data.commandCount
	);
	Message::send(connection, &data, dataHeaderSize + sizeof(NetworkCommand) * data.commandCount);
}

// =====================================================
//	class TextMessage
// =====================================================

TextMessage::TextMessage(const string &text, const string &sender, int teamIndex){
	data.messageType = MessageType::TEXT;
	data.messageSize = sizeof(*this) - 4;
	data.text = text;
	data.sender = sender;
	data.teamIndex = teamIndex;
}

TextMessage::TextMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	memcpy(&data.text, raw.data, raw.size);
	delete raw.data;
}

bool TextMessage::receive(NetworkConnection* connection){
	return Message::receive(connection, &data, sizeof(Data));
}

void TextMessage::send(NetworkConnection* connection) const{
	assert(data.messageType == MessageType::TEXT);
	Message::send(connection, &data, sizeof(Data));
}

// =====================================================
//	class QuitMessage
// =====================================================

QuitMessage::QuitMessage() {
	data.messageType = MessageType::QUIT;
	data.messageSize = 0;
}

QuitMessage::QuitMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	assert(raw.size == 0);
}

bool QuitMessage::receive(NetworkConnection* connection) {
	return Message::receive(connection, &data, sizeof(Data));
}

void QuitMessage::send(NetworkConnection* connection) const {
	assert(data.messageType == MessageType::QUIT);
	Message::send(connection, &data, sizeof(Data));
}

// =====================================================
//	class KeyFrame
// =====================================================

#pragma pack(push, 1)
struct KeyFrameMsgHeader {
	uint8	cmdCount;
	IF_MAD_SYNC_CHECKS(
		uint16	checksumCount;
	)
	uint16 moveUpdateCount;
	uint16 projUpdateCount;
	uint16 updateSize;
	int32 frame;

};
#pragma pack(pop)

KeyFrame::KeyFrame(RawMessage raw) {
	reset();
	uint8 *ptr = raw.data;
	KeyFrameMsgHeader header;
	memcpy(&header, ptr, sizeof(KeyFrameMsgHeader));
	ptr += sizeof(KeyFrameMsgHeader);

	frame = header.frame;
	IF_MAD_SYNC_CHECKS(
		checksumCount = header.checksumCount;
	)
	moveUpdateCount = header.moveUpdateCount;
	projUpdateCount = header.projUpdateCount;
	updateSize = header.updateSize;
	cmdCount = header.cmdCount;

	IF_MAD_SYNC_CHECKS(
		if (checksumCount) {
			memcpy(checksums, ptr, checksumCount * sizeof(int32));
			ptr += checksumCount * sizeof(int32);
		}
	)
	if (updateSize) {
		memcpy(updateBuffer, ptr, updateSize);
		ptr += updateSize;
	}
	if (cmdCount) {
		memcpy(commands, ptr, cmdCount * sizeof(NetworkCommand));
	}
	delete raw.data;
}

bool KeyFrame::receive(NetworkConnection* connection) {
	throw runtime_error(string(__FUNCTION__) + "() called.");
	return true;
}

void KeyFrame::send(NetworkConnection* connection) const {
	MsgHeader msgHeader;
	KeyFrameMsgHeader header;
	msgHeader.messageType = MessageType::KEY_FRAME;
	header.frame = this->frame;
	header.cmdCount = this->cmdCount;
	IF_MAD_SYNC_CHECKS(
		header.checksumCount = this->checksumCount;
	)
	header.moveUpdateCount = this->moveUpdateCount;
	header.projUpdateCount = this->projUpdateCount;
	header.updateSize = this->updateSize;

	size_t commandsSize = header.cmdCount * sizeof(NetworkCommand);
	size_t headerSize = sizeof(KeyFrameMsgHeader);
	msgHeader.messageSize = sizeof(KeyFrameMsgHeader) + updateSize + commandsSize;
	IF_MAD_SYNC_CHECKS(
		msgHeader.messageSize += checksumCount * sizeof(int32);
	)
	size_t totalSize = msgHeader.messageSize + sizeof(MsgHeader);

	NETWORK_LOG( "KeyFrame message size: " << msgHeader.messageSize );

	uint8 *buf = new uint8[totalSize];
	uint8 *ptr = buf;
	memcpy(ptr, &msgHeader, sizeof(MsgHeader));
	ptr += sizeof(MsgHeader);
	memcpy(ptr, &header, sizeof(KeyFrameMsgHeader));
	ptr += sizeof(KeyFrameMsgHeader);
	IF_MAD_SYNC_CHECKS(
		if (checksumCount) {
			memcpy(ptr, checksums, checksumCount * sizeof(int32));
			ptr += checksumCount * sizeof(int32);
		}
	)
	if (updateSize) {
		memcpy(ptr, updateBuffer, updateSize);
		ptr += updateSize;
	}
	if (commandsSize) {
		memcpy(ptr, commands, commandsSize);
	}
	try {
		Message::send(connection, buf, totalSize);
	} catch (...) {
		delete [] buf;
		throw;
	}
	delete [] buf;
}

#if MAD_SYNC_CHECKING

int32 KeyFrame::getNextChecksum() {
	if (checksumCounter >= checksumCount) {
		NETWORK_LOG( "Attempt to retrieve checksum #" << checksumCounter
			<< ", Insufficient checksums in keyFrame. Sync Error."
		);
		throw GameSyncError();
	}
	return checksums[checksumCounter++];
}

void KeyFrame::addChecksum(int32 cs) {
	if (checksumCount >= max_checksums) {
		throw runtime_error("Error: insufficient room for checksums in keyframe, increase KeyFrame::max_checksums.");
	}
	checksums[checksumCount++] = cs;
}

#endif

void KeyFrame::add(NetworkCommand &nc) {
	assert(cmdCount < max_cmds);
	memcpy(&commands[cmdCount++], &nc, sizeof(NetworkCommand));
}

void KeyFrame::reset() {
	IF_MAD_SYNC_CHECKS(
		checksumCounter = checksumCount = 0;
	)
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
		throw GameSyncError("Insufficient move skill updates in keyframe");
	}
	MoveSkillUpdate res = *((MoveSkillUpdate*)readPtr);
	readPtr += sizeof(MoveSkillUpdate);
	--moveUpdateCount;
	return res;
}

ProjectileUpdate KeyFrame::getProjUpdate() {
	assert(readPtr < updateBuffer + updateSize);
	if (!projUpdateCount) {
		throw GameSyncError("Insufficient projectile updates in keyframe");
	}
	ProjectileUpdate res = *((ProjectileUpdate*)readPtr);
	readPtr += sizeof(ProjectileUpdate);
	--projUpdateCount;
	return res;
}

#if _GAE_DEBUG_EDITION_

SyncErrorMsg::SyncErrorMsg(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	memcpy(&data.frameCount, raw.data, raw.size);
	delete raw.data;
}

bool SyncErrorMsg::receive(NetworkConnection* connection){
	return Message::receive(connection, &data, sizeof(data));
}

void SyncErrorMsg::send(NetworkConnection* connection) const{
	assert(data.messageType == MessageType::SYNC_ERROR);
	Message::send(connection, &data, sizeof(data));
}

#endif

}}//end namespace
