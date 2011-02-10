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
	int sh = g_metrics.getScreenH();
	int sixtyPercent = int(0.6f * sh);
	int fiftyPercent = int(0.5f * sh);
	int fortyPercent = int(0.4f * sh);
	int tenPercent = int(0.1f * sh);

	int logoHeight = std::min(fortyPercent, 256);
	int logoYPos = 0;

	int btnPnlHeight = fiftyPercent;
	int btnPnlYPos = logoHeight + 5;

	int gplHeight = tenPercent;
	int gplYPos = sh - tenPercent - sh / 50;

	int widgetPad = btnPnlHeight / 25;

	// Buttons Panel
	Vec2i pos(g_metrics.getScreenW() / 2 - 125, btnPnlYPos);
	Vec2i size(250, btnPnlHeight);
	Widgets::Panel *pnl = new Widgets::Panel(&program, pos, size);
	
	stringstream ss;
	ss << "Created button panel at:" << pos << " size:" << size;
	g_logger.logProgramEvent(ss.str());

	pnl->setPaddingParams(10, widgetPad);
	BorderStyle borderStyle;
	borderStyle.m_type = BorderType::SOLID;
	borderStyle.setSolid(g_widgetConfig.getColourIndex(Colour(0xFFu, 0u, 0u, 0xFFu)));
	borderStyle.setSizes(1);
	pnl->setBorderStyle(borderStyle);

	Font *font = g_coreData.getFTMenuFontNormal();
	int btnHeight = (btnPnlHeight - RootMenuItem::COUNT * 10) / (RootMenuItem::COUNT + 2);

	// Buttons
	foreach_enum (RootMenuItem, i) {
		//Vec2f dims = font->getMetrics()->getTextDiminsions(RootMenuItemNames[i]);
		m_buttons[i] = new Widgets::Button(pnl, Vec2i(0,0), Vec2i(200, btnHeight));
		m_buttons[i]->setTextParams(g_lang.get(RootMenuItemNames[i]), Vec4f(1.f), font, true);
		m_buttons[i]->Clicked.connect(this, &MenuStateRoot::onButtonClick);
	}
	pnl->setLayoutParams(true, Orientation::VERTICAL, Origin::FROM_TOP);
	pnl->layoutChildren();

	// Glest Logo PicturePanel
	int logoWidth = logoHeight * 2;
	pos = Vec2i(g_metrics.getScreenW() / 2 - logoHeight, logoYPos);
	Widgets::PicturePanel *pp = new Widgets::PicturePanel(&program, pos, Vec2i(logoWidth, logoHeight));
	pp->setPadding(0);
	pp->setImage(g_coreData.getLogoTexture());
	pp->setAutoLayout(false);

	Widgets::StaticText *label;
	if (!mainMenu->isTotalConversion()) {
		// Advanced Engine labels
		font = g_coreData.getGAEFontBig();
		label = new Widgets::StaticText(pp);
		label->setTextParams(g_lang.get("AdvEng1"), Vec4f(1.f), font);
		Vec2i sz = label->getTextDimensions() + Vec2i(10,5);
		int tx = int(255.f / 512.f * logoWidth);
		int ty = int(60.f / 256.f * logoHeight);
		label->setPos(Vec2i(tx - sz.w, logoHeight - ty - sz.h));
		label->setSize(sz);
		label->centreText();
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

		label = new Widgets::StaticText(pp);
		label->setTextParams(g_lang.get("AdvEng2"), Vec4f(1.f), font);
		tx = int(285.f / 512.f * logoWidth);
		label->setPos(Vec2i(tx, logoHeight - ty - sz.h));
		label->setSize(label->getTextDimensions() + Vec2i(10,5));
		label->centreText();
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

		// Version label
		int bigHeight = int(font->getMetrics()->getHeight());
		font = g_coreData.getGAEFontSmall();
		int szDiff = bigHeight - int(font->getMetrics()->getHeight());
		pos = Vec2i(tx + label->getSize().x, logoHeight - ty - sz.h + szDiff - 2);

		label = new Widgets::StaticText(pp);
		label->setTextParams(gaeVersionString, Vec4f(1.f), font);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

		sz = label->getTextDimensions() + Vec2i(10,5);
		label->setPos(pos/*Vec2i(256 - sz.x / 2, 10)*/);
		label->setSize(sz);
		label->centreText();
	} else {
		label = new Widgets::StaticText(&program);
		Vec2i pos, size;
		label->setTextParams("Glest Advanced Engine " + gaeVersionString,
			Vec4f(1.f), g_coreData.getGAEFontSmall());
		size = label->getTextDimensions() + Vec2i(5,5);
		pos = Vec2i(g_metrics.getScreenW() - size.w - 15, g_config.getDisplayHeight() - size.h - 10);
		label->setPos(pos);
		label->setSize(size);
	}

	if (!mainMenu->gaeLogoOnRoot()) {
		if (mainMenu->gplLogoOnRoot()) {
			// gpl logo
			int gplWidth = gplHeight * 2;
			pos = Vec2i(g_metrics.getScreenW() / 2 - gplHeight, gplYPos);
			new Widgets::StaticImage(&program, pos, Vec2i(gplWidth, gplHeight), g_coreData.getGplTexture());
		}
	} else {
		if (mainMenu->gplLogoOnRoot()) {
			int gplWidth = gplHeight * 2;
			int xpos = g_metrics.getScreenW() / 2 - gplWidth - gplWidth / 2 - 10;
			pos = Vec2i(xpos, gplYPos);
			new Widgets::StaticImage(&program, pos, Vec2i(gplWidth + gplWidth / 2, gplHeight + gplHeight / 2),
				g_coreData.getGaeSplashTexture());

			xpos = g_metrics.getScreenW() / 2 + 10;
			pos = Vec2i(xpos, gplYPos + gplHeight / 4);
			new Widgets::StaticImage(&program, pos, Vec2i(gplWidth, gplHeight), g_coreData.getGplTexture());
		} else {
			int gplWidth = gplHeight * 2;
			int xpos = g_metrics.getScreenW() / 2 - (gplWidth + gplWidth / 2) / 2;
			pos = Vec2i(xpos, gplYPos);
			new Widgets::StaticImage(&program, pos, Vec2i(gplWidth + gplWidth / 2, gplHeight + gplHeight / 2),
				g_coreData.getGaeSplashTexture());
		}
	}
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
		case RootMenuItem::TEST:     targetState = MenuStates::TEST;        break;
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
			case RootMenuItem::TEST: newState = new MenuStateTest(program, mainMenu); break;
			default: break;
		}
		if (newState) {
			mainMenu->setState(newState);
		} else {
			program.exit();
		}
	}
}

// =====================================================
//  class MenuStateTest
// =====================================================

WidgetStrip *ws;

MenuStateTest::MenuStateTest(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu) {
	Font *font = g_coreData.getFTMenuFontNormal();
	// create
	int gap = (g_metrics.getScreenW() - 450) / 4;
	int x = gap, w = 150, y = g_config.getDisplayHeight() - 80, h = 30;
	m_returnButton = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	m_returnButton->setTextParams(g_lang.get("Return"), Vec4f(1.f), font);
	m_returnButton->Clicked.connect(this, &MenuStateTest::onButtonClick);

	Anchors anchors;
	anchors.set(Edge::LEFT, 10, true);
	anchors.set(Edge::RIGHT, 10, true);
	anchors.set(Edge::TOP, 15, true);
	anchors.set(Edge::BOTTOM, 15, true);

	BorderStyle bs;
	bs.setSolid(g_widgetConfig.getColourIndex(Colour(255u, 0u, 0u, 255u)));
	bs.setSizes(1);

	Vec2i size = Vec2i(std::min(g_config.getDisplayWidth() / 3, 300),
	                   std::min(g_config.getDisplayHeight() / 2, 400));
	Vec2i pos = g_metrics.getScreenDims() / 2 - size / 2;
	ws = new WidgetStrip(&program, Orientation::VERTICAL);
	ws->setBorderStyle(bs);
	ws->setPos(pos);
	ws->setSize(size);
	ws->setDefaultAnchors(anchors);
	
	// some buttons
	for (int i=0; i < RootMenuItem::COUNT; ++i) {
		Vec2i pos(0, 0);
		Vec2i size(150, 40);
		Button *btn = new Button(ws, pos, size);
		btn->setTextParams(g_lang.get(RootMenuItemNames[i]), Vec4f(1.f), font, true);
	}
}

void MenuStateTest::update() {
	static int counter = 0;
	++counter;
	MenuState::update();
	if (m_transition) {
		program.clear();
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
	} else {
		const int var = 300;
		int womble = counter % var;
		if (womble > var / 2) {
			womble = var - womble;
		}
		int frog = std::min(g_config.getDisplayHeight() / 2, 300);
		ws->setSize(Vec2i(ws->getWidth(), womble + frog));
	}
}

void MenuStateTest::onButtonClick(Button* btn) {
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	soundRenderer.playFx(g_coreData.getClickSoundA());
	mainMenu->setCameraTarget(MenuStates::ROOT);
	doFadeOut();
}

}}//end namespace
