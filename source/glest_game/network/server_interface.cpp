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

#include "server_interface.h"

#include <cassert>
#include <stdexcept>
#include <fstream>
#include <zlib.h>

#include "platform_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "client_interface.h"


using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class ServerInterface
// =====================================================

ServerInterface::ServerInterface(){
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		slots[i]= NULL;
	}
	serverSocket.setBlock(false);
	serverSocket.bind(GameConstants::serverPort);
}

ServerInterface::~ServerInterface(){
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		delete slots[i];
	}
}

void ServerInterface::addSlot(int playerIndex){
	assert(playerIndex>=0 && playerIndex<GameConstants::maxPlayers);

	delete slots[playerIndex];
	slots[playerIndex]= new ConnectionSlot(this, playerIndex, false);
	updateListen();
}

void ServerInterface::removeSlot(int playerIndex){
	delete slots[playerIndex];
	slots[playerIndex]= NULL;
	updateListen();
}

ConnectionSlot* ServerInterface::getSlot(int playerIndex){
	return slots[playerIndex];
}

int ServerInterface::getConnectedSlotCount(){
	int connectedSlotCount= 0;

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(slots[i]!= NULL){
			++connectedSlotCount;
		}
	}
	return connectedSlotCount;
}

void ServerInterface::update(){

	//update all slots
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(slots[i]!= NULL){
			slots[i]->update();
		}
	}

	//process text messages
	chatText.clear();
	chatSender.clear();
	chatTeamIndex= -1;

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		ConnectionSlot* connectionSlot= slots[i];

		if(connectionSlot!= NULL){
			if(connectionSlot->isConnected()){
				if(connectionSlot->getNextMessageType()==nmtText){
					NetworkMessageText networkMessageText;
					if(connectionSlot->receiveMessage(&networkMessageText)){
						broadcastMessage(&networkMessageText, i);
						chatText= networkMessageText.getText();
						chatSender= networkMessageText.getSender();
						chatTeamIndex= networkMessageText.getTeamIndex();
						break;
					}
				}
			}
		}
	}
}

void ServerInterface::updateKeyframe(int frameCount){

	NetworkMessageCommandList networkMessageCommandList(frameCount);

	//build command list, remove commands from requested and add to pending
	for(int i = 0; i < requestedCommands.size(); i++){
		if(networkMessageCommandList.addCommand(&requestedCommands[i])){
			pendingCommands.push_back(requestedCommands[i]);
		}
		else{
			break;
		}
	}
	requestedCommands.clear();

	//broadcast commands
	broadcastMessage(&networkMessageCommandList);
}

void ServerInterface::waitUntilReady(Checksum* checksum){

	Chrono chrono;
	bool allReady= false;

	chrono.start();

	//wait until we get a ready message from all clients
	while(!allReady){

		allReady= true;
		for(int i= 0; i<GameConstants::maxPlayers; ++i){
			ConnectionSlot* connectionSlot= slots[i];

			if(connectionSlot!=NULL){
				if(!connectionSlot->isReady()){
					NetworkMessageType networkMessageType= connectionSlot->getNextMessageType();
					NetworkMessageReady networkMessageReady;

					if(networkMessageType==nmtReady && connectionSlot->receiveMessage(&networkMessageReady)){
						connectionSlot->setReady();
					}
					else if(networkMessageType!=nmtInvalid){
						throw runtime_error("Unexpected network message: " + intToStr(networkMessageType));
					}

					allReady= false;
				}
			}
		}

		//check for timeout
		if(chrono.getMillis()>readyWaitTimeout){
			throw runtime_error("Timeout waiting for clients");
		}
	}

	//send ready message after, so clients start delayed
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		NetworkMessageReady networkMessageReady(checksum->getSum());
		ConnectionSlot* connectionSlot= slots[i];

		if(connectionSlot!=NULL){
			connectionSlot->sendMessage(&networkMessageReady);
		}
	}
}

void ServerInterface::sendTextMessage(const string &text, int teamIndex){
	NetworkMessageText networkMessageText(text, getHostName(), teamIndex);
	broadcastMessage(&networkMessageText);
}

void ServerInterface::quitGame(){
	NetworkMessageQuit networkMessageQuit;
	broadcastMessage(&networkMessageQuit);
}

string ServerInterface::getNetworkStatus() const{
	Lang &lang= Lang::getInstance();
	string str;

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		ConnectionSlot* connectionSlot= slots[i];

		str+= intToStr(i)+ ": ";

		if(connectionSlot!= NULL){
			if(connectionSlot->isConnected()){
				str+= connectionSlot->getName();
			}
		}
		else
		{
			str+= lang.get("NotConnected");
		}

		str+= '\n';
	}
	return str;
}

void ServerInterface::launchGame(const GameSettings* gameSettings, const string savedGameFile){
	NetworkMessageLaunch networkMessageLaunch(gameSettings);
	if(savedGameFile != "") {
		sendFile(savedGameFile, "resumed_network_game.sav", true);
	}
	broadcastMessage(&networkMessageLaunch);
}

void ServerInterface::sendFile(const string path, const string remoteName, bool compress) {
	size_t fileSize;
	ifstream in;
	int zstatus;

	fileSize = getFileSize(path);

	in.open(path.c_str(), ios_base::in | ios_base::binary);
	if(in.fail()) {
		throw runtime_error("Failed to open file " + path);
	}

	char *inbuf = new char[fileSize];
	in.read(inbuf, fileSize);
	assert(in.gcount() == fileSize && in.read((char*)&zstatus, 1).eof());
	char *outbuf;
	uLongf outbuflen;
	if(compress) {
		outbuflen = (float)fileSize * 0.1f + 12;
		outbuf = new char[outbuflen];
		zstatus = ::compress((Bytef *)outbuf, &outbuflen, (const Bytef *)inbuf, fileSize);
		if(zstatus != Z_OK) {
			throw runtime_error("Call to compress failed for some unknown fucking reason.");
		}
	} else {
		outbuf = inbuf;
		outbuflen = fileSize;
	}

	NetworkMessageFileHeader headerMsg(remoteName, compress);
	broadcastMessage(&headerMsg);
	size_t i;
	size_t off;
	for(i = 0, off = 0; off < outbuflen; ++i, off += NetworkMessageFileFragment::bufSize) {
		if(outbuflen - off > NetworkMessageFileFragment::bufSize) {
			NetworkMessageFileFragment msg(&outbuf[off], NetworkMessageFileFragment::bufSize, i, false);
			broadcastMessage(&msg);
		} else {
			NetworkMessageFileFragment msg(&outbuf[off], outbuflen - off, i, true);
			broadcastMessage(&msg);
		}
	}
	delete[] inbuf;
	if(compress) {
		delete[] outbuf;
	}

/*	FUCK FUCK FUCK FUCK FUUUUCK!
	int outready = 0;
	int flush;
	z_stream z;
//	unsigned char inbuf[32768];
//	char outbuf[NetworkMessageFileFragment::bufSize];

	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;
	z.next_in = Z_NULL;
	z.avail_in = 0;

	NetworkMessageFileHeader headerMsg(remoteName);
	broadcastMessage(&headerMsg);

	if(deflateInit(&z, Z_BEST_COMPRESSION) != Z_OK) {
		throw runtime_error(string("Error initializing zstream: ") + z.msg);
	}

	do {
		in.read((char*)inbuf, sizeof(inbuf));
		z.next_in = (Bytef*)inbuf;
		z.avail_in = in.gcount();
		if(in.bad()) {
			deflateEnd(&z);
			throw runtime_error("Error while reading file " + path);
		}
		flush = in.eof() ? Z_FINISH : Z_NO_FLUSH;

		do {
			z.next_out = (Bytef*)outbuf;
			z.avail_out = sizeof(outbuf);

			zstatus = deflate(&z, flush);
			assert(zstatus != Z_STREAM_ERROR);

			outready = sizeof(outbuf) - z.avail_out;
			if(outready) {
				NetworkMessageFileFragment msg(outbuf, outready, false);
				broadcastMessage(&msg);
			}
		} while (!z.avail_out);
		assert(z.avail_in == 0);

	} while(flush != Z_FINISH);
	assert(zstatus == Z_STREAM_END);

	{
		NetworkMessageFileFragment msg(outbuf, 0, true);
		broadcastMessage(&msg);
	}

	deflateEnd(&z);*/
}

void ServerInterface::broadcastMessage(const NetworkMessage* networkMessage, int excludeSlot){
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		ConnectionSlot* connectionSlot= slots[i];

		if(i!= excludeSlot && connectionSlot!= NULL){
			if(connectionSlot->isConnected()){
				connectionSlot->sendMessage(networkMessage);
			}
			else{
				removeSlot(i);
			}
		}
	}
}

void ServerInterface::updateListen(){
	int openSlotCount= 0;

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(slots[i]!= NULL && !slots[i]->isConnected()){
			++openSlotCount;
		}
	}

	serverSocket.listen(openSlotCount);
}

}}//end namespace
