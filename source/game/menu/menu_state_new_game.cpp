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
	while (m_running) {
		if (m_freeSlots) {
			try {
				m_socket.sendAnnounce(4950); //TODO: change with game constant port
			} catch (SocketException) {
				// do nothing
				printf("SocketException.\n");
			}
			printf("announcing\n");
		}
		sleep(1000);
	}
}

// =====================================================
//  class MenuStateNewGame
// =====================================================

MenuStateNewGame::MenuStateNewGame(Program &program, MainMenu *mainMenu, bool openNetworkSlots)
		: MenuState(program, mainMenu)
		, m_targetTransition(Transition::INVALID)
		, m_humanSlot(0) {
	_PROFILE_FUNCTION();
	const Metrics &metrics = Metrics::getInstance();
	const CoreData &coreData = CoreData::getInstance();
	Lang &lang = Lang::getInstance();
	Config &config = Config::getInstance();

	Font *font = coreData.getFTMenuFontNormal();

	// initialize network interface
	// just set to SERVER now, we'll change it back to LOCAL if necessary before launch
	program.getSimulationInterface()->changeRole(GameRole::SERVER);
	GameSettings &gs = g_simInterface->getGameSettings();

	vector<string> results;

	// create
	int gap = (metrics.getScreenW() - 300) / 3;
	int x = gap, w = 150, y = 50, h = 30;
	m_returnButton = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	m_returnButton->setTextParams(lang.get("Return"), Vec4f(1.f), font);
	m_returnButton->Clicked.connect(this, &MenuStateNewGame::onButtonClick);

	x += w + gap;
	m_playNow = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	m_playNow->setTextParams(lang.get("PlayNow"), Vec4f(1.f), font);
	m_playNow->Clicked.connect(this, &MenuStateNewGame::onButtonClick);

	gap = (metrics.getScreenW() - 600) / 4;

	//map listBox
	findAll("maps/*.gbm", results, false);

	// hack... _only_ match '.gbm' (and not .gbm8 or anything else like that).
	foreach (vector<string>, it, results) {
		string::size_type dotPos = it->find_last_of('.');
		assert(dotPos != string::npos);
		if (it->substr(dotPos + 1).size() != 3U) {
			it = results.erase(it) - 1;
		} else {
			*it = cutLastExt(*it);
		}
	}

	if (results.empty()) {
		throw runtime_error("There are no maps");
	}
	m_mapFiles = results;
	int match = 0;
	for (int i = 0; i < results.size(); ++i) {
		if (results[i] == config.getUiLastMap()) {
			match = i;
		}
		results[i] = formatString(results[i]);
	}
	x = gap, w = 200, y = 170, h = 30;
	m_mapList = new DropList(&program, Vec2i(x, y), Vec2i(w, h));
	m_mapList->addItems(results);
	m_mapList->setDropBoxHeight(140);
	m_mapList->setSelected(match);
	m_mapList->SelectionChanged.connect(this, &MenuStateNewGame::onChangeMap);

	m_mapLabel = new StaticText(&program, Vec2i(x,  y + h + 5), Vec2i(w, h));
	m_mapLabel->setTextParams(lang.get("Map"), Vec4f(1.f), font);

	gs.setDescription(results[match]);
	gs.setMapPath(string("maps/") + m_mapFiles[match] + ".gbm");

	m_mapInfo.load("maps/" + m_mapFiles[match] + ".gbm");

	m_mapInfoLabel = new StaticText(&program, Vec2i(x, y - (h*2 + 10)), Vec2i(w, h * 2));
	//m_mapInfoLabel->setBorderParams(BorderStyle::SOLID, 2, Vec3f(1.f, 0.f, 0.f), 0.7f);
	m_mapInfoLabel->setTextParams(m_mapInfo.desc, Vec4f(1.f), font);

	//tileset listBox
	match = 0;
	findAll("tilesets/*.", results);
	if (results.size() == 0) {
		throw runtime_error("There are no tile sets");
	}
	m_tilesetFiles = results;
	for (int i = 0; i < results.size(); ++i) {
		if(results[i] == config.getUiLastTileset()) {
			match = i;
		}
		results[i] = formatString(results[i]);
	}
	x = gap * 2 + w;
	m_tilesetList = new DropList(&program, Vec2i(x, y), Vec2i(w, 30));
	m_tilesetList->addItems(results);
	m_tilesetList->setDropBoxHeight(140);
	m_tilesetList->SelectionChanged.connect(this, &MenuStateNewGame::onChangeTileset);
	m_tilesetList->setSelected(match);

	m_tilesetLabel = new StaticText(&program, Vec2i(x, y + h + 5), Vec2i(w, h));
	m_tilesetLabel->setTextParams(lang.get("Tileset"), Vec4f(1.f), font);

	//tech Tree listBox
	match = 0;
	findAll("techs/*.", results);
	if (results.size() == 0) {
		throw runtime_error("There are no tech trees");
	}
	m_techTreeFiles = results;
	for (int i = 0; i < results.size(); ++i) {
		if(results[i] == config.getUiLastTechTree()) {
			match = i;
		}
		results[i] = formatString(results[i]);
	}
	x = gap * 3 + w * 2;
	m_techTreeList = new DropList(&program, Vec2i(x, y), Vec2i(w, h));
	m_techTreeList->addItems(results);
	m_techTreeList->setDropBoxHeight(140);
	m_techTreeList->setSelected(match);
	m_techTreeList->SelectionChanged.connect(this, &MenuStateNewGame::onChangeTechtree);

	m_techTreeLabel = new StaticText(&program, Vec2i(x,  y + h + 5), Vec2i(w, h));
	m_techTreeLabel->setTextParams(lang.get("Techtree"), Vec4f(1.f), font);

	gap = (metrics.getScreenW() - 400) / 3, x = gap, y += 70, h = 30;
	int cbw = 75, stw = 200;

	m_randomLocsCheckbox = new CheckBox(&program, Vec2i(x+65,y), Vec2i(cbw,h));
	m_randomLocsCheckbox->Clicked.connect(this, &MenuStateNewGame::onCheckChanged);
	cout << "CheckBox pref size = " << m_randomLocsCheckbox->getPrefSize() << endl;

	m_randomLocsLabel = new StaticText(&program, Vec2i(x, y + 35), Vec2i(stw,h));
	m_randomLocsLabel->setTextParams(lang.get("RandomizeLocations"), Vec4f(1.f), font);

	x = gap * 2 + stw;
	m_fogOfWarCheckbox = new CheckBox(&program, Vec2i(x+65,y), Vec2i(cbw,h));
	m_fogOfWarCheckbox->setChecked(true);
	m_fogOfWarCheckbox->Clicked.connect(this, &MenuStateNewGame::onCheckChanged);

	m_fogOfWarLabel = new StaticText(&program, Vec2i(x, y + 35), Vec2i(stw, h));
	m_fogOfWarLabel->setTextParams(lang.get("FogOfWar"), Vec4f(1.f), font);

	y += 75, h = 35, x = (metrics.getScreenW() - 700) / 2, w = 700;

	int sty = metrics.getScreenH() - 70;
	m_controlLabel = new StaticText(&program, Vec2i(x + 150, sty), Vec2i(200, 30));
	m_factionLabel = new StaticText(&program, Vec2i(x + 390, sty), Vec2i(200, 30));
	m_teamLabel = new StaticText(&program, Vec2i(x + 620, sty), Vec2i(80, 30));

	m_controlLabel->setTextParams("Control", Vec4f(1.f), font);
	m_factionLabel->setTextParams("Faction", Vec4f(1.f), font);
	m_teamLabel->setTextParams("Team", Vec4f(1.f), font);

	int numPlayer = GameConstants::maxPlayers;

	int vSpace = (sty - y);
	int vgap = (vSpace - (numPlayer * 35)) / (numPlayer + 1);

	for (int i = 0; i < numPlayer; ++i) {
		y = sty - (vgap * (i + 1)) - (h * (i + 1));
		m_playerSlots[i] = new PlayerSlotWidget(&program, Vec2i(x, y), Vec2i(w, h));
		m_playerSlots[i]->setNameText(string("Player #") + intToStr(i+1));
		m_playerSlots[i]->setSelectedColour(i);
	}

	reloadFactions();

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

	for (int i = 0; i < numPlayer; ++i) {
		m_playerSlots[i]->ControlChanged.connect(this, &MenuStateNewGame::onChangeControl);
		m_playerSlots[i]->FactionChanged.connect(this, &MenuStateNewGame::onChangeFaction);
		m_playerSlots[i]->TeamChanged.connect(this, &MenuStateNewGame::onChangeTeam);
		m_playerSlots[i]->ColourChanged.connect(this, &MenuStateNewGame::onChangeColour);
	}

	//CHECK_HEAP();
}

int getSlotIndex(PlayerSlotWidget::Ptr psw, PlayerSlotWidget::Ptr *slots) {
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (psw == slots[i]) {
			return i;
		}
	}
	assert(false);
	return -1;
}

void MenuStateNewGame::onChangeFaction(PlayerSlotWidget::Ptr psw) {
	GameSettings &gs = g_simInterface->getGameSettings();
	int ndx = getSlotIndex(psw, m_playerSlots);
	assert(ndx >= 0 && ndx < GameConstants::maxPlayers);
	if (psw->getSelectedFactionIndex() >= 0) {
		int n = psw->getSelectedFactionIndex();
		assert(n >= 0 && n < m_factionFiles.size());
		gs.setFactionTypeName(ndx, m_factionFiles[n]);
	} else {
		gs.setFactionTypeName(ndx, "");
	}
}

void MenuStateNewGame::onChangeControl(PlayerSlotWidget::Ptr ps) {
	static bool noRecurse = false;
	if (noRecurse) {
		return; // control was changed progmatically
	}
	noRecurse = true;
	// check for humanity
	assert(m_humanSlot >= 0);
	bool humanInPrevSlot = m_playerSlots[m_humanSlot]->getControlType() == ControlType::HUMAN;
	int newHumanSlot = -1;
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		if (i == m_humanSlot) continue;
		if (m_playerSlots[i]->getControlType() == ControlType::HUMAN) {
			newHumanSlot = i;
			break;
		}
	}
	if (humanInPrevSlot) {
		if (newHumanSlot != -1) { // human moved slots
			assert(m_humanSlot >= 0);
			m_playerSlots[m_humanSlot]->setSelectedControl(ControlType::CLOSED);
			m_humanSlot = newHumanSlot;
		}
	} else {
		if (newHumanSlot == -1) { // human went away
			m_playerSlots[0]->setSelectedControl(ControlType::HUMAN);
			m_humanSlot = 0;
		}
	}
	updateControlers();
	updateNetworkSlots();
	noRecurse = false;
}

void MenuStateNewGame::onChangeTeam(PlayerSlotWidget::Ptr psw) {
	GameSettings &gs = g_simInterface->getGameSettings();
	int ndx = getSlotIndex(psw, m_playerSlots);
	assert(ndx >= 0 && ndx < GameConstants::maxPlayers);
	gs.setTeam(ndx, psw->getSelectedTeamIndex());
}

int getLowestFreeColourIndex(PlayerSlotWidget::Ptr *slots) {
	bool slotUsed[GameConstants::maxPlayers];
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		slotUsed[i] = false;
	}
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (slots[i]->getSelectedColourIndex() != -1) {
			ASSERT(slots[i]->getControlType() != ControlType::CLOSED, "Closed slot has colour set.");
			slotUsed[slots[i]->getSelectedColourIndex()] = true;
		}
	}
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (!slotUsed[i]) {
			return i;
		}
	}
	ASSERT(false, "No free colours");
	return -1;
}

void MenuStateNewGame::onChangeColour(PlayerSlotWidget::Ptr psw) {
	GameSettings &gs = g_simInterface->getGameSettings();
	int ndx = getSlotIndex(psw, m_playerSlots);
	assert(ndx >= 0 && ndx < GameConstants::maxPlayers);
	int ci = psw->getSelectedColourIndex();
	gs.setColourIndex(ndx, ci);
	if (ci == -1) {
		return;
	}
	assert(ci >= 0 && ci < GameConstants::maxPlayers);
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (ndx == i) continue;
		if (m_playerSlots[i]->getSelectedColourIndex() == ci) {
			m_playerSlots[i]->setSelectedColour(getLowestFreeColourIndex(m_playerSlots));
			gs.setColourIndex(i, m_playerSlots[i]->getSelectedColourIndex());
		}
	}
}

void MenuStateNewGame::onCheckChanged(Button::Ptr cb) {
	GameSettings &gs = g_simInterface->getGameSettings();
	if (cb == m_fogOfWarCheckbox) {
		gs.setFogOfWar(m_fogOfWarCheckbox->isChecked());
	} else if (cb == m_randomLocsCheckbox) {
		gs.setRandomStartLocs(m_randomLocsCheckbox->isChecked());
	} else {
		ASSERT(false, "Unknown CheckBox!?!");
	}

}

void MenuStateNewGame::onChangeMap(ListBase::Ptr) {
	assert(m_mapList->getSelectedIndex() >= 0 && m_mapList->getSelectedIndex() < m_mapFiles.size());
	string mapBaseName = m_mapFiles[m_mapList->getSelectedIndex()];
	string mapFile = "maps/" + mapBaseName + ".gbm";

	m_mapInfo.load(mapFile);
	m_mapInfoLabel->setText(m_mapInfo.desc);
	updateControlers();
	g_config.setUiLastMap(mapBaseName);

	GameSettings &gs = g_simInterface->getGameSettings();
	gs.setDescription(formatString(mapBaseName));
	gs.setMapPath(mapFile);
}

void MenuStateNewGame::onChangeTileset(ListBase::Ptr) {
	g_config.setUiLastTileset(m_tilesetFiles[m_tilesetList->getSelectedIndex()]);
	GameSettings &gs = g_simInterface->getGameSettings();
	assert(m_tilesetList->getSelectedIndex() >= 0);
	gs.setTilesetPath(string("tilesets/") + m_tilesetFiles[m_tilesetList->getSelectedIndex()]);
}

void MenuStateNewGame::onChangeTechtree(ListBase::Ptr) {
	reloadFactions();
	g_config.setUiLastTechTree(m_techTreeFiles[m_techTreeList->getSelectedIndex()]);
}

void MenuStateNewGame::onButtonClick(Button::Ptr btn) {
	if (btn == m_returnButton) {
		m_targetTransition = Transition::RETURN;
		mainMenu->setCameraTarget(MenuStates::ROOT);
		g_soundRenderer.playFx(CoreData::getInstance().getClickSoundA());
	} else {
		m_targetTransition = Transition::PLAY;
		g_soundRenderer.playFx(CoreData::getInstance().getClickSoundC());
	}
	doFadeOut();
}

void MenuStateNewGame::onDismissDialog(BasicDialog::Ptr) {
	program.removeFloatingWidget(m_messageDialog);
	doFadeIn();
}

void MenuStateNewGame::update() {
	MenuState::update();

	bool configAnnounce = true; // TODO: put in config
	if (configAnnounce) {
		m_announcer.doAnnounce(hasUnconnectedSlots());
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
				g_simInterface->getGameSettings().compact();
				g_config.save();
				if (!hasNetworkSlots()) {
					GameSettings gs = g_simInterface->getGameSettings();
					program.getSimulationInterface()->changeRole(GameRole::LOCAL);
					g_simInterface->getGameSettings() = gs;
					program.clear();
					program.setState(new GameState(program));
				} else {
					g_simInterface->asServerInterface()->doLaunchBroadcast();
					program.clear();
					program.setState(new GameState(program));
				}
			}
		}
	}
}

// ============ PRIVATE ===========================

void MenuStateNewGame::reloadFactions() {
	GameSettings &gs = g_simInterface->getGameSettings();
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
		if (m_playerSlots[i]->getControlType() != ControlType::CLOSED) {
			m_playerSlots[i]->setSelectedFaction(i % results.size());
		}
	}
}

void MenuStateNewGame::updateControlers() {
	GameSettings &gs = g_simInterface->getGameSettings();
	assert(m_humanSlot >= 0);
	if (m_playerSlots[m_humanSlot]->getControlType() != ControlType::HUMAN) {
		m_playerSlots[0]->setSelectedControl(ControlType::HUMAN);
		m_humanSlot = 0;
	}
	for (int i = m_mapInfo.players; i < GameConstants::maxPlayers; ++i) {
		m_playerSlots[i]->setSelectedControl(ControlType::CLOSED);
	}
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		gs.setFactionControl(i, m_playerSlots[i]->getControlType());
		if (m_playerSlots[i]->getControlType() == ControlType::CLOSED) {
			gs.setFactionTypeName(i, "");
			gs.setPlayerName(i, "");
			gs.setTeam(i, -1);
			gs.setStartLocationIndex(i, -1);
			gs.setColourIndex(i, -1);
			m_playerSlots[i]->setNameText("No Player");
			m_playerSlots[i]->setSelectedFaction(-1);
			m_playerSlots[i]->setSelectedTeam(-1);
			m_playerSlots[i]->setSelectedColour(-1);
		} else {
			if (m_playerSlots[i]->getSelectedFactionIndex() == -1) {
				m_playerSlots[i]->setSelectedFaction(i % m_factionFiles.size());
			}
			if (m_playerSlots[i]->getSelectedTeamIndex() == -1) {
				m_playerSlots[i]->setSelectedTeam(i);
			}
			if (m_playerSlots[i]->getSelectedColourIndex() == -1) {
				m_playerSlots[i]->setSelectedColour(i);
			}
			assert(m_playerSlots[i]->getSelectedFactionIndex() >= 0 && m_playerSlots[i]->getSelectedFactionIndex() < GameConstants::maxPlayers);
			gs.setFactionTypeName(i, m_factionFiles[m_playerSlots[i]->getSelectedFactionIndex()]);
			switch (m_playerSlots[i]->getControlType()) {
				case ControlType::HUMAN:
					gs.setPlayerName(i, g_config.getNetPlayerName());
					m_playerSlots[i]->setNameText(g_config.getNetPlayerName());
					break;
				case ControlType::NETWORK: {
						ConnectionSlot *slot = g_simInterface->asServerInterface()->getSlot(i);
						if (slot->isConnected()) {
							gs.setPlayerName(i, slot->getName());
							m_playerSlots[i]->setNameText(slot->getName());
						} else {
							m_playerSlots[i]->setNameText("Unconnected");
						}
					}
					break;
				default:
					gs.setPlayerName(i, "AI Player");
					m_playerSlots[i]->setNameText("AI Player");
			}
			assert(m_playerSlots[i]->getSelectedTeamIndex() >= 0 && m_playerSlots[i]->getSelectedTeamIndex() < GameConstants::maxPlayers);
			gs.setTeam(i, m_playerSlots[i]->getSelectedTeamIndex());
			gs.setStartLocationIndex(i, i);
			assert(m_playerSlots[i]->getSelectedColourIndex() >= 0 && m_playerSlots[i]->getSelectedColourIndex() < GameConstants::maxPlayers);
			gs.setColourIndex(i, m_playerSlots[i]->getSelectedColourIndex());
		}
		if (m_playerSlots[i]->getControlType() == ControlType::HUMAN) {
			gs.setThisFactionIndex(i);
		}
	}
}

bool MenuStateNewGame::hasUnconnectedSlots() {
	ServerInterface* serverInterface = g_simInterface->asServerInterface();
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
	ServerInterface* serverInterface = g_simInterface->asServerInterface();
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

}}//end namespace
