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
#include "menu_state_character_creator.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "menu_state_graphic_info.h"
#include "util.h"
#include "FSFactory.hpp"
#include "character_creator.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Menu {

// =====================================================
// 	class MenuStateCharacterCreator
// =====================================================
MenuStateCharacterCreator::MenuStateCharacterCreator(Program &program, MainMenu *mainMenu)
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
	// need to keep CharacterCreator instance alive to use callbacks
	m_characterCreator = new CharacterCreator(rootStrip, this);
	m_characterCreator->setCell(0);
	m_characterCreator->setPos(pad);
	m_characterCreator->setSize(Vec2i(g_config.getDisplayWidth() - pad.w * 2, g_config.getDisplayHeight()));

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
	m_returnButton->Clicked.connect(this, &MenuStateCharacterCreator::onButtonClick);

	m_saveButton = new Button(btnPanel, Vec2i(0), sz);
	m_saveButton->setCell(1);
	m_saveButton->setAnchors(anchors);
	m_saveButton->setText(lang.get("Save"));
	m_saveButton->Clicked.connect(this, &MenuStateCharacterCreator::onButtonClick);

	program.setFade(0.f);
}

void MenuStateCharacterCreator::onButtonClick(Widget *source) {
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if (source == m_returnButton) {
		soundRenderer.playFx(coreData.getClickSoundA());
		m_transitionTarget = Transition::RETURN;
		mainMenu->setCameraTarget(MenuStates::ROOT);
	} else if (source == m_saveButton) {
		soundRenderer.playFx(coreData.getClickSoundA());
		m_transitionTarget = Transition::SAVE;
	}
    doFadeOut();
}

void MenuStateCharacterCreator::update() {
	MenuState::update();
	if (m_transition) {
		switch (m_transitionTarget) {
			case Transition::RETURN:
                program.clear();
				mainMenu->setState(new MenuStateRoot(program, mainMenu));
				break;
			case Transition::RE_LOAD:
                program.clear();
				mainMenu->setState(this);
				break;
            case Transition::SAVE:
                m_characterCreator->save();
                program.clear();
				mainMenu->setState(new MenuStateRoot(program, mainMenu));
                break;
		}
	}
}

void MenuStateCharacterCreator::reload() {
    m_transition = true;
	m_transitionTarget = Transition::RE_LOAD;
}

}}//end namespace
