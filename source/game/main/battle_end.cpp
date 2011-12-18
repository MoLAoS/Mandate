// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//                2009-2011 James McCulloch
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
#include "metrics.h"
#include "stats.h"

#include "leak_dumper.h"

#include "sim_interface.h"

using namespace Shared::Util;

namespace Glest { namespace Main {
using namespace Sim;
using namespace Menu;

CellStrip* buildRow(CellStrip *parent, int cell) {
	Anchors anchors(Anchor(AnchorType::SPRINGY, 5), Anchor(AnchorType::RIGID, 0));
	CellStrip *strip = new CellStrip(parent, Orientation::HORIZONTAL, Origin::CENTRE, 8);
	strip->setCell(cell);
	strip->setAnchors(anchors);
	int hints[8] = { 30, 10, 10, 10, 10, 10, 10, 10 };
	strip->setPercentageHints(hints);
	return strip;
}

StaticText* buildLabel(CellStrip *parent, int cell, const string &txt, int font, int colour) {
	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	StaticText *label = new StaticText(parent);
	label->setCell(cell);
	label->setAnchors(anchors);
	label->setAlignment(Alignment::CENTERED);
	label->textStyle().m_colourIndex = colour;
	label->textStyle().m_fontIndex = font;
	label->setText(txt);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	return label;
}

GameStatsWidget::GameStatsWidget(Container* parent, Vec2i pos, Vec2i size)
		: CellStrip(parent, pos, size, Orientation::VERTICAL, Origin::CENTRE, 0) {
	setWidgetStyle(WidgetType::GAME_STATS);
	GameSettings &gs = g_simInterface.getGameSettings();
	Stats &stats = *g_simInterface.getStats();

	int font = m_textStyle.m_fontIndex;
	int textColour = m_textStyle.m_colourIndex;

	float u = clamp(g_metrics.getScreenH() / (15.f * g_widgetConfig.getDefaultItemHeight()), 1.f, 2.f);
	int ch = int(u * g_widgetConfig.getDefaultItemHeight());

	int cells = 3 + GameConstants::maxPlayers + 2;
	int playerStartCell = 4;
	addCells(cells);

	setSizeHint(0, SizeHint(-1, ch + ch / 2));
	for (int i=1; i < cells; ++i) {
		setSizeHint(i, SizeHint(-1, ch));
	}

	CellStrip *row = buildRow(this, 0);

	string header = gs.getDescription() + " - " + (stats.getVictory(gs.getThisFactionIndex())
													? g_lang.get("Victory") : g_lang.get("Defeat"));
	StaticText* label = buildLabel(row, 0, header, font, textColour);
	label->textStyle().m_fontIndex = g_widgetConfig.getDefaultFontIndex(FontUsage::TITLE);
	label->setAlignment(Alignment::FLUSH_LEFT);

	row = buildRow(this, 2);
	label = buildLabel(row, 5, g_lang.get("Units"), font, textColour);
	label = buildLabel(row, 7, g_lang.get("Resources"), font, textColour);

	row = buildRow(this, 3);
	const char *hdrs[] = {
		"Result", "Score", "Team", "killed", "Lost", "Produced", "Harvested"
	};
	for (int i=1; i < 8; ++i) {
		label = buildLabel(row, i, g_lang.get(hdrs[i-1]), font, textColour);
	}

	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (gs.getFactionControl(i) != ControlType::CLOSED) {
			row = buildRow(this, 4 + i);
			string name = gs.getPlayerName(i) + " [" + gs.getFactionTypeName(i) + "] - ";
			Vec4f colour = Vec4f(factionColours[gs.getColourIndex(i)]) / Vec4f(255.f);
			name += g_lang.get(ControlTypeNames[gs.getFactionControl(i)]);

			label = buildLabel(row, 0, name, font, textColour);
			label->setDoubleShadow(colour, Vec4f(0.f, 0.f, 0.f, 1.f), 1);
			label->setAlignment(Alignment::FLUSH_RIGHT);

			string winlose = stats.getVictory(i) ? "Victory" : "Defeat";
			label = buildLabel(row, 1, winlose, font, textColour);

			int kills = stats.getKills(i);
			int deaths = stats.getDeaths(i);
			int produced = stats.getUnitsProduced(i);
			int harvested = stats.getResourcesHarvested(i);
			int score = kills * 100 + produced * 50 + harvested / 10;
	
			label = buildLabel(row, 2, intToStr(score), font, textColour);
			label = buildLabel(row, 3, intToStr(gs.getTeam(i)), font, textColour);
			label = buildLabel(row, 4, intToStr(kills), font, textColour);
			label = buildLabel(row, 5, intToStr(deaths), font, textColour);
			label = buildLabel(row, 6, intToStr(produced), font, textColour);
			label = buildLabel(row, 7, intToStr(harvested), font, textColour);
		}
	}

	setSize(size);
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
	g_renderer.reset();
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

}} // end namespace
