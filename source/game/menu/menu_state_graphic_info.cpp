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

	Font *normFont = CoreData::getInstance().getFTMenuFontNormal();
	Font *smallFont = CoreData::getInstance().getFTMenuFontSmall();

	// text dimensions
	Vec2f infoDims = smallFont->getMetrics()->getTextDiminsions(glInfo);
	Vec2f glExtDims = smallFont->getMetrics()->getTextDiminsions(glExt);
	Vec2f plExtDims = smallFont->getMetrics()->getTextDiminsions(glExt2);

	const Metrics &metrics = Metrics::getInstance();
	int w = int(std::max(infoDims.x, std::max(glExtDims.x, plExtDims.x)));
	w += 10;
	int gap = (metrics.getScreenW() - 3 * w) / 4;

	// basic info
	int x = gap;
	int y = metrics.getScreenH() - 110 - int(infoDims.y);
	int h = int(infoDims.y) + 10;
	Widgets::StaticText::Ptr l_text = new Widgets::StaticText(&program, Vec2i(x, y), Vec2i(w, h));
	l_text->setTextParams(glInfo, Vec4f(1.f), smallFont);

	// gl extensions
	x += gap + w;
	y = 200;
	h = metrics.getScreenH() - 300;
	ScrollText::Ptr l_stext = new ScrollText(&program, Vec2i(x, y), Vec2i(w, h));
	l_stext->setTextParams(glExt, Vec4f(1.f), smallFont, false);
	l_stext->init();

	// platform extensions
	x += gap + w;
	l_stext = new ScrollText(&program, Vec2i(x, y), Vec2i(w, h));
	l_stext->setTextParams(glExt2, Vec4f(1.f), smallFont, false);
	l_stext->init();

	// return button
	x = (metrics.getScreenW() - 150) / 2;
	w = 150, y = 85, h = 30;
	Button::Ptr l_button = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	l_button->setTextParams(lang.get("Return"), Vec4f(1.f), normFont);
	l_button->Clicked.connect(this, &MenuStateGraphicInfo::onButtonClick);

	//program.setFade(0.f);
}

void MenuStateGraphicInfo::onButtonClick(Button::Ptr btn) {
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
