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
#include "menu_state_new_game.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "metrics.h"
#include "network_message.h"
#include "server_interface.h"
#include "conversion.h"
#include "socket.h"
#include "game.h"
#include "random.h"
#include "auto_test.h"

#include "leak_dumper.h"
#include "sim_interface.h"

// debug
#include "xml_parser.h"
using namespace Shared::Xml;

using Glest::Sim::SimulationInterface;
using namespace Glest::Net;

namespace Glest { namespace Menu {

using namespace Shared::Util;

// =====================================================
// 	class Announcer
// =====================================================

void AnnouncerThread::execute() {
	static int counter = 1;
	while (m_running) {
		if (m_freeSlots) {
			if (counter % 10 == 0) {
				try {
					m_socket.sendAnnounce(4950); ///@todo change with game constant port
				} catch (SocketException) {
					// do nothing
					printf("SocketException while announcing game on LAN.\n");
				}
			}
		}
		sleep(100);
	}
}

// =====================================================
//  class MenuStateNewGame
// =====================================================

MenuStateNewGame::MenuStateNewGame(Program &program, MainMenu *mainMenu, bool openNetworkSlots)
		: MenuState(program, mainMenu)
		, m_targetTransition(Transition::INVALID)
		, m_humanSlot(0)
		, m_origMusicVolume(1.f)
		, m_fadeMusicOut(false) {
//	_PROFILE_FUNCTION();
	const Metrics &metrics = Metrics::getInstance();
	Lang &lang = Lang::getInstance();
	Font *font = g_widgetConfig.getMenuFont()[FontSize::NORMAL];

	// initialize network interface
	// just set to SERVER now, we'll change it back to LOCAL if necessary before launch
	program.getSimulationInterface()->changeRole(GameRole::SERVER);
	GameSettings &gs = g_simInterface.getGameSettings();

	vector<string> results;
	const int defWidgetHeight = g_widgetConfig.getDefaultItemHeight();
	const int defCellHeight = defWidgetHeight * 3 / 2;

	// top level strip
	CellStrip *topStrip = 
		new CellStrip(static_cast<Container*>(&program), Orientation::VERTICAL, Origin::FROM_TOP, 4);
	Vec2i pad(15, 45);
	topStrip->setPos(pad);
	topStrip->setSize(Vec2i(g_config.getDisplayWidth() - pad.w * 2, g_config.getDisplayHeight() - pad.h * 2));
	// cell 0 : player slot space
	// cell 1 : strip for random locations, shroud and fog check boxes
	// cell 2 : strip for map label/drop-list/info, tilset drop-list and tech drop-list
	// cell 3 : strip for return and play buttons
	topStrip->setSizeHint(0, SizeHint());                      // all remaining space
	topStrip->setSizeHint(1, SizeHint(-1, 1 * defCellHeight)); // one default item height
	topStrip->setSizeHint(2, SizeHint(-1, 3 * defCellHeight)); // 3 * default item height
	topStrip->setSizeHint(3, SizeHint(10));                    // 10 %

	Anchors a(Anchor(AnchorType::RIGID, 0));
	Anchors a2(Anchor(AnchorType::SPRINGY, 5), Anchor(AnchorType::RIGID, 0));
	Anchors a3;
	a3.setCentre(true);
	
	// slot widget container
	CellStrip *strip = 
		new CellStrip(topStrip, Orientation::VERTICAL, Origin::CENTRE, GameConstants::maxPlayers + 1);
	strip->setCell(0);
	strip->setAnchors(a2);

	PlayerSlotLabels *labels = new PlayerSlotLabels(strip);
	labels->setCell(0);
	labels->setAnchors(a3);
	labels->setSize(Vec2i(topStrip->getWidth() * 90 / 100, defWidgetHeight));

	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		m_playerSlots[i] = new PlayerSlotWidget(strip);
		m_playerSlots[i]->setCell(i + 1);
		m_playerSlots[i]->setNameText(string("Player #") + intToStr(i + 1));
		m_playerSlots[i]->setSelectedColour(i);
		m_playerSlots[i]->setAnchors(a3);
		m_playerSlots[i]->setSize(Vec2i(topStrip->getWidth() * 90 / 100, defWidgetHeight));
	}

	// check-box strip
	strip = new CellStrip(topStrip, Orientation::HORIZONTAL, Origin::CENTRE, 3);
	strip->setCell(1);
	strip->setAnchors(a);

	CellStrip *combo = new CellStrip(strip, Orientation::HORIZONTAL, Origin::CENTRE, 2);
	combo->setCell(0);
	combo->setAnchors(a);

	combo->setSizeHint(0, SizeHint());
	combo->setSizeHint(1, SizeHint(-1, defWidgetHeight * 3));

	m_randomLocsLabel = new StaticText(combo);
	m_randomLocsLabel->setCell(0);
	m_randomLocsLabel->setTextParams(lang.get("RandomizeLocations"), Vec4f(1.f), font);
	m_randomLocsLabel->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	m_randomLocsLabel->setAnchors(a);

	m_randomLocsCheckbox = new CheckBox(combo);
	m_randomLocsCheckbox->setCell(1);
	m_randomLocsCheckbox->setAnchors(a3);
	m_randomLocsCheckbox->Clicked.connect(this, &MenuStateNewGame::onCheckChanged);

	combo = new CellStrip(strip, Orientation::HORIZONTAL, Origin::CENTRE, 2);
	combo->setCell(1);
	combo->setAnchors(a);
	combo->setSizeHint(0, SizeHint());
	combo->setSizeHint(1, SizeHint(-1, defWidgetHeight * 3));

	m_SODLabel = new StaticText(combo);
	m_SODLabel->setCell(0);
	m_SODLabel->setTextParams(lang.get("ShroudOfDarkness"), Vec4f(1.f), font);
	m_SODLabel->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	m_SODLabel->setAnchors(a);

	m_SODCheckbox = new CheckBox(combo);
	m_SODCheckbox->setCell(1);
	m_SODCheckbox->setAnchors(a3);
	m_SODCheckbox->setChecked(true);
	m_SODCheckbox->Clicked.connect(this, &MenuStateNewGame::onCheckChanged);

	combo = new CellStrip(strip, Orientation::HORIZONTAL, Origin::CENTRE, 2);
	combo->setCell(2);
	combo->setAnchors(a);
	combo->setSizeHint(0, SizeHint());
	combo->setSizeHint(1, SizeHint(-1, defWidgetHeight * 3));

	m_FOWLabel = new StaticText(combo);
	m_FOWLabel->setCell(0);
	m_FOWLabel->setTextParams(lang.get("FogOfWar"), Vec4f(1.f), font);
	m_FOWLabel->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	m_FOWLabel->setAnchors(a);

	m_FOWCheckbox = new CheckBox(combo);
	m_FOWCheckbox->setCell(1);
	m_FOWCheckbox->setAnchors(a3);
	m_FOWCheckbox->setChecked(true);
	m_FOWCheckbox->Clicked.connect(this, &MenuStateNewGame::onCheckChanged);

	// Map / Tileset / Tech-Tree
	strip = new CellStrip(topStrip, Orientation::HORIZONTAL, Origin::CENTRE, 3);
	strip->setCell(2);
	strip->setAnchors(a);

	// map listBox & info
	combo = new CellStrip(strip, Orientation::VERTICAL, Origin::CENTRE, 3);
	combo->setCell(0);
	a2 = Anchors(Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 5));
	combo->setAnchors(a2);
	int mttHints[] = { 25, 25, 50}; // 25 % for label and drop list, 50 % for info
	combo->setPercentageHints(mttHints);

	if (!getMapList()) {
		throw runtime_error("There are no maps");
	}
	results.clear();
	foreach (vector<string>, it, m_mapFiles) {
		results.push_back(formatString(*it));
	}
	m_mapLabel = new StaticText(combo);
	m_mapLabel->setCell(0);
	m_mapLabel->setAnchors(a);
	m_mapLabel->setTextParams(lang.get("Map"), Vec4f(1.f), font);
	m_mapLabel->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

	m_mapList = new DropList(combo);
	m_mapList->setCell(1);
	m_mapList->setSize(Vec2i(8 * defWidgetHeight, defWidgetHeight));
	m_mapList->setAnchors(a3);
	m_mapList->addItems(results);
	m_mapList->setDropBoxHeight(140);
	m_mapList->setSelected(0);
	m_mapList->SelectionChanged.connect(this, &MenuStateNewGame::onChangeMap);

	gs.setDescription(results[0]);
	gs.setMapPath(string("maps/") + m_mapFiles[0]);
	m_mapInfo.load("maps/" + m_mapFiles[0]);

	m_mapInfoLabel = new StaticText(combo);
	m_mapInfoLabel->setCell(2);
	m_mapInfoLabel->setAnchors(a);
	m_mapInfoLabel->setTextParams(m_mapInfo.desc, Vec4f(1.f), font);
	m_mapInfoLabel->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

	// tileset listBox
	combo = new CellStrip(strip, Orientation::VERTICAL, Origin::CENTRE, 3);
	combo->setCell(1);
	combo->setPercentageHints(mttHints);
	combo->setAnchors(a2);

	findAll("tilesets/*.", results);
	if (results.size() == 0) {
		throw runtime_error("There are no tile sets");
	}
	m_tilesetFiles = results;
	for (int i = 0; i < results.size(); ++i) {
		results[i] = formatString(results[i]);
	}
	m_tilesetLabel = new StaticText(combo);
	m_tilesetLabel->setCell(0);
	m_tilesetLabel->setAnchors(a);
	m_tilesetLabel->setTextParams(lang.get("Tileset"), Vec4f(1.f), font);
	m_tilesetLabel->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

	m_tilesetList = new DropList(combo);
	m_tilesetList->setCell(1);
	m_tilesetList->setSize(Vec2i(8 * defWidgetHeight, defWidgetHeight));
	m_tilesetList->setAnchors(a3);
	m_tilesetList->addItems(results);
	m_tilesetList->setDropBoxHeight(140);
	m_tilesetList->SelectionChanged.connect(this, &MenuStateNewGame::onChangeTileset);
	m_tilesetList->setSelected(0);

	//tech Tree listBox
	combo = new CellStrip(strip, Orientation::VERTICAL, Origin::CENTRE, 3);
	combo->setCell(2);
	combo->setPercentageHints(mttHints);
	combo->setAnchors(a2);

	findAll("techs/*.", results);
	if (results.size() == 0) {
		throw runtime_error("There are no tech trees");
	}
	m_techTreeFiles = results;
	for (int i = 0; i < results.size(); ++i) {
		results[i] = formatString(results[i]);
	}
	m_techTreeLabel = new StaticText(combo);
	m_techTreeLabel->setCell(0);
	m_techTreeLabel->setAnchors(a);
	m_techTreeLabel->setTextParams(lang.get("Techtree"), Vec4f(1.f), font);
	m_techTreeLabel->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

	m_techTreeList = new DropList(combo);
	m_techTreeList->setCell(1);
	m_techTreeList->setSize(Vec2i(8 * defWidgetHeight, defWidgetHeight));
	m_techTreeList->setAnchors(a3);
	m_techTreeList->addItems(results);
	m_techTreeList->setDropBoxHeight(140);
	m_techTreeList->setSelected(0);
	m_techTreeList->SelectionChanged.connect(this, &MenuStateNewGame::onChangeTechtree);

	// Buttons strip
	strip = new CellStrip(topStrip, Orientation::HORIZONTAL, Origin::CENTRE, 2);
	strip->setCell(3);
	strip->setAnchors(a);

	int w = 8 * defWidgetHeight, h = defWidgetHeight;
	m_returnButton = new Button(strip, Vec2i(0), Vec2i(w, h));
	m_returnButton->setCell(0);
	m_returnButton->setAnchors(a3);
	m_returnButton->setTextParams(lang.get("Return"), Vec4f(1.f), font);
	m_returnButton->Clicked.connect(this, &MenuStateNewGame::onButtonClick);

	m_playNow = new Button(strip, Vec2i(0), Vec2i(w, h));
	m_playNow->setCell(1);
	m_playNow->setAnchors(a3);
	m_playNow->setTextParams(lang.get("PlayNow"), Vec4f(1.f), font);
	m_playNow->Clicked.connect(this, &MenuStateNewGame::onButtonClick);

	reloadFactions(true);

	// init controllers
	m_playerSlots[0]->setSelectedControl(ControlType::HUMAN);
	m_playerSlots[0]->setSelectedTeam(0);
	int i = 1;
	if (openNetworkSlots) {
		for (; i < m_mapInfo.players; ++i) {
			m_playerSlots[i]->setSelectedControl(ControlType::NETWORK);
			m_playerSlots[i]->setSelectedTeam(i);
		}
	} else {
		m_playerSlots[i]->setSelectedControl(ControlType::CPU);
		m_playerSlots[i]->setSelectedTeam(1);
		++i;
	}
	for ( ; i < GameConstants::maxPlayers; ++i) {
		m_playerSlots[i]->setSelectedControl(ControlType::CLOSED);
	}
	updateControlers();
	updateNetworkSlots();

	if (fileExists("last_gamesettings.gs")) {
		if (loadGameSettings()) {
			updateNetworkSlots();
		}
	}
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		m_playerSlots[i]->ControlChanged.connect(this, &MenuStateNewGame::onChangeControl);
		m_playerSlots[i]->FactionChanged.connect(this, &MenuStateNewGame::onChangeFaction);
		m_playerSlots[i]->TeamChanged.connect(this, &MenuStateNewGame::onChangeTeam);
		m_playerSlots[i]->ColourChanged.connect(this, &MenuStateNewGame::onChangeColour);
	}
}

//  === util ===

bool MenuStateNewGame::getMapList() {
	vector<string> results;
	set<string> mapFiles;
	// FIXME: change findAll to return empty result when nothing found
	try {
		findAll("maps/*.gbm", results, true);
	} catch (runtime_error err) { 
	}
	foreach (vector<string>, it, results) {
		mapFiles.insert(*it);
	}
	results.clear();

	try {
		findAll("maps/*.mgm", results, true);
	} catch (runtime_error err) { }
	foreach (vector<string>, it, results) {
		mapFiles.insert(*it);
	}
	results.clear();

	if (mapFiles.empty()) {
		return false;
	}
	m_mapFiles.clear();
	foreach (set<string>, it, mapFiles) {
		m_mapFiles.push_back(*it);
	}
	return true;
}

int getSlotIndex(PlayerSlotWidget* psw, PlayerSlotWidget* *slots) {
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (psw == slots[i]) {
			return i;
		}
	}
	assert(false);
	return -1;
}

int getLowestFreeColourIndex(PlayerSlotWidget* *slots) {
	bool colourUsed[GameConstants::maxColours];
	for (int i=0; i < GameConstants::maxColours; ++i) {
		colourUsed[i] = false;
	}
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (slots[i]->getSelectedColourIndex() != -1) {
			ASSERT(slots[i]->getControlType() != ControlType::CLOSED, "Closed slot has colour set.");
			colourUsed[slots[i]->getSelectedColourIndex()] = true;
		}
	}
	for (int i=0; i < GameConstants::maxColours; ++i) {
		if (!colourUsed[i]) {
			return i;
		}
	}
	ASSERT(false, "No free colours");
	return -1;
}

//  === === ===

void MenuStateNewGame::onChangeFaction(PlayerSlotWidget* psw) {
	GameSettings &gs = g_simInterface.getGameSettings();
	int ndx = getSlotIndex(psw, m_playerSlots);
	assert(ndx >= 0 && ndx < GameConstants::maxPlayers);
	if (psw->getSelectedFactionIndex() >= 0) {
		int n = psw->getSelectedFactionIndex();
		assert(n >= 0 && n < m_factionFiles.size() + 1); // + 1 for 'Random'
		if (n < m_factionFiles.size()) {
			gs.setFactionTypeName(ndx, m_factionFiles[n]);
		} else {
			gs.setFactionTypeName(ndx, "Random");
		}
	} else {
		gs.setFactionTypeName(ndx, "");
	}
}

void MenuStateNewGame::onChangeControl(PlayerSlotWidget* ps) {
	static bool noRecurse = false;
	if (noRecurse) {
		return; // control was changed progmatically
	}
	noRecurse = true;
	int ndx = getSlotIndex(ps, m_playerSlots);
	if (m_playerSlots[ndx]->getControlType() == ControlType::HUMAN && ndx != m_humanSlot) {
		// human moved slots
		assert(m_humanSlot >= 0);
		m_playerSlots[m_humanSlot]->setSelectedControl(ControlType::CLOSED);
		m_playerSlots[ndx]->setSelectedColour(m_playerSlots[m_humanSlot]->getSelectedColourIndex());
		m_humanSlot = ndx;
	} else {
		// check for humanity
		if (m_playerSlots[m_humanSlot]->getControlType() != ControlType::HUMAN) {
			// human tried to go away...
			if (m_humanSlot != 0) {
				m_playerSlots[m_humanSlot]->setSelectedControl(ControlType::CLOSED);
			}
			m_playerSlots[0]->setSelectedControl(ControlType::HUMAN);
			m_playerSlots[0]->setSelectedColour(getLowestFreeColourIndex(m_playerSlots));
			noRecurse = false;
			return;
		}
	}
	updateControlers();
	updateNetworkSlots();
	noRecurse = false;
}

void MenuStateNewGame::onChangeTeam(PlayerSlotWidget* psw) {
	GameSettings &gs = g_simInterface.getGameSettings();
	int ndx = getSlotIndex(psw, m_playerSlots);
	assert(ndx >= 0 && ndx < GameConstants::maxPlayers);
	gs.setTeam(ndx, psw->getSelectedTeamIndex());
}

void MenuStateNewGame::onChangeColour(PlayerSlotWidget* psw) {
	GameSettings &gs = g_simInterface.getGameSettings();
	int ndx = getSlotIndex(psw, m_playerSlots);
	assert(ndx >= 0 && ndx < GameConstants::maxPlayers);
	int ci = psw->getSelectedColourIndex();
	gs.setColourIndex(ndx, ci);
	if (ci == -1) {
		return;
	}
	assert(ci >= 0 && ci < GameConstants::maxColours);
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (ndx == i) continue;
		if (m_playerSlots[i]->getSelectedColourIndex() == ci) {
			m_playerSlots[i]->setSelectedColour(getLowestFreeColourIndex(m_playerSlots));
			gs.setColourIndex(i, m_playerSlots[i]->getSelectedColourIndex());
		}
	}
}

void MenuStateNewGame::onCheckChanged(Button* cb) {
	GameSettings &gs = g_simInterface.getGameSettings();
	if (cb == m_FOWCheckbox) {
		gs.setFogOfWar(m_FOWCheckbox->isChecked());
	} else if (cb == m_SODCheckbox) {
		gs.setShroudOfDarkness(m_SODCheckbox->isChecked());
	} else if (cb == m_randomLocsCheckbox) {
		gs.setRandomStartLocs(m_randomLocsCheckbox->isChecked());
	} else {
		ASSERT(false, "Unknown CheckBox!?!");
	}

}

void MenuStateNewGame::onChangeMap(ListBase*) {
	assert(m_mapList->getSelectedIndex() >= 0 && m_mapList->getSelectedIndex() < m_mapFiles.size());
	string mapBaseName = m_mapFiles[m_mapList->getSelectedIndex()];
	string mapFile = "maps/" + mapBaseName;

	m_mapInfo.load(mapFile);
	m_mapInfoLabel->setText(m_mapInfo.desc);

	updateControlers();

	GameSettings &gs = g_simInterface.getGameSettings();
	gs.setDescription(formatString(mapBaseName));
	gs.setMapPath(mapFile);
}

void MenuStateNewGame::onChangeTileset(ListBase*) {
	GameSettings &gs = g_simInterface.getGameSettings();
	assert(m_tilesetList->getSelectedIndex() >= 0);
	gs.setTilesetPath(string("tilesets/") + m_tilesetFiles[m_tilesetList->getSelectedIndex()]);
}

void MenuStateNewGame::onChangeTechtree(ListBase*) {
	reloadFactions(true);
}

void MenuStateNewGame::onButtonClick(Button* btn) {
	if (btn == m_returnButton) {
		m_targetTransition = Transition::RETURN;
		mainMenu->setCameraTarget(MenuStates::ROOT);
		g_soundRenderer.playFx(CoreData::getInstance().getClickSoundA());
	} else {
		m_targetTransition = Transition::PLAY;
		g_soundRenderer.playFx(CoreData::getInstance().getClickSoundC());
		m_origMusicVolume = g_coreData.getMenuMusic()->getVolume();
		m_fadeMusicOut = true;
	}
	doFadeOut();
}

void MenuStateNewGame::onDismissDialog(BasicDialog*) {
	program.removeFloatingWidget(m_messageDialog);
	doFadeIn();
}

void MenuStateNewGame::update() {
	MenuState::update();

	if (m_fadeMusicOut) {
		float vol = m_origMusicVolume * m_fade;
		g_coreData.getMenuMusic()->setVolume(vol);
	}

	bool configAnnounce = true; ///@todo put in config
	if (configAnnounce) {
		m_announcer.doAnnounce(hasUnconnectedSlots());
	}

	static int counter = 0;
	if (++counter % 6 == 0) { // update controlers periodically to get network player names
		updateControlers();
	}

	if (m_transition) {
		if (m_targetTransition == Transition::RETURN) {
			program.clear();
			mainMenu->setState(new MenuStateRoot(program, mainMenu));
		} else if (m_targetTransition == Transition::PLAY) {
			if (hasUnconnectedSlots()) {
				Vec2i sz(330, 256);
				m_messageDialog = MessageDialog::showDialog(g_metrics.getScreenDims() / 2 - sz / 2,
					sz, g_lang.get("Error"), g_lang.get("WaitingForConnections"), g_lang.get("Ok"), "");
				m_messageDialog->Button1Clicked.connect(this, &MenuStateNewGame::onDismissDialog);
				m_transition = false;
			} else {
				GameSettings &gs = g_simInterface.getGameSettings();
				gs.compact();
				g_config.save();
				XmlTree *doc = new XmlTree("game-settings");
				gs.save(doc->getRootNode());
				doc->save("last_gamesettings.gs");
				delete doc;
				if (gs.getRandomStartLocs()) {
					randomiseStartLocs();
				}
				randomiseFactions();
				if (!hasNetworkSlots()) {
					GameSettings gs = g_simInterface.getGameSettings();
					program.getSimulationInterface()->changeRole(GameRole::LOCAL);
					g_simInterface.getGameSettings() = gs;
					program.clear();
					program.setState(new GameState(program));
				} else {
					g_simInterface.asServerInterface()->doLaunchBroadcast();
					program.clear();
					program.setState(new GameState(program));
				}
			}
		}
	}
}

// ============ PRIVATE ===========================

void MenuStateNewGame::reloadFactions(bool setStagger) {
	GameSettings &gs = g_simInterface.getGameSettings();
	vector<string> results;
	assert(m_techTreeList->getSelectedIndex() >= 0);
	findAll("techs/" + m_techTreeFiles[m_techTreeList->getSelectedIndex()] + "/factions/*.", results);
	if (results.empty()) {
		throw runtime_error("There are no factions for this tech tree");
	}
	gs.setTechPath(string("techs/") + m_techTreeFiles[m_techTreeList->getSelectedIndex()]);

	m_factionFiles.clear();
	m_factionFiles = results;
	for (int i = 0; i < results.size(); ++i) {
		results[i] = formatString(results[i]);
	}
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		m_playerSlots[i]->setFactionItems(results);
		if (setStagger) {
			if (m_playerSlots[i]->getControlType() != ControlType::CLOSED) {
				m_playerSlots[i]->setSelectedFaction(i % results.size());
			}
		}
	}
}

bool MenuStateNewGame::loadGameSettings() {
	XmlTree *doc = new XmlTree();
	try {
		doc->load("last_gamesettings.gs");
		GameSettings gs(doc->getRootNode());
		g_simInterface.getGameSettings() = gs;
	} catch (const runtime_error &e) {
		delete doc;
		return false;
	}
	GameSettings &gs = g_simInterface.getGameSettings();
	bool techReset = false;
	vector<string>::iterator it;
	it = std::find(m_techTreeFiles.begin(), m_techTreeFiles.end(), basename(gs.getTechPath()));
	if (it != m_techTreeFiles.end()) {
	m_techTreeList->setSelected(formatString(basename(gs.getTechPath())));
	} else {
		m_techTreeList->setSelected(0);
		string s = gs.getTechPath();
		gs.setTechPath("techs/" + m_techTreeFiles[0]);
		techReset = true;
	}
	it = std::find(m_tilesetFiles.begin(), m_tilesetFiles.end(), basename(gs.getTilesetPath()));
	if (it != m_tilesetFiles.end()) {
	m_tilesetList->setSelected(formatString(basename(gs.getTilesetPath())));
	} else {
		m_tilesetList->setSelected(0);
		gs.setTilesetPath("tilesets/" + m_tilesetFiles[0]);
	}
	reloadFactions(false);
	for (int i=0 ; i < GameConstants::maxPlayers; ++i) {
		m_playerSlots[i]->setSelectedControl(ControlType::CLOSED);
	}
	for (int i=0; i < gs.getFactionCount(); ++i) {
		int slot = gs.getStartLocationIndex(i);
		m_playerSlots[slot]->setSelectedControl(gs.getFactionControl(i));
		switch (gs.getFactionControl(i)) {
			case ControlType::HUMAN:
				m_humanSlot = slot;
				m_playerSlots[slot]->setNameText(g_config.getNetPlayerName());
				break;
			case ControlType::NETWORK:
				m_playerSlots[slot]->setNameText(g_lang.get("Network") + ": " + g_lang.get("NotConnected"));
				break;
			default:
				m_playerSlots[slot]->setNameText(g_lang.get("AiPlayer"));
				break;
		}
		int ndx = -1;
		string fName = gs.getFactionTypeName(i);
		if (fName == "Random") {
			ndx = m_factionFiles.size();
		} else {
			if (techReset) {
				ndx = i % m_factionFiles.size();
			} else {
			foreach (vector<string>, it, m_factionFiles) {
				++ndx;
				if (*it == fName) {
					break;
				}
			}
		}
		}
		m_playerSlots[slot]->setSelectedFaction(ndx);
		m_playerSlots[slot]->setSelectedTeam(gs.getTeam(i));
		m_playerSlots[slot]->setSelectedColour(gs.getColourIndex(i));
		//m_playerSlots[i]->setStartLocation(gs.getStartLocationIndex(i));
	}
	string mapFile = basename(gs.getMapPath());
	
	string mapPath = gs.getMapPath();
	if (!fileExists(mapPath + ".gbm") && !fileExists(mapPath + ".mgm")) {
		mapFile = "maps/" + m_mapFiles[0];
		gs.setMapPath(mapFile);

	}
//	m_mapList->SelectionChanged.disconnect(this);
	m_mapList->setSelected(formatString(mapFile));
//	m_mapList->SelectionChanged.connect(this, &MenuStateNewGame::onChangeMap);
	m_mapInfo.load(gs.getMapPath());
	m_mapInfoLabel->setText(m_mapInfo.desc);
	m_randomLocsCheckbox->setChecked(gs.getRandomStartLocs());
	m_FOWCheckbox->setChecked(gs.getFogOfWar());
	m_SODCheckbox->setChecked(gs.getShroudOfDarkness());

	delete doc;
	updateControlers();
	return true;
}

void MenuStateNewGame::updateControlers() {
	GameSettings &gs = g_simInterface.getGameSettings();
	assert(m_humanSlot >= 0);
	//assert(m_playerSlots[m_humanSlot]->getControlType() == ControlType::HUMAN);
	for (int i = 0; i < m_mapInfo.players; ++i) {
		m_playerSlots[i]->setEnabled(true);
	}
	for (int i = m_mapInfo.players; i < GameConstants::maxPlayers; ++i) {
		m_playerSlots[i]->setSelectedControl(ControlType::CLOSED);
		m_playerSlots[i]->setEnabled(false);
	}
	if (m_humanSlot >= m_mapInfo.players) {
		m_humanSlot = 0;
		m_playerSlots[m_humanSlot]->setSelectedControl(ControlType::HUMAN);
		m_playerSlots[m_humanSlot]->setSelectedColour(getLowestFreeColourIndex(m_playerSlots));
	}
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		gs.setFactionControl(i, m_playerSlots[i]->getControlType());
		if (m_playerSlots[i]->getControlType() == ControlType::CLOSED) {
			gs.setFactionTypeName(i, "");
			gs.setPlayerName(i, "");
			gs.setTeam(i, -1);
			gs.setStartLocationIndex(i, -1);
			gs.setColourIndex(i, -1);
			gs.setResourceMultiplier(i, 1.f);
			m_playerSlots[i]->setNameText("No Player");
			m_playerSlots[i]->setSelectedFaction(-1);
			m_playerSlots[i]->setSelectedTeam(-1);
			m_playerSlots[i]->setSelectedColour(-1);
			if (i < m_mapInfo.players) {
				m_playerSlots[i]->setFree(true);
			} else {
				m_playerSlots[i]->setEnabled(false);
			}
		} else {
			if (m_playerSlots[i]->getSelectedFactionIndex() == -1) {
				m_playerSlots[i]->setSelectedFaction(i % m_factionFiles.size());
			}
			if (m_playerSlots[i]->getSelectedTeamIndex() == -1) {
				m_playerSlots[i]->setSelectedTeam(i);
			}
			if (m_playerSlots[i]->getSelectedColourIndex() == -1) {
				m_playerSlots[i]->setSelectedColour(getLowestFreeColourIndex(m_playerSlots));
			}
			assert(m_playerSlots[i]->getSelectedFactionIndex() >= 0 && m_playerSlots[i]->getSelectedFactionIndex() < GameConstants::maxPlayers);
			int factionIndex = m_playerSlots[i]->getSelectedFactionIndex();
			if (factionIndex < m_factionFiles.size()) {
				gs.setFactionTypeName(i, m_factionFiles[factionIndex]);
			} else {
				gs.setFactionTypeName(i, "Random");
			}
			switch (m_playerSlots[i]->getControlType()) {
				case ControlType::HUMAN:
					gs.setPlayerName(i, g_config.getNetPlayerName());
					m_playerSlots[i]->setNameText(g_config.getNetPlayerName());
					break;
				case ControlType::NETWORK: {
						ConnectionSlot *slot = g_simInterface.asServerInterface()->getSlot(i);
						if (!slot) {
							g_simInterface.asServerInterface()->addSlot(i);
							slot = g_simInterface.asServerInterface()->getSlot(i);
						}
						if (slot->isConnected()) {
							gs.setPlayerName(i, slot->getName());
							m_playerSlots[i]->setNameText(slot->getName());
						} else {
							m_playerSlots[i]->setNameText(g_lang.get("NotConnected"));
						}
					}
					break;
				default:
					gs.setPlayerName(i, "AI Player");
					m_playerSlots[i]->setNameText(g_lang.get("AiPlayer"));
			}
			switch (m_playerSlots[i]->getControlType()) {
				case ControlType::HUMAN:
				case ControlType::NETWORK:
				case ControlType::CPU:
					gs.setResourceMultiplier(i, 1.f);
					break;
				case ControlType::CPU_EASY:
					gs.setResourceMultiplier(i, 0.5f);
					break;
				case ControlType::CPU_ULTRA:
					gs.setResourceMultiplier(i, 3.f);
					break;
				case ControlType::CPU_MEGA:
					gs.setResourceMultiplier(i, 4.f);
					break;
			}

			assert(m_playerSlots[i]->getSelectedTeamIndex() >= 0 && m_playerSlots[i]->getSelectedTeamIndex() < GameConstants::maxPlayers);
			gs.setTeam(i, m_playerSlots[i]->getSelectedTeamIndex());
			gs.setStartLocationIndex(i, i);
			assert(m_playerSlots[i]->getSelectedColourIndex() >= 0 && m_playerSlots[i]->getSelectedColourIndex() < GameConstants::maxColours);
			gs.setColourIndex(i, m_playerSlots[i]->getSelectedColourIndex());
			m_playerSlots[i]->setFree(false);
		}
		if (m_playerSlots[i]->getControlType() == ControlType::HUMAN) {
			gs.setThisFactionIndex(i);
		}
	}
}

bool MenuStateNewGame::hasUnconnectedSlots() {
	ServerInterface* serverInterface = g_simInterface.asServerInterface();
	for (int i = 0; i < m_mapInfo.players; ++i) {
		if (m_playerSlots[i]->getControlType() == ControlType::NETWORK) {
			if (!serverInterface->getSlot(i)->isConnected()) {
				return true;
			}
		}
	}
	return false;
}

bool MenuStateNewGame::hasNetworkSlots() {
	for (int i = 0; i < m_mapInfo.players; ++i) {
		if (m_playerSlots[i]->getControlType() == ControlType::NETWORK) {
			return true;
		}
	}
	return false;
}

void MenuStateNewGame::updateNetworkSlots() {
	ServerInterface* serverInterface = g_simInterface.asServerInterface();
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		if (serverInterface->getSlot(i) == NULL
		&& m_playerSlots[i]->getControlType() == ControlType::NETWORK) {
			serverInterface->addSlot(i);
		}
		if (serverInterface->getSlot(i) != NULL
		&& m_playerSlots[i]->getControlType() != ControlType::NETWORK) {
			serverInterface->removeSlot(i);
		}
	}
}

void MenuStateNewGame::randomiseStartLocs() {
	GameSettings &gs = g_simInterface.getGameSettings();
	vector<int> freeStartLocs;
	for (int i=0; i < m_mapInfo.players; ++i) {
		freeStartLocs.push_back(i);
	}
	Random random(Chrono::getCurMillis());
	for (int i=0; i < gs.getFactionCount(); ++i) {
		int sli = freeStartLocs[random.randRange(0, freeStartLocs.size() - 1)];
		gs.setStartLocationIndex(i, sli);
		freeStartLocs.erase(std::find(freeStartLocs.begin(), freeStartLocs.end(), sli));
	}
}

void MenuStateNewGame::randomiseFactions() {
	GameSettings &gs = g_simInterface.getGameSettings();
	Random random(Chrono::getCurMillis());
	for (int i=0; i < gs.getFactionCount(); ++i) {
		if (gs.getFactionTypeName(i) == "Random") {
			int ndx = random.randRange(0, m_factionFiles.size() - 1);
			gs.setFactionTypeName(i, m_factionFiles[ndx]);
		}
	}
}

//void MenuStateNewGame::randomiseMap() {
//}
//
//void MenuStateNewGame::randomiseTileset() {
//}

}}//end namespace
