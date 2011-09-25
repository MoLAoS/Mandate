// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "game_menu.h"
#include "core_data.h"
#include "game.h"
#include "program.h"

#include "leak_dumper.h"

namespace Glest { namespace Gui {

using Global::CoreData;

typedef void (GameMenu::*SlotFunc)(Widget*);

Button* buildButton(GameMenu *menu, CellStrip *strip, int cell, const string &txt, SlotFunc func) {
	int dh = g_widgetConfig.getDefaultItemHeight();
	Vec2i btnSize(dh * 7, dh);
	Button *btn = new Button(strip, Vec2i(0), btnSize);
	btn->setCell(cell);
	btn->setAnchors(Anchors::getCentreAnchors());
	btn->setText(txt);
	btn->Clicked.connect(menu, func);
	return btn;
}

GameMenu::GameMenu()
		: Frame((Container*)(&g_widgetWindow), ButtonFlags::CLOSE | ButtonFlags::ROLL_UPDOWN)
		, m_btnStrip(0) {
	setTitleText(g_lang.get("GameMenu"));

	m_btnStrip = new CellStrip(this, Orientation::VERTICAL, 8); // 8 buttons
	m_btnStrip->setCell(1);
	m_btnStrip->setAnchors(Anchors::getFillAnchors());
	Close.connect(this, &GameMenu::onReturnToGame);
	init();
}

void GameMenu::init() {
	m_btnStrip->clear();
	m_btnStrip->addCells(7);

	int dh = g_widgetConfig.getDefaultItemHeight();
	Vec2i size = Vec2i(dh * 8, dh * 11);
	Vec2i pos = Vec2i(g_metrics.getScreenDims() - size) / 2;
	setPos(pos);
	setSize(size);

	buildButton(this, m_btnStrip, 6, g_lang.get("ExitProgram"), &GameMenu::onExit);
	buildButton(this, m_btnStrip, 5, g_lang.get("QuitGame"), &GameMenu::onQuit);
	buildButton(this, m_btnStrip, 4, g_lang.get("SaveGame"), &GameMenu::onSaveGame);
	m_pinWidgetsBtn = buildButton(this, m_btnStrip, 3, 
		g_config.getUiPinWidgets() ? g_lang.get("UnPinWidgets") : g_lang.get("PinWidgets"),
		&GameMenu::onPinWidgets);
	buildButton(this, m_btnStrip, 2, g_lang.get("TogglePhotoMode"), &GameMenu::onTogglePhotoMode);
	//buildButton(this, m_btnStrip, 2, g_lang.get("ToggleDebug"), &GameMenu::onDebugToggle);
	buildButton(this, m_btnStrip, 1, g_lang.get("Options"), &GameMenu::onOptions);
	buildButton(this, m_btnStrip, 0, g_lang.get("ReturnToGame"), &GameMenu::onReturnToGame);
}

//GameMenu* GameMenu::showDialog(Vec2i pos, Vec2i size) {
//	GameMenu* menu = new GameMenu(pos, size);
//	g_widgetWindow.setFloatingWidget(menu, true);
//	return menu;
//}

void GameMenu::onPinWidgets(Widget*) {
	g_gameState.togglePinWidgets(0);
	m_pinWidgetsBtn->setText(g_config.getUiPinWidgets() ? g_lang.get("UnPinWidgets") : g_lang.get("PinWidgets"));
}

void GameMenu::onReturnToGame(Widget*) {
	g_gameState.toggleGameMenu();
}

void GameMenu::onOptions(Widget*) {
	g_gameState.toggleOptions();
}

//void GameMenu::onDebugToggle(Widget*) {
//	g_gameState.toggleDebug();
//}

void GameMenu::onTogglePhotoMode(Widget*) {
	g_config.setUiPhotoMode(!g_config.getUiPhotoMode());
}

void GameMenu::onSaveGame(Widget*) {
	g_gameState.toggleGameMenu();
	g_gameState.doSaveBox();
}

void GameMenu::onQuit(Widget*) {
	g_gameState.toggleGameMenu();
	g_gameState.confirmQuitGame();
}

void GameMenu::onExit(Widget*) {
	g_gameState.toggleGameMenu();
	g_gameState.confirmExitProgram();
}

}}

