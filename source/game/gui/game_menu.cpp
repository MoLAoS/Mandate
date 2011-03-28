// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
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

GameMenu::GameMenu(Vec2i pos, Vec2i size)
		: Frame(&g_widgetWindow, ButtonFlags::CLOSE | ButtonFlags::ROLL_UPDOWN) {
	setPos(pos);
	setSize(size);
	setTitleText(g_lang.get("GameMenu"));

	static const int numItems = 6;

	Vec2i room = Vec2i(size.x - getBordersHoriz(), size.y - getBordersVert() - m_titleBar->getHeight());
	Vec2i btnSize(room.x - 20, 30);
	int btnGap = (room.y - 30 * numItems) / (numItems + 1);
	Vec2i btnPos(10, btnGap);

	Button* btn = new Button(this, btnPos, btnSize);
	btn->setText(g_lang.get("ExitProgram"));
	btn->Clicked.connect(this, &GameMenu::onExit);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setText(g_lang.get("QuitGame"));
	btn->Clicked.connect(this, &GameMenu::onQuit);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setText(g_lang.get("SaveGame"));
	btn->Clicked.connect(this, &GameMenu::onSaveGame);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setText(g_lang.get("TogglePhotoMode"));
	btn->Clicked.connect(this, &GameMenu::onTogglePhotoMode);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setText(g_lang.get("ToggleDebug"));
	btn->Clicked.connect(this, &GameMenu::onDebugToggle);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setText(g_lang.get("ReturnToGame"));
	btn->Clicked.connect(this, &GameMenu::onReturnToGame);
}

GameMenu* GameMenu::showDialog(Vec2i pos, Vec2i size) {
	GameMenu* menu = new GameMenu(pos, size);
	g_widgetWindow.setFloatingWidget(menu, true);
	return menu;
}

void GameMenu::onReturnToGame(Widget*) {
	g_gameState.destroyDialog();
}

void GameMenu::onDebugToggle(Widget*) {
	g_config.setMiscDebugMode(!g_config.getMiscDebugMode());
}

void GameMenu::onTogglePhotoMode(Widget*) {
	g_config.setUiPhotoMode(!g_config.getUiPhotoMode());
}

void GameMenu::onSaveGame(Widget*) {
	g_gameState.destroyDialog();
	g_gameState.doSaveBox();
}

void GameMenu::onQuit(Widget*) {
	g_gameState.destroyDialog();
	g_gameState.confirmQuitGame();
}

void GameMenu::onExit(Widget*) {
	g_gameState.destroyDialog();
	g_gameState.confirmExitProgram();
}

}}

