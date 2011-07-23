// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Jaagup Repän <jrepan@gmail.com>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//				  2010 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATELOADGAME_H_
#define _GLEST_GAME_MENUSTATELOADGAME_H_

#include "main_menu.h"
#include "thread.h"
#include "framed_widgets.h"

namespace Glest { namespace Menu {

using namespace Widgets;

class MenuStateLoadGame;

// ===============================
// 	class SavedGamePreviewLoader
// ===============================

class SavedGamePreviewLoader : public Thread {
	MenuStateLoadGame &menu;
	string *fileName;		//only modify with mutex locked
	bool running;
	Mutex mutex;

public:
	SavedGamePreviewLoader(MenuStateLoadGame &menu)
			: menu(menu), fileName(0), running(true) {
		start();
	}

	virtual void execute();

	void goAway() {
		MutexLock lock(mutex);
		running = false;
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

class MenuStateLoadGame: public MenuState {
private:
	WRAPPED_ENUM( Transition, RETURN, PLAY );

private:
	SavedGamePreviewLoader loaderThread;

	Transition		 m_targetTransition;
	Button			*m_returnButton,
					*m_deleteButton,
					*m_playNowButton;
	DropList		*m_savedGameList;
	MessageDialog	*m_messageDialog;

	Mutex mutex;
	// only modify with mutex locked ==>
	ScrollText*	m_infoLabel;
	string fileName;
	const XmlNode *savedGame;
	GameSettings *gs;
	// <== only modify with mutex locked

	vector<string> fileNames;
	vector<string> prettyNames;

private:
	MenuStateLoadGame(const MenuStateLoadGame &);
	const MenuStateLoadGame &operator =(const MenuStateLoadGame &);

	void onButtonClick(Widget*);
	void onSaveSelected(Widget*);

	void onConfirmDelete(Widget*);
	void onCancelDelete(Widget*);

	void onConfirmReturn(Widget*);

public:
	MenuStateLoadGame(Program &program, MainMenu *mainMenu);
	~MenuStateLoadGame();

	virtual void update() override;

	/** returns the name of the new file name if the user moved along since the load started. */
	string *setGameInfo(const string &fileName, const XmlNode *root, const string &err);

	MenuStates getIndex() const { return MenuStates::LOAD_GAME; }

private:
	bool loadGameList();
	void loadGame();
	string getFileName();
	void selectionChanged();
	void initGameInfo();
	void updateNetworkSlots();
};


}}//end namespace

#endif
