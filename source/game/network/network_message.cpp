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
#include "zlib.h"
#include "config.h"
#include "logger.h"
#include "network_manager.h"

#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;

namespace Glest{ namespace Game{

// =====================================================
//	class NetworkMessage
// =====================================================

const char* NetworkMessage::msgTypeName[nmtCount] = {
	"nmtInvalid",
	"nmtIntro",
	"nmtPing",
	"nmtReady",
	"nmtLaunch",
	"nmtCommandList",
	"nmtText",
	"nmtQuit",
	"nmtFileHeader",
	"nmtFileFragment",
	"nmtUpdate",
	"nmtUpdateRequest"
};

void NetworkMessage::writeMsg(NetworkDataBuffer &buf) const {
	size_t startBufSize = buf.size();
	assert(getMaxNetSize() < USHRT_MAX && getNetSize() <= getMaxNetSize());
	uint16 msgSize = getNetSize() + sizeof(uint16) + sizeof(type);
	buf.write(msgSize);
	buf.write(type);
	write(buf);
	size_t dataSent = buf.size() - startBufSize;
	assert(dataSent == msgSize);
}

NetworkMessage *NetworkMessage::readMsg(NetworkDataBuffer &buf) {
	size_t bytesReady = buf.size();
	NetworkMessage *msg;
	uint16 msgSize;
	uint8 type;

	if(bytesReady < sizeof(msgSize) + sizeof(type)) {
		return NULL;
	}

	buf.peek(msgSize);
	if(bytesReady < msgSize) {
		return NULL;
	}

	buf.read(msgSize);
	buf.read(type);

	switch(type) {
	case nmtIntro:
		msg = new NetworkMessageIntro(buf);
		break;

	case nmtPing:
		msg = new NetworkMessagePing(buf);
		break;

	case nmtReady:
		msg = new NetworkMessageReady(buf);
		break;

	case nmtLaunch:
		msg = new NetworkMessageLaunch(buf);
		break;

	case nmtCommandList:
		msg = new NetworkMessageCommandList(buf);
		break;

	case nmtText:
		msg = new NetworkMessageText(buf);
		break;

	case nmtQuit:
		msg = new NetworkMessageQuit(buf);
		break;

	case nmtFileHeader:
		msg = new NetworkMessageFileHeader(buf);
		break;

	case nmtFileFragment:
		msg = new NetworkMessageFileFragment(buf);
		break;

	case nmtUpdate:
		msg = new NetworkMessageUpdate(buf);
		break;

	case nmtUpdateRequest:
		msg = new NetworkMessageUpdateRequest(buf);
		break;

	default: {
			string msg;
			if(Config::getInstance().getMiscDebugMode()) {
				NetworkManager &netmgr = NetworkManager::getInstance();
				Logger &log = (netmgr.isServer() ? Logger::getServerLog() : Logger::getClientLog());

				msg = "Invalid message type " + intToStr(type)
						+ ", size = " + intToStr(msgSize);
				Logger::getInstance().add(msg);
			}
			throw NetworkException(msg);
		}
	}

	assert(buf.size() == bytesReady - msgSize);
	assert(msg->getNetSize() + sizeof(msgSize) + sizeof(type) == msgSize);
	
	if(Config::getInstance().getMiscDebugMode()) {
		NetworkManager &netmgr = NetworkManager::getInstance();
		Logger &log = (netmgr.isServer() ? Logger::getServerLog() : Logger::getClientLog());

		string logmsg = string("received msg type ")
				+ msgTypeName[msg->getType()] + " "
				+ intToStr(msg->getNetSize()) + " + 3 bytes.";
		log.add(logmsg);
		if(type == nmtUpdate || type == nmtUpdateRequest) {
			((NetworkMessageXmlDoc*)msg)->log(log);
		}
	}

	return msg;
}

// =====================================================
//	class NetworkMessageIntro
// =====================================================

NetworkMessageIntro::NetworkMessageIntro(const string &versionString, const string &hostName,
		const string &playerName, int playerIndex, bool resumeSaved) :
		NetworkMessage(nmtIntro),
		versionString(versionString),
		hostName(hostName),
		playerName(playerName),
		playerIndex(static_cast<int16>(playerIndex)),
		resumeSaved(resumeSaved ? 1 : 0) {
}

size_t NetworkMessageIntro::getNetSize() const {
	return versionString.getNetSize()
			+ hostName.getNetSize()
			+ playerName.getNetSize()
			+ sizeof(playerIndex)
			+ sizeof(resumeSaved);
}

size_t NetworkMessageIntro::getMaxNetSize() const {
	return versionString.getMaxNetSize()
			+ hostName.getMaxNetSize()
			+ playerName.getMaxNetSize()
			+ sizeof(playerIndex)
			+ sizeof(resumeSaved);
}

void NetworkMessageIntro::read(NetworkDataBuffer &buf) {
	versionString.read(buf);
	hostName.read(buf);
	playerName.read(buf);
	buf.read(playerIndex);
	buf.read(resumeSaved);
}

void NetworkMessageIntro::write(NetworkDataBuffer &buf) const {
	versionString.write(buf);
	hostName.write(buf);
	playerName.write(buf);
	buf.write(playerIndex);
	buf.write(resumeSaved);
}

// =====================================================
//	class NetworkMessageReady
// =====================================================

NetworkMessageReady::NetworkMessageReady(int32 checksum) : NetworkMessage(nmtReady) {
	this->checksum = checksum;
}

void NetworkMessageReady::read(NetworkDataBuffer &buf) {
	buf.read(checksum);
}

void NetworkMessageReady::write(NetworkDataBuffer &buf) const {
	buf.write(checksum);
}

// =====================================================
//	class NetworkMessageLaunch
// =====================================================

NetworkMessageLaunch::NetworkMessageLaunch(const GameSettings *gameSettings) : NetworkMessage(nmtLaunch) {

	description= gameSettings->getDescription();
	mapPath= gameSettings->getMapPath();
	tilesetPath= gameSettings->getTilesetPath();
	techPath= gameSettings->getTechPath();
	scenarioPath= gameSettings->getScenarioPath();
	factionCount= gameSettings->getFactionCount();
	thisFactionIndex= gameSettings->getThisFactionIndex();

	for(int i = 0; i < factionCount; ++i) {
		factionTypeNames[i]= gameSettings->getFactionTypeName(i);
		factionControls[i]= gameSettings->getFactionControl(i);
		teams[i]= gameSettings->getTeam(i);
		startLocationIndex[i]= gameSettings->getStartLocationIndex(i);
	}
}

void NetworkMessageLaunch::buildGameSettings(GameSettings *gameSettings) const{
	gameSettings->setDescription(description.getString());
	gameSettings->setMapPath(mapPath.getString());
	gameSettings->setTilesetPath(tilesetPath.getString());
	gameSettings->setTechPath(techPath.getString());
	gameSettings->setScenarioPath(scenarioPath.getString());
	gameSettings->setFactionCount(factionCount);
	gameSettings->setThisFactionIndex(thisFactionIndex);

	for(int i = 0; i < factionCount; ++i) {
		gameSettings->setFactionTypeName(i, factionTypeNames[i].getString());
		gameSettings->setFactionControl(i, static_cast<ControlType>(factionControls[i]));
		gameSettings->setTeam(i, teams[i]);
		gameSettings->setStartLocationIndex(i, startLocationIndex[i]);
	}
}

size_t NetworkMessageLaunch::getNetSize() const {
	size_t size = 0;
	size += description.getNetSize();
	size += mapPath.getNetSize();
	size += tilesetPath.getNetSize();
	size += techPath.getNetSize();
	size += scenarioPath.getNetSize();
	size += sizeof(thisFactionIndex);
	size += sizeof(factionCount);
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		size += factionTypeNames[i].getNetSize();
	}
	size += sizeof(factionControls);
	size += sizeof(teams);
	size += sizeof(startLocationIndex);
	return size;
}

size_t NetworkMessageLaunch::getMaxNetSize() const {
	size_t size = 0;
	size += description.getMaxNetSize();
	size += mapPath.getMaxNetSize();
	size += tilesetPath.getMaxNetSize();
	size += techPath.getMaxNetSize();
	size += scenarioPath.getMaxNetSize();
	size += sizeof(thisFactionIndex);
	size += sizeof(factionCount);
	size += factionTypeNames[0].getMaxNetSize() * GameConstants::maxPlayers;
	size += sizeof(factionControls);
	size += sizeof(teams);
	size += sizeof(startLocationIndex);
	return size;
}

void NetworkMessageLaunch::read(NetworkDataBuffer &buf) {
	description.read(buf);
	mapPath.read(buf);
	tilesetPath.read(buf);
	techPath.read(buf);
	scenarioPath.read(buf);
	buf.read(thisFactionIndex);
	buf.read(factionCount);
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		factionTypeNames[i].read(buf);
		buf.read(factionControls[i]);
		buf.read(teams[i]);
		buf.read(startLocationIndex[i]);
	}
}

void NetworkMessageLaunch::write(NetworkDataBuffer &buf) const {
	description.write(buf);
	mapPath.write(buf);
	tilesetPath.write(buf);
	techPath.write(buf);
	scenarioPath.write(buf);
	buf.write(thisFactionIndex);
	buf.write(factionCount);
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		factionTypeNames[i].write(buf);
		buf.write(factionControls[i]);
		buf.write(teams[i]);
		buf.write(startLocationIndex[i]);
	}
}



// =====================================================
//	class NetworkMessageCommandList
// =====================================================

NetworkMessageCommandList::NetworkMessageCommandList(int32 frameCount) :
		NetworkMessage(nmtCommandList), frameCount(frameCount), commands() {
}

size_t NetworkMessageCommandList::getNetSize() const {
	size_t size = sizeof(uint8) + sizeof(frameCount);
	for(vector<Command*>::const_iterator i = commands.begin(); i != commands.end(); ++i) {
		size += (*i)->getNetSize();
	}
	return size;			
}


void NetworkMessageCommandList::read(NetworkDataBuffer &buf) {
	assert(commands.empty());
	uint8 commandCount;
	buf.read(commandCount);
	buf.read(frameCount);
	assert(commandCount <= maxCommandCount);
	for(int i = 0; i < commandCount; ++i) {		
		commands.push_back(new Command(buf));
	}
}

void NetworkMessageCommandList::write(NetworkDataBuffer &buf) const {
	assert(commands.size() <= maxCommandCount);
	uint8 commandCount = commands.size();
	buf.write(commandCount);
	buf.write(frameCount);

	for(Commands::const_iterator i = commands.begin(); i != commands.end(); ++i) {
		(*i)->write(buf);	
	}
}


// =====================================================
//	class NetworkMessageText
// =====================================================

NetworkMessageText::NetworkMessageText(const string &text, const string &sender, int teamIndex) :
		NetworkMessage(nmtText) {
	this->text = text;
	this->sender = sender;
	this->teamIndex = teamIndex;
}

size_t NetworkMessageText::getNetSize() const {
	return text.getNetSize()
			+ sender.getNetSize()
			+ sizeof(teamIndex);
}

size_t NetworkMessageText::getMaxNetSize() const {
	return text.getMaxNetSize()
			+ sender.getMaxNetSize()
			+ sizeof(teamIndex);
}

void NetworkMessageText::read(NetworkDataBuffer &buf) {
	text.read(buf);
	sender.read(buf);
	buf.read(teamIndex);
}

void NetworkMessageText::write(NetworkDataBuffer &buf) const {
	text.write(buf);
	sender.write(buf);
	buf.write(teamIndex);
}

// =====================================================
//	class NetworkMessageFileFragment
// =====================================================

NetworkMessageFileHeader::NetworkMessageFileHeader(const string &name, bool compressed) : NetworkMessage(nmtFileHeader) {
	this->name = name;
	this->compressed = compressed;
}

size_t NetworkMessageFileHeader::getNetSize() const {
	return name.getNetSize() + sizeof(compressed);
}

size_t NetworkMessageFileHeader::getMaxNetSize() const {
	return name.getMaxNetSize() + sizeof(compressed);
}

void NetworkMessageFileHeader::read(NetworkDataBuffer &buf) {
	name.read(buf);
	buf.read(compressed);
}

void NetworkMessageFileHeader::write(NetworkDataBuffer &buf) const {
	name.write(buf);
	buf.write(compressed);
}


// =====================================================
//	class NetworkMessageFileFragment
// =====================================================

NetworkMessageFileFragment::NetworkMessageFileFragment(char *data, size_t size, int seq, bool last) :
		NetworkMessage(nmtFileFragment)  {
	assert(size <= sizeof(this->data));
	memcpy(this->data, data, size);
	this->size = size;
	this->seq = seq;
	this->last = last;
}

size_t NetworkMessageFileFragment::getNetSize() const {
	return sizeof(size)
			+ size
			+ sizeof(seq)
			+ sizeof(last);
}

size_t NetworkMessageFileFragment::getMaxNetSize() const {
	return sizeof(size)
			+ sizeof(data)
			+ sizeof(seq)
			+ sizeof(last);
}

void NetworkMessageFileFragment::read(NetworkDataBuffer &buf) {
	buf.read(size);
	assert(size <= sizeof(data));
	buf.read(data, size);
	buf.read(seq);
	buf.read(last);
}

void NetworkMessageFileFragment::write(NetworkDataBuffer &buf) const {
	assert(size <= sizeof(data));
	buf.write(size);
	buf.write(data, size);
	buf.write(seq);
	buf.write(last);
}


// =====================================================
//	class NetworkMessageXmlDoc
// =====================================================

void NetworkMessageXmlDoc::freeData() {
	if ( data ) {
		free(data);
	}
}

void NetworkMessageXmlDoc::freeNode() {
	if(rootNode && cleanupNode) {
		delete rootNode;
	}
}

void NetworkMessageXmlDoc::read(NetworkDataBuffer &buf) {
	int zstatus;
	uLongf destLen;

	buf.read(size);
	buf.read(compressedSize);
	destLen = size;

	/*
	if(size > maxSize || compressedSize > maxSize) {
		throw runtime_error("Server attempted to transmit Xml Document over 512kb, which is not allowed.");
	}*/

	if(compressedSize > buf.size()) {
		throw runtime_error("Buffer does not contain the specified number of bytes for compressed XML document.");
	}

	data = static_cast<char *>(malloc(size));

	if(!data) {
		throw runtime_error(string("Failed to allocate ") + intToStr(size) + " bytes of data");
	}

	zstatus = ::uncompress((Bytef *)data, &destLen, (const Bytef *)buf.data(), compressedSize);
	buf.pop(compressedSize);

	if(zstatus != Z_OK) {
		free(data);
		data = NULL;
		throw runtime_error("error in zstream while decompressing xml document");
	}

	if(destLen != size) {
		free(data);
		data = NULL;
		throw runtime_error(string("Decompressed data in xml document not the specified size. Should be ")
				+ intToStr(size) + " bytes but it's " + intToStr(destLen));
	}
}

void NetworkMessageXmlDoc::parse() {
	assert(data);
	assert(!rootNode);
	assert(!compressed);
	rootNode = XmlIo::getInstance().parseString(data);
	cleanupNode = true;
	freeData();
	data = NULL;
}

void NetworkMessageXmlDoc::writeXml() {
	assert(!data);
	assert(rootNode);
	auto_ptr<string> strData = rootNode->toString(Config::getInstance().getMiscDebugMode());
	size = strData->length() + 1;
	data = strncpy(static_cast<char *>(malloc(size)), strData->c_str(), size);
	data[size - 1] = 0;	// just in case
}

void NetworkMessageXmlDoc::compress() {
	uLongf destLen = (uLongf)(size * 1.001f + 12);
	char *compressedData = static_cast<char *>(malloc(destLen));
	int zstatus = ::compress2((Bytef *)compressedData, &destLen, (const Bytef *)data, size, Z_BEST_COMPRESSION);

	if(zstatus != Z_OK) {
		free(compressedData);
		throw runtime_error("error in zstream while compressing xml document");
	}

	// free old data buffer and shrink the new one
	freeData();
	data = static_cast<char *>(realloc(compressedData, compressedSize = destLen));
	compressed = true;
}

void NetworkMessageXmlDoc::write(NetworkDataBuffer &buf) const {
	assert(data);
	assert(compressed);
	buf.write(size);
	buf.write(compressedSize);
	buf.write(data, compressedSize);
}

void NetworkMessageXmlDoc::uncompress() {
	throw runtime_error("not implemented");
}

void NetworkMessageXmlDoc::log(Logger &logger) {
	logger.add(string("\n======================\n")
			+ getData() + "\n======================\n");
}

// =====================================================
//	class NetworkMessageUnitUpdate
// =====================================================



// =====================================================
//	class EffectReference
// =====================================================
/*
void EffectReference::init(Unit *unit, const Effect *e) {
}

void EffectReference::read(NetworkDataBuffer &buf, World *world) {
	source.read(buf, world);
	buf.read(typeId);
	buf.read(strength);
	buf.read(duration);
	buf.read(recourse);
}

void EffectReference::write(NetworkDataBuffer &buf) const {
	source.write(buf);
	buf.write(typeId);
	buf.write(strength);
	buf.write(duration);
	buf.write(recourse);
}
*/

}}//end namespace
