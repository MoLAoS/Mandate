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
#include "xml_parser.h"
#include "sim_interface.h"
#include "server_interface.h"

#include "leak_dumper.h"

using namespace Glest::Sim;
using namespace Glest::Net;

namespace Glest { namespace Menu {
using Shared::PhysFS::FSFactory;
using namespace Shared::Util;
// =====================================================
// 	class MenuStateLoadGame
// =====================================================

void SavedGamePreviewLoader::execute() {
	while(running) {
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

MenuStateLoadGame::MenuStateLoadGame(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu)
		, loaderThread(*this) {
	Shared::Platform::mkdir("savegames", true);
	savedGame = NULL;
	gs = NULL;

	// game info lables
	labelInfoHeader.init(350, 500, 440, 225, false);

    for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].init(350, 450-i*30);
        labelControls[i].init(425, 450-i*30);
        labelFactions[i].init(500, 450-i*30);
		labelTeams[i].init(575, 450-i*30, 60);
		labelNetStatus[i].init(600, 450-i*30, 60);
	}

	Font *font = g_coreData.getFTMenuFontNormal();
	int gap = (g_metrics.getScreenW() - 450) / 4;
	int x = gap, w = 150, y = 150, h = 30;
	m_returnButton = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	m_returnButton->setTextParams(g_lang.get("Return"), Vec4f(1.f), font);
	m_returnButton->Clicked.connect(this, &MenuStateLoadGame::onButtonClick);

	x += w + gap;
	m_deleteButton = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	m_deleteButton->setTextParams(g_lang.get("Delete"), Vec4f(1.f), font);
	m_deleteButton->Clicked.connect(this, &MenuStateLoadGame::onButtonClick);

	x += w + gap;
	m_playNowButton = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	m_playNowButton->setTextParams(g_lang.get("PlayNow"), Vec4f(1.f), font);
	m_playNowButton->Clicked.connect(this, &MenuStateLoadGame::onButtonClick);

	Vec2i dim = g_metrics.getScreenDims();

	m_infoLabel = new StaticText(&program, Vec2i(dim.x / 2 - 200, dim.y / 2 ), Vec2i(400, 200));
	m_infoLabel->setTextParams("", Vec4f(1.f), font);
	m_infoLabel->setBorderParams(BorderStyle::SOLID, 2, Vec3f(1.f, 0.f, 0.f), 0.6f);

	m_savedGameList = new DropList(&program, Vec2i(dim.x / 2 - 150, dim.y / 2 - 100), Vec2i(300, 30));
	m_savedGameList->SelectionChanged.connect(this, &MenuStateLoadGame::onSaveSelected);

	// savegames listBoxGames
	if(!loadGameList()) {
		Vec2i sz(330, 256);
		program.clear();
		m_messageDialog = new MessageDialog(&program, g_metrics.getScreenDims() / 2 - sz / 2, sz);
		m_messageDialog->setTitleText(g_lang.get("Error"));
		m_messageDialog->setMessageText(g_lang.get("NoSavedGames"));
		m_messageDialog->setButtonText(g_lang.get("Ok"));
		m_messageDialog->Button1Clicked.connect(this, &MenuStateLoadGame::onConfirmReturn);
	}
}

MenuStateLoadGame::~MenuStateLoadGame() {
	loaderThread.goAway();
	loaderThread.join();
}

void MenuStateLoadGame::onButtonClick(Button::Ptr btn) {
	if (btn == m_returnButton) {
		m_targetTransition = Transition::RETURN;
		g_soundRenderer.playFx(g_coreData.getClickSoundA());
		doFadeOut();
	} else if (btn == m_deleteButton) {
		g_soundRenderer.playFx(g_coreData.getClickSoundC());
		Vec2i sz(330, 256);

		m_messageDialog = new MessageDialog(&program);
		program.setFloatingWidget(m_messageDialog, true);
		m_messageDialog->setPos(g_metrics.getScreenDims() / 2 - sz / 2);
		m_messageDialog->setSize(sz);
		m_messageDialog->setTitleText(g_lang.get("Confirm"));
		m_messageDialog->setMessageText(g_lang.get("Delete") + " " + m_savedGameList->getSelectedItem()->getText() + "?");
		m_messageDialog->setButtonText(g_lang.get("Yes"), g_lang.get("No"));
		m_messageDialog->Button1Clicked.connect(this, &MenuStateLoadGame::onConfirmDelete);
		m_messageDialog->Button2Clicked.connect(this, &MenuStateLoadGame::onCancelDelete);
		program.setFade(0.5f);

	} else if (btn == m_playNowButton) {
		m_targetTransition = Transition::PLAY;
		g_soundRenderer.playFx(g_coreData.getClickSoundC());
		doFadeOut();
	}
}

void MenuStateLoadGame::onCancelDelete(MessageDialog::Ptr) {
	program.setFade(1.0f);
	program.removeFloatingWidget(m_messageDialog);
}

void MenuStateLoadGame::onConfirmDelete(MessageDialog::Ptr) {
	program.setFade(1.0f);
	program.removeFloatingWidget(m_messageDialog);

	FSFactory::getInstance()->removeFile(getFileName());
	if (!loadGameList()) {
		m_targetTransition = Transition::RETURN;
		doFadeOut();
	}
}

void MenuStateLoadGame::onConfirmReturn(MessageDialog::Ptr) {
	g_soundRenderer.playFx(g_coreData.getClickSoundA());
	m_targetTransition = Transition::RETURN;
	mainMenu->setCameraTarget(MenuStates::ROOT);
	doFadeOut();
}

void MenuStateLoadGame::onSaveSelected(ListBase::Ptr list) {
	selectionChanged();
}

void MenuStateLoadGame::render(){
	Renderer &renderer= Renderer::getInstance();

	if (savedGame) {
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
}

void MenuStateLoadGame::update() {
	MenuState::update();

	if (m_transition) {
		switch (m_targetTransition) {
			case Transition::RETURN:
				program.clear();
				mainMenu->setState(new MenuStateRoot(program, mainMenu));
				break;
			case Transition::PLAY:
				loadGame();
				break;
		}
	}
}

// ============ misc ===========================

string *MenuStateLoadGame::setGameInfo(const string &fileName, const XmlNode *root, const string &err) {
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

	if (savedGame) {
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
	m_savedGameList->clearItems();
	m_savedGameList->addItems(prettyNames);
	m_savedGameList->setSelected(0);
	return true;
}

void MenuStateLoadGame::loadGame() {
	XmlNode *root;
	root = XmlIo::getInstance().load(getFileName());
	g_simInterface->getGameSettings() = *gs;
	g_simInterface->getSavedGame() = root;
	program.clear();
	program.setState(new GameState(program));
}

string MenuStateLoadGame::getFileName() {
	return "savegames/" + fileNames[m_savedGameList->getSelectedIndex()] + ".sav";
}

void MenuStateLoadGame::mouseClick(int, int, MouseButton) {}
void MenuStateLoadGame::mouseMove(int, int, const MouseState&) {}

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

const string oldSaveMsg = "Old savegame format, no longer supported, sorry.";
const string unknownSaveMsg = "Unknown savegame version. Possibly saved with a newer GAE than this.";

void MenuStateLoadGame::initGameInfo() {
	try {
		if(gs) {
			delete gs;
			gs = NULL;
		}
		int version;
		try {
			version = savedGame->getAttribute("version")->getIntValue();
		} catch (...) {
			version = 0;
		}
		if (version != GameConstants::saveGameVersion) {
			throw runtime_error(version < GameConstants::saveGameVersion ? oldSaveMsg : unknownSaveMsg);
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
		MapInfo mapInfo;
		mapInfo.load(mapPath);
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
		}
*/
		if(elapsedHours) {
			sprintf(elapsedTime, "%d:%02d:%02d", elapsedHours, elapsedMinutes, elapsedSeconds);
		} else {
			sprintf(elapsedTime, "%02d:%02d", elapsedMinutes, elapsedSeconds);
		}

		string mapDescr = " (Max Players: " + intToStr(mapInfo.players)
				+ ", Size: " + intToStr(mapInfo.size.x) + " x " + intToStr(mapInfo.size.y) + ")";

		labelInfoHeader.setText(m_savedGameList->getSelectedItem()->getText()
				+ ": " + gs->getDescription()
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
				if(control >= ControlType::COUNT || control < 0) {
					throw runtime_error("Invalid control type (" + intToStr(control)
							+ ") in saved game.");
				}

				labelPlayers[i].setText(string("Player ") + intToStr(i));
				labelControls[i].setText(ControlTypeNames[gs->getFactionControl(i)]);
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
	ServerInterface* serverInterface= g_simInterface->asServerInterface();
	if (!serverInterface) {
		return;
	}
	assert(gs);

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(serverInterface->getSlot(i)==NULL && gs->getFactionControl(i) == ControlType::NETWORK){
			serverInterface->addSlot(i);
		}
		if(serverInterface->getSlot(i) != NULL && gs->getFactionControl(i) != ControlType::NETWORK){
			serverInterface->removeSlot(i);
		}
	}
}
}}//end namespace
