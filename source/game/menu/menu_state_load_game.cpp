// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Jaagup Repän <jrepan@gmail.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "menu_state_load_game.h"

#include "renderer.h"
#include "menu_state_root.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "game.h"
#include "network_manager.h"
#include "xml_parser.h"

#include "leak_dumper.h"


namespace Glest{ namespace Game{

using namespace Shared::Util;
// =====================================================
// 	class MenuStateLoadGame
// =====================================================

void SavedGamePreviewLoader::execute() {
	while(seeJaneRun) {
		string *fileName;
		{
			MutexLock lock(mutex);
			fileName = this->fileName;
		}
		if(fileName) {
			loadPreview(fileName);
		} else {
			sleep(10);
		}
	}
}

void SavedGamePreviewLoader::loadPreview(string *fileName) {
	string err;
	XmlNode *root = NULL;

	try {
		root = XmlIo::getInstance().load(*fileName);
	} catch (exception &e) {
		err = "Can't open game " + *fileName + ": " + e.what();
	}

	fileName = menu.setGameInfo(*fileName, root, err);
	{
		MutexLock lock(mutex);
		delete this->fileName;
		this->fileName = fileName;
	}
}


// =====================================================
// 	class MenuStateLoadGame
// =====================================================

MenuStateLoadGame::MenuStateLoadGame(Program &program, MainMenu *mainMenu) :
		MenuStateStartGameBase(program, mainMenu, "loadgame"), loaderThread(*this) {
	confirmMessageBox = NULL;
	//msgBox = NULL;
	savedGame = NULL;
	gs = NULL;

	Shared::Platform::mkdir("savegames", true);

	Lang &lang= Lang::getInstance();

	//create
	buttonReturn.init(350, 200, 100);
	buttonDelete.init(462, 200, 100);
	buttonPlayNow.init(575, 200, 100);

	//savegames listBoxGames
	listBoxGames.init(400, 300, 225);
	if(!loadGameList()) {
		msgBox = new GraphicMessageBox();
		msgBox->init(Lang::getInstance().get("NoSavedGames"), Lang::getInstance().get("Ok"));
		criticalError = true;
		return;
	}

	//texts
	buttonReturn.setText(lang.get("Return"));
	buttonDelete.setText(lang.get("Delete"));
	buttonPlayNow.setText(lang.get("Load"));

	//game info lables
	labelInfoHeader.init(350, 500, 440, 225, false);

    for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].init(350, 450-i*30);
        labelControls[i].init(425, 450-i*30);
        labelFactions[i].init(500, 450-i*30);
		labelTeams[i].init(575, 450-i*30, 60);
		labelNetStatus[i].init(600, 450-i*30, 60);
	}
	//initialize network interface
	NetworkManager &networkManager= NetworkManager::getInstance();
	networkManager.init(nrServer);
	labelNetwork.init(50, 50);
	try {
		labelNetwork.setText(lang.get("Address") + ": " + networkManager.getServerInterface()->getIp() + ":" + intToStr(GameConstants::serverPort));
	} catch(const exception &e) {
		labelNetwork.setText(lang.get("Address") + ": ? " + e.what());
	}
	//updateNetworkSlots();
	selectionChanged();
}

MenuStateLoadGame::~MenuStateLoadGame() {
	loaderThread.goAway();
	if(confirmMessageBox) {
		delete confirmMessageBox;
	}
	if(msgBox) {
		delete msgBox;
	}
	loaderThread.join();
}

void MenuStateLoadGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	Lang &lang= Lang::getInstance();

	if (confirmMessageBox != NULL){
		int button = 1;
		if(confirmMessageBox->mouseClick(x,y, button)){
			if (button == 1){
				remove(getFileName().c_str());
				if(!loadGameList()) {
					mainMenu->setState(new MenuStateRoot(program, mainMenu));
					return;
				}
				selectionChanged();
			}
			delete confirmMessageBox;
			confirmMessageBox = NULL;
		}
		return;
	}

	if((msgBox && criticalError && msgBox->mouseClick(x,y)) || buttonReturn.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
		return;
	}

	if(msgBox) {
		if(msgBox->mouseClick(x,y)) {
			soundRenderer.playFx(coreData.getClickSoundC());
			delete msgBox;
			msgBox = NULL;
		}
	}
	else if(buttonDelete.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundC());
		confirmMessageBox = new GraphicMessageBox();
		confirmMessageBox->init(lang.get("Delete") + " " + listBoxGames.getSelectedItem() + "?",
				lang.get("Yes"), lang.get("No"));
	}
	else if(buttonPlayNow.mouseClick(x,y) && gs){
		soundRenderer.playFx(coreData.getClickSoundC());
		if(!loadGame()) {
			buttonPlayNow.mouseMove(1, 1);
			msgBox = new GraphicMessageBox();
			msgBox->init(Lang::getInstance().get("WaitingForConnections"), Lang::getInstance().get("Ok"));
			criticalError = false;
		}
	}
	else if(listBoxGames.mouseClick(x, y)){
		selectionChanged();
	}
}

void MenuStateLoadGame::mouseMove(int x, int y, const MouseState &ms){

	if (confirmMessageBox != NULL){
		confirmMessageBox->mouseMove(x,y);
		return;
	}

	if (msgBox != NULL){
		msgBox->mouseMove(x,y);
		return;
	}

	listBoxGames.mouseMove(x, y);

	buttonReturn.mouseMove(x, y);
	buttonDelete.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);
}

void MenuStateLoadGame::render(){
	Renderer &renderer= Renderer::getInstance();
	if(msgBox && criticalError) {
		renderer.renderMessageBox(msgBox);
		return;
	}

	if(savedGame) {
		initGameInfo();
	}

	renderer.renderLabel(&labelInfoHeader);
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		renderer.renderLabel(&labelPlayers[i]);
		renderer.renderLabel(&labelControls[i]);
		renderer.renderLabel(&labelFactions[i]);
		renderer.renderLabel(&labelTeams[i]);
		renderer.renderLabel(&labelNetStatus[i]);
	}

	renderer.renderLabel(&labelNetwork);
	renderer.renderListBox(&listBoxGames);
	renderer.renderButton(&buttonReturn);
	renderer.renderButton(&buttonDelete);
	renderer.renderButton(&buttonPlayNow);

	if(confirmMessageBox) {
		renderer.renderMessageBox(confirmMessageBox);
	}

	if(msgBox) {
		renderer.renderMessageBox(msgBox);
	}

}

void MenuStateLoadGame::update(){
	if (!gs) {
		return;
	}

	ServerInterface* serverInterface = NetworkManager::getInstance().getServerInterface();
	Lang& lang = Lang::getInstance();

	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		if (gs->getFactionControl(i) == ctNetwork) {
			ConnectionSlot* connectionSlot = serverInterface->getSlot(i);

			assert(connectionSlot != NULL);

			if (connectionSlot->isConnected()) {
				labelNetStatus[i].setText(connectionSlot->getDescription());
			} else {
				labelNetStatus[i].setText(lang.get("NotConnected"));
			}
		} else {
			labelNetStatus[i].setText("");
		}
	}
}

// ============ misc ===========================

string *MenuStateLoadGame::setGameInfo(const string &fileName, const XmlNode *root,
		const string &err) {
	MutexLock lock(mutex);
	if(this->fileName != fileName) {
		return new string(this->fileName);
	}

	if(!root) {
		labelInfoHeader.setText(err);
		for(int i=0; i<GameConstants::maxPlayers; ++i){
			labelPlayers[i].setText("");
			labelControls[i].setText("");
			labelFactions[i].setText("");
			labelTeams[i].setText("");
			labelNetStatus[i].setText("");
		}
		return NULL;
	}

	if(savedGame) {
		//oops, that shouldn't happen
		delete root;
	}
	savedGame = root;

	return NULL;
}

// ============ PRIVATE ===========================

bool MenuStateLoadGame::loadGameList() {
	try {
		findAll("savegames/*.sav", fileNames, true);
	} catch (exception e){
		fileNames.clear();
		prettyNames.clear();
		return false;
	}

	prettyNames.resize(fileNames.size());

	for(int i = 0; i < fileNames.size(); ++i){
		prettyNames[i] = formatString(fileNames[i]);
	}
	listBoxGames.setItems(prettyNames);
	return true;
}

bool MenuStateLoadGame::loadGame() {
	XmlNode *root;
	ServerInterface* serverInterface = NULL;

	if(!gs) {
		return false;
	}

	for(int i = 0; i < gs->getFactionCount(); ++i) {
		if(gs->getFactionControl(i) == ctNetwork) {
			if(!serverInterface) {
				serverInterface = NetworkManager::getInstance().getServerInterface();
			}

			if(!serverInterface->getSlot(i)->isConnected()) {
				return false;
			}
		}
	}

	root = XmlIo::getInstance().load(getFileName());

	if(serverInterface) {
		serverInterface->launchGame(gs, getFileName());
	}
	program.setState(new Game(program, *gs, root));
	return true;
}

string MenuStateLoadGame::getFileName() {
	return "savegames/" + fileNames[listBoxGames.getSelectedItemIndex()] + ".sav";
}

void MenuStateLoadGame::selectionChanged() {
	{
		MutexLock lock(mutex);
		fileName = getFileName();
		labelInfoHeader.setText("Loading...");
		for(int i=0; i<GameConstants::maxPlayers; ++i){
			labelPlayers[i].setText("");
			labelControls[i].setText("");
			labelFactions[i].setText("");
			labelTeams[i].setText("");
			labelNetStatus[i].setText("");
		}
	}
	loaderThread.setFileName(fileName);
}

void MenuStateLoadGame::initGameInfo() {
	try {
		if(gs) {
			delete gs;
			gs = NULL;
		}
		gs = new GameSettings(savedGame->getChild("settings"));
		string techPath = gs->getTechPath();
		string tilesetPath = gs->getTilesetPath();
		string mapPath = gs->getMapPath();
		string scenarioPath = gs->getScenarioPath();
		int elapsedSeconds = savedGame->getChild("world")->getChildIntValue("frameCount") / 60;
		int elapsedMinutes = elapsedSeconds / 60;
		int elapsedHours = elapsedMinutes / 60;
		elapsedSeconds = elapsedSeconds % 60;
		elapsedMinutes = elapsedMinutes % 60;
		char elapsedTime[0x100];
		loadMapInfo(mapPath, &mapInfo);
/*
		if(techTree.size() > strlen("techs/")) {
			techTree.erase(0, strlen("techs/"));
		}
		if(tileset.size() > strlen("tilesets/")) {
			tileset.erase(0, strlen("tilesets/"));
		}
		if(map.size() > strlen("maps/")) {
			map.erase(0, strlen("maps/"));
		}
		if(map.size() > strlen(".gbm")) {
			map.resize(map.size() - strlen(".gbm"));
		}*/
		if(elapsedHours) {
			sprintf(elapsedTime, "%d:%02d:%02d", elapsedHours, elapsedMinutes, elapsedSeconds);
		} else {
			sprintf(elapsedTime, "%02d:%02d", elapsedMinutes, elapsedSeconds);
		}

		string mapDescr = " (Max Players: " + intToStr(mapInfo.players)
				+ ", Size: " + intToStr(mapInfo.size.x) + " x " + intToStr(mapInfo.size.y) + ")";

		labelInfoHeader.setText(listBoxGames.getSelectedItem() + ": " + gs->getDescription()
				+ "\nTech Tree: " + formatString(basename(techPath))
				+ "\nTileset: " + formatString(basename(tilesetPath))
				+ "\nMap: " + formatString(basename(cutLastExt(mapPath))) + mapDescr
				+ "\nScenario: " + formatString(basename(scenarioPath))
				+ "\nElapsed Time: " + elapsedTime);

		if(gs->getFactionCount() > GameConstants::maxPlayers || gs->getFactionCount() < 0) {
			throw runtime_error("Invalid faction count (" + intToStr(gs->getFactionCount())
					+ ") in saved game.");
		}

		for(int i = 0; i < GameConstants::maxPlayers; ++i){
			if(i < gs->getFactionCount()) {
				int control = gs->getFactionControl(i);
				//beware the buffer overflow -- it's possible for others to send
				//saved game files that are intended to exploit buffer overruns
				if(control >= ctCount || control < 0) {
					throw runtime_error("Invalid control type (" + intToStr(control)
							+ ") in saved game.");
				}

				labelPlayers[i].setText(string("Player ") + intToStr(i));
				labelControls[i].setText(controlTypeNames[gs->getFactionControl(i)]);
				labelFactions[i].setText(gs->getFactionTypeName(i));
				labelTeams[i].setText(intToStr(gs->getTeam(i)));
				labelNetStatus[i].setText("");
			} else {
				labelPlayers[i].setText("");
				labelControls[i].setText("");
				labelFactions[i].setText("");
				labelTeams[i].setText("");
				labelNetStatus[i].setText("");
			}
		}

	} catch (exception &e) {
		labelInfoHeader.setText(string("Bad game file.\n") + e.what());
		for(int i = 0; i < GameConstants::maxPlayers; ++i){
			labelPlayers[i].setText("");
			labelControls[i].setText("");
			labelFactions[i].setText("");
			labelTeams[i].setText("");
			labelNetStatus[i].setText("");
		}
		if(gs) {
			delete gs;
			gs = NULL;
		}
	}
	if(gs) {
		updateNetworkSlots();
	}

	delete savedGame;
	savedGame = NULL;
}

void MenuStateLoadGame::updateNetworkSlots(){
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
	assert(gs);

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(serverInterface->getSlot(i)==NULL && gs->getFactionControl(i) == ctNetwork){
			serverInterface->addSlot(i);
		}
		if(serverInterface->getSlot(i) != NULL && gs->getFactionControl(i) != ctNetwork){
			serverInterface->removeSlot(i);
		}
	}
}
}}//end namespace
