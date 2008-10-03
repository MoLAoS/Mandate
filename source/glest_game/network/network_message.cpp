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

#include "network_message.h"

#include <cassert>
#include <stdexcept>

#include "types.h"
#include "util.h"
#include "game_settings.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;

namespace Glest{ namespace Game{

// =====================================================
//	class NetworkMessage
// =====================================================

bool NetworkMessage::receive(Socket* socket, void* data, int dataSize){
	if(socket->getDataToRead()>=dataSize){
		if(socket->receive(data, dataSize)!=dataSize){
			throw runtime_error("Error receiving NetworkMessage");
		}
		return true;
	}
	return false;
}

void NetworkMessage::send(Socket* socket, const void* data, int dataSize) const{
	if(socket->send(data, dataSize)!=dataSize){
		throw runtime_error("Error sending NetworkMessage");
	}
}

// =====================================================
//	class NetworkMessageIntro
// =====================================================

NetworkMessageIntro::NetworkMessageIntro(){
	data.messageType= -1;
	data.playerIndex= -1;
}

NetworkMessageIntro::NetworkMessageIntro(const string &versionString, const string &name, int playerIndex, bool resumeSaved){
	data.messageType=nmtIntro;
	data.versionString= versionString;
	data.name= name;
	data.playerIndex= static_cast<int16>(playerIndex);
	data.resumeSaved = resumeSaved ? 1 : 0;
}

bool NetworkMessageIntro::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageIntro::send(Socket* socket) const{
	assert(data.messageType==nmtIntro);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageReady
// =====================================================

NetworkMessageReady::NetworkMessageReady(){
	data.messageType= nmtReady;
}

NetworkMessageReady::NetworkMessageReady(int32 checksum){
	data.messageType= nmtReady;
	data.checksum= checksum;
}

bool NetworkMessageReady::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageReady::send(Socket* socket) const{
	assert(data.messageType==nmtReady);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageLaunch
// =====================================================

NetworkMessageLaunch::NetworkMessageLaunch(){
	data.messageType=-1;
}

NetworkMessageLaunch::NetworkMessageLaunch(const GameSettings *gameSettings){
	data.messageType=nmtLaunch;

	data.description= gameSettings->getDescription();
	data.mapPath= gameSettings->getMapPath();
	data.tilesetPath= gameSettings->getTilesetPath();
	data.techPath= gameSettings->getTechPath();
	data.factionCount= gameSettings->getFactionCount();
	data.thisFactionIndex= gameSettings->getThisFactionIndex();

	for(int i= 0; i<data.factionCount; ++i){
		data.factionTypeNames[i]= gameSettings->getFactionTypeName(i);
		data.factionControls[i]= gameSettings->getFactionControl(i);
		data.teams[i]= gameSettings->getTeam(i);
		data.startLocationIndex[i]= gameSettings->getStartLocationIndex(i);
	}
}

void NetworkMessageLaunch::buildGameSettings(GameSettings *gameSettings) const{
	gameSettings->setDescription(data.description.getString());
	gameSettings->setMapPath(data.mapPath.getString());
	gameSettings->setTilesetPath(data.tilesetPath.getString());
	gameSettings->setTechPath(data.techPath.getString());
	gameSettings->setFactionCount(data.factionCount);
	gameSettings->setThisFactionIndex(data.thisFactionIndex);

	for(int i= 0; i<data.factionCount; ++i){
		gameSettings->setFactionTypeName(i, data.factionTypeNames[i].getString());
		gameSettings->setFactionControl(i, static_cast<ControlType>(data.factionControls[i]));
		gameSettings->setTeam(i, data.teams[i]);
		gameSettings->setStartLocationIndex(i, data.startLocationIndex[i]);
	}
}

bool NetworkMessageLaunch::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageLaunch::send(Socket* socket) const{
	assert(data.messageType==nmtLaunch);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageCommandList
// =====================================================

NetworkMessageCommandList::NetworkMessageCommandList(int32 frameCount){
	data.messageType= nmtCommandList;
	data.frameCount= frameCount;
	data.commandCount= 0;
}

bool NetworkMessageCommandList::addCommand(const NetworkCommand* networkCommand){
	if(data.commandCount<maxCommandCount){
		data.commands[static_cast<int>(data.commandCount)]= *networkCommand;
		data.commandCount++;
		return true;
	}
	return false;
}

bool NetworkMessageCommandList::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageCommandList::send(Socket* socket) const{
	assert(data.messageType==nmtCommandList);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageText
// =====================================================

NetworkMessageText::NetworkMessageText(const string &text, const string &sender, int teamIndex){
	data.messageType= nmtText;
	data.text= text;
	data.sender= sender;
	data.teamIndex= teamIndex;
}

bool NetworkMessageText::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageText::send(Socket* socket) const{
	assert(data.messageType==nmtText);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageQuit
// =====================================================

NetworkMessageQuit::NetworkMessageQuit(){
	data.messageType= nmtQuit;
}

bool NetworkMessageQuit::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageQuit::send(Socket* socket) const{
	assert(data.messageType==nmtQuit);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageFileFragment
// =====================================================

NetworkMessageFileHeader::NetworkMessageFileHeader(const string &name, bool compressed) {
	data.messageType = nmtFileHeader;
	data.name= name;
	data.compressed = compressed;
}

bool NetworkMessageFileHeader::receive(Socket* socket) {
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageFileHeader::send(Socket* socket) const {
	assert(data.messageType == nmtFileHeader);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageFileFragment
// =====================================================

NetworkMessageFileFragment::NetworkMessageFileFragment(char *data, size_t size, int seq, bool last) {
	assert(size <= sizeof(this->data.data));
	this->data.messageType = nmtFileFragment;
	memcpy(this->data.data, data, size);
	this->data.size = size;
	this->data.seq = seq;
	this->data.last = last;
}

bool NetworkMessageFileFragment::receive(Socket* socket) {
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageFileFragment::send(Socket* socket) const {
	assert(data.messageType == nmtFileFragment);
	NetworkMessage::send(socket, &data, sizeof(data));
}


}}//end namespace
