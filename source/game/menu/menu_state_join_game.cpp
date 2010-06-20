// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
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
//  class MenuStateJoinGame
// ===============================

const int MenuStateJoinGame::newServerIndex = 0;
const string MenuStateJoinGame::serverFileName = "servers.ini";

MenuStateJoinGame::MenuStateJoinGame(Program &program, MainMenu *mainMenu, bool connect, Ip serverIp):
		MenuState(program, mainMenu/*, "join-game"*/) {
	Lang &lang = Lang::getInstance();
	Config &config = Config::getInstance();

	servers.load(serverFileName);

	//buttons
	buttonReturn.init(325, 300, 125);
	buttonReturn.setText(lang.get("Return"));

	buttonConnect.init(475, 300, 125);
	buttonConnect.setText(lang.get("Connect"));

	//server type label
	labelServerType.init(330, 460);
	labelServerType.setText(lang.get("ServerType") + ":");

	//server type list box
	listBoxServerType.init(465, 460);
	listBoxServerType.pushBackItem(lang.get("ServerTypeNew"));
	listBoxServerType.pushBackItem(lang.get("ServerTypePrevious"));

	//server label
	labelServer.init(330, 430);
	labelServer.setText(lang.get("Server") + ": ");

	//server listbox
	listBoxServers.init(465, 430);

	const Properties::PropertyMap &pm = servers.getPropertyMap();
	for (Properties::PropertyMap::const_iterator i = pm.begin(); i != pm.end(); ++i) {
		listBoxServers.pushBackItem(i->first);
	}

	//server ip
	labelServerIp.init(465, 430);

	labelStatus.init(330, 400);
	labelStatus.setText("");

	labelInfo.init(330, 370);
	labelInfo.setText("");

	program.getSimulationInterface()->changeRole(GameRole::CLIENT);
	connected = false;
	playerIndex = -1;

	//server ip
	if (connect) {
		labelServerIp.setText(serverIp.getString() + "_");
		connectToServer();
	} else {
		labelServerIp.setText(config.getNetServerIp() + "_");
	}

	msgBox = NULL;
}

void MenuStateJoinGame::mouseClick(int x, int y, MouseButton mouseButton) {
	CoreData &coreData = CoreData::getInstance();
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();
	ClientInterface* clientInterface = g_simInterface->asClientInterface();

	if (!clientInterface->isConnected()) {
		//server type
		if (listBoxServerType.mouseClick(x, y)) {
			if (!listBoxServers.getText().empty()) {
				labelServerIp.setText(servers.getString(listBoxServers.getText()) + "_");
			}
		//server list
		} else if (listBoxServerType.getSelectedItemIndex() != newServerIndex) {
			if (listBoxServers.mouseClick(x, y)) {
				labelServerIp.setText(servers.getString(listBoxServers.getText()) + "_");
			}
		}
	}
	//return
	if (buttonReturn.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));

	} else if (msgBox) {
		if (msgBox->mouseClick(x, y)) {
			soundRenderer.playFx(coreData.getClickSoundC());
			delete msgBox;
			msgBox = NULL;
		}
	//connect
	} else if (buttonConnect.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundA());
		labelInfo.setText("");

		if (clientInterface->isConnected()) {
			clientInterface->reset();
		} else {
			connectToServer();
		}
	}
}

void MenuStateJoinGame::mouseMove(int x, int y, const MouseState &ms) {
	buttonReturn.mouseMove(x, y);
	buttonConnect.mouseMove(x, y);
	listBoxServerType.mouseMove(x, y);

	if (msgBox != NULL) {
		msgBox->mouseMove(x, y);
		return;
	}

	//hide-show options depending on the selection
	if (listBoxServers.getSelectedItemIndex() == newServerIndex) {
		labelServerIp.mouseMove(x, y);
	} else {
		listBoxServers.mouseMove(x, y);
	}
}

void MenuStateJoinGame::render() {
	Renderer &renderer = Renderer::getInstance();

	renderer.renderButton(&buttonReturn);
	renderer.renderLabel(&labelServer);
	renderer.renderLabel(&labelServerType);
	renderer.renderLabel(&labelStatus);
	renderer.renderLabel(&labelInfo);
	renderer.renderButton(&buttonConnect);
	renderer.renderListBox(&listBoxServerType);

	if (listBoxServerType.getSelectedItemIndex() == newServerIndex) {
		renderer.renderLabel(&labelServerIp);
	} else {
		renderer.renderListBox(&listBoxServers);
	}

	if (msgBox != NULL) {
		renderer.renderMessageBox(msgBox);
	}
}

void MenuStateJoinGame::update() {
	ClientInterface* clientInterface = g_simInterface->asClientInterface();
	Lang &lang = Lang::getInstance();

	//update status label
	if (clientInterface->isConnected()) {
		buttonConnect.setText(lang.get("Disconnect"));
		labelStatus.setText(clientInterface->getDescription());
	} else {
		buttonConnect.setText(lang.get("Connect"));
		labelStatus.setText(lang.get("NotConnected"));
		labelInfo.setText("");
	}

	//process network messages
	if (clientInterface->isConnected()) {

		//update lobby
		clientInterface->updateLobby();

		//intro
		if (clientInterface->getIntroDone()) {
			labelInfo.setText(lang.get("WaitingHost"));
			servers.setString(clientInterface->getDescription(), Ip(labelServerIp.getText()).getString());
		}

		//launch
		if (clientInterface->getLaunchGame()) {
			servers.save(serverFileName);
			program.setState(new GameState(program));
		}
	}
}

void MenuStateJoinGame::keyDown(const Key &key) {
	ClientInterface* clientInterface = g_simInterface->asClientInterface();

	if (!clientInterface->isConnected()) {
		if (key == KeyCode::BACK_SPACE) {
			string text = labelServerIp.getText();

			if (text.size() > 1) {
				text.erase(text.end() - 2);
			}

			labelServerIp.setText(text);
		}
	}

	if(key == KeyCode::ESCAPE) {
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
	}
}

void MenuStateJoinGame::keyPress(char c) {
	ClientInterface* clientInterface = g_simInterface->asClientInterface();

	if (!clientInterface->isConnected()) {
		int maxTextSize = 16;

		if (c >= '0' && c <= '9') {

			if (labelServerIp.getText().size() < maxTextSize) {
				string text = labelServerIp.getText();

				text.insert(text.end() - 1, c);

				labelServerIp.setText(text);
			}
		} else if (c == '.') {
			if (labelServerIp.getText().size() < maxTextSize) {
				string text = labelServerIp.getText();

				text.insert(text.end() - 1, '.');

				labelServerIp.setText(text);
			}
		}
	}
}

void MenuStateJoinGame::connectToServer() {
	ClientInterface* clientInterface = g_simInterface->asClientInterface();
	Config& config = Config::getInstance();
	Lang &lang = Lang::getInstance();
	Ip serverIp(labelServerIp.getText());
	
	try {
		clientInterface->connect(serverIp, GameConstants::serverPort);

		//save server ip
		config.setNetServerIp(serverIp.getString());
		config.save();
	} catch(exception &e) {
		// tell the user
		msgBox = new GraphicMessageBox();
		msgBox->init(lang.get("UnableToJoin"), lang.get("Ok"));
		printf("\n%s", e.what());
		LOG_NETWORK(e.what());
	}

	labelServerIp.setText(serverIp.getString() + '_');
	labelInfo.setText("");
}

}}//end namespace
