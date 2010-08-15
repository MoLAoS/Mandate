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
// 	class MenuStateJoinGame
// ===============================

class MenuStateJoinGame: public MenuState {
	friend class ConnectThread;

	WRAPPED_ENUM( Transition, RETURN, PLAY );

private:
	static const int newServerIndex;
	static const string serverFileName;

private:
	Panel::Ptr			m_connectPanel;
	Panel::Ptr			m_gameSetupPanel;
	MessageDialog::Ptr	m_messageBox;

	TextBox::Ptr		m_serverTextBox;
	StaticText::Ptr		m_connectLabel;

	ConnectThread*		m_connectThread;

	Transition			m_targetTansition;

	Mutex				m_mutex;

	void buildConnectPanel();
	void buildGameSetupPanel();

	void onReturn(Button::Ptr);
	void onConnect(Button::Ptr);
	void onSearchForGame(Button::Ptr);

	void onServerSelected(ListBase::Ptr);

	void onCancelConnect(MessageDialog::Ptr);
	void onDisconnect(MessageDialog::Ptr);

	void connectThreadDone(ConnectResult result);

	bool connected;
	int playerIndex;
	Properties servers;

public:
	MenuStateJoinGame(Program &program, MainMenu *mainMenu, bool connect = false, Ip serverIp = Ip());

	void update();

	MenuStates getIndex() const { return MenuStates::JOIN_GAME; }
};

}}//end namespace

#endif
