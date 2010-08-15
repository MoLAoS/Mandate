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

ConnectThread::ConnectThread(MenuStateJoinGame &menu, Ip serverIp)
		: m_menu(menu)
		, m_server(serverIp)
		, m_connecting(true)
		, m_result(ConnectResult::INVALID) {
	start();
}

void ConnectThread::execute() {
	ClientInterface* clientInterface = g_simInterface->asClientInterface();
	
	try {
		clientInterface->connect(m_server, GameConstants::serverPort);
		{
			m_result = ConnectResult::SUCCESS;
			MutexLock lock(m_mutex);
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
		g_simInterface->asClientInterface()->reset();
		m_result = ConnectResult::CANCELLED;
	}
}

string ConnectThread::getErrorMsg() {
	return m_errorMsg;
}

// ===============================
//  class MenuStateJoinGame
// ===============================

const int MenuStateJoinGame::newServerIndex = 0;
const string MenuStateJoinGame::serverFileName = "servers.ini";

MenuStateJoinGame::MenuStateJoinGame(Program &program, MainMenu *mainMenu, bool connect, Ip serverIp)
		: MenuState(program, mainMenu)
		, m_connectThread(0)
		, m_messageBox(0) {
	Lang &lang = Lang::getInstance();
	Config &config = Config::getInstance();

	if (fileExists(serverFileName)) {
		servers.load(serverFileName);
	} else {
		servers.save(serverFileName);
	}

	////buttons
	//buttonReturn.init(325, 300, 125);
	//buttonReturn.setText(lang.get("Return"));

	//buttonConnect.init(475, 300, 125);
	//buttonConnect.setText(lang.get("Connect"));

	////server type label
	//labelServerType.init(330, 460);
	//labelServerType.setText(lang.get("ServerType") + ":");

	////server type list box
	//listBoxServerType.init(465, 460);
	//listBoxServerType.pushBackItem(lang.get("ServerTypeNew"));
	//listBoxServerType.pushBackItem(lang.get("ServerTypePrevious"));

	////server label
	//labelServer.init(330, 430);
	//labelServer.setText(lang.get("Server") + ": ");

	////server listbox
	//listBoxServers.init(465, 430);

	//const Properties::PropertyMap &pm = servers.getPropertyMap();
	//for (Properties::PropertyMap::const_iterator i = pm.begin(); i != pm.end(); ++i) {
	//	listBoxServers.pushBackItem(i->first);
	//}

	////server ip
	//labelServerIp.init(465, 430);

	//labelStatus.init(330, 400);
	//labelStatus.setText("");

	//labelInfo.init(330, 370);
	//labelInfo.setText("");

	program.getSimulationInterface()->changeRole(GameRole::CLIENT);
	connected = false;
	playerIndex = -1;

	////server ip
	//if (connect) {
	//	labelServerIp.setText(serverIp.getString() + "_");
	//	connectToServer();
	//} else {
	//	labelServerIp.setText(config.getNetServerIp() + "_");
	//}

	//msgBox = NULL;

	buildConnectPanel();
}

void MenuStateJoinGame::buildConnectPanel() {
	Vec2i pos, size(500, 300);
	pos = g_metrics.getScreenDims() / 2 - size / 2;

	m_connectPanel = new Panel(&program, pos, size);
	BorderStyle bStyle;
	Colour colour(255, 0, 0, 255);
	bStyle.setSolid(g_widgetConfig.getColourIndex(colour));
	bStyle.setSizes(2);
	m_connectPanel->setBorderStyle(bStyle);
	m_connectPanel->setAutoLayout(false);

	Font *font = g_coreData.getFTMenuFontNormal();

	int yGap = (size.y - 30 * 4) / 5;

	int xGap = (size.x - 120 * 3) / 4;
	int x = xGap, w = 120, y = yGap, h = 30;
	Button::Ptr returnButton = new Button(m_connectPanel, Vec2i(x, y), Vec2i(w, h));
	returnButton->setTextParams(g_lang.get("Return"), Vec4f(1.f), font);
	returnButton->Clicked.connect(this, &MenuStateJoinGame::onReturn);

	x += w + xGap;
	Button::Ptr connectButton = new Button(m_connectPanel, Vec2i(x, y), Vec2i(w, h));
	connectButton->setTextParams(g_lang.get("Connect"), Vec4f(1.f), font);
	connectButton->Clicked.connect(this, &MenuStateJoinGame::onConnect);

	x += w + xGap;
	Button::Ptr searchButton = new Button(m_connectPanel, Vec2i(x, y), Vec2i(w, h));
	searchButton->setTextParams(g_lang.get("Search"), Vec4f(1.f), font);
	searchButton->Clicked.connect(this, &MenuStateJoinGame::onSearchForGame);

	y += 30 + yGap;
	w = (size.x - 20) / 2;
	x = size.x / 2 - w / 2;
	m_connectLabel = new StaticText(m_connectPanel, Vec2i(x, y), Vec2i(w, h));
	m_connectLabel->setTextParams(g_lang.get("NotConnected"), Vec4f(1.f), font);

	w = 150;
	x = size.x / 2 - w - 5;
	y += 30 + yGap;
	StaticText::Ptr serverLabel = new StaticText(m_connectPanel, Vec2i(x, y), Vec2i(w, h));
	serverLabel->setTextParams(g_lang.get("Server") + " Ip: ", Vec4f(1.f), font);
	
	x = size.x / 2 + 5;
	m_serverTextBox = new TextBox(m_connectPanel, Vec2i(x, y), Vec2i(w, h));
	m_serverTextBox->setTextParams("", Vec4f(1.f), font, true);

	w = 200;
	x = size.x / 2 - w - 5;
	y += 30 + yGap;
	StaticText::Ptr historyLabel = new StaticText(m_connectPanel, Vec2i(x, y), Vec2i(w, h));
	historyLabel->setTextParams(g_lang.get("RecentHosts"), Vec4f(1.f), font);
	x = size.x / 2 + 5;
	DropList::Ptr historyList = new DropList(m_connectPanel, Vec2i(x, y), Vec2i(w, h));

	const Properties::PropertyMap &pm = servers.getPropertyMap();
	if (pm.empty()) {
		historyList->setEnabled(false);
	} else {
		foreach_const (Properties::PropertyMap, i, pm) {
			historyList->addItem(i->first);
		}
	}
	historyList->SelectionChanged.connect(this, &MenuStateJoinGame::onServerSelected);
}

void MenuStateJoinGame::onReturn(Button::Ptr) {
	m_targetTansition = Transition::RETURN;
	doFadeOut();
}

void MenuStateJoinGame::onConnect(Button::Ptr) {
	g_config.setNetServerIp(m_serverTextBox->getText());
	g_config.save();

	// validate Ip ??

	assert(!m_connectThread);
	m_connectThread = new ConnectThread(*this, Ip(m_serverTextBox->getText()));
	m_connectPanel->setVisible(false);
	Vec2i pos, size(300, 200);
	pos = g_metrics.getScreenDims() / 2 - size / 2;
	assert(!m_messageBox);

	m_messageBox = new MessageDialog(&program);
	program.setFloatingWidget(m_messageBox, true);
	m_messageBox->setPos(pos);
	m_messageBox->setSize(size);
	m_messageBox->setTitleText("Connecting...");///@todo localise
	m_messageBox->setMessageText("Connecting, Please wait.");///@todo localise
	m_messageBox->setButtonText(g_lang.get("Cancel"));
	m_messageBox->Button1Clicked.connect(this, &MenuStateJoinGame::onCancelConnect);

}

void MenuStateJoinGame::onCancelConnect(MessageDialog::Ptr) {
	MutexLock lock(m_mutex);
	assert(m_connectThread);
	m_connectThread->cancel();
}

void MenuStateJoinGame::connectThreadDone(ConnectResult result) {
	MutexLock lock(m_mutex);
	program.removeFloatingWidget(m_messageBox);
	m_messageBox = 0;

	if (result == ConnectResult::SUCCESS) {
		connected = true;
		Vec2i pos, size(300, 200);
		pos = g_metrics.getScreenDims() / 2 - size / 2;
		m_messageBox = new MessageDialog(&program);
		program.setFloatingWidget(m_messageBox, true);
		m_messageBox->setPos(pos);
		m_messageBox->setSize(size);
		m_messageBox->setTitleText("Connected.");///@todo localise
		m_messageBox->setMessageText("Connected, waiting for server to launch game.");///@todo localise
		m_messageBox->setButtonText(g_lang.get("Disconnect"));
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

void MenuStateJoinGame::onDisconnect(MessageDialog::Ptr) {
	program.removeFloatingWidget(m_messageBox);
	m_messageBox = 0;
	g_simInterface->asClientInterface()->reset();
	m_connectPanel->setVisible(true);
	m_connectLabel->setText("Not connected. Last connection terminated.");///@todo localise
}

void MenuStateJoinGame::onSearchForGame(Button::Ptr) {

	///@todo apply hailtone's patch

}

void MenuStateJoinGame::onServerSelected(ListBase::Ptr list) {
	DropList::Ptr historyList = static_cast<DropList::Ptr>(list);
	string selected = historyList->getSelectedItem()->getText();
	string ipString = servers.getString(selected);
	m_serverTextBox->setText(ipString);
}

void MenuStateJoinGame::update() {
	MenuState::update();

	if (m_connectThread) { // don't touch ClientInterface if ConnectThread is alive
		return;
	}

	ClientInterface* clientInterface = g_simInterface->asClientInterface();

	if (connected && !clientInterface->isConnected()) {
		connected = false;
		program.removeFloatingWidget(m_messageBox);
		m_messageBox = 0;
		m_connectPanel->setVisible(true);
		m_connectLabel->setText("Not connected. Last connection was severed."); ///@todo localise
	}

	// process network messages
	if (clientInterface->isConnected()) {
		//update lobby
		clientInterface->updateLobby();

		//intro
		if (clientInterface->getIntroDone()) {
			servers.setString(clientInterface->getDescription(), m_serverTextBox->getText());
		}

		//launch
		if (clientInterface->getLaunchGame()) {
			servers.save(serverFileName);
			m_targetTansition = Transition::PLAY;
			doFadeOut();
			//program.setState(new GameState(program));
		}
	}
	if (m_transition) {
		program.clear();
		switch (m_targetTansition) {
			case Transition::RETURN:
				mainMenu->setState(new MenuStateRoot(program, mainMenu));
				break;
			case Transition::PLAY:
				program.setState(new GameState(program));
				break;
		}
	}
}

}}//end namespace
