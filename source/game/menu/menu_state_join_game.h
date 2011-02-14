// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//				  2010 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATEJOINGAME_H_
#define _GLEST_GAME_MENUSTATEJOINGAME_H_

#include "properties.h"
#include "main_menu.h"
#include "compound_widgets.h"
#include "thread.h"

using Shared::Util::Properties;

namespace Glest { namespace Menu {

class IntroMessage;
class MenuStateJoinGame;

WRAPPED_ENUM( ConnectResult, CANCELLED, FAILED, SUCCESS );

// ===============================
// 	class ConnectThread
// ===============================

class ConnectThread : public Thread {
private:
	MenuStateJoinGame &m_menu;
	Ip m_server;
	bool m_connecting;
	ConnectResult m_result;
	string m_errorMsg;
	Mutex m_mutex;

public:
	ConnectThread(MenuStateJoinGame &menu, Ip serverIp);
	void execute();
	void cancel();
	string getErrorMsg();
};

// ===============================
// 	class FindServerThread
// ===============================
/** A thread to listen for game announcements */

class FindServerThread : public Thread {
private:
	MenuStateJoinGame&	m_menu;
	ClientSocket		m_socket;

public:
	FindServerThread(MenuStateJoinGame &menu) : m_menu(menu) {
		start();
	}

	virtual void execute();

	void stop() {
		m_socket.disconnectUdp();
	}
};

// ===============================
// 	class MenuStateJoinGame
// ===============================

class MenuStateJoinGame: public MenuState {
private:
	WRAPPED_ENUM( Transition, RETURN, PLAY );
	static const string serverFileName;

private:
	CellStrip        *m_connectPanel;
	Panel            *m_gameSetupPanel;
	MessageDialog    *m_messageBox;

	DropList         *m_historyList;
	TextBox          *m_serverTextBox;
	StaticText       *m_connectLabel;

	ConnectThread    *m_connectThread;
	FindServerThread *m_findServerThread;

	Transition        m_targetTansition;

	Mutex             m_connectMutex;
	Mutex             m_findServerMutex;

private:
	void buildConnectPanel();
	void buildGameSetupPanel();

	void onReturn(Button*);
	void onConnect(Button*);
	void onSearchForGame(Button*);

	void onServerSelected(ListBase*);
	void onTextModified(TextBox*);

	void onCancelConnect(BasicDialog*);
	void onDisconnect(BasicDialog*);
	void onCancelSearch(BasicDialog*);

	bool connected;
	int playerIndex;
	Properties servers;

public:
	MenuStateJoinGame(Program &program, MainMenu *mainMenu, bool connect = false, Ip serverIp = Ip());

	void update();

	MenuStates getIndex() const { return MenuStates::JOIN_GAME; }

	void connectThreadDone(ConnectResult result);
	void foundServer(Ip ip);
};

}}//end namespace

#endif
