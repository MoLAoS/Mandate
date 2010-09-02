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

	// Buttons
	Font *font = g_coreData.getFTMenuFontNormal();
	foreach_enum (RootMenuItem, i) {
		Vec2f dims = font->getMetrics()->getTextDiminsions(RootMenuItemNames[i]);
		m_buttons[i] = new Widgets::Button(pnl, Vec2i(0,0), Vec2i(200, int(dims.y + 5.f)));				
		m_buttons[i]->setTextParams(g_lang.get(RootMenuItemNames[i]), Vec4f(1.f), font, true);
		m_buttons[i]->Clicked.connect(this, &MenuStateRoot::onButtonClick);
	}
	pnl->setLayoutParams(true, Panel::LayoutDirection::VERTICAL);
	pnl->layoutChildren();

	// Glest Logo PicturePanel
	int logoWidth = logoHeight * 2;
	pos = Vec2i(g_metrics.getScreenW() / 2 - logoHeight, logoYPos);
	Widgets::PicturePanel *pp = new Widgets::PicturePanel(&program, pos, Vec2i(logoWidth, logoHeight));
//	pp->setBorderSize(0);
	pp->setPadding(0);
	pp->setImage(g_coreData.getLogoTexture());
	pp->setAutoLayout(false);
	
	// Advanced Engine labels
	font = g_coreData.getGAEFontBig();
	Widgets::StaticText *label = new Widgets::StaticText(pp);
	label->setTextParams(g_lang.get("Advanced"), Vec4f(1.f), font);
	Vec2i sz = label->getTextDimensions() + Vec2i(10,5);
	int tx = int(255.f / 512.f * logoWidth);
	int ty = int(60.f / 256.f * logoHeight);
	label->setPos(Vec2i(tx - sz.x, ty));
	label->setSize(sz);
	label->centreText();
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

	label = new Widgets::StaticText(pp);
	label->setTextParams(g_lang.get("Engine"), Vec4f(1.f), font);
	tx = int(285.f / 512.f * logoWidth);
	label->setPos(Vec2i(tx, ty));
	label->setSize(label->getTextDimensions() + Vec2i(10,5));
	label->centreText();
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

	pos = Vec2i(tx + label->getSize().x, ty + 3);
	// Version label
	font = g_coreData.getGAEFontSmall();
	label = new Widgets::StaticText(pp);
	label->setTextParams(gaeVersionString, Vec4f(1.f), font);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	
	sz = label->getTextDimensions() + Vec2i(10,5);
	label->setPos(pos/*Vec2i(256 - sz.x / 2, 10)*/);
	label->setSize(sz);
	label->centreText();
	
	// gpl logo
	int gplWidth = gplHeight * 2;
	pos = Vec2i(g_metrics.getScreenW() / 2 - gplHeight, gplYPos);
	new Widgets::StaticImage(&program, pos, Vec2i(gplWidth, gplHeight), g_coreData.getGplTexture());

	// end network interface
	program.getSimulationInterface()->changeRole(GameRole::LOCAL);
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
