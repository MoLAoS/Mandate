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
	Font *font = g_coreData.getFTMenuFontNormal();
	CellStrip *strip = new CellStrip(&program, Orientation::VERTICAL, 4);
	strip->setPos(Vec2i(0,0));
	strip->setSize(Vec2i(g_config.getDisplayWidth(), g_config.getDisplayHeight()));
	int hints[] = {
		30, 50, 10, 10 // main logo 30 %, button panel 50 %, gpl logo 10 %, engine label 10%
	};
	strip->setPercentageHints(hints);

	// Glest Logo PicturePanel
	int logoHeight = std::min(int(0.3f * g_metrics.getScreenH()), 256);
	int logoWidth = logoHeight * 2;
	PicturePanel *pp = new PicturePanel(strip->getCell(0), Vec2i(0), Vec2i(logoWidth, logoHeight));
	pp->setPadding(0);
	pp->setImage(g_coreData.getLogoTexture());
	pp->setAutoLayout(false);

	Anchors anchors;
	anchors.setCentre(true); // centre in cell
	pp->setAnchors(anchors);

	if (!mainMenu->isTotalConversion()) {
		// Advanced Engine labels
		font = g_coreData.getGAEFontBig();
		Widgets::StaticText *label = new Widgets::StaticText(pp);
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
		Vec2i pos = Vec2i(tx + label->getSize().x, logoHeight - ty - sz.h + szDiff - 2);

		label = new Widgets::StaticText(pp);
		label->setTextParams(gaeVersionString, Vec4f(1.f), font);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

		sz = label->getTextDimensions() + Vec2i(10,5);
		label->setPos(pos);
		label->setSize(sz);
		label->centreText();
	}

	font = g_coreData.getFTMenuFontNormal();

	// Buttons Panel
	anchors.set(Edge::COUNT, 0, false); // anchor all (fill cell)
	CellStrip *pnl = new CellStrip(strip->getCell(1), Orientation::VERTICAL, RootMenuItem::COUNT);
	pnl->setAnchors(anchors);

	// Buttons
	anchors.setCentre(true); // anchor centre
	foreach_enum (RootMenuItem, i) {
		m_buttons[i] = new Widgets::Button(pnl->getCell(i));
		m_buttons[i]->setSize(Vec2i(256, 32));
		m_buttons[i]->setTextParams(g_lang.get(RootMenuItemNames[i]), Vec4f(1.f), font, true);
		m_buttons[i]->setAnchors(anchors);
		m_buttons[i]->Clicked.connect(this, &MenuStateRoot::onButtonClick);
	}

	int gplHeight = int(0.1f * g_metrics.getScreenH());
	CellStrip *logoPnl = 0;
	if (mainMenu->gplLogoOnRoot() || mainMenu->gaeLogoOnRoot()) {
		int n = 1;
		if (mainMenu->gplLogoOnRoot() && mainMenu->gaeLogoOnRoot()) {
			n = 2;
		}
		anchors.set(Edge::COUNT, 0, false); // fill cell
		logoPnl = new CellStrip(strip->getCell(2), Orientation::HORIZONTAL, n);
		logoPnl->setAnchors(anchors);
	}
	// anchor centre
	anchors.setCentre(true);
	int gplWidth = gplHeight * 2;
	if (!mainMenu->gaeLogoOnRoot()) {
		if (mainMenu->gplLogoOnRoot()) {
			// gpl logo
			StaticImage *si = new StaticImage(logoPnl->getCell(0),
				Vec2i(0), Vec2i(gplWidth, gplHeight), g_coreData.getGplTexture());
			si->setAnchors(anchors);
		}
	} else {
		if (mainMenu->gplLogoOnRoot()) {
			StaticImage *si = new Widgets::StaticImage(logoPnl->getCell(0), 
				Vec2i(0), Vec2i(gplWidth, gplHeight), g_coreData.getGaeSplashTexture());
			si->setAnchors(anchors);

			si = new Widgets::StaticImage(logoPnl->getCell(1),
				Vec2i(0), Vec2i(gplWidth, gplHeight), g_coreData.getGplTexture());
			si->setAnchors(anchors);
		} else {
			StaticImage *si = new Widgets::StaticImage(logoPnl->getCell(0),
				Vec2i(0), Vec2i(gplWidth, gplHeight), g_coreData.getGaeSplashTexture());
			si->setAnchors(anchors);
		}
	}

	// adv eng label if total conversion
	if (mainMenu->isTotalConversion()) {
		CellStrip *pnl = new CellStrip(strip->getCell(3), Orientation::HORIZONTAL, 1);
		anchors.set(Edge::COUNT, 0, false); // fill
		pnl->setAnchors(anchors);
		StaticText *label = new Widgets::StaticText(pnl->getCell(0));

		// anchor bottom-right
		anchors = Anchors(Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0),
			Anchor(AnchorType::RIGID, 15), Anchor(AnchorType::RIGID, 15));
		label->setAnchors(anchors);

		label->setTextParams("Glest : " + g_lang.get("AdvEng1") + " " + g_lang.get("AdvEng2") + gaeVersionString,
			Vec4f(1.f), g_coreData.getGAEFontSmall());
		Vec2i size = label->getTextDimensions() + Vec2i(5,5);
		label->setSize(size);
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

CellStrip *ws;

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
	                   std::min(g_config.getDisplayHeight() / 2, 300));
	Vec2i pos = g_metrics.getScreenDims() / 2 - size / 2;
	ws = new CellStrip(&program, Orientation::VERTICAL, RootMenuItem::COUNT);
	ws->setBorderStyle(bs);
	ws->setPos(pos);
	ws->setSize(size);

	// some buttons
	for (int i=0; i < RootMenuItem::COUNT; ++i) {
		ws->getCell(i)->setAnchors(anchors);
		Button *btn = new Button(ws->getCell(i), Vec2i(0), Vec2i(150, 40), false);
		BorderStyle borderStyle;
		borderStyle.setSizes(3);
		int li = g_widgetConfig.getColourIndex(Colour(0xBFu, 0x5Eu, 0x5Eu, 0xAF));
		int di = g_widgetConfig.getColourIndex(Colour(0x6Fu, 0x00u, 0x00u, 0xAF));
		borderStyle.setRaise(li, di);
		btn->setBorderStyle(borderStyle);

		BackgroundStyle backStyle;
		int bi = g_widgetConfig.getColourIndex(Colour(0x9Fu, 0x00u, 0x00u, 0xAF));
		backStyle.setColour(bi);
		btn->setBackgroundStyle(backStyle);

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
