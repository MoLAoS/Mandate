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
#include "menu_state_about.h"

#include "renderer.h"
#include "menu_state_root.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "menu_state_options.h"
#include "game_util.h"

#include "leak_dumper.h"

namespace Glest { namespace Menu {
using namespace Util;

// =====================================================
// 	class MenuStateAbout
// =====================================================

MenuStateAbout::MenuStateAbout(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu) {
	int centreX = g_metrics.getScreenW() / 2;
	Font *font = g_widgetConfig.getMenuFont()[FontSize::SMALL];
	Font *fontBig = g_widgetConfig.getMenuFont()[FontSize::NORMAL];
	const FontMetrics *fm = font->getMetrics();
	const FontMetrics *fmBig = fontBig->getMetrics();
	Vec2i btnSize(150, 30);
	Vec2i btnPos(centreX - btnSize.x / 2, 50);
	m_returnButton = new Button(&program, btnPos, btnSize);
	m_returnButton->setTextParams(g_lang.get("Return"), Vec4f(1.f), font, true);
	m_returnButton->Clicked.connect(this, &MenuStateAbout::onReturn);

	int topY = g_metrics.getScreenH();
	//int centreY = topY / 2;
	int y = topY - 50;
	int x;
	int fh = int(fm->getHeight() + 1.f);
	int fhBig = int(fmBig->getHeight() + 1.f);

	Vec2i dims;
	StaticText* label;
	for (int i=0; i < 4; ++i) {
		y -= fh;
		label = new StaticText(&program);
		label->setTextParams(getAboutString1(i), Vec4f(1.f), font, false);
		dims = label->getTextDimensions();
		x = centreX - 10 - dims.x;
		label->setPos(x, y);

		if (i < 3) {
			label = new StaticText(&program);
			label->setTextParams(getAboutString2(i), Vec4f(1.f), font, false);
			x = centreX + 10;
			label->setPos(x, y - fh / 2);
		}
	}

	y -= fh;
	int sy = y;
	int thirdX = g_metrics.getScreenW() / 3;//centreX / 2;

	y -= fhBig;
	label = new StaticText(&program);
	label->setTextParams("The Glest Team:", Vec4f(1.f), fontBig, false);
	dims = label->getTextDimensions();
	x = thirdX - dims.x / 2;
	label->setPos(x, y);

	for (int i=0; i < getGlestTeamMemberCount(); ++i) {
		y -= fh;
		label = new StaticText(&program);
		label->setTextParams(getGlestTeamMemberName(i), Vec4f(1.f), font, false);
		dims = Vec2i(fm->getTextDiminsions(getGlestTeamMemberNameNoDiacritics(i)) + Vec2f(1.f));
		x = thirdX - 10 - dims.x;
		label->setPos(x, y);

		label = new StaticText(&program);
		label->setTextParams(getGlestTeamMemberRole(i), Vec4f(1.f), font, false);
		x = thirdX + 10;
		label->setPos(x, y);		
	}

	y = sy;
	thirdX *= 2;//3;

	y -= fhBig;
	label = new StaticText(&program);
	label->setTextParams("The GAE Team:", Vec4f(1.f), fontBig, false);
	dims = label->getTextDimensions();
	x = thirdX - dims.x / 2;
	label->setPos(x, y);

	for (int i=0; i < getGAETeamMemberCount(); ++i) {
		y -= fh;
		label = new StaticText(&program);
		label->setTextParams(getGAETeamMemberName(i), Vec4f(1.f), font, false);
		dims = label->getTextDimensions();
		x = thirdX - 10 - dims.x;
		label->setPos(x, y);

		label = new StaticText(&program);
		label->setTextParams(getGAETeamMemberRole(i), Vec4f(1.f), font, false);
		x = thirdX + 10;
		label->setPos(x, y);		
	}

	y -= fh;
	y -= fhBig;
	label = new StaticText(&program);
	label->setTextParams("GAE Contributors:", Vec4f(1.f), fontBig, false);
	dims = label->getTextDimensions();
	x = thirdX - dims.x / 2;
	label->setPos(x, y);

	for (int i=0; i < getGAEContributorCount(); ++i) {
		y -= fh;
		label = new StaticText(&program);
		label->setTextParams(getGAEContributorName(i), Vec4f(1.f), font, false);
		dims = label->getTextDimensions();
		x = thirdX - 10 - dims.x;
		label->setPos(x, y);

		label = new StaticText(&program);
		label->setTextParams(getGAETeamMemberRole(0), Vec4f(1.f), font, false);
		x = thirdX + 10;
		label->setPos(x, y);
	}
}

void MenuStateAbout::onReturn(Button*) {
	mainMenu->setCameraTarget(MenuStates::ROOT);
	g_soundRenderer.playFx(g_coreData.getClickSoundB());
	doFadeOut();
}

void MenuStateAbout::update() {
	MenuState::update();

	if (m_transition) {
		program.clear();
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
	}
}

}}//end namespace
