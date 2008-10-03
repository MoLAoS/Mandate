// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Jaagup Repän <jrepan@gmail.com>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATELOADGAME_H_
#define _GLEST_GAME_MENUSTATELOADGAME_H_

#include "menu_state_start_game_base.h"
#include "thread.h"

namespace Glest{ namespace Game{

class MenuStateLoadGame;

// ===============================
// 	class SavedGamePreviewLoader
// ===============================

class SavedGamePreviewLoader : public Thread {
	MenuStateLoadGame &menu;
	string *fileName;		//only modify with mutex locked
	bool seeJaneRun;
	Mutex mutex;

public:
	SavedGamePreviewLoader(MenuStateLoadGame &menu) : menu(menu) {
		fileName = NULL;
		seeJaneRun = true;
		start();
	}

	virtual void execute();

	void goAway() {
		MutexLock lock(mutex);
		seeJaneRun = false;
	}

	void setFileName(const string &fileName) {
		MutexLock lock(mutex);
		if(!this->fileName) {
			this->fileName = new string(fileName);
		}
	}

private:
	void loadPreview(string *fileName);
};


// ===============================
// 	class MenuStateLoadGame
// ===============================

class MenuStateLoadGame: public MenuStateStartGameBase {
private:
	SavedGamePreviewLoader loaderThread;
	//GraphicButton buttonReturn;
	GraphicButton buttonDelete;
	//GraphicButton buttonPlayNow;

	// only modify with mutex locked ==>
	GraphicLabel labelInfoHeader;
	GraphicLabel labelPlayers[GameConstants::maxPlayers];
	GraphicLabel labelControls[GameConstants::maxPlayers];
	GraphicLabel labelFactions[GameConstants::maxPlayers];
	GraphicLabel labelTeams[GameConstants::maxPlayers];
	//GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	ControlType controlTypes[GameConstants::maxPlayers];
	string fileName;
	const XmlNode *savedGame;
	GameSettings *gs;
	// <== only modify with mutex locked

	GraphicListBox listBoxGames;
	GraphicLabel labelNetwork;

	GraphicMessageBox *confirmMessageBox;
	//GraphicMessageBox *msgBox;
	bool criticalError;
	Mutex mutex;
	vector<string> fileNames;
	vector<string> prettyNames;

public:
	MenuStateLoadGame(Program *program, MainMenu *mainMenu);
	~MenuStateLoadGame();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();

	/** returns the name of the new file name if the user moved along since the load started. */
	string *setGameInfo(const string &fileName, const XmlNode *root, const string &err);

private:
	bool loadGameList();
	bool loadGame();
	string getFileName();
	void selectionChanged();
	void initGameInfo();
	void updateNetworkSlots();
};


}}//end namespace

#endif
