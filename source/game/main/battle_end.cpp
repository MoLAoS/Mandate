// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "battle_end.h"

#include "main_menu.h"
#include "program.h"
#include "core_data.h"
#include "lang.h"
#include "util.h"
#include "renderer.h"
#include "main_menu.h"
#include "sound_renderer.h"
#include "components.h"
#include "metrics.h"
#include "stats.h"

#include "leak_dumper.h"

#include "sim_interface.h"

using namespace Shared::Util;

namespace Glest { namespace Main {
using namespace Sim;
using namespace Menu;


GameStatsWidget::GameStatsWidget(Container* parent, Vec2i pos, Vec2i size)
		: Container(parent, pos, size) {
	GameSettings &gs = g_simInterface.getGameSettings();
	Stats &stats = *g_simInterface.getStats();

	int font = g_widgetConfig.getDefaultFontIndex(FontUsage::GAME);
	int white = g_widgetConfig.getColourIndex(Colour(255u));

	int y_lines = 3 + /*GameConstants::maxPlayers*/ 12 + 2; // headers, player row and 2 for spacing
	int y_gap = g_metrics.getScreenH() / y_lines;
	int y = g_metrics.getScreenH() - y_gap;

	int w = g_metrics.getScreenW();
	int x_centres[8];
	int startX = w / 20; // 5% gap
	int runningX = startX;
	x_centres[0] = runningX + (w / 5) / 2; // x pos for centre of player label
	runningX += (w / 5); // 20% for player label
	for (int i=1; i < 8; ++i) {
		x_centres[i] = runningX + (w / 10) / 2; // x pos for centres of each data column
		runningX += (w / 10); // 10% for others
	}
	// 5% gap at end

	StaticText* label;

	string header = gs.getDescription() + " - " + (stats.getVictory(gs.getThisFactionIndex())
													? g_lang.get("Victory") : g_lang.get("Defeat"));
	pos = Vec2i(100, y);
	size = Vec2i(0);
	label = new StaticText(this, pos, size);
	label->setText(header);
	label->setAlignment(Alignment::NONE);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	
	font = g_widgetConfig.getDefaultFontIndex(FontUsage::MENU);
	const Font *fontPtr = g_widgetConfig.getMenuFont();
	const FontMetrics *fm = fontPtr->getMetrics();

	y -= y_gap;
	int x = x_centres[5] - int(fm->getTextDiminsions(g_lang.get("Units")).x) / 2;
	label = new StaticText(this, Vec2i(x, y), size);
	label->setText(g_lang.get("Units"));
	label->setAlignment(Alignment::NONE);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

	x = x_centres[7] - int(fm->getTextDiminsions(g_lang.get("Resources")).x) / 2;
	label = new StaticText(this, Vec2i(x, y), size);
	label->setText(g_lang.get("Resources"));
	label->setAlignment(Alignment::NONE);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

	y -= y_gap;

	const char *hdrs[] = {
		"Result", "Score", "Team", "killed", "Lost", "Produced", "Harvested"
	};
	for (int i=1; i < 8; ++i) {
		x = x_centres[i] - int(fm->getTextDiminsions(g_lang.get(hdrs[i-1])).x) / 2;
		label = new StaticText(this, Vec2i(x, y), size);
		label->setText(g_lang.get(hdrs[i-1]));
		label->setAlignment(Alignment::NONE);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	}

	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		y -= y_gap;
		if (gs.getFactionControl(i) != ControlType::CLOSED) {
			string name = gs.getPlayerName(i) + " [" + gs.getFactionTypeName(i) + "] - ";
			Vec4f colour = Vec4f(factionColours[gs.getColourIndex(i)]) / Vec4f(255.f);
			name += g_lang.get(ControlTypeNames[gs.getFactionControl(i)]);
			x = x_centres[0] - int(fm->getTextDiminsions(name).x) / 2;
			label = new StaticText(this, Vec2i(x, y), size);
			label->setText(name);
			label->setAlignment(Alignment::NONE);
			label->setDoubleShadow(colour, Vec4f(0.f, 0.f, 0.f, 1.f), 1);

			string winlose = stats.getVictory(i) ? "Victory" : "Defeat";
			x = x_centres[1] - int(fm->getTextDiminsions(winlose).x) / 2;
			label = new StaticText(this, Vec2i(x, y), size);
			label->setText(winlose);
			label->setAlignment(Alignment::NONE);
			label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

			int kills = stats.getKills(i);
			int deaths = stats.getDeaths(i);
			int produced = stats.getUnitsProduced(i);
			int harvested = stats.getResourcesHarvested(i);
			int score = kills * 100 + produced * 50 + harvested / 10;
	
			string tmp = intToStr(score);
			x = x_centres[2] - int(fm->getTextDiminsions(tmp).x) / 2;
			label = new StaticText(this, Vec2i(x, y), size);
			label->setText(tmp);
			label->setAlignment(Alignment::NONE);
			label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

			tmp = intToStr(gs.getTeam(i));
			x = x_centres[3] - int(fm->getTextDiminsions(tmp).x) / 2;
			label = new StaticText(this, Vec2i(x, y), size);
			label->setText(tmp);
			label->setAlignment(Alignment::NONE);
			label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

			tmp = intToStr(kills);
			x = x_centres[4] - int(fm->getTextDiminsions(tmp).x) / 2;
			label = new StaticText(this, Vec2i(x, y), size);
			label->setText(tmp);
			label->setAlignment(Alignment::NONE);
			label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

			tmp = intToStr(deaths);
			x = x_centres[5] - int(fm->getTextDiminsions(tmp).x) / 2;
			label = new StaticText(this, Vec2i(x, y), size);
			label->setText(tmp);
			label->setAlignment(Alignment::NONE);
			label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

			tmp = intToStr(produced);
			x = x_centres[6] - int(fm->getTextDiminsions(tmp).x) / 2;
			label = new StaticText(this, Vec2i(x, y), size);
			label->setText(tmp);
			label->setAlignment(Alignment::NONE);
			label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));

			tmp = intToStr(harvested);
			x = x_centres[7] - int(fm->getTextDiminsions(tmp).x) / 2;
			label = new StaticText(this, Vec2i(x, y), size);
			label->setText(tmp);
			label->setAlignment(Alignment::NONE);
			label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		}
	}
}

// =====================================================
//  class BattleEnd
// =====================================================

BattleEnd::BattleEnd(Program &program, bool quickExit)
		: ProgramState(program)
		, isQuickExit(quickExit) {
	// note: creates the gui even though widget is not used
	GameStatsWidget *widget =  new GameStatsWidget(&program, Vec2i(0), g_metrics.getScreenDims());
}

BattleEnd::~BattleEnd() {
	g_soundRenderer.playMusic(g_coreData.getMenuMusic());
}

void BattleEnd::update() {
	//TOOD: add AutoTest to config
	/*
	if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateBattleEnd(program);
	}*/
}

void BattleEnd::renderBg() {
	g_renderer.clearBuffers();
	g_renderer.renderBackground(g_coreData.getBackgroundTexture());
}

void BattleEnd::renderFg() {
	g_renderer.swapBuffers();
}

void BattleEnd::keyDown(const Key &key) {
	if (!key.isModifier()) {
		if (isQuickExit) {
			program.exit();
		} else {
			program.clear();
			program.setState(new MainMenu(program));
		}
	}
}

void BattleEnd::mouseDownLeft(int x, int y) {
	if (isQuickExit) {
		program.exit();
	} else {
		program.clear();
		program.setState(new MainMenu(program));
	}
}

}}//end namespace
