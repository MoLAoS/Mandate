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

MenuStateScenario::MenuStateScenario(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu, "scenario")
		, msgBox(0), failAction(FailAction::INVALID) {
	Config &config = Config::getInstance();
	vector<string> results;
	int match = -1;
	
	labelInfo.init(350, 350);
	labelInfo.setFont(CoreData::getInstance().getMenuFontNormal());

	buttonReturn.init(350, 200, 125);
	buttonPlayNow.init(525, 200, 125);

	listBoxCategory.init(350, 500, 190);
	labelCategory.init(350, 530);

	listBoxScenario.init(350, 400, 190);
	labelScenario.init(350, 430);

	buttonReturn.setText(theLang.get("Return"));
	buttonPlayNow.setText(theLang.get("PlayNow"));

	labelCategory.setText(theLang.get("Category"));
	labelScenario.setText(theLang.get("Scenario"));

	//categories listBox
	findAll("gae/scenarios/*.", results);
	
	// remove empty directories...
	for (vector<string>::iterator cat = results.begin(); cat != results.end(); ) {
		vector<string> scenarios;
		findAll("gae/scenarios/" + *cat + "/*.", scenarios);
		if (scenarios.empty()) {
			cat = results.erase(cat);
		} else {
			++cat;
		}
	}
	// fail gracefully
	if (results.empty()) {
		msgBox = new GraphicMessageBox();
		msgBox->init(theLang.get("NoCategoryDirectories"), theLang.get("Ok"));
		failAction = FailAction::MAIN_MENU;
		return;
	}
	for(int i = 0; i < results.size(); ++i) {
		if (results[i] == config.getUiLastScenarioCatagory()) {
			match = i;
		}
	}

	categories = results;
	listBoxCategory.setItems(results);
	if (match != -1) {
		listBoxCategory.setSelectedItemIndex(match);
	}
	updateScenarioList(categories[listBoxCategory.getSelectedItemIndex()], true);
	// stay LOCAL
	//program.getSimulationInterface()->changeRole(GameRole::SERVER);
}


void MenuStateScenario::mouseClick(int x, int y, MouseButton mouseButton) {
	Config &config = Config::getInstance();
	CoreData &coreData = CoreData::getInstance();
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();

	if (msgBox) {
		if (msgBox->mouseClick(x,y)) {
			delete msgBox;
			msgBox = 0;
			switch (failAction) {
				case FailAction::MAIN_MENU:
					soundRenderer.playFx(coreData.getClickSoundA());
					mainMenu->setState(new MenuStateRoot(program, mainMenu));
					return;
				case FailAction::SCENARIO_MENU:
					soundRenderer.playFx(coreData.getClickSoundA());
					break;
			}
		}
		return;
	}
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
	if (!msgBox) {
		listBoxScenario.mouseMove(x, y);
		listBoxCategory.mouseMove(x, y);

		buttonReturn.mouseMove(x, y);
		buttonPlayNow.mouseMove(x, y);
	} else {
		msgBox->mouseMove(x, y);
	}
}

void MenuStateScenario::render() {
	Renderer &renderer = Renderer::getInstance();

	if(msgBox) {
		renderer.renderMessageBox(msgBox);
		return;
	}
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
	int match = -1;

	findAll("gae/scenarios/" + category + "/*.", results);

	//update scenarioFiles
	scenarioFiles = results;
	if (results.empty()) {
		// this shouldn't happen, empty directories have been weeded out earlier
		throw runtime_error("No scenario directories found for category: " + category);
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
	if (selectDefault && match != -1) {
		listBoxScenario.setSelectedItemIndex(match);
	}

	//update scenario info
	Scenario::loadScenarioInfo(scenarioFiles[listBoxScenario.getSelectedItemIndex()],
					categories[listBoxCategory.getSelectedItemIndex()],
					&scenarioInfo);
	labelInfo.setText(scenarioInfo.desc);
}

}}//end namespace
