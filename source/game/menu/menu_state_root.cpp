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
	Font *font = g_widgetConfig.getMenuFont()[FontSize::NORMAL];
	CellStrip *strip = new CellStrip(static_cast<Container*>(&program), Orientation::VERTICAL, Origin::CENTRE, 4);
	strip->setPos(Vec2i(0,0));
	strip->setSize(Vec2i(g_config.getDisplayWidth(), g_config.getDisplayHeight()));
	int hints[] = {
		30, 50, 10, 10 // main logo 30 %, button panel 50 %, gpl logo 10 %, engine label 10%
	};
	strip->setPercentageHints(hints);

	// Glest Logo PicturePanel
	int logoHeight = std::min(int(0.3f * g_metrics.getScreenH()), 256);
	int logoWidth = logoHeight * 2;
	PicturePanel *pp = new PicturePanel(strip, Vec2i(0), Vec2i(logoWidth, logoHeight));
	pp->setCell(0);
	pp->setImage(g_coreData.getLogoTexture());
	pp->setAutoLayout(false);

	Anchors anchors;
	anchors.setCentre(true); // centre in cell
	pp->setAnchors(anchors);

	if (!mainMenu->isTotalConversion()) {
		// Advanced Engine labels
		font = g_widgetConfig.getTitleFont()[FontSize::NORMAL];
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
		font = g_widgetConfig.getTitleFont()[FontSize::SMALL];
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

	font = g_widgetConfig.getMenuFont()[FontSize::NORMAL];

	// Buttons Panel
	anchors.set(Edge::COUNT, 0, false); // anchor all (fill cell)
	CellStrip *pnl = new CellStrip(strip, Orientation::VERTICAL, Origin::CENTRE, RootMenuItem::COUNT);
	pnl->setCell(1);
	pnl->setAnchors(anchors);

	// Buttons
	anchors.setCentre(true); // anchor centre
	foreach_enum (RootMenuItem, i) {
		int height = g_widgetConfig.getDefaultItemHeight();
		m_buttons[i] = new Widgets::Button(pnl);
		m_buttons[i]->setCell(i);
		m_buttons[i]->setSize(Vec2i(8 * height, height));
		m_buttons[i]->setTextParams(g_lang.get(RootMenuItemNames[i]), Vec4f(1.f), font, true);
		m_buttons[i]->setAnchors(anchors);
		m_buttons[i]->Clicked.connect(this, &MenuStateRoot::onButtonClick);
	}
	m_buttons[RootMenuItem::TEST]->setEnabled(false);

	int gplHeight = int(0.1f * g_metrics.getScreenH());
	CellStrip *logoPnl = 0;
	if (mainMenu->gplLogoOnRoot() || mainMenu->gaeLogoOnRoot()) {
		int n = 1;
		if (mainMenu->gplLogoOnRoot() && mainMenu->gaeLogoOnRoot()) {
			n = 2;
		}
		anchors.set(Edge::COUNT, 0, false); // fill cell
		logoPnl = new CellStrip(strip, Orientation::HORIZONTAL, Origin::CENTRE, n);
		logoPnl->setCell(2);
		logoPnl->setAnchors(anchors);
	}
	// anchor centre
	anchors.setCentre(true);
	int gplWidth = gplHeight * 2;
	if (!mainMenu->gaeLogoOnRoot()) {
		if (mainMenu->gplLogoOnRoot()) {
			// gpl logo
			StaticImage *si
				= new StaticImage(logoPnl, Vec2i(0), Vec2i(gplWidth, gplHeight), g_coreData.getGplTexture());
			si->setCell(0);
			si->setAnchors(anchors);
		}
	} else {
		if (mainMenu->gplLogoOnRoot()) {
			StaticImage *si
				= new StaticImage(logoPnl, Vec2i(0), Vec2i(gplWidth, gplHeight), g_coreData.getGaeSplashTexture());
			si->setCell(0);
			si->setAnchors(anchors);

			si = new StaticImage(logoPnl, Vec2i(0), Vec2i(gplWidth, gplHeight), g_coreData.getGplTexture());
			si->setCell(1);
			si->setAnchors(anchors);
		} else {
			StaticImage *si
				= new StaticImage(logoPnl, Vec2i(0), Vec2i(gplWidth, gplHeight), g_coreData.getGaeSplashTexture());
			si->setCell(0);
			si->setAnchors(anchors);
		}
	}

	// adv eng label if total conversion
	if (mainMenu->isTotalConversion()) {
		CellStrip *pnl = new CellStrip(strip, Orientation::HORIZONTAL, Origin::CENTRE, 1);
		pnl->setCell(3);
		anchors.set(Edge::COUNT, 0, false); // fill
		pnl->setAnchors(anchors);
		StaticText *label = new Widgets::StaticText(pnl);
		label->setCell(0);

		// anchor bottom-right
		anchors = Anchors(Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0),
			Anchor(AnchorType::RIGID, 15), Anchor(AnchorType::RIGID, 15));
		label->setAnchors(anchors);

		label->setTextParams("Glest : " + g_lang.get("AdvEng1") + " " + g_lang.get("AdvEng2") + gaeVersionString,
			Vec4f(1.f), g_widgetConfig.getTitleFont()[FontSize::SMALL]);
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
