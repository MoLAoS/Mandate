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
	Font *font = g_coreData.getfreeTypeMenuFont();

	// create
	int gap = (g_metrics.getScreenW() - 300) / 3;
	int x = gap, w = 150, y = 150, h = 30;
	m_returnButton = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	m_returnButton->setTextParams(g_lang.get("Return"), Vec4f(1.f), font);
	m_returnButton->Clicked.connect(this, &MenuStateScenario::onButtonClick);

	x += w + gap;
	m_playNowButton = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	m_playNowButton->setTextParams(g_lang.get("PlayNow"), Vec4f(1.f), font);
	m_playNowButton->Clicked.connect(this, &MenuStateScenario::onButtonClick);

	w = 200;
	h = int(font->getMetrics()->getHeight()) + 6;
	x = g_metrics.getScreenW() / 2 - (w * 2 + 50) / 2;
	y = g_metrics.getScreenH() / 2 + (h * 2) / 2 + 100;
	StaticText::Ptr l_text = new StaticText(&program, Vec2i(x, y), Vec2i(w, h));
	l_text->setTextParams(g_lang.get("Category"), Vec4f(1.f), font);
//	l_text->setBorderParams(BorderStyle::SOLID, 2, Vec3f(1.f, 0.f, 0.f), 0.5f);

	y = g_metrics.getScreenH() / 2 - (h * 2) / 2 + 100;
	l_text = new StaticText(&program, Vec2i(x, y), Vec2i(w,h));
	l_text->setTextParams(g_lang.get("Scenario"), Vec4f(1.f), font);
//	l_text->setBorderParams(BorderStyle::SOLID, 2, Vec3f(1.f, 0.f, 0.f), 0.5f);

	m_infoLabel = new StaticText(&program, Vec2i(x, y - 220), Vec2i(450, 200));
	m_infoLabel->setTextParams("", Vec4f(1.f), font);
//	m_infoLabel->setBorderParams(BorderStyle::SOLID, 2, Vec3f(1.f, 0.f, 0.f), 0.5f);

	x += 220;
	w = 230;
	y = g_metrics.getScreenH() / 2 + (h * 2) / 2 + 100;
	m_categoryList = new DropList(&program, Vec2i(x, y), Vec2i(w,h));
	y = g_metrics.getScreenH() / 2 - (h * 2) / 2 + 100;
	m_scenarioList = new DropList(&program, Vec2i(x, y), Vec2i(w,h));
	m_scenarioList->SelectionChanged.connect(this, &MenuStateScenario::onScenarioChanged);

	vector<string> results;
	int match = -1;	

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
		program.clear();
		m_messageDialog = new MessageDialog(&program);
		Vec2i sz(330, 256);
		program.setFloatingWidget(m_messageDialog, true);
		m_messageDialog->setPos(g_metrics.getScreenDims() / 2 - sz / 2);
		m_messageDialog->setSize(sz);
		m_messageDialog->setTitleText(g_lang.get("Error"));
		m_messageDialog->setMessageText(g_lang.get("NoCategoryDirectories"));
		m_messageDialog->setButtonText(g_lang.get("Yes"));
		m_messageDialog->Button1Clicked.connect(this, &MenuStateScenario::onConfirmReturn);
		return;
	}
	for(int i = 0; i < results.size(); ++i) {
		if (results[i] == g_config.getUiLastScenarioCatagory()) {
			match = i;
		}
	}
	categories = results;
	m_categoryList->addItems(categories);
	m_categoryList->SelectionChanged.connect(this, &MenuStateScenario::onCategoryChanged);
	if (match != -1) {
		m_categoryList->setSelected(match);
	} else {
		m_categoryList->setSelected(0);
	}
}

void MenuStateScenario::onConfirmReturn(MessageDialog::Ptr) {
	m_targetTansition = Transition::RETURN;
	g_soundRenderer.playFx(g_coreData.getClickSoundA());
	mainMenu->setCameraTarget(MenuStates::ROOT);
	doFadeOut();
}

void MenuStateScenario::onButtonClick(Button::Ptr btn) {
	if (btn == m_returnButton) {
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
	if (g_config.getMiscAutoTest()) {
		AutoTest::getInstance().updateScenario(this);
	}
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

void MenuStateScenario::onCategoryChanged(ListBase::Ptr) {
	updateScenarioList(categories[m_categoryList->getSelectedIndex()]);
	updateConfig();
}

void MenuStateScenario::onScenarioChanged(ListBase::Ptr) {
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
}

void MenuStateScenario::updateScenarioList(const string &category, bool selectDefault) {
	vector<string> results;
	int match = -1;

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
	if (selectDefault && match != -1) {
		m_scenarioList->setSelected(match);
	} else {
		m_scenarioList->setSelected(0);
	}
}

}}//end namespace
