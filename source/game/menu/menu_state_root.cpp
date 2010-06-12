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
#include "menu_state_root.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "game_util.h"
#include "config.h"
#include "menu_state_new_game.h"
#include "menu_state_join_game.h"
#include "menu_state_scenario.h"
#include "menu_state_load_game.h"
#include "menu_state_options.h"
#include "menu_state_about.h"
#include "metrics.h"
#include "network_message.h"
#include "socket.h"
#include "auto_test.h"

#include "widgets.h"

#include "leak_dumper.h"
using namespace Glest::Net;

namespace Glest { namespace Menu {

// =====================================================
// 	class MenuStateRoot
// =====================================================

MenuStateRoot::MenuStateRoot(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu)
		, selectedItem(RootMenuItem::INVALID)
		, fade(0.f)
		, fadeIn(true)
		, fadeOut(false)
		, transition(false) {
	_PROFILE_FUNCTION();
	Lang &lang= Lang::getInstance();
	const Metrics &metrics = Metrics::getInstance();
	const CoreData &coreData = CoreData::getInstance();

	// Buttons Panel
	Vec2i pos(metrics.getScreenW() / 2 - 125, 100);
	Widgets::Panel *pnl = new Widgets::Panel(&program, pos, Vec2i(250, 350));
	pnl->setPaddingParams(10, 15);
	pnl->setBorderStyle(Widgets::BorderStyle::RAISE);

	// Buttons
	Font *font = coreData.getfreeTypeMenuFont();//coreData.getMenuFontNormal();
	foreach_enum (RootMenuItem, i) {
		buttons[i] = new Widgets::Button(pnl, Vec2i(0,0), Vec2i(200,30));
		buttons[i]->setTextParams(lang.get(RootMenuItemNames[i]), Vec4f(1.f), font, true);
		buttons[i]->Clicked.connect(this, &MenuStateRoot::onButtonClick);
	}

	// Glest Logo PicturePanel
	pos = Vec2i(metrics.getScreenW() / 2 - 256, 450);
	Widgets::PicturePanel *pp = new Widgets::PicturePanel(&program, pos, Vec2i(512, 256));
	pp->setBorderSize(0);
	pp->setPadding(0);
	pp->setImage(coreData.getLogoTexture());
	pp->setAutoLayout(false);
	
	// Advanced Engine labels
	font = coreData.getAdvancedEngineFont();
	Widgets::StaticText *label = new Widgets::StaticText(pp);
	label->setTextParams(lang.get("Advanced"), Vec4f(1.f), font);
	Vec2i sz = label->getTextDimensions() + Vec2i(10,5);
	label->setPos(Vec2i(255 - sz.x, 60));
	label->setSize(sz);
	label->centreText();

	label = new Widgets::StaticText(pp);
	label->setTextParams(lang.get("Engine"), Vec4f(1.f), font);
	label->setPos(Vec2i(285, 60));
	label->setSize(label->getTextDimensions() + Vec2i(10,5));
	label->centreText();

	pos = Vec2i(285 + label->getSize().x, 62);
	// Version label
	font = coreData.getFreeTypeFont();
	label = new Widgets::StaticText(pp);
	label->setTextParams(gaeVersionString, Vec4f(1.f), font);
	
	sz = label->getTextDimensions() + Vec2i(10,5);
	label->setPos(pos/*Vec2i(256 - sz.x / 2, 10)*/);
	label->setSize(sz);
	label->centreText();
	
	// gpl logo
	pos = Vec2i(metrics.getScreenW() / 2 - 64, 25);
	new Widgets::StaticImage(&program, pos, Vec2i(128, 64), coreData.getGplTexture());

	if (program.getCmdArgs().isTest("widgets")) {
		// testing TextBox
		font = coreData.getfreeTypeMenuFont();
		int h = int(font->getMetrics()->getHeight() + 1.f);
		Widgets::TextBox *txtBox = new Widgets::TextBox(&program, Vec2i(10,10), Vec2i(200, h));
		txtBox->setTextParams("", Vec4f(1.f), font, false);
		txtBox->setTextPos(Vec2i(5,0));
		txtBox->TextChanged.connect(this, &MenuStateRoot::onTextChanged);

		// testing ListBox
		int yPos = 10 + txtBox->getHeight() + 20;
		Widgets::ListBox *listBox = new Widgets::ListBox(&program, Vec2i(10, yPos), Vec2i(200, 250));
		
		vector<string> items;
		items.push_back("Carrot");
		items.push_back("Broccoli");
		items.push_back("Capsicum");
		items.push_back("Cauliflower");
		items.push_back("Asparagus");
		items.push_back("Cucumber");
		items.push_back("Eggplant");
		items.push_back("Avacado");
		
		listBox->addItems(items);
		listBox->SelectionChanged.connect(this, &MenuStateRoot::onListBoxChanged);
		
		Vec2i lbSize(listBox->getWidth(), listBox->getPrefHeight());

		items.clear();
		items.push_back("Pear");
		items.push_back("Apple");
		items.push_back("Orange");
		items.push_back("Banana");

		listBox->addItems(items);
		listBox->setSize(lbSize);

		// testing DropList
		yPos = listBox->getScreenPos().y + listBox->getHeight() + 20;
		Widgets::DropList *cmbBox = new Widgets::DropList(&program, Vec2i(10, yPos), Vec2i(200, h + 6));
		cmbBox->addItems(items);
		cmbBox->setSelected(0);

		cmbBox->ListExpanded.connect(this, &MenuStateRoot::onComboBoxExpanded);
		cmbBox->ListCollapsed.connect(this, &MenuStateRoot::onComboBoxCollapsed);

		// CheckBox
		yPos += cmbBox->getHeight() + 20;
		Widgets::CheckBox *checkBox = new Widgets::CheckBox(&program);
		checkBox->setPos(Vec2i(10, yPos));
		checkBox->setSize(checkBox->getPrefSize());

		pos = Vec2i(250, 128);
		sz = Vec2i(32,256);
		Widgets::VerticalScrollBar *vsb = new Widgets::VerticalScrollBar(&program, pos, sz);
		vsb->setRanges(100, 35);

	} // test_widgets

	// set fade == 0
	program.setFade(fade);

	// end network interface
	program.getSimulationInterface()->changeRole(GameRole::LOCAL);
}

void MenuStateRoot::onTextChanged(Widgets::TextBox *txtBox) {
	cout << "TextChanged: " << txtBox->getText() << endl;
}

void MenuStateRoot::onListBoxChanged(Widgets::ListBase *lst) {
	cout << "SelectionChanged: " << lst->getSelectedItem()->getText() << endl;
}

void MenuStateRoot::onComboBoxExpanded(Widgets::DropList::Ptr cb) {
//	program.getLayer("root")->setFade(0.5f);
}

void MenuStateRoot::onComboBoxCollapsed(Widgets::DropList::Ptr cb) {
//	program.getLayer("root")->setFade(1.f);
}

void MenuStateRoot::onButtonClick(Widgets::Button *btn) {
	// which button ?
	foreach_enum (RootMenuItem, i) {
		buttons[i]->setEnabled(false);
		if (btn == buttons[i]) {
			selectedItem = i;
		}
	}
	MenuStates targetState = MenuStates::INVALID;
	switch (selectedItem) {
		case RootMenuItem::NEWGAME:	 targetState = MenuStates::NEW_GAME;	break;
		case RootMenuItem::JOINGAME: targetState = MenuStates::JOIN_GAME;	break;
		case RootMenuItem::SCENARIO: targetState = MenuStates::SCENARIO;	break;
		case RootMenuItem::OPTIONS:  targetState = MenuStates::OPTIONS;		break;
		case RootMenuItem::LOADGAME: targetState = MenuStates::LOAD_GAME;	break;
		case RootMenuItem::ABOUT:	 targetState = MenuStates::ABOUT;		break;
		default: break;
	}
	if (targetState != MenuStates::INVALID) {
		mainMenu->setCameraTarget(targetState);
	}
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();
	CoreData &coreData = CoreData::getInstance();
	StaticSound *clickSound;
	if (selectedItem == RootMenuItem::EXIT) {
		clickSound = coreData.getClickSoundA();
	} else {
		clickSound = coreData.getClickSoundB();
	}
	soundRenderer.playFx(clickSound);
	fadeOut = true;
}

void MenuStateRoot::update(){
	if (Config::getInstance().getMiscAutoTest()) {
		AutoTest::getInstance().updateRoot(program, mainMenu);
	}
	if (fadeIn) {
		fade += 0.05;
		if (fade > 1.f) {
			fade = 1.f;
			fadeIn = false;
		}
		program.setFade(fade);
	} else if (fadeOut) {
		fade -= 0.05;
		if (fade < 0.f) {
			fade = 0.f;
			transition = true;
			fadeOut = false;
		}
		program.setFade(fade);
	}
	if (transition) {
		program.clear();
		MenuState *newState = 0;
		switch (selectedItem) {
			case RootMenuItem::NEWGAME: newState = new MenuStateNewGame(program, mainMenu); break;
			case RootMenuItem::JOINGAME: newState = new MenuStateJoinGame(program, mainMenu); break;
			case RootMenuItem::SCENARIO: newState = new MenuStateScenario(program, mainMenu); break;
			case RootMenuItem::OPTIONS: newState = new MenuStateOptions(program, mainMenu); break;
			case RootMenuItem::LOADGAME: newState = new MenuStateLoadGame(program, mainMenu); break;
			case RootMenuItem::ABOUT: newState = new MenuStateAbout(program, mainMenu); break;
			default: break;
		}
		if (newState) {
			mainMenu->setState(newState);
		} else {
			program.exit();
		}
	}
}

}}//end namespace
