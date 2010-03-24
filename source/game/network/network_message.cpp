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

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;

namespace Glest { namespace Game {

	// NETWORK: nearly whole file is different

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
	assert(data.messageType==NetworkMessageType::QUIT);
	NetworkMessage::send(socket, &data, sizeof(data));
}

}}//end namespace
