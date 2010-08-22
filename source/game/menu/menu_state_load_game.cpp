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

	m_infoLabel = new ScrollText(&program, Vec2i(dim.x / 2 - 200, dim.y / 2 ), Vec2i(400, 200));
	m_infoLabel->init();
	//m_infoLabel->setTextParams("", Vec4f(1.f), font);
//	m_infoLabel->setBorderParams(BorderStyle::SOLID, 2, Vec3f(1.f, 0.f, 0.f), 0.6f);

	m_savedGameList = new DropList(&program, Vec2i(dim.x / 2 - 150, dim.y / 2 - 100), Vec2i(300, 30));
	m_savedGameList->SelectionChanged.connect(this, &MenuStateLoadGame::onSaveSelected);

	// savegames listBoxGames
	if(!loadGameList()) {
		Vec2i sz(330, 256);
		program.clear();
		m_messageDialog = MessageDialog::showDialog(g_metrics.getScreenDims() / 2 - sz / 2, sz,
			g_lang.get("Error"), g_lang.get("NoSavedGames"), g_lang.get("Ok"), "");
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

		const string &fileName = m_savedGameList->getSelectedItem()->getText();
		m_messageDialog = MessageDialog::showDialog(g_metrics.getScreenDims() / 2 - sz / 2, sz, 
			g_lang.get("Confirm"), g_lang.get("Delete") + " " + fileName + "?", 
			g_lang.get("Yes"), g_lang.get("No"));
		m_messageDialog->Button1Clicked.connect(this, &MenuStateLoadGame::onConfirmDelete);
		m_messageDialog->Button2Clicked.connect(this, &MenuStateLoadGame::onCancelDelete);
		program.setFade(0.5f);

	} else if (btn == m_playNowButton) {
		m_targetTransition = Transition::PLAY;
		g_soundRenderer.playFx(g_coreData.getClickSoundC());
		doFadeOut();
	}
}

void MenuStateLoadGame::onCancelDelete(BasicDialog::Ptr) {
	program.setFade(1.0f);
	program.removeFloatingWidget(m_messageDialog);
}

void MenuStateLoadGame::onConfirmDelete(BasicDialog::Ptr) {
	program.setFade(1.0f);
	program.removeFloatingWidget(m_messageDialog);

	FSFactory::getInstance()->removeFile(getFileName());
	if (!loadGameList()) {
		m_targetTransition = Transition::RETURN;
		doFadeOut();
	}
}

void MenuStateLoadGame::onConfirmReturn(BasicDialog::Ptr) {
	g_soundRenderer.playFx(g_coreData.getClickSoundA());
	m_targetTransition = Transition::RETURN;
	mainMenu->setCameraTarget(MenuStates::ROOT);
	doFadeOut();
}

void MenuStateLoadGame::onSaveSelected(ListBase::Ptr list) {
	selectionChanged();
}

void MenuStateLoadGame::update() {
	MenuState::update();

	if (savedGame) {
		initGameInfo();
	}
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
	if (this->fileName != fileName) {
		return new string(this->fileName);
	}

	if (!root) {
		m_infoLabel->setText(err);
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

void MenuStateLoadGame::selectionChanged() {
	{
		MutexLock lock(mutex);
		fileName = getFileName();
		m_infoLabel->setText("Loading...");
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

		int elapsedSeconds = savedGame->getChild("world")->getChildIntValue("frameCount") / 60;
		int elapsedMinutes = elapsedSeconds / 60;
		int elapsedHours = elapsedMinutes / 60;
		elapsedSeconds = elapsedSeconds % 60;
		elapsedMinutes = elapsedMinutes % 60;
		char elapsedTime[256];
		MapInfo mapInfo;
		mapInfo.load(gs->getMapPath());

		if (elapsedHours) {
			sprintf(elapsedTime, "%d:%02d:%02d", elapsedHours, elapsedMinutes, elapsedSeconds);
		} else {
			sprintf(elapsedTime, "%02d:%02d", elapsedMinutes, elapsedSeconds);
		}

		string mapDescr = " (Max Players: " + intToStr(mapInfo.players)
				+ ", Size: " + intToStr(mapInfo.size.x) + " x " + intToStr(mapInfo.size.y) + ")";


		stringstream ss;
		ss  << m_savedGameList->getSelectedItem()->getText() << ": " << gs->getDescription()
			<< "\nTech-tree: " << formatString(basename(gs->getTechPath()))
			<< "\nTileset: " << formatString(basename(gs->getTilesetPath()))
			<< "\nMap: " << formatString(basename(cutLastExt(gs->getMapPath()))) << mapDescr
			<< "\nElapsed Time: " << elapsedTime
			<< "\n";

		if (gs->getFactionCount() > GameConstants::maxPlayers || gs->getFactionCount() < 0) {
			throw runtime_error("Invalid faction count (" + intToStr(gs->getFactionCount())
					+ ") in saved game.");
		}

		for(int i = 0; i < gs->getFactionCount(); ++i){
			int control = gs->getFactionControl(i);
			//beware the buffer overflow -- it's possible for others to send
			//saved game files that are intended to exploit buffer overruns
			if(control >= ControlType::COUNT || control < 0) {
				throw runtime_error("Invalid control type (" + intToStr(control)
						+ ") in saved game.");
			}
			ss	<< "\nPlayer " << i << ": " << ControlTypeNames[gs->getFactionControl(i)]
				<< " : " << gs->getFactionTypeName(i)
				<< " Team: " << gs->getTeam(i);
		}
		m_infoLabel->setText(ss.str());
		m_infoLabel->init();

	} catch (exception &e) {
		m_infoLabel->setText(string("Bad game file.\n") + e.what());
		if(gs) {
			delete gs;
			gs = NULL;
		}
	}
	delete savedGame;
	savedGame = NULL;
}

}}//end namespace
