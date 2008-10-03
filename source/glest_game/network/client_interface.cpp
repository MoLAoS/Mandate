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

#include "client_interface.h"

#include <stdexcept>
#include <cassert>

#include "platform_util.h"
#include "game_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest{ namespace Game{


ClientInterface::FileReceiver::FileReceiver(const NetworkMessageFileHeader &msg, const string &outdir) :
		name(outdir + "/" + msg.getName()),
		out(name.c_str(), ios::binary | ios::out | ios::trunc) {
	if(out.bad()) {
		throw runtime_error("Failed to open new file for output: " + msg.getName());
	}
	compressed = msg.isCompressed();
	finished = false;
	nextseq = 0;
}

ClientInterface::FileReceiver::~FileReceiver() {
	//inflateEnd(&z);
}

bool ClientInterface::FileReceiver::processFragment(const NetworkMessageFileFragment &msg) {
    int zstatus;

	assert(!finished);
	if(finished) {
		throw runtime_error(string("Received file fragment after download of file ")
				+ name + " was already completed.");
	}

	if(!compressed) {
		out.write(msg.getData(), msg.getDataSize());
		if(out.bad()) {
			throw runtime_error("Error while writing file " + name);
		}
	}

	if(nextseq == 0){
		z.zalloc = Z_NULL;
		z.zfree = Z_NULL;
		z.opaque = Z_NULL;
		z.avail_in = 0;
		z.next_in = Z_NULL;

		if(inflateInit(&z) != Z_OK) {
			throw runtime_error(string("Failed to initialize zstream: ") + z.msg);
		}
	}

	if(nextseq++ != msg.getSeq()) {
		throw runtime_error("File fragments arrived out of sequence, which isn't "
				"supposed to happen with stream sockets.  Did somebody change "
				"the socket implementation to datagrams?");
	}

	z.avail_in = msg.getDataSize();
	z.next_in = (Bytef*)msg.getData();
	do {
		z.avail_out = sizeof(buf);
		z.next_out = (Bytef*)buf;
		zstatus = inflate(&z, Z_NO_FLUSH);
		assert(zstatus != Z_STREAM_ERROR);	// state not clobbered
		switch (zstatus) {
		case Z_NEED_DICT:
			zstatus = Z_DATA_ERROR;
			// intentional fall-through
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			throw runtime_error(string("error in zstream: ") + z.msg);
		}
		out.write(buf, sizeof(buf) - z.avail_out);
		if(out.bad()) {
			throw runtime_error("Error while writing file " + name);
		}
	} while (z.avail_out == 0);

	if(msg.isLast() && zstatus != Z_STREAM_END) {
		throw runtime_error("Unexpected end of zstream data.");
	}

	if(msg.isLast() || zstatus == Z_STREAM_END) {
		finished = true;
		inflateEnd(&z);
	}

	return msg.isLast();
}


// =====================================================
//	class ClientInterface
// =====================================================

const int ClientInterface::messageWaitTimeout= 10000;	//10 seconds
const int ClientInterface::waitSleepTime= 50;

ClientInterface::ClientInterface(){
	clientSocket= NULL;
	launchGame= false;
	introDone= false;
	playerIndex= -1;
	fileReceiver = NULL;
	savedGameFile = "";
}

ClientInterface::~ClientInterface(){
	delete clientSocket;
	if(fileReceiver) {
		delete fileReceiver;
	}
}

void ClientInterface::connect(const Ip &ip, int port){
	delete clientSocket;
	clientSocket= new ClientSocket();
	clientSocket->setBlock(false);
	clientSocket->connect(ip, port);
}

void ClientInterface::reset(){
	delete clientSocket;
	clientSocket= NULL;
}

void ClientInterface::update(){
	NetworkMessageCommandList networkMessageCommandList;

	//send as many commands as we can
	while(!requestedCommands.empty()){
		if(networkMessageCommandList.addCommand(&requestedCommands.back())){
			requestedCommands.pop_back();
		}
		else{
			break;
		}
	}
	if(networkMessageCommandList.getCommandCount()>0){
		sendMessage(&networkMessageCommandList);
	}

	//clear chat variables
	chatText.clear();
	chatSender.clear();
	chatTeamIndex= -1;
}

void ClientInterface::updateLobby(){
	NetworkMessageType networkMessageType= getNextMessageType();

	switch(networkMessageType){
		case nmtInvalid:
			break;

		case nmtIntro:{
			NetworkMessageIntro msg;

			if(receiveMessage(&msg)){

				//check consistency
				if(Config::getInstance().getNetworkConsistencyChecks() && msg.getVersionString() != getNetworkVersionString()){
					throw runtime_error("Server and client versions do not match ("
							+ msg.getVersionString() + ").");
				}

				//send intro message
				NetworkMessageIntro sendNetworkMessageIntro(getNetworkVersionString(), getHostName(), -1, false);

				playerIndex= msg.getPlayerIndex();
				serverName= msg.getName();
				sendMessage(&sendNetworkMessageIntro);

				assert(playerIndex>=0 && playerIndex<GameConstants::maxPlayers);
				introDone= true;
			}
		}
		break;

		case nmtFileHeader:{
			NetworkMessageFileHeader msg;

			if(receiveMessage(&msg)){
				if(fileReceiver) {
					throw runtime_error("Can't receive file from server because I'm already receiving one.");
				}

				if(savedGameFile != "") {
					throw runtime_error("Saved game file already downloaded and server tried to send me another.");
				}

				if(strchr(msg.getName().c_str(), '/') || strchr(msg.getName().c_str(), '\\')) {
					throw runtime_error("Server tried to send a file name with a path component, which is not allowed.");
				}

				Shared::Platform::mkdir("incoming", true);
				fileReceiver = new FileReceiver(msg, "incoming");
			}
		}
		break;

		case nmtFileFragment:{
			NetworkMessageFileFragment msg;

			if(receiveMessage(&msg)){
				if(!fileReceiver) {
					throw runtime_error("Recieved file fragment, but did not get header.");
				}
				if(fileReceiver->processFragment(msg)) {
					savedGameFile = fileReceiver->getName();
					delete fileReceiver;
					fileReceiver = NULL;
				}
			}
		}
		break;

		case nmtLaunch:{
			NetworkMessageLaunch msg;

			if(receiveMessage(&msg)){
				msg.buildGameSettings(&gameSettings);

				//replace server player by network
				for(int i= 0; i<gameSettings.getFactionCount(); ++i){

					//replace by network
					if(gameSettings.getFactionControl(i)==ctHuman){
						gameSettings.setFactionControl(i, ctNetwork);
					}
				}
				gameSettings.setFactionControl(playerIndex, ctHuman);
				gameSettings.setThisFactionIndex(playerIndex);
				launchGame= true;
			}
		}
		break;

		default:
			throw runtime_error("Unexpected network message: " + intToStr(networkMessageType));
	}
}

void ClientInterface::updateKeyframe(int frameCount){

	bool done= false;

	while(!done){
		//wait for the next message
		waitForMessage();

		//check we have an expected message
		NetworkMessageType networkMessageType= getNextMessageType();

		switch(networkMessageType){
			case nmtCommandList:{
				//make sure we read the message
				NetworkMessageCommandList networkMessageCommandList;
				while(!receiveMessage(&networkMessageCommandList)){
					sleep(waitSleepTime);
				}

				//check that we are in the right frame
				if(networkMessageCommandList.getFrameCount()!=frameCount){
					throw runtime_error("Network synchronization error, frame counts do not match");
				}

				// give all commands
				for(int i= 0; i<networkMessageCommandList.getCommandCount(); ++i){
					pendingCommands.push_back(*networkMessageCommandList.getCommand(i));
				}

				done= true;
			}
			break;

			case nmtQuit:{
				NetworkMessageQuit networkMessageQuit;
				if(receiveMessage(&networkMessageQuit)){
					quit= true;
				}
				done= true;
			}
			break;

			case nmtText:{
				NetworkMessageText networkMessageText;
				if(receiveMessage(&networkMessageText)){
					chatText= networkMessageText.getText();
					chatSender= networkMessageText.getSender();
					chatTeamIndex= networkMessageText.getTeamIndex();
				}
			}
			break;

			default:
				throw runtime_error("Unexpected message in client interface: " + intToStr(networkMessageType));
		}
	}
}

void ClientInterface::waitUntilReady(Checksum* checksum){

	NetworkMessageReady networkMessageReady;
	Chrono chrono;

	chrono.start();

	//send ready message
	sendMessage(&networkMessageReady);

	//wait until we get a ready message from the server
	while(true){

		NetworkMessageType networkMessageType= getNextMessageType();

		if(networkMessageType==nmtReady){
			if(receiveMessage(&networkMessageReady)){
				break;
			}
		}
		else if(networkMessageType==nmtInvalid){
			if(chrono.getMillis()>readyWaitTimeout){
				throw runtime_error("Timeout waiting for server");
			}
		}
		else{
			throw runtime_error("Unexpected network message: " + intToStr(networkMessageType) );
		}

		// sleep a bit
		sleep(waitSleepTime);
	}

	//check checksum
	if(Config::getInstance().getNetworkConsistencyChecks() && networkMessageReady.getChecksum()!=checksum->getSum()){
		throw runtime_error("Checksum error, you don't have the same data as the server");
	}

	//delay the start a bit, so clients have nore room to get messages
	sleep(GameConstants::networkExtraLatency);
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex){
	NetworkMessageText networkMessageText(text, getHostName(), teamIndex);
	sendMessage(&networkMessageText);
}

string ClientInterface::getNetworkStatus() const{
	return Lang::getInstance().get("Server") + ": " + serverName;
}

void ClientInterface::waitForMessage(){
	Chrono chrono;

	chrono.start();

	while(getNextMessageType()==nmtInvalid){

		if(!isConnected()){
			throw runtime_error("Disconnected");
		}

		if(chrono.getMillis()>messageWaitTimeout){
			throw runtime_error("Timeout waiting for message");
		}

		sleep(waitSleepTime);
	}
}

}}//end namespace
