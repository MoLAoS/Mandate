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
#include "menu_state_scenario.h"

#include "renderer.h"
#include "menu_state_root.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "menu_state_options.h"
#include "game.h"
#include "auto_test.h"

#include "leak_dumper.h"
#include "sim_interface.h"

namespace Glest { namespace Menu {
using namespace Net;
using Sim::SimulationInterface;

using namespace Shared::Xml;

// =====================================================
//  class MenuStateScenario
// =====================================================

MenuStateScenario::MenuStateScenario(Program &program, MainMenu *mainMenu):
		MenuState(program, mainMenu, "scenario") {
	Config &config = Config::getInstance();
	Lang &lang = Lang::getInstance();
	vector<string> results;
	int match = 0;

	labelInfo.init(350, 350);
	labelInfo.setFont(CoreData::getInstance().getMenuFontNormal());

	buttonReturn.init(350, 200, 125);
	buttonPlayNow.init(525, 200, 125);

	listBoxCategory.init(350, 500, 190);
	labelCategory.init(350, 530);

	listBoxScenario.init(350, 400, 190);
	labelScenario.init(350, 430);

	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));

	labelCategory.setText(lang.get("Category"));
	labelScenario.setText(lang.get("Scenario"));

	//categories listBox
	findAll("gae/scenarios/*.", results);
	categories = results;

	if (results.size() == 0) {
		throw runtime_error("There are no categories");
	}
	for(int i = 0; i < results.size(); ++i) {
		if (results[i] == config.getUiLastScenarioCatagory()) {
			match = i;
		}
	}

	listBoxCategory.setItems(results);
	listBoxCategory.setSelectedItemIndex(match);
	updateScenarioList(categories[listBoxCategory.getSelectedItemIndex()], true);
	// stay LOCAL
	//program.getSimulationInterface()->changeRole(GameRole::SERVER);
}


void MenuStateScenario::mouseClick(int x, int y, MouseButton mouseButton) {
	Config &config = Config::getInstance();
	CoreData &coreData = CoreData::getInstance();
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();

	if (buttonReturn.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundA());
		config.save();
		mainMenu->setState(new MenuStateRoot(program, mainMenu)); //TO CHANGE
	} else if (buttonPlayNow.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundC());
		config.save();
		launchGame();
	} else if (listBoxScenario.mouseClick(x, y)) {
		const string &category = categories[listBoxCategory.getSelectedItemIndex()];
		const string &scenario = scenarioFiles[listBoxScenario.getSelectedItemIndex()];

		Scenario::loadScenarioInfo(scenario, category, &scenarioInfo);
		labelInfo.setText(scenarioInfo.desc);
		config.setUiLastScenario(category + "/" + scenario);
	} else if (listBoxCategory.mouseClick(x, y)) {
		const string &catagory = categories[listBoxCategory.getSelectedItemIndex()];

		updateScenarioList(catagory);
		config.setUiLastScenarioCatagory(catagory);
	}
}

void MenuStateScenario::mouseMove(int x, int y, const MouseState &ms) {

	listBoxScenario.mouseMove(x, y);
	listBoxCategory.mouseMove(x, y);

	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);
}

void MenuStateScenario::render() {

	Renderer &renderer = Renderer::getInstance();

	renderer.renderLabel(&labelInfo);

	renderer.renderLabel(&labelCategory);
	renderer.renderListBox(&listBoxCategory);

	renderer.renderLabel(&labelScenario);
	renderer.renderListBox(&listBoxScenario);

	renderer.renderButton(&buttonReturn);
	renderer.renderButton(&buttonPlayNow);
}

void MenuStateScenario::update() {
	if (Config::getInstance().getMiscAutoTest()) {
		AutoTest::getInstance().updateScenario(this);
	}
}

void MenuStateScenario::launchGame() {
	Scenario::loadGameSettings(scenarioFiles[listBoxScenario.getSelectedItemIndex()],
					categories[listBoxCategory.getSelectedItemIndex()],
					&scenarioInfo);
	program.setState(new GameState(program));
}

void MenuStateScenario::setScenario(int i) {
	listBoxScenario.setSelectedItemIndex(i);
	Scenario::loadScenarioInfo(scenarioFiles[listBoxScenario.getSelectedItemIndex()],
					categories[listBoxCategory.getSelectedItemIndex()],
					&scenarioInfo);
}

void MenuStateScenario::updateScenarioList(const string &category, bool selectDefault) {
	const Config &config = Config::getInstance();
	vector<string> results;
	int match = 0;

	findAll("gae/scenarios/" + category + "/*.", results);

	//update scenarioFiles
	scenarioFiles = results;
	if (results.size() == 0) {
		throw runtime_error("There are no scenarios for category, " + category + ".");
	}
	for (int i = 0; i < results.size(); ++i) {
		string path = category + "/" + results[i];
		cout << path << " / " << config.getUiLastScenario() << endl;
		if (path == config.getUiLastScenario()) {
			match = i;
		}
		results[i] = formatString(results[i]);
	}
	listBoxScenario.setItems(results);
	if(selectDefault) {
		listBoxScenario.setSelectedItemIndex(match);
	}

	//update scenario info
	Scenario::loadScenarioInfo(scenarioFiles[listBoxScenario.getSelectedItemIndex()],
					categories[listBoxCategory.getSelectedItemIndex()],
					&scenarioInfo);
	labelInfo.setText(scenarioInfo.desc);
}

}}//end namespace
