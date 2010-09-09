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

namespace Glest { namespace Gui {

using Global::CoreData;

GameMenu::GameMenu(Vec2i pos, Vec2i size)
		: Frame(&g_widgetWindow) {
	init(pos, size, "Game Menu");

	Vec2i room = Vec2i(size.x - getBordersHoriz(), size.y - getBordersVert() - m_titleBar->getHeight());
	Vec2i btnSize(room.x - 20, 30);
	int btnGap = (room.y - 30 * 4) / 5;
	Vec2i btnPos(10, btnGap);

	Button::Ptr btn = new Button(this, btnPos, btnSize);
	btn->setTextParams("Exit Program", Vec4f(1.f), g_coreData.getFTMenuFontNormal());
	btn->Clicked.connect(this, &GameMenu::onExit);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setTextParams("Quit Game", Vec4f(1.f), g_coreData.getFTMenuFontNormal());
	btn->Clicked.connect(this, &GameMenu::onQuit);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setTextParams("Toggle Debug", Vec4f(1.f), g_coreData.getFTMenuFontNormal());
	btn->Clicked.connect(this, &GameMenu::onDebugToggle);

	btnPos.y += btnGap + btnSize.y;
	btn = new Button(this, btnPos, btnSize);
	btn->setTextParams("Return to Game", Vec4f(1.f), g_coreData.getFTMenuFontNormal());
	btn->Clicked.connect(this, &GameMenu::onReturnToGame);
}

GameMenu::Ptr GameMenu::showDialog(Vec2i pos, Vec2i size) {
	GameMenu::Ptr menu = new GameMenu(pos, size);
	g_widgetWindow.setFloatingWidget(menu, true);
	return menu;
}

void GameMenu::onReturnToGame(Button::Ptr) {
	g_gameState.destroyDialog();
}

void GameMenu::onQuit(Button::Ptr) {
	g_gameState.quitGame();
}

void GameMenu::onExit(Button::Ptr) {
	g_program.exit();
}

void GameMenu::onDebugToggle(Button::Ptr) {
	g_config.setMiscDebugMode(!g_config.getMiscDebugMode());
}

}}
