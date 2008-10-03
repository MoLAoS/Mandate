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

#ifndef _GLEST_GAME_MENUSTATENEWGAME_H_
#define _GLEST_GAME_MENUSTATENEWGAME_H_

#include "menu_state_start_game_base.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateNewGame
// ===============================

class MenuStateNewGame: public MenuStateStartGameBase {
private:
	//GraphicButton buttonReturn;
	//GraphicButton buttonPlayNow;
	GraphicLabel labelNetwork;
	GraphicLabel labelControl;
	GraphicLabel labelFaction;
	GraphicLabel labelTeam;
	GraphicLabel labelMap;
	GraphicLabel labelTechTree;
	GraphicLabel labelTileset;
	GraphicLabel labelMapInfo;
	GraphicListBox listBoxMap;
	GraphicListBox listBoxTechTree;
	GraphicListBox listBoxTileset;
	vector<string> mapFiles;
	vector<string> techTreeFiles;
	vector<string> tilesetFiles;
	vector<string> factionFiles;
	//GraphicLabel labelPlayers[GameConstants::maxPlayers];
	GraphicListBox listBoxControls[GameConstants::maxPlayers];
	GraphicListBox listBoxFactions[GameConstants::maxPlayers];
	GraphicListBox listBoxTeams[GameConstants::maxPlayers];
	//GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	//MapInfo mapInfo;
	GraphicLabel labelRandomize;
	GraphicListBox listBoxRandomize;
	//GraphicMessageBox *msgBox;


public:
	MenuStateNewGame(Program *program, MainMenu *mainMenu, bool openNetworkSlots = false);

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();

private:
    void loadGameSettings(GameSettings *gameSettings);
	//void loadMapInfo(string file, MapInfo *mapInfo);
	void reloadFactions();
	void updateControlers();
	bool isUnconnectedSlots();
	void updateNetworkSlots();
};

}}//end namespace

#endif
