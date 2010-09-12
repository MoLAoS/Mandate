// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_CONNECTIONSLOT_H_
#define _GLEST_GAME_CONNECTIONSLOT_H_

#include "socket.h"
#include "network_interface.h"

using Shared::Platform::ServerSocket;
using Shared::Platform::Socket;

namespace Glest { namespace Net {

class ServerInterface;

// =====================================================
//	class ConnectionSlot
// =====================================================

class ConnectionSlot: public NetworkConnection {
private:
	ServerInterface* serverInterface;
	Socket* socket;
	int playerIndex;
	bool ready;

public:
	ConnectionSlot(ServerInterface* serverInterface, int playerIndex);
	~ConnectionSlot();

	virtual void update();

	void setReady()					{ready = true;}
	int getPlayerIndex() const		{return playerIndex;}
	bool isReady() const			{return ready;}
	string getName() const			{return getRemotePlayerName();}

//protected:
	virtual Socket* getSocket()					{return socket;}
	virtual const Socket* getSocket() const		{return socket;}

private:
	void close();
};

}}//end namespace

#endif
