// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//				  2010 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATENEWGAME_H_
#define _GLEST_GAME_MENUSTATENEWGAME_H_

#include "main_menu.h"
#include "sim_interface.h"
#include "thread.h"

#include "player_slot_widget.h"

namespace Glest { namespace Menu {
using namespace Widgets;

/** Periodically announces the game, when slots are free, to the master
  * server or local lan.
  * NOTE: currently only lan */
class AnnouncerThread : public Thread {
	Mutex	m_mutex;
	bool	m_running;
	bool	m_freeSlots;
	ServerSocket m_socket;

public:
	AnnouncerThread()
			: m_freeSlots(false)
			, m_running(true) {
		start();
	}
	virtual void execute();

	void doAnnounce(bool v) {
		MutexLock lock(m_mutex);
		m_freeSlots = v;
	}

	void stop() {
		MutexLock lock(m_mutex);
		m_running = false;
	}
};

// ===============================
// 	class MenuStateNewGame
// ===============================

class MenuStateNewGame: public MenuState {
private:
	WRAPPED_ENUM( Transition, RETURN, PLAY );

private:
	Transition			 m_targetTransition;
	int					 m_humanSlot;
	Button				*m_returnButton,
						*m_playNow;
	PlayerSlotWidget	*m_playerSlots[GameConstants::maxPlayers];
	StaticText			*m_mapLabel,
						*m_mapInfoLabel;
	DropList			*m_mapList;
	StaticText			*m_techTreeLabel,
						*m_tilesetLabel,
						*m_sovereignLabel;
	DropList			*m_techTreeList,
						*m_tilesetList,
						*m_sovereignList;
	StaticText			*m_FOWLabel;
	CheckBox			*m_FOWCheckbox;
	StaticText			*m_SODLabel;
	CheckBox			*m_SODCheckbox;
	StaticText			*m_randomLocsLabel;
	CheckBox			*m_randomLocsCheckbox;
	MessageDialog		*m_messageDialog;
	vector<string>		 m_mapFiles;
	vector<string>		 m_techTreeFiles;
	vector<string>		 m_tilesetFiles;
	vector<string>		 m_sovereignFiles;
	vector<string>		 m_factionFiles;
	MapInfo				 m_mapInfo;
	AnnouncerThread		 m_announcer;
	float				 m_origMusicVolume;
	bool				 m_fadeMusicOut;

public:
	MenuStateNewGame(Program &program, MainMenu *mainMenu, bool openNetworkSlots = false);
	~MenuStateNewGame() {
		m_announcer.stop();
		m_announcer.join();
	}

	virtual void update() override;

	MenuStates getIndex() const { return MenuStates::NEW_GAME; }

private:
	bool getMapList();
	bool loadGameSettings();
	void reloadFactions(bool setStagger);
	void reloadSovereigns();
	void updateControlers();
	void updateNetworkSlots();

	bool hasUnconnectedSlots();
	bool hasNetworkSlots();

	void randomiseStartLocs();
	//void randomiseMap();
	//void randomiseTileset();

	void onChangeFaction(Widget*);
	void onChangeControl(Widget*);
	void onChangeTeam(Widget*);
	void onChangeColour(Widget*);

	void onChangeMap(Widget*);
	void onChangeTileset(Widget*);
	void onChangeTechtree(Widget*);
	void onChangeSovereign(Widget*);

	void onCheckChanged(Widget*);

	void onButtonClick(Widget*);
	void onDismissDialog(Widget*);
	void onCloseUnconnectedSlots(Widget*);
};

}}//end namespace

#endif
