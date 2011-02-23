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
		: Frame(&g_widgetWindow) {
	init(pos, size, g_lang.get("GameMenu"));

	static const int numItems = 6;

	Vec2i room = Vec2i(size.x - getBordersHoriz(), size.y - getBordersVert() - m_titleBar->getHeight());
	Vec2i btnSize(room.x - 20, 30);
	int btnGap = (room.y - 30 * numItems) / (numItems + 1);
	Vec2i btnPos(10, btnGap);

	Font *font = g_widgetConfig.getMenuFont()[FontSize::NORMAL];

	Button* btn = new Button(this, btnPos, btnSize);
	btn->setTextParams(g_lang.get("ExitProgram"), Vec4f(1.f), font);
	btn->Clicked.connect(this, &GameMenu::onExit);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setTextParams(g_lang.get("QuitGame"), Vec4f(1.f), font);
	btn->Clicked.connect(this, &GameMenu::onQuit);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setTextParams(g_lang.get("SaveGame"), Vec4f(1.f), font);
	btn->Clicked.connect(this, &GameMenu::onSaveGame);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setTextParams(g_lang.get("TogglePhotoMode"), Vec4f(1.f), font);
	btn->Clicked.connect(this, &GameMenu::onTogglePhotoMode);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setTextParams(g_lang.get("ToggleDebug"), Vec4f(1.f), font);
	btn->Clicked.connect(this, &GameMenu::onDebugToggle);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setTextParams(g_lang.get("ReturnToGame"), Vec4f(1.f), font);
	btn->Clicked.connect(this, &GameMenu::onReturnToGame);
}

GameMenu* GameMenu::showDialog(Vec2i pos, Vec2i size) {
	GameMenu* menu = new GameMenu(pos, size);
	g_widgetWindow.setFloatingWidget(menu, true);
	return menu;
}

void GameMenu::onReturnToGame(Button*) {
	g_gameState.destroyDialog();
}

void GameMenu::onDebugToggle(Button*) {
	g_config.setMiscDebugMode(!g_config.getMiscDebugMode());
}

void GameMenu::onTogglePhotoMode(Button*) {
	g_config.setUiPhotoMode(!g_config.getUiPhotoMode());
}

void GameMenu::onSaveGame(Button*) {
	g_gameState.destroyDialog();
	g_gameState.doSaveBox();
}

void GameMenu::onQuit(Button*) {
	g_gameState.destroyDialog();
	g_gameState.confirmQuitGame();
}

void GameMenu::onExit(Button*) {
	g_gameState.destroyDialog();
	g_gameState.confirmExitProgram();
}

}}

