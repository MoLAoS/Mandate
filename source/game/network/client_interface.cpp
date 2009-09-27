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
const int ClientInterface::waitSleepTime= 5;

ClientInterface::ClientInterface(){
	clientSocket = NULL;
	launchGame = false;
	introDone = false;
	playerIndex = -1;
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
	clientSocket = new ClientSocket();
	clientSocket->connect(ip, port);
	clientSocket->setBlock(false);
}

void ClientInterface::reset(){
	delete clientSocket;
	clientSocket = NULL;
}

void ClientInterface::update() {
	NetworkMessageCommandList networkMessageCommandList(-1);

	//send as many commands as we can
	while(!requestedCommands.empty()
			&& networkMessageCommandList.addCommand(requestedCommands.front())) {
		requestedCommands.erase(requestedCommands.begin());
	}

	if(networkMessageCommandList.getCommandCount() > 0) {
		send(&networkMessageCommandList);
		flush();
	}

	//clear chat variables
	chatText.clear();
	chatSender.clear();
	chatTeamIndex = -1;

	if(isConnected()) {
		receive();
		MsgQueue newQ;

		// pull out updates for immediate processing
		for(MsgQueue::iterator i = q.begin(); i != q.end(); ++i) {
			if((*i)->getType() == nmtUpdate) {
				updates.push_back((NetworkMessageUpdate*)*i);
			} else {
				newQ.push_back(*i);
			}
		}
		q.swap(newQ);
		NetworkStatus::update();
	}
	flush();
}

void ClientInterface::sendUpdateRequests() {
	if(isConnected() && updateRequests.size()) {
		NetworkMessageUpdateRequest msg;
		UnitReferences::iterator i;
		for(i = updateRequests.begin(); i != updateRequests.end(); ++i) {
			msg.addUnit(*i, false);
		}
		for(i = fullUpdateRequests.begin(); i != fullUpdateRequests.end(); ++i) {
			msg.addUnit(*i, true);
		}
		msg.writeXml();
		msg.compress();
		send(&msg, true);
		updateRequests.clear();
	}
}


void ClientInterface::updateLobby(){
	NetworkMessage *genericMsg= nextMsg();
	if(!genericMsg) {
		return;
	}

	try {
		switch(genericMsg->getType()){
			case nmtIntro: {
				NetworkMessageIntro *msg = (NetworkMessageIntro *)genericMsg;

				//check consistency
				if(Config::getInstance().getNetConsistencyChecks() && msg->getVersionString() != getNetworkVersionString()) {
					throw SocketException("Server and client versions do not match (" + msg->getVersionString() + ").");
				}

				//send intro message
				NetworkMessageIntro sendNetworkMessageIntro(getNetworkVersionString(),
						getHostName(), Config::getInstance().getNetPlayerName(), -1, false);

				playerIndex = msg->getPlayerIndex();
				setRemoteNames(msg->getHostName(), msg->getPlayerName());
				send(&sendNetworkMessageIntro);

				assert(playerIndex >= 0 && playerIndex < GameConstants::maxPlayers);
				introDone = true;
			}
			break;

			case nmtFileHeader: {
				NetworkMessageFileHeader *msg = (NetworkMessageFileHeader *)genericMsg;

				if(fileReceiver) {
					throw runtime_error("Can't receive file from server because I'm already receiving one.");
				}

				if(savedGameFile != "") {
					throw runtime_error("Saved game file already downloaded and server tried to send me another.");
				}

				if(strchr(msg->getName().c_str(), '/') || strchr(msg->getName().c_str(), '\\')) {
					throw SocketException("Server tried to send a file name with a path component, which is not allowed.");
				}

				Shared::Platform::mkdir("incoming", true);
				fileReceiver = new FileReceiver(*msg, "incoming");
			}
			break;

			case nmtFileFragment: {
				NetworkMessageFileFragment *msg = (NetworkMessageFileFragment*)genericMsg;

				if(!fileReceiver) {
					throw runtime_error("Recieved file fragment, but did not get header.");
				}
				if(fileReceiver->processFragment(*msg)) {
					savedGameFile = fileReceiver->getName();
					delete fileReceiver;
					fileReceiver = NULL;
				}
			}
			break;

			case nmtLaunch: {
				NetworkMessageLaunch *msg = (NetworkMessageLaunch*)genericMsg;

				msg->buildGameSettings(&gameSettings);

				//replace server player by network
				for(int i = 0; i < gameSettings.getFactionCount(); ++i){

					//replace by network
					if(gameSettings.getFactionControl(i) == ctHuman){
						gameSettings.setFactionControl(i, ctNetwork);
					}
				}
				gameSettings.setFactionControl(playerIndex, ctHuman);
				gameSettings.setThisFactionIndex(playerIndex);
				launchGame= true;
			}
			break;

			default:
				throw SocketException("Unexpected network message: " + intToStr(genericMsg->getType()));
		}
		flush();
	} catch (runtime_error &e) {
		delete genericMsg;
		throw e;
	}
	delete genericMsg;
}

void ClientInterface::updateKeyframe(int frameCount){
	bool done = false;

	while(!done) {
		//wait for the next message
		NetworkMessage *genericMsg = waitForMessage();

		try {
			switch(genericMsg->getType()) {
				case nmtCommandList: {
					//make sure we read the message
					NetworkMessageCommandList *msg = (NetworkMessageCommandList*)genericMsg;

					//check that we are in the right frame
					if(msg->getFrameCount() != frameCount){
						throw SocketException("Network synchronization error, frame counts do not match");
					}

					// give all commands
					for(int i= 0; i < msg->getCommandCount(); ++i){
						pendingCommands.push_back(msg->getCommand(i));
					}

					done = true;
					delete msg;
				}
				break;

				case nmtQuit: {
					quit = true;
					done=  true;
					delete genericMsg;
				}
				break;

				case nmtText:{
					NetworkMessageText *msg = (NetworkMessageText*)genericMsg;
					chatText = msg->getText();
					chatSender = msg->getSender();
					chatTeamIndex = msg->getTeamIndex();
					delete msg;
				}
				break;

				case nmtUpdate: {
					updates.push_back((NetworkMessageUpdate*)genericMsg);
				}
				break;

				default:
					throw SocketException("Unexpected message in client interface: " + intToStr(genericMsg->getType()));
			}
			flush();
		} catch(runtime_error &e) {
			delete genericMsg;
			throw e;
		}
	}
}

void ClientInterface::waitUntilReady(Checksum &checksum){
	NetworkMessage *msg = NULL;
	NetworkMessageReady networkMessageReady(checksum.getSum());
	Chrono chrono;

	chrono.start();

	//send ready message
	send(&networkMessageReady);

	try {
		//wait until we get a ready message from the server
		while(!(msg = nextMsg())) {
			if(chrono.getMillis() > readyWaitTimeout){
				throw SocketException("Timeout waiting for server");
			}

			// sleep a bit
			sleep(waitSleepTime);
		}

		if(msg->getType() != nmtReady) {
			SocketException("Unexpected network message: " + intToStr(msg->getType()));
		}

		//check checksum
		if(Config::getInstance().getNetConsistencyChecks()
				&& ((NetworkMessageReady*)msg)->getChecksum() != checksum.getSum()) {
			throw SocketException("Checksum error, you don't have the same data as the server");
		}
		flush();
	} catch (runtime_error &e) {
		if(msg) {
			delete msg;
		}
		throw e;
	}

	//delay the start a bit, so clients have nore room to get messages
	sleep(GameConstants::networkExtraLatency);
	delete msg;
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex) {
	NetworkMessageText networkMessageText(text, Config::getInstance().getNetPlayerName(), teamIndex);
	send(&networkMessageText);
}

string ClientInterface::getStatus() const {
	return getRemotePlayerName() + ": " + NetworkStatus::getStatus();
}

NetworkMessage *ClientInterface::waitForMessage() {
	NetworkMessage *msg = NULL;
	Chrono chrono;

	chrono.start();

	while(!(msg = nextMsg())) {
		if(!isConnected()){
			throw SocketException("Disconnected");
		}

		if(chrono.getMillis()>messageWaitTimeout){
			throw SocketException("Timeout waiting for message");
		}

		sleep(waitSleepTime);
	}
	return msg;
}

void ClientInterface::requestCommand(Command *command) {
	if(!command->isAuto()) {
		requestedCommands.push_back(new Command(*command));
	}
	pendingCommands.push_back(command);
}

}}//end namespace
