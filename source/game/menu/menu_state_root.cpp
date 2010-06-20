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
		, m_selectedItem(RootMenuItem::INVALID) {
	_PROFILE_FUNCTION();
	int sh = g_metrics.getScreenH();
	int sixtyPercent = int(0.6f * sh);

	int logoHeight = sh - sixtyPercent;
	int logoYPos = sixtyPercent;
	
	int fiftyPercent = int(0.5f * sh);
	int tenPercent = int(0.1f * sh);

	int btnPnlHeight = fiftyPercent;
	int btnPnlYPos = tenPercent + sh / 25;

	int gplHeight = tenPercent;
	int gplYPos = sh / 50;

	int widgetPad = btnPnlHeight / 25;

	// Buttons Panel
	Vec2i pos(g_metrics.getScreenW() / 2 - 125, btnPnlYPos);
	Widgets::Panel *pnl = new Widgets::Panel(&program, pos, Vec2i(250, btnPnlHeight));
	pnl->setPaddingParams(10, widgetPad);
//	pnl->setBorderStyle(Widgets::BorderStyle::RAISE);

	// Buttons
	Font *font = g_coreData.getfreeTypeMenuFont();//g_coreData.getMenuFontNormal();
	foreach_enum (RootMenuItem, i) {
		Vec2f dims = font->getMetrics()->getTextDiminsions(RootMenuItemNames[i]);
		m_buttons[i] = new Widgets::Button(pnl, Vec2i(0,0), Vec2i(200, int(dims.y + 3.f)));				
		m_buttons[i]->setTextParams(g_lang.get(RootMenuItemNames[i]), Vec4f(1.f), font, true);
		m_buttons[i]->Clicked.connect(this, &MenuStateRoot::onButtonClick);
	}

	// Glest Logo PicturePanel
	int logoWidth = logoHeight * 2;
	pos = Vec2i(g_metrics.getScreenW() / 2 - logoHeight, logoYPos);
	Widgets::PicturePanel *pp = new Widgets::PicturePanel(&program, pos, Vec2i(logoWidth, logoHeight));
//	pp->setBorderSize(0);
	pp->setPadding(0);
	pp->setImage(g_coreData.getLogoTexture());
	pp->setAutoLayout(false);
	
	// Advanced Engine labels
	font = g_coreData.getAdvancedEngineFont();
	Widgets::StaticText *label = new Widgets::StaticText(pp);
	label->setTextParams(g_lang.get("Advanced"), Vec4f(1.f), font);
	Vec2i sz = label->getTextDimensions() + Vec2i(10,5);
	int tx = int(255.f / 512.f * logoWidth);
	int ty = int(60.f / 256.f * logoHeight);
	label->setPos(Vec2i(tx - sz.x, ty));
	label->setSize(sz);
	label->centreText();

	label = new Widgets::StaticText(pp);
	label->setTextParams(g_lang.get("Engine"), Vec4f(1.f), font);
	tx = int(285.f / 512.f * logoWidth);
	label->setPos(Vec2i(tx, ty));
	label->setSize(label->getTextDimensions() + Vec2i(10,5));
	label->centreText();

	pos = Vec2i(tx + label->getSize().x, ty + 3);
	// Version label
	font = g_coreData.getFreeTypeFont();
	label = new Widgets::StaticText(pp);
	label->setTextParams(gaeVersionString, Vec4f(1.f), font);
	
	sz = label->getTextDimensions() + Vec2i(10,5);
	label->setPos(pos/*Vec2i(256 - sz.x / 2, 10)*/);
	label->setSize(sz);
	label->centreText();
	
	// gpl logo
	int gplWidth = gplHeight * 2;
	pos = Vec2i(g_metrics.getScreenW() / 2 - gplHeight, gplYPos);
	new Widgets::StaticImage(&program, pos, Vec2i(gplWidth, gplHeight), g_coreData.getGplTexture());

	if (program.getCmdArgs().isTest("widgets")) {
		// testing TextBox
		font = g_coreData.getfreeTypeMenuFont();
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

		yPos += checkBox->getHeight() + 15;
		sz = Vec2i(330, 256);

		MessageDialog::Ptr msg = new MessageDialog(&program, Vec2i(10, yPos), sz);
		msg->setTitleText("Test MessageDialog");
		msg->setButtonText(g_lang.get("Ok"), g_lang.get("Cancel"));

		Vec2i sliderPos(250, 10);
		Vec2i sliderSize(400, 32);
		string title = "Test Slider";
		Widgets::Slider::Ptr slider = new Widgets::Slider(&program, sliderPos, sliderSize, title);


	} // test_widgets

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
		m_buttons[i]->setEnabled(false);
		if (btn == m_buttons[i]) {
			m_selectedItem = i;
		}
	}
	MenuStates targetState = MenuStates::INVALID;
	switch (m_selectedItem) {
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
	StaticSound *l_clickSound;
	if (m_selectedItem == RootMenuItem::EXIT) {
		l_clickSound = g_coreData.getClickSoundA();
	} else {
		l_clickSound = g_coreData.getClickSoundB();
	}
	g_soundRenderer.playFx(l_clickSound);
	doFadeOut();
}

void MenuStateRoot::update(){
	if (Config::getInstance().getMiscAutoTest()) {
		AutoTest::getInstance().updateRoot(program, mainMenu);
	}
	MenuState::update();
	if (m_transition) {
		program.clear();
		MenuState *newState = 0;
		switch (m_selectedItem) {
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
