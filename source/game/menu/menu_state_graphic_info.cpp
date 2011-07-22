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
#include "menu_state_graphic_info.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "menu_state_options.h"

#include "leak_dumper.h"

namespace Glest { namespace Menu {

// =====================================================
// 	class MenuStateGraphicInfo
// =====================================================

MenuStateGraphicInfo::MenuStateGraphicInfo(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu) {
	Lang &lang= Lang::getInstance();
	Renderer &renderer = Renderer::getInstance();

	string glInfo = renderer.getGlInfo();
	string glExt = renderer.getGlMoreInfo();
	string glExt2 = renderer.getGlMoreInfo2();

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

	CellStrip *mainStrip = new CellStrip(rootStrip, Orientation::HORIZONTAL, 3);
	mainStrip->setCell(0);
	mainStrip->setAnchors(fillAnchors);

	Widgets::StaticText* l_text = new Widgets::StaticText(mainStrip);
	l_text->setCell(0);
	l_text->setAnchors(fillAnchors);
	l_text->setText(glInfo);

	ScrollText* l_stext = new ScrollText(mainStrip);
	l_stext->setCell(1);
	l_stext->setAnchors(fillAnchors);
	l_stext->setText(glExt);

	l_stext = new ScrollText(mainStrip);
	l_stext->setCell(2);
	l_stext->setAnchors(fillAnchors);
	l_stext->setText(glExt2);

	// buttons panel
	CellStrip *btnPanel = new CellStrip(rootStrip, Orientation::HORIZONTAL, 3);
	btnPanel->setCell(1);
	btnPanel->setAnchors(fillAnchors);

	// create buttons
	Vec2i sz(itemHeight * 7, itemHeight);
	Button* l_button = new Button(btnPanel, Vec2i(0), sz);
	l_button->setCell(0);
	l_button->setAnchors(centreAnchors);
	l_button->setText(lang.get("Return"));
	l_button->Clicked.connect(this, &MenuStateGraphicInfo::onButtonClick);

	program.setFade(0.f);
}

void MenuStateGraphicInfo::onButtonClick(Widget*) {
	mainMenu->setCameraTarget(MenuStates::OPTIONS);
	doFadeOut();
}

void MenuStateGraphicInfo::update() {
	MenuState::update();
	if (m_transition) {
		program.clear();
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
	}
}

}}//end namespace
