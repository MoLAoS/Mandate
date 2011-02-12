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
		clientInterface->connect(m_server, GameConstants::serverPort);
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
	Vec2i pos, size(500, 300);
	pos = g_metrics.getScreenDims() / 2 - size / 2;

	m_connectPanel = new WidgetStrip(&program, Orientation::VERTICAL);
	//m_connectPanel = new Panel(&program, pos, size);
	//m_connectPanel->setAutoLayout(false);

	Font *font = g_coreData.getFTMenuFontNormal();
	Anchors a;
	a.set(Edge::COUNT, 0, false);
	m_connectPanel->setAnchors(a);
	Vec2i pad(45, 45);
	m_connectPanel->setPos(pad);
	m_connectPanel->setSize(Vec2i(g_config.getDisplayWidth() - pad.w * 2, g_config.getDisplayHeight() - pad.h * 2));

	WidgetStrip *pnl = new WidgetStrip(m_connectPanel, Orientation::HORIZONTAL);
	pnl->setSizeHint(SizeHint(-1, 50));
	a.set(Edge::LEFT, 25, true);
	a.set(Edge::RIGHT, 25, true);
	pnl->setAnchors(a);

	StaticText* historyLabel = new StaticText(pnl, Vec2i(0), Vec2i(200, 34));
	historyLabel->setTextParams(g_lang.get("RecentHosts"), Vec4f(1.f), font);
	historyLabel->setCentreInCell(true);
	m_historyList = new DropList(pnl, Vec2i(0), Vec2i(300, 34));
	m_historyList->setCentreInCell(true);

	pnl = new WidgetStrip(m_connectPanel, Orientation::HORIZONTAL);
	pnl->setSizeHint(SizeHint(-1, 50));
	pnl->setAnchors(a);

	StaticText* serverLabel = new StaticText(pnl, Vec2i(0), Vec2i(200, 34));
	serverLabel->setTextParams(g_lang.get("Server") + " Ip: ", Vec4f(1.f), font);
	serverLabel->setCentreInCell(true);
	
	m_serverTextBox = new TextBox(pnl, Vec2i(0), Vec2i(300, 34));
	m_serverTextBox->setTextParams("", Vec4f(1.f), font, true);
	m_serverTextBox->TextChanged.connect(this, &MenuStateJoinGame::onTextModified);
	m_serverTextBox->setCentreInCell(true);

	m_connectLabel = new StaticText(m_connectPanel, Vec2i(0), Vec2i(500, 34));
	m_connectLabel->setTextParams(g_lang.get("NotConnected"), Vec4f(1.f), font);
	m_connectLabel->setCentreInCell(true);
	m_connectLabel->setSizeHint(SizeHint(-1, 50));

	a.set(Edge::LEFT, 0, false);
	a.set(Edge::RIGHT, 0, false);
	pnl = new WidgetStrip(m_connectPanel, Orientation::HORIZONTAL);
	pnl->setAnchors(a);
	pnl->setSizeHint(SizeHint(25));

	// buttons
	Button* returnButton = new Button(pnl, Vec2i(0), Vec2i(256, 32));
	returnButton->setTextParams(g_lang.get("Return"), Vec4f(1.f), font);
	returnButton->Clicked.connect(this, &MenuStateJoinGame::onReturn);
	returnButton->setCentreInCell(true);

	Button* connectButton = new Button(pnl, Vec2i(0), Vec2i(256, 32));
	connectButton->setTextParams(g_lang.get("Connect"), Vec4f(1.f), font);
	connectButton->Clicked.connect(this, &MenuStateJoinGame::onConnect);
	connectButton->setCentreInCell(true);

	Button* searchButton = new Button(pnl, Vec2i(0), Vec2i(256, 32));
	searchButton->setTextParams(g_lang.get("Search"), Vec4f(1.f), font);
	searchButton->Clicked.connect(this, &MenuStateJoinGame::onSearchForGame);
	searchButton->setCentreInCell(true);

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

void MenuStateJoinGame::onReturn(Button*) {
	m_targetTansition = Transition::RETURN;
	doFadeOut();
}

void MenuStateJoinGame::onConnect(Button*) {
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
	{
		MutexLock lock(m_connectMutex);
		assert(!m_connectThread);
		m_connectThread = new ConnectThread(*this, Ip(m_serverTextBox->getText()));
	}
}

void MenuStateJoinGame::onCancelConnect(BasicDialog*) {
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

void MenuStateJoinGame::onDisconnect(BasicDialog*) {
	program.removeFloatingWidget(m_messageBox);
	m_messageBox = 0;
	g_simInterface.asClientInterface()->reset();
	m_connectPanel->setVisible(true);
	m_connectLabel->setText("Not connected. Last connection terminated.");///@todo localise
}

void MenuStateJoinGame::onTextModified(TextBox*) {
	m_historyList->setSelected(-1);
}

void MenuStateJoinGame::onSearchForGame(Button*) {
	m_historyList->setSelected(-1);
	m_serverTextBox->setText("");
	m_connectPanel->setVisible(false);
	Vec2i pos, size(300, 200);
	pos = g_metrics.getScreenDims() / 2 - size / 2;
	assert(!m_messageBox);
	m_messageBox = MessageDialog::showDialog(pos, size,"Searching...",
		"Searching, Please wait.", g_lang.get("Cancel"), "");
	m_messageBox->Button1Clicked.connect(this, &MenuStateJoinGame::onCancelSearch);
	{
		MutexLock lock(m_findServerMutex);
		assert(!m_findServerThread);
		m_findServerThread = new FindServerThread(*this);
	}
}

void MenuStateJoinGame::onCancelSearch(BasicDialog*) {
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

void MenuStateJoinGame::onServerSelected(ListBase* list) {
	DropList* historyList = static_cast<DropList*>(list);
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
