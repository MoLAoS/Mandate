// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
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
//  class MenuStateNewGame
// =====================================================

MenuStateNewGame::MenuStateNewGame(Program &program, MainMenu *mainMenu, bool openNetworkSlots)
		: MenuState(program, mainMenu)
		, targetTransition(NewGameTransition::INVALID)
		, humanSlot(0) {
	_PROFILE_FUNCTION();
	const Metrics &metrics = Metrics::getInstance();
	const CoreData &coreData = CoreData::getInstance();
	Lang &lang = Lang::getInstance();
	Config &config = Config::getInstance();

	Font *font = coreData.getfreeTypeMenuFont();

	// initialize network interface
	// just set to SERVER now, we'll change it back to LOCAL if necessary before launch
	program.getSimulationInterface()->changeRole(GameRole::SERVER);
	GameSettings &gs = theSimInterface->getGameSettings();

	vector<string> results;

	// create
	int gap = (metrics.getScreenW() - 300) / 3;
	int x = gap, w = 150, y = 50, h = 30;
	btnReturn = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	btnReturn->setTextParams(lang.get("Return"), Vec4f(1.f), font);
	btnReturn->Clicked.connect(this, &MenuStateNewGame::onButtonClick);

	x += w + gap;
	btnPlayNow = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	btnPlayNow->setTextParams(lang.get("PlayNow"), Vec4f(1.f), font);
	btnPlayNow->Clicked.connect(this, &MenuStateNewGame::onButtonClick);

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
	mapFiles = results;
	int match = 0;
	for (int i = 0; i < results.size(); ++i) {
		if (results[i] == config.getUiLastMap()) {
			match = i;
		}
		results[i] = formatString(results[i]);
	}
	x = gap, w = 200, y = 170, h = 30;
	dlMaps = new DropList(&program, Vec2i(x, y), Vec2i(w, h));
	dlMaps->addItems(results);
	dlMaps->setDropBoxHeight(140);
	dlMaps->setSelected(match);
	dlMaps->SelectionChanged.connect(this, &MenuStateNewGame::onChangeMap);

	stMap = new StaticText(&program, Vec2i(x,  y + h + 5), Vec2i(w, h));
	stMap->setTextParams(lang.get("Map"), Vec4f(1.f), font);
	
	gs.setDescription(results[match]);
	gs.setMapPath(string("maps/") + mapFiles[match] + ".gbm");

	mapInfo.load("maps/" + mapFiles[match] + ".gbm");

	stMapInfo = new StaticText(&program, Vec2i(x, y - (h*2 + 10)), Vec2i(w, h * 2));
	//stMapInfo->setBorderParams(BorderStyle::SOLID, 2, Vec3f(1.f, 0.f, 0.f), 0.7f);
	stMapInfo->setTextParams(mapInfo.desc, Vec4f(1.f), font);

	//tileset listBox
	match = 0;
	findAll("tilesets/*.", results);
	if (results.size() == 0) {
		throw runtime_error("There are no tile sets");
	}
	tilesetFiles = results;
	for (int i = 0; i < results.size(); ++i) {
		if(results[i] == config.getUiLastTileset()) {
			match = i;
		}
		results[i] = formatString(results[i]);
	}
	x = gap * 2 + w;
	dlTileset = new DropList(&program, Vec2i(x, y), Vec2i(w, 30));
	dlTileset->addItems(results);
	dlTileset->setDropBoxHeight(140);
	dlTileset->setSelected(match);
	dlTileset->SelectionChanged.connect(this, &MenuStateNewGame::onChangeTileset);

	stTileset = new StaticText(&program, Vec2i(x, y + h + 5), Vec2i(w, h));
	stTileset->setTextParams(lang.get("Tileset"), Vec4f(1.f), font);

	//tech Tree listBox
	match = 0;
	findAll("techs/*.", results);
	if (results.size() == 0) {
		throw runtime_error("There are no tech trees");
	}
	techTreeFiles = results;
	for (int i = 0; i < results.size(); ++i) {
		if(results[i] == config.getUiLastTechTree()) {
			match = i;
		}
		results[i] = formatString(results[i]);
	}
	x = gap * 3 + w * 2;
	dlTechtree = new DropList(&program, Vec2i(x, y), Vec2i(w, h));
	dlTechtree->addItems(results);
	dlTechtree->setDropBoxHeight(140);
	dlTechtree->setSelected(match);
	dlTechtree->SelectionChanged.connect(this, &MenuStateNewGame::onChangeTechtree);

	stTechtree = new StaticText(&program, Vec2i(x,  y + h + 5), Vec2i(w, h));
	stTechtree->setTextParams(lang.get("Techtree"), Vec4f(1.f), font);

	gs.setTilesetPath(string("tilesets/") + tilesetFiles[match]);

	gap = (metrics.getScreenW() - 400) / 3, x = gap, y += 70, h = 30;
	int cbw = 75, stw = 200;
	
	cbRandomLocs = new CheckBox(&program, Vec2i(x+65,y), Vec2i(cbw,h));
	cout << "CheckBox pref size = " << cbRandomLocs->getPrefSize() << endl;
	
	stRandomLocs = new StaticText(&program, Vec2i(x, y + 35), Vec2i(stw,h));
	stRandomLocs->setTextParams(lang.get("RandomizeLocations"), Vec4f(1.f), font);

	x = gap * 2 + stw;
	cbFogOfWar = new CheckBox(&program, Vec2i(x+65,y), Vec2i(cbw,h));
	cbFogOfWar->setChecked(true);

	stFogOfWar = new StaticText(&program, Vec2i(x, y + 35), Vec2i(stw, h));
	stFogOfWar->setTextParams(lang.get("FogOfWar"), Vec4f(1.f), font);

	y += 75, h = 35, x = (metrics.getScreenW() - 700) / 2, w = 700;

	int sty = metrics.getScreenH() - 70;
	stControl = new StaticText(&program, Vec2i(x + 150, sty), Vec2i(200, 30));
	stFaction = new StaticText(&program, Vec2i(x + 390, sty), Vec2i(200, 30));
	stTeam = new StaticText(&program, Vec2i(x + 620, sty), Vec2i(80, 30));

	stControl->setTextParams("Control", Vec4f(1.f), font);
	stFaction->setTextParams("Faction", Vec4f(1.f), font);
	stTeam->setTextParams("Team", Vec4f(1.f), font);

	int numPlayer = GameConstants::maxPlayers;

	int vSpace = (sty - y);
	int vgap = (vSpace - (numPlayer * 35)) / (numPlayer + 1);

	for (int i = 0; i < numPlayer; ++i) {
		y = sty - (vgap * (i + 1)) - (h * (i + 1));
		psWidgets[i] = new PlayerSlotWidget(&program, Vec2i(x, y), Vec2i(w, h));
		psWidgets[i]->setNameText(string("Player #") + intToStr(i+1));
	}

	reloadFactions();

	// init controllers
	psWidgets[0]->setSelectedControl(ControlType::HUMAN);
	psWidgets[0]->setSelectedTeam(0);
	int i = 1;
	if (openNetworkSlots) {
		for (; i < mapInfo.players; ++i) {
			psWidgets[i]->setSelectedControl(ControlType::NETWORK);
			psWidgets[i]->setSelectedTeam(i);
		}
	} else {
		psWidgets[i]->setSelectedControl(ControlType::CPU);
		psWidgets[i]->setSelectedTeam(1);
		++i;
	}
	for ( ; i < GameConstants::maxPlayers; ++i) {
		psWidgets[i]->setSelectedControl(ControlType::CLOSED);
	}
	updateControlers();
	updateNetworkSlots();

	for (int i = 0; i < numPlayer; ++i) {
		psWidgets[i]->ControlChanged.connect(this, &MenuStateNewGame::onChangeControl);
		psWidgets[i]->FactionChanged.connect(this, &MenuStateNewGame::onChangeFaction);
		psWidgets[i]->TeamChanged.connect(this, &MenuStateNewGame::onChangeTeam);
	}
}

void MenuStateNewGame::onChangeFaction(PlayerSlotWidget::Ptr psw) {
	GameSettings &gs = theSimInterface->getGameSettings();
	int ndx = -1;
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (psw == psWidgets[i]) {
			ndx = i;
			break;
		}
	}
	assert(ndx != -1);
	if (psw->getSelectedFactionIndex() >= 0) {
		gs.setFactionTypeName(ndx, factionFiles[psw->getSelectedFactionIndex()]);
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
	bool humanInPrevSlot = psWidgets[humanSlot]->getControlType() == ControlType::HUMAN;
	int newHumanSlot = -1;
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		if (i == humanSlot) continue;
		if (psWidgets[i]->getControlType() == ControlType::HUMAN) {
			newHumanSlot = i;
			break;
		}
	}
	if (humanInPrevSlot) {
		if (newHumanSlot != -1) { // human moved slots
			psWidgets[humanSlot]->setSelectedControl(ControlType::CLOSED);
			humanSlot = newHumanSlot;
		}
	} else {
		if (newHumanSlot == -1) { // human went away
			psWidgets[0]->setSelectedControl(ControlType::HUMAN);
			humanSlot = 0;
		}
	}
	updateControlers();
	updateNetworkSlots();
	noRecurse = false;
}

void MenuStateNewGame::onChangeTeam(PlayerSlotWidget::Ptr psw) {
	GameSettings &gs = theSimInterface->getGameSettings();
	int ndx = -1;
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (psw == psWidgets[i]) {
			ndx = i;
			break;
		}
	}
	assert(ndx != -1);
	gs.setTeam(ndx, psw->getSelectedTeamIndex());
}

void MenuStateNewGame::onChangeMap(ListBase::Ptr) {
	string mapBaseName = mapFiles[dlMaps->getSelectedIndex()];
	string mapFile = "maps/" + mapBaseName + ".gbm";

	mapInfo.load(mapFile);
	stMapInfo->setText(mapInfo.desc);
	updateControlers();
	theConfig.setUiLastMap(mapBaseName);

	GameSettings &gs = theSimInterface->getGameSettings();
	gs.setDescription(formatString(mapBaseName));
	gs.setMapPath(mapFile);
}

void MenuStateNewGame::onChangeTileset(ListBase::Ptr) {
	theConfig.setUiLastTileset(tilesetFiles[dlTileset->getSelectedIndex()]);
	GameSettings &gs = theSimInterface->getGameSettings();
	gs.setTilesetPath(string("tilesets/") + tilesetFiles[dlTileset->getSelectedIndex()]);
}

void MenuStateNewGame::onChangeTechtree(ListBase::Ptr) {
	reloadFactions();
	theConfig.setUiLastTechTree(techTreeFiles[dlTechtree->getSelectedIndex()]);
}

void MenuStateNewGame::onButtonClick(Button::Ptr btn) {
	if (btn == btnReturn) {
		targetTransition = NewGameTransition::RETURN;
		mainMenu->setCameraTarget(MenuStates::ROOT);
		theSoundRenderer.playFx(CoreData::getInstance().getClickSoundA());
	} else {
		targetTransition = NewGameTransition::PLAY;
		//GameSettings &gs = theSimInterface->getGameSettings();
		//Shared::Xml::XmlTree xmlTree("game-settings");
		//gs.save(xmlTree.getRootNode());
		//xmlTree.save("game-settings.xml");
		theSoundRenderer.playFx(CoreData::getInstance().getClickSoundC());
	}
	fadeIn = false;
	fadeOut = true;
}

void MenuStateNewGame::update() {
	MenuState::update();
	if (transition) {
		if (targetTransition == NewGameTransition::RETURN) {
			program.clear();
			mainMenu->setState(new MenuStateRoot(program, mainMenu));
		} else if (targetTransition == NewGameTransition::PLAY) {
			if (hasUnconnectedSlots()) {
				msgBox = new GraphicMessageBox();
				msgBox->init(theLang.get("WaitingForConnections"), theLang.get("Ok"));
				transition = false;
				///@todo MessageBox Widget, onDissmiss reset fadeIn
			} else {
				theSimInterface->getGameSettings().compact();
				theConfig.save();
				if (!hasNetworkSlots()) {
					GameSettings gs = theSimInterface->getGameSettings();
					program.getSimulationInterface()->changeRole(GameRole::LOCAL);
					theSimInterface->getGameSettings() = gs;
					program.clear();
					program.setState(new GameState(program));
				} else {
					theSimInterface->asServerInterface()->doLaunchBroadcast();
					program.clear();
					program.setState(new GameState(program));
				}
			}
		}
	}
}

// ============ PRIVATE ===========================

void MenuStateNewGame::reloadFactions() {
	GameSettings &gs = theSimInterface->getGameSettings();
	vector<string> results;
	findAll("techs/" + techTreeFiles[dlTechtree->getSelectedIndex()] + "/factions/*.", results);
	if (results.empty()) {
		throw runtime_error("There are no factions for this tech tree");
	}
	gs.setTechPath(string("techs/") + techTreeFiles[dlTechtree->getSelectedIndex()]);

	factionFiles.clear();
	factionFiles = results;
	for (int i = 0; i < results.size(); ++i) {
		results[i] = formatString(results[i]);
	}
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		psWidgets[i]->setFactionItems(results);
		if (psWidgets[i]->getControlType() != ControlType::CLOSED) {
			psWidgets[i]->setSelectedFaction(i % results.size());
		}
	}
}

void MenuStateNewGame::updateControlers() {
	GameSettings &gs = theSimInterface->getGameSettings();
	if (psWidgets[humanSlot]->getControlType() != ControlType::HUMAN) {
		psWidgets[0]->setSelectedControl(ControlType::HUMAN);
		humanSlot = 0;
	}
	for (int i = mapInfo.players; i < GameConstants::maxPlayers; ++i) {
		psWidgets[i]->setSelectedControl(ControlType::CLOSED);
	}
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		gs.setFactionControl(i, psWidgets[i]->getControlType());
		if (psWidgets[i]->getControlType() == ControlType::CLOSED) {
			gs.setFactionTypeName(i, "");
			gs.setTeam(i, -1);
			gs.setStartLocationIndex(i, -1);
			psWidgets[i]->setSelectedFaction(-1);
			psWidgets[i]->setSelectedTeam(-1);
		} else {
			if (psWidgets[i]->getSelectedFactionIndex() == -1) {
				psWidgets[i]->setSelectedFaction(i % factionFiles.size());
			}
			gs.setFactionTypeName(i, factionFiles[psWidgets[i]->getSelectedFactionIndex()]);
			if (psWidgets[i]->getSelectedTeamIndex() == -1) {
				psWidgets[i]->setSelectedTeam(i);
			}
			gs.setTeam(i, psWidgets[i]->getSelectedTeamIndex());
			gs.setStartLocationIndex(i, i);
		}
		if (psWidgets[i]->getControlType() == ControlType::HUMAN) {
			gs.setThisFactionIndex(i);
		}
	}
}

bool MenuStateNewGame::hasUnconnectedSlots() {
	ServerInterface* serverInterface = theSimInterface->asServerInterface();
	for (int i = 0; i < mapInfo.players; ++i) {
		if (psWidgets[i]->getControlType() == ControlType::NETWORK) {
			if (!serverInterface->getSlot(i)->isConnected()) {
				return true;
			}
		}
	}
	return false;
}

bool MenuStateNewGame::hasNetworkSlots() {
	for (int i = 0; i < mapInfo.players; ++i) {
		if (psWidgets[i]->getControlType() == ControlType::NETWORK) {
			return true;
		}
	}
	return false;
}

void MenuStateNewGame::updateNetworkSlots() {
	ServerInterface* serverInterface = theSimInterface->asServerInterface();
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		if (serverInterface->getSlot(i) == NULL
		&& psWidgets[i]->getControlType() == ControlType::NETWORK) {
			serverInterface->addSlot(i);
		}
		if (serverInterface->getSlot(i) != NULL
		&& psWidgets[i]->getControlType() != ControlType::NETWORK) {
			serverInterface->removeSlot(i);
		}
	}
}

}}//end namespace
