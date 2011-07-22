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
#include "menu_state_options.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "menu_state_graphic_info.h"
#include "util.h"
#include "FSFactory.hpp"
#include "options.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Menu {

// =====================================================
// 	class MenuStateOptions
// =====================================================

MenuStateOptions::MenuStateOptions(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu)
		, m_transitionTarget(Transition::INVALID) {
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();
	const Metrics &metrics = Metrics::getInstance();

	int s = g_widgetConfig.getDefaultItemHeight();

	CellStrip *rootStrip = new CellStrip((Container*)&program, Orientation::VERTICAL, 2);
	Vec2i pad(15, 25);
	rootStrip->setPos(pad);
	rootStrip->setSize(Vec2i(g_config.getDisplayWidth() - pad.w * 2, g_config.getDisplayHeight() - pad.h * 2));

	// Option panel
	rootStrip->setSizeHint(0, SizeHint());
	// need to keep Options instance alive to use callbacks
	m_options = new Options(rootStrip, this);
	m_options->setCell(0);
	m_options->setPos(pad);
	m_options->setSize(Vec2i(g_config.getDisplayWidth() - pad.w * 2, g_config.getDisplayHeight()));
	//m_options->setSizeHint(0, SizeHint(-1, s));
	//m_options->setSizeHint(1, SizeHint(-1, 1000));

	Anchors anchors(Anchor(AnchorType::RIGID, 0));

	// Buttons panel
	rootStrip->setSizeHint(1, SizeHint(-1, s * 3));
	CellStrip *btnPanel = new CellStrip(rootStrip, Orientation::HORIZONTAL, 3);
	btnPanel->setCell(1);
	btnPanel->setAnchors(anchors);

	anchors.setCentre(true, true);
	Vec2i sz(s * 7, s);

	// create buttons
	m_returnButton = new Button(btnPanel, Vec2i(0), sz);
	m_returnButton->setCell(0);
	m_returnButton->setAnchors(anchors);
	m_returnButton->setText(lang.get("Return"));
	m_returnButton->Clicked.connect(this, &MenuStateOptions::onButtonClick);

	m_autoConfigButton = new Button(btnPanel, Vec2i(0), sz);
	m_autoConfigButton->setCell(1);
	m_autoConfigButton->setAnchors(anchors);
	m_autoConfigButton->setText(lang.get("AutoConfig"));
	m_autoConfigButton->Clicked.connect(this, &MenuStateOptions::onButtonClick);
	
	m_openGlInfoButton = new Button(btnPanel, Vec2i(0), sz);
	m_openGlInfoButton->setCell(2);
	m_openGlInfoButton->setAnchors(anchors);
	m_openGlInfoButton->setText(lang.get("GraphicInfo"));
	m_openGlInfoButton->Clicked.connect(this, &MenuStateOptions::onButtonClick);

	program.setFade(0.f);
}

void MenuStateOptions::onButtonClick(Widget *source) {
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if (source == m_autoConfigButton) {
		soundRenderer.playFx(coreData.getClickSoundA());
		Renderer::getInstance().autoConfig();
		saveConfig();
		m_transitionTarget = Transition::RE_LOAD;
	} else if (source == m_returnButton) {
		soundRenderer.playFx(coreData.getClickSoundA());
		saveConfig();
		m_transitionTarget = Transition::RETURN;
		mainMenu->setCameraTarget(MenuStates::ROOT);
	} else if (source == m_openGlInfoButton) {
		soundRenderer.playFx(coreData.getClickSoundB());
		m_transitionTarget = Transition::GL_INFO;
		mainMenu->setCameraTarget(MenuStates::GFX_INFO);
	}
	doFadeOut();
}

void MenuStateOptions::update() {
	MenuState::update();
	if (m_transition) {
		program.clear();
		switch (m_transitionTarget) {
			case Transition::RETURN:
				mainMenu->setState(new MenuStateRoot(program, mainMenu));
				break;
			case Transition::GL_INFO:
				mainMenu->setState(new MenuStateGraphicInfo(program, mainMenu));
				break;
			case Transition::RE_LOAD:
				g_lang.setLocale(g_config.getUiLocale());
				saveConfig();
				mainMenu->setState(new MenuStateOptions(program, mainMenu));
				break;
		}
	}
}

void MenuStateOptions::reload() {
	m_transitionTarget = Transition::RE_LOAD;
	int foo = m_options->getActivePage();
	g_config.setUiLastOptionsPage(foo);
	doFadeOut();
}

// private

void MenuStateOptions::saveConfig(){
	//m_options->save();
	g_config.save();

	Renderer::getInstance().loadConfig();
	SoundRenderer::getInstance().loadConfig();
}

}}//end namespace
