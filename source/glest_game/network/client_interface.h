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

#ifndef _GLEST_GAME_CLIENTINTERFACE_H_
#define _GLEST_GAME_CLIENTINTERFACE_H_

#include <vector>
#include <zlib.h>

#include "network_interface.h"
#include "game_settings.h"

#include "socket.h"

using Shared::Platform::Ip;
using Shared::Platform::ClientSocket;
using std::vector;

namespace Glest{ namespace Game{

// =====================================================
//	class ClientInterface
// =====================================================

class ClientInterface: public GameNetworkInterface{
private:
	class FileReceiver {
		string name;
		ofstream out;
		z_stream z;
		char buf[4096];
		bool compressed;
		bool finished;
		int nextseq;

	public:
		FileReceiver(const NetworkMessageFileHeader &msg, const string &outdir);
		~FileReceiver();

		/** @return true when file download is complete. */
		bool processFragment(const NetworkMessageFileFragment &msg);
		const string &getName()	const		{return name;}
	};

	static const int messageWaitTimeout;
	static const int waitSleepTime;

	ClientSocket *clientSocket;
	GameSettings gameSettings;
	string serverName;
	bool introDone;
	bool launchGame;
	int playerIndex;
	FileReceiver *fileReceiver;
	string savedGameFile;

public:
	ClientInterface();
	virtual ~ClientInterface();

	virtual Socket* getSocket()					{return clientSocket;}
	virtual const Socket* getSocket() const		{return clientSocket;}

	//message processing
	virtual void update();
	virtual void updateLobby();
	virtual void updateKeyframe(int frameCount);
	virtual void waitUntilReady(Checksum* checksum);

	// message sending
	virtual void sendTextMessage(const string &text, int teamIndex);
	virtual void quitGame(){}

	//misc
	virtual string getNetworkStatus() const;

	//accessors
	string getServerName() const				{return serverName;}
	bool getLaunchGame() const					{return launchGame;}
	bool getIntroDone() const					{return introDone;}
	int getPlayerIndex() const					{return playerIndex;}
	const GameSettings *getGameSettings() const	{return &gameSettings;}
	const string &getSavedGameFile() const		{return savedGameFile;}

	void connect(const Ip &ip, int port);
	void reset();

private:
	void waitForMessage();
};

}}//end namespace

#endif
