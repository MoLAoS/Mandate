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
		: MenuState(program, mainMenu)
		, m_targetTansition(Transition::INVALID) {
	WidgetConfig &cfg = g_widgetConfig;

	int itemHeight = cfg.getDefaultItemHeight();

	CellStrip *rootStrip = new CellStrip((Container*)&program, Orientation::VERTICAL, 2);
	rootStrip->setSizeHint(0, SizeHint());
	rootStrip->setSizeHint(1, SizeHint(-1, itemHeight * 3));
	Vec2i pad(15, 25);
	rootStrip->setPos(pad);
	rootStrip->setSize(Vec2i(g_config.getDisplayWidth() - pad.w * 2, g_config.getDisplayHeight() - pad.h * 2));

	Anchors centreAnchors;
	centreAnchors.setCentre(true, true);
	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));

	// create scenario panel
	CellStrip *sPanel = new CellStrip(rootStrip, Orientation::VERTICAL, 3);
	sPanel->setSize(Vec2i(itemHeight * 24, itemHeight * 10));
	sPanel->setCell(0);
	sPanel->setAnchors(centreAnchors);
	sPanel->setSizeHint(0, SizeHint(-1, itemHeight * 3 / 2));
	sPanel->setSizeHint(1, SizeHint(-1, itemHeight * 3 / 2));
	sPanel->setSizeHint(2, SizeHint());

	OptionWidget *ow = new OptionWidget(sPanel, g_lang.get("Category"));
	ow->setCell(0);
	m_categoryList = new DropList(ow);
	m_categoryList->setCell(1);
	m_categoryList->setAnchors(fillAnchors);

	ow = new OptionWidget(sPanel, g_lang.get("Scenario"));
	ow->setCell(1);
	m_scenarioList = new DropList(ow);
	m_scenarioList->setCell(1);
	m_scenarioList->setAnchors(fillAnchors);
	m_scenarioList->SelectionChanged.connect(this, &MenuStateScenario::onScenarioChanged);

	m_infoLabel = new StaticText(sPanel);
	m_infoLabel->setCell(2);
	m_infoLabel->setAnchors(fillAnchors);
	m_infoLabel->setText("");

	// buttons panel
	CellStrip *btnPanel = new CellStrip(rootStrip, Orientation::HORIZONTAL, 3);
	btnPanel->setCell(1);
	btnPanel->setAnchors(fillAnchors);

	Vec2i sz(itemHeight * 7, itemHeight);

	// create buttons
	m_returnButton = new Button(btnPanel, Vec2i(0), sz);
	m_returnButton->setCell(0);
	m_returnButton->setAnchors(centreAnchors);
	m_returnButton->setText(g_lang.get("Return"));
	m_returnButton->Clicked.connect(this, &MenuStateScenario::onButtonClick);

	m_playNowButton = new Button(btnPanel, Vec2i(0), sz);
	m_playNowButton->setCell(2);
	m_playNowButton->setAnchors(centreAnchors);
	m_playNowButton->setText(g_lang.get("PlayNow"));
	m_playNowButton->Clicked.connect(this, &MenuStateScenario::onButtonClick);

	// scan directories...
	vector<string> results;
	int match = 0;
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
		rootStrip->clear();
		Vec2i sz = g_widgetConfig.getDefaultDialogSize();
		m_messageDialog = MessageDialog::showDialog(g_metrics.getScreenDims() / 2 - sz / 2,
			sz, g_lang.get("Error"), g_lang.get("NoCategoryDirectories"), g_lang.get("Yes"), "");
		m_messageDialog->Button1Clicked.connect(this, &MenuStateScenario::onConfirmReturn);
		return;
	}
	for(int i = 0; i < results.size(); ++i) {
		if (results[i] == g_config.getUiLastScenarioCatagory()) {
			match = i;
		}
	}
	categories = results;
	for (int i=0; i < results.size(); ++i) {
		results[i] = formatString(results[i]);
	}
	m_categoryList->addItems(results);
	m_categoryList->SelectionChanged.connect(this, &MenuStateScenario::onCategoryChanged);
	m_categoryList->setSelected(match);
	program.setFade(0.f);
}

void MenuStateScenario::onConfirmReturn(Widget*) {
	m_targetTansition = Transition::RETURN;
	g_soundRenderer.playFx(g_coreData.getClickSoundA());
	mainMenu->setCameraTarget(MenuStates::ROOT);
	doFadeOut();
}

void MenuStateScenario::onButtonClick(Widget* source) {
	if (source == m_returnButton) {
		m_targetTansition = Transition::RETURN;
		g_soundRenderer.playFx(g_coreData.getClickSoundA());
		mainMenu->setCameraTarget(MenuStates::ROOT);
	} else {
		m_targetTansition = Transition::PLAY;
		g_soundRenderer.playFx(g_coreData.getClickSoundC());
	}
	doFadeOut();
}

void MenuStateScenario::update() {
	MenuState::update();
	if (m_transition) {
		switch (m_targetTansition) {
			case Transition::RETURN:
				program.clear();
				mainMenu->setState(new MenuStateRoot(program, mainMenu));
				break;
			case Transition::PLAY:
				launchGame();
				break;
		}
	}
}

void MenuStateScenario::launchGame() {
	Scenario::loadGameSettings(scenarioFiles[m_scenarioList->getSelectedIndex()],
					categories[m_categoryList->getSelectedIndex()], &scenarioInfo);
	program.clear();
	program.setState(new GameState(program));
}

void MenuStateScenario::setScenario(int i) {
	m_scenarioList->setSelected(i);
}

void MenuStateScenario::onCategoryChanged(Widget*) {
	updateScenarioList(categories[m_categoryList->getSelectedIndex()]);
	updateConfig();
}

void MenuStateScenario::onScenarioChanged(Widget*) {
	//update scenario info
	Scenario::loadScenarioInfo(scenarioFiles[m_scenarioList->getSelectedIndex()],
					categories[m_categoryList->getSelectedIndex()], &scenarioInfo);
	m_infoLabel->setText(scenarioInfo.desc);
	updateConfig();
}

void MenuStateScenario::updateConfig() {
	const string &category = categories[m_categoryList->getSelectedIndex()];
	const string &scenario = scenarioFiles[m_scenarioList->getSelectedIndex()];
	g_config.setUiLastScenario(category + "/" + scenario);
	g_config.setUiLastScenarioCatagory(category);
	g_config.save();
}

void MenuStateScenario::updateScenarioList(const string &category, bool selectDefault) {
	vector<string> results;
	int match = 0;

	findAll("gae/scenarios/" + category + "/*.", results);

	// update scenarioFiles
	scenarioFiles = results;
	if (results.empty()) {
		// this shouldn't happen, empty directories have been weeded out earlier
		throw runtime_error("No scenario directories found for category: " + category);
	}
	for (int i = 0; i < results.size(); ++i) {
		string path = category + "/" + results[i];
		cout << path << " / " << g_config.getUiLastScenario() << endl;
		if (path == g_config.getUiLastScenario()) {
			match = i;
		}
		results[i] = formatString(results[i]);
	}
	m_scenarioList->clearItems();
	m_scenarioList->addItems(results);
	m_scenarioList->setSelected(match);
}

}}//end namespace
