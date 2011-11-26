// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Jaagup Rep�n <jrepan@gmail.com>
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
	savedGame = NULL;
	gs = NULL;

	int font = g_widgetConfig.getDefaultFontIndex(FontUsage::MENU);
	int white = g_widgetConfig.getColourIndex(Colour(255u));
//	Font *font = g_widgetConfig.getMenuFont()[FontSize::NORMAL];

	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));
	Anchors centreAnchors;
	centreAnchors.setCentre(true);
	
	CellStrip *strip = new CellStrip(static_cast<Container*>(&program), Orientation::VERTICAL, Origin::CENTRE, 3);
	strip->setSizeHint(0, SizeHint(-1, 50));  // 50 px cell for game drop-list
	strip->setSizeHint(1, SizeHint(-1, 250)); // 250 px for info box
	strip->setSizeHint(2, SizeHint(25)); // 25 % of the rest for the button panel
	strip->setAnchors(fillAnchors);
	Vec2i pad(45, 45);
	strip->setPos(pad);
	strip->setSize(Vec2i(g_config.getDisplayWidth() - pad.w * 2, g_config.getDisplayHeight() - pad.h * 2));

	// savegames list
	m_savedGameList = new DropList(strip, Vec2i(0), Vec2i(300, 34));
	m_savedGameList->setCell(0);
	m_savedGameList->SelectionChanged.connect(this, &MenuStateLoadGame::onSaveSelected);
	m_savedGameList->setAnchors(centreAnchors);

	// savegame info box
	m_infoLabel = new ScrollText(strip, Vec2i(0), Vec2i(600, 200));
	m_infoLabel->setCell(1);
	m_infoLabel->setAnchors(centreAnchors);

	CellStrip *btnPnl = new CellStrip(strip, Orientation::HORIZONTAL, Origin::CENTRE, 3);
	btnPnl->setCell(2);
	btnPnl->setAnchors(fillAnchors);

	int h = g_widgetConfig.getDefaultItemHeight();
	Vec2i sz(7 * h, h);
	// buttons
	m_returnButton = new Button(btnPnl, Vec2i(0), sz);
	m_returnButton->setCell(0);
	m_returnButton->setText(g_lang.get("Return"));
	m_returnButton->Clicked.connect(this, &MenuStateLoadGame::onButtonClick);
	m_returnButton->setAnchors(centreAnchors);

	m_deleteButton = new Button(btnPnl, Vec2i(0), sz);
	m_deleteButton->setCell(1);
	m_deleteButton->setText(g_lang.get("Delete"));
	m_deleteButton->Clicked.connect(this, &MenuStateLoadGame::onButtonClick);
	m_deleteButton->setAnchors(centreAnchors);

	m_playNowButton = new Button(btnPnl, Vec2i(0), sz);
	m_playNowButton->setCell(2);
	m_playNowButton->setText(g_lang.get("PlayNow"));
	m_playNowButton->Clicked.connect(this, &MenuStateLoadGame::onButtonClick);
	m_playNowButton->setAnchors(centreAnchors);

	Vec2i dim = g_metrics.getScreenDims();

	if (!loadGameList()) {
		Vec2i sz = g_widgetConfig.getDefaultDialogSize();
		strip->clear();
		m_messageDialog = MessageDialog::showDialog(g_metrics.getScreenDims() / 2 - sz / 2, sz,
			g_lang.get("Error"), g_lang.get("NoSavedGames"), g_lang.get("Ok"), "");
		m_messageDialog->Button1Clicked.connect(this, &MenuStateLoadGame::onConfirmReturn);
		m_messageDialog->Close.connect(this, &MenuStateLoadGame::onConfirmReturn);
	}
	program.setFade(0.f);
}

MenuStateLoadGame::~MenuStateLoadGame() {
	loaderThread.goAway();
	loaderThread.join();
}

void MenuStateLoadGame::onButtonClick(Widget *source) {
	Button* btn = static_cast<Button*>(source);
	if (btn == m_returnButton) {
		m_targetTransition = Transition::RETURN;
		g_soundRenderer.playFx(g_coreData.getClickSoundA());
		doFadeOut();
	} else if (btn == m_deleteButton) {
		g_soundRenderer.playFx(g_coreData.getClickSoundC());
		Vec2i sz = g_widgetConfig.getDefaultDialogSize();

		const string &fileName = m_savedGameList->getSelectedItem()->getText();
		m_messageDialog = MessageDialog::showDialog(g_metrics.getScreenDims() / 2 - sz / 2, sz, 
			g_lang.get("Confirm"), g_lang.get("Delete") + " " + fileName + "?", 
			g_lang.get("Yes"), g_lang.get("No"));
		m_messageDialog->Button1Clicked.connect(this, &MenuStateLoadGame::onConfirmDelete);
		m_messageDialog->Button2Clicked.connect(this, &MenuStateLoadGame::onCancelDelete);
		m_messageDialog->Close.connect(this, &MenuStateLoadGame::onCancelDelete);
		program.setFade(0.5f);

	} else if (btn == m_playNowButton) {
		m_targetTransition = Transition::PLAY;
		g_soundRenderer.playFx(g_coreData.getClickSoundC());
		doFadeOut();
	}
}

void MenuStateLoadGame::onCancelDelete(Widget*) {
	program.setFade(1.0f);
	program.removeFloatingWidget(m_messageDialog);
}

void MenuStateLoadGame::onConfirmDelete(Widget*) {
	program.setFade(1.0f);
	program.removeFloatingWidget(m_messageDialog);

	FSFactory::getInstance()->removeFile(getFileName());
	if (!loadGameList()) {
		m_targetTransition = Transition::RETURN;
		doFadeOut();
	}
}

void MenuStateLoadGame::onConfirmReturn(Widget*) {
	g_soundRenderer.playFx(g_coreData.getClickSoundA());
	m_targetTransition = Transition::RETURN;
	mainMenu->setCameraTarget(MenuStates::ROOT);
	doFadeOut();
}

void MenuStateLoadGame::onSaveSelected(Widget*) {
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
	g_simInterface.getGameSettings() = *gs;
	g_simInterface.getSavedGame() = root;
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

		///@todo track time properly?
		int elapsedSeconds = savedGame->getChild("world")->getChildIntValue("frameCount") / WORLD_FPS;
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

		string mapDescr = " (" + g_lang.get("MaxPlayers") + ": " + intToStr(mapInfo.players)
				+ ", " + g_lang.get("Size") + ": " + intToStr(mapInfo.size.x) + " x " + intToStr(mapInfo.size.y) + ")";

		const string techName = basename(gs->getTechPath());
		g_lang.loadTechStrings(techName);

		stringstream ss;
		ss  << m_savedGameList->getSelectedItem()->getText() << ": " << gs->getDescription()
			<< endl << g_lang.get("Techtree") << ": " << formatString(basename(gs->getTechPath()))
			<< endl << g_lang.get("Tileset") << ": " << formatString(basename(gs->getTilesetPath()))
			<< endl << g_lang.get("Map") << ": " << formatString(basename(gs->getMapPath())) << mapDescr
			<< endl << g_lang.get("ElapsedTime") << ": " << elapsedTime
			<< endl;

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
			const string rawFacName = basename(gs->getFactionTypeName(i));
			string facName = g_lang.getTechString(rawFacName);
			if (facName == rawFacName) {
				facName = formatString(rawFacName);
			}
			ss	<< "\nPlayer " << i << ": " << g_lang.get(ControlTypeNames[gs->getFactionControl(i)])
				<< " : " << facName << " Team: " << gs->getTeam(i);
		}
		m_infoLabel->setText(ss.str());
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
