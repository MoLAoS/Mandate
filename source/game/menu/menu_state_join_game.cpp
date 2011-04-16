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

#include "pch.h"
#include "menu_state_join_game.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "metrics.h"
#include "network_message.h"
#include "client_interface.h"
#include "conversion.h"
#include "game.h"
#include "socket.h"

#include "leak_dumper.h"
#include "logger.h"

using namespace Shared::Util;
using namespace Glest::Net;

namespace Glest { namespace Menu {

// ===============================
// 	class ConnectThread
// ===============================

ConnectThread::ConnectThread(MenuStateJoinGame &menu, Ip serverIp)
		: m_menu(menu)
		, m_server(serverIp)
		, m_connecting(true)
		, m_result(ConnectResult::INVALID) {
	start();
}

void ConnectThread::execute() {
	ClientInterface* clientInterface = g_simInterface.asClientInterface();
	
	try {
		clientInterface->connect(m_server, g_config.getNetServerPort());
		{
			MutexLock lock(m_mutex);
			m_result = ConnectResult::SUCCESS;
			m_connecting = false;
		}
	} catch (exception &e) {
		MutexLock lock(m_mutex);
		m_connecting = false;
		if (m_result == ConnectResult::INVALID) {
			m_result = ConnectResult::FAILED;
		}
		m_errorMsg = e.what();
	}
	m_menu.connectThreadDone(m_result); // will delete 'this', no more code beyond here!
}

void ConnectThread::cancel() {
	MutexLock lock(m_mutex);
	if (m_connecting) {
		g_simInterface.asClientInterface()->reset();
		m_result = ConnectResult::CANCELLED;
	}
}

string ConnectThread::getErrorMsg() {
	return m_errorMsg;
}

// ===============================
// 	class FindServerThread
// ===============================

void FindServerThread::execute() {
	const int MSG_SIZE = 100;
	char msg[MSG_SIZE];

	try {
		Ip serverIp = m_socket.receiveAnnounce(4950, msg, MSG_SIZE); // blocking call @todo fix port
		m_menu.foundServer(serverIp);
	} catch(SocketException &e) {
		// do nothing
	}
}

// ===============================
//  class MenuStateJoinGame
// ===============================

//const int MenuStateJoinGame::newServerIndex = 0;
const string MenuStateJoinGame::serverFileName = "servers.ini";

MenuStateJoinGame::MenuStateJoinGame(Program &program, MainMenu *mainMenu, bool connect, Ip serverIp)
		: MenuState(program, mainMenu)
		, m_messageBox(0) 
		, m_connectThread(0)
		, m_findServerThread(0) {
	if (fileExists(serverFileName)) {
		servers.load(serverFileName);
	} else {
		servers.save(serverFileName);
	}

	program.getSimulationInterface()->changeRole(GameRole::CLIENT);
	connected = false;
	playerIndex = -1;

	buildConnectPanel();
}

void MenuStateJoinGame::buildConnectPanel() {
	const int defWidgetHeight = g_widgetConfig.getDefaultItemHeight();
	const int defCellHeight = defWidgetHeight * 3 / 2;

	Vec2i pos, size(500, 300);
	pos = g_metrics.getScreenDims() / 2 - size / 2;

	m_connectPanel = new CellStrip(static_cast<Container*>(&program), Orientation::VERTICAL, Origin::CENTRE, 4);
	m_connectPanel->setSizeHint(0, SizeHint(-1, defCellHeight)); // def px for recent hosts lbl & list
	m_connectPanel->setSizeHint(1, SizeHint(-1, defCellHeight)); // def px for server lbl & txtBox
	m_connectPanel->setSizeHint(2, SizeHint(-1, defCellHeight)); // def px for connected label
	m_connectPanel->setSizeHint(3, SizeHint(25));     // 25 % of the rest for button panel

	int font = g_widgetConfig.getDefaultFontIndex(FontUsage::MENU);
	int white = g_widgetConfig.getColourIndex(Colour(255u));
//	Font *font = g_widgetConfig.getMenuFont()[FontSize::NORMAL];
	
	Anchors a(Anchor(AnchorType::RIGID, 0)); // fill
	m_connectPanel->setAnchors(a);
	Vec2i pad(45, 45);
	m_connectPanel->setPos(pad);
	m_connectPanel->setSize(Vec2i(g_config.getDisplayWidth() - pad.w * 2, g_config.getDisplayHeight() - pad.h * 2));

	CellStrip *pnl = new CellStrip(m_connectPanel, Orientation::HORIZONTAL, Origin::CENTRE, 2);
	pnl->setCell(0);
	// fill vertical, set 25 % in from left / right edges
	a = Anchors(Anchor(AnchorType::SPRINGY, 25), Anchor(AnchorType::RIGID, 0));
	pnl->setAnchors(a);

	Anchors a2;
	a2.setCentre(true);

	StaticText* historyLabel = new StaticText(pnl, Vec2i(0), Vec2i(200, 34));
	historyLabel->setCell(0);
	historyLabel->setText(g_lang.get("RecentHosts"));
	historyLabel->setAnchors(a2);
	m_historyList = new DropList(pnl, Vec2i(0), Vec2i(300, 34));
	m_historyList->setCell(1);
	m_historyList->setAnchors(a2);

	pnl = new CellStrip(m_connectPanel, Orientation::HORIZONTAL, Origin::CENTRE, 2);
	pnl->setCell(1);
	pnl->setAnchors(a);

	StaticText* serverLabel = new StaticText(pnl, Vec2i(0), Vec2i(6 * defWidgetHeight, defWidgetHeight));
	serverLabel->setCell(0);
	serverLabel->setText(g_lang.get("Server") + " Ip: ");
	serverLabel->setAnchors(a2);
	
	m_serverTextBox = new TextBox(pnl, Vec2i(0), Vec2i(10 * defWidgetHeight, defWidgetHeight));
	m_serverTextBox->setCell(1);
	m_serverTextBox->setText("");
	m_serverTextBox->TextChanged.connect(this, &MenuStateJoinGame::onTextModified);
	m_serverTextBox->setAnchors(a2);

	m_connectLabel = new StaticText(m_connectPanel, Vec2i(0), Vec2i(16 * defWidgetHeight, defWidgetHeight));
	m_connectLabel->setCell(2);
	m_connectLabel->setText(g_lang.get("NotConnected"));
	m_connectLabel->setAnchors(a2);

	a.set(Edge::LEFT, 0, false);
	a.set(Edge::RIGHT, 0, false);
	pnl = new CellStrip(m_connectPanel, Orientation::HORIZONTAL, Origin::CENTRE, 3);
	pnl->setCell(3);
	pnl->setAnchors(a);

	// buttons
	Button* returnButton = new Button(pnl, Vec2i(0), Vec2i(7 * defWidgetHeight, defWidgetHeight));
	returnButton->setCell(0);
	returnButton->setText(g_lang.get("Return"));
	returnButton->Clicked.connect(this, &MenuStateJoinGame::onReturn);
	returnButton->setAnchors(a2);

	Button* connectButton = new Button(pnl, Vec2i(0), Vec2i(7 * defWidgetHeight, defWidgetHeight));
	connectButton->setCell(1);
	connectButton->setText(g_lang.get("Connect"));
	connectButton->Clicked.connect(this, &MenuStateJoinGame::onConnect);
	connectButton->setAnchors(a2);

	Button* searchButton = new Button(pnl, Vec2i(0), Vec2i(7 * defWidgetHeight, defWidgetHeight));
	searchButton->setCell(2);
	searchButton->setText(g_lang.get("Search"));
	searchButton->Clicked.connect(this, &MenuStateJoinGame::onSearchForGame);
	searchButton->setAnchors(a2);

	const Properties::PropertyMap &pm = servers.getPropertyMap();
	if (pm.empty()) {
		m_historyList->setEnabled(false);
	} else {
		foreach_const (Properties::PropertyMap, i, pm) {
			m_historyList->addItem(i->first);
		}
	}
	m_historyList->SelectionChanged.connect(this, &MenuStateJoinGame::onServerSelected);
}

void MenuStateJoinGame::onReturn(Widget*) {
	m_targetTansition = Transition::RETURN;
	doFadeOut();
}

void MenuStateJoinGame::onConnect(Widget*) {
	g_config.setNetServerIp(m_serverTextBox->getText());
	g_config.save();

	// validate Ip ??

	m_connectPanel->setVisible(false);
	Vec2i pos, size(300, 200);
	pos = g_metrics.getScreenDims() / 2 - size / 2;
	assert(!m_messageBox);
	m_messageBox = MessageDialog::showDialog(pos, size, "Connecting...",
		"Connecting, Please wait.", g_lang.get("Cancel"), "");
	m_messageBox->Button1Clicked.connect(this, &MenuStateJoinGame::onCancelConnect);
	m_messageBox->Close.connect(this, &MenuStateJoinGame::onCancelConnect);
	{
		MutexLock lock(m_connectMutex);
		assert(!m_connectThread);
		m_connectThread = new ConnectThread(*this, Ip(m_serverTextBox->getText()));
	}
}

void MenuStateJoinGame::onCancelConnect(Widget*) {
	MutexLock lock(m_connectMutex);
	if (m_connectThread) {
		m_connectThread->cancel();
	} // else it had finished already
}

void MenuStateJoinGame::connectThreadDone(ConnectResult result) {
	MutexLock lock(m_connectMutex);
	program.removeFloatingWidget(m_messageBox);
	m_messageBox = 0;

	if (result == ConnectResult::SUCCESS) {
		connected = true;
		Vec2i pos, size(300, 200);
		pos = g_metrics.getScreenDims() / 2 - size / 2;
		m_messageBox = MessageDialog::showDialog(pos, size, g_lang.get("Connected"),
			g_lang.get("Connected") + "\n" + g_lang.get("WaitingHost"), g_lang.get("Disconnect"), "");
		m_messageBox->Button1Clicked.connect(this, &MenuStateJoinGame::onDisconnect);
		m_messageBox->Close.connect(this, &MenuStateJoinGame::onDisconnect);
	} else if (result == ConnectResult::CANCELLED) {
		m_connectPanel->setVisible(true);
		m_connectLabel->setText("Not connected. Last attempt cancelled.");///@todo localise
	} else {
		m_connectPanel->setVisible(true);
		m_connectLabel->setText("Not connected. Last attempt failed.");///@todo localise
		///@todo show a message...
		//string err = m_connectThread->getErrorMsg();
		//
		// don't set m_connectPanel visible, 
		// recreate Dialog with error msg, 
		// on dismiss show m_connectPanel
	}
	delete m_connectThread;
	m_connectThread = 0;
}

void MenuStateJoinGame::onDisconnect(Widget*) {
	program.removeFloatingWidget(m_messageBox);
	m_messageBox = 0;
	g_simInterface.asClientInterface()->reset();
	m_connectPanel->setVisible(true);
	m_connectLabel->setText("Not connected. Last connection terminated.");///@todo localise
}

void MenuStateJoinGame::onTextModified(Widget*) {
	m_historyList->setSelected(-1);
}

void MenuStateJoinGame::onSearchForGame(Widget*) {
	m_historyList->setSelected(-1);
	m_serverTextBox->setText("");
	m_connectPanel->setVisible(false);
	Vec2i pos, size(300, 200);
	pos = g_metrics.getScreenDims() / 2 - size / 2;
	assert(!m_messageBox);
	m_messageBox = MessageDialog::showDialog(pos, size,"Searching...",
		"Searching, Please wait.", g_lang.get("Cancel"), "");
	m_messageBox->Button1Clicked.connect(this, &MenuStateJoinGame::onCancelSearch);
	m_messageBox->Close.connect(this, &MenuStateJoinGame::onCancelSearch);
	{
		MutexLock lock(m_findServerMutex);
		assert(!m_findServerThread);
		m_findServerThread = new FindServerThread(*this);
	}
}

void MenuStateJoinGame::onCancelSearch(Widget*) {
	MutexLock lock(m_findServerMutex);
	if (m_findServerThread) {
		m_findServerThread->stop();
		delete m_findServerThread;
		m_findServerThread = 0;
		assert(m_messageBox);
		program.removeFloatingWidget(m_messageBox);
		m_messageBox = 0;
		m_connectPanel->setVisible(true);
		m_connectLabel->setText("Not connected. Last search cancelled.");///@todo localise
	} // else it had finished already
}

void MenuStateJoinGame::foundServer(Ip ip) {
	MutexLock lock(m_findServerMutex);
	program.removeFloatingWidget(m_messageBox);
	m_messageBox = 0;
	m_serverTextBox->setText(ip.getString());
	delete m_findServerThread;
	m_findServerThread = 0;
	onConnect(0);
}

void MenuStateJoinGame::onServerSelected(Widget* source) {
	DropList* historyList = static_cast<DropList*>(source);
	if (historyList->getSelectedIndex() != -1) {
		string selected = historyList->getSelectedItem()->getText();
		string ipString = servers.getString(selected);
		m_serverTextBox->setText(ipString);
	}
}

void MenuStateJoinGame::update() {
	MenuState::update();

	if (m_connectThread) { // don't touch ClientInterface if ConnectThread is alive
		return;
	}

	ClientInterface* clientInterface = g_simInterface.asClientInterface();

	if (connected && !clientInterface->isConnected()) {
		connected = false;
		program.removeFloatingWidget(m_messageBox);
		m_messageBox = 0;
		m_connectPanel->setVisible(true);
		m_connectLabel->setText("Not connected. Last connection was severed."); ///@todo localise
	}

	// process network messages
	if (clientInterface->isConnected()) {
		// update lobby
		clientInterface->updateLobby();

		// intro
		if (clientInterface->getIntroDone()) {
			servers.setString(clientInterface->getDescription(), m_serverTextBox->getText());
		}

		// launch
		if (clientInterface->getLaunchGame()) {
			servers.save(serverFileName);
			m_targetTansition = Transition::PLAY;
			program.clear();
			program.setState(new GameState(program));
			return;
		}
	}
	if (m_transition) {
		program.clear();
		switch (m_targetTansition) {
			case Transition::RETURN:
				mainMenu->setState(new MenuStateRoot(program, mainMenu));
				break;
		}
	}
}

}}//end namespace
