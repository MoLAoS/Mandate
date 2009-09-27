// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATESTARTGAMEBASE_H_
#define _GLEST_GAME_MENUSTATESTARTGAMEBASE_H_

#include "main_menu.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateNewGame
// ===============================

//TODO: Cleanup: Too much commonality between new game and loag game menus.
//Consolidate like functions
class MenuStateStartGameBase: public MenuState {
protected:
	GraphicButton buttonReturn;
	GraphicButton buttonPlayNow;

	GraphicLabel labelPlayers[GameConstants::maxPlayers];
	GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	MapInfo mapInfo;
	GraphicMessageBox *msgBox;

public:
	MenuStateStartGameBase(Program &program, MainMenu *mainMenu, const string &stateName);

	//void mouseClick(int x, int y, MouseButton mouseButton);
	//void mouseMove(int x, int y, const MouseState &mouseState);
	//void render();
	//void update();

protected:
//	void loadGameSettings(GameSettings *gameSettings);
	void loadMapInfo(string file, MapInfo *mapInfo);
//	void reloadFactions();
//	void updateControlers();

//	bool isUnconnectedSlots();
//	void updateNetworkSlots();
};

}}//end namespace

#endif
