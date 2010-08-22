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

#include "compound_widgets.h"

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
	Transition				m_targetTransition;
	int						m_humanSlot;

	Button::Ptr				m_returnButton,
							m_playNow;

	StaticText::Ptr			m_controlLabel,
							m_factionLabel,
							m_teamLabel;

	PlayerSlotWidget::Ptr	m_playerSlots[GameConstants::maxPlayers];

	StaticText::Ptr			m_mapLabel,
							m_mapInfoLabel;

	DropList::Ptr			m_mapList;

	StaticText::Ptr			m_techTreeLabel,
							m_tilesetLabel;

	DropList::Ptr			m_techTreeList,
							m_tilesetList;

	StaticText::Ptr			m_fogOfWarLabel;
	CheckBox::Ptr			m_fogOfWarCheckbox;

	StaticText::Ptr			m_randomLocsLabel;
	CheckBox::Ptr			m_randomLocsCheckbox;

	MessageDialog::Ptr		m_messageDialog;

	vector<string> m_mapFiles;
	vector<string> m_techTreeFiles;
	vector<string> m_tilesetFiles;
	vector<string> m_factionFiles;

	MapInfo m_mapInfo;

	AnnouncerThread m_announcer;

public:
	MenuStateNewGame(Program &program, MainMenu *mainMenu, bool openNetworkSlots = false);
	~MenuStateNewGame() {
		m_announcer.stop();
		m_announcer.join();
	}

	void update();

	MenuStates getIndex() const { return MenuStates::NEW_GAME; }

private:
	void reloadFactions();
	void updateControlers();
	void updateNetworkSlots();

	bool hasUnconnectedSlots();
	bool hasNetworkSlots();

	void onChangeFaction(PlayerSlotWidget::Ptr);
	void onChangeControl(PlayerSlotWidget::Ptr);
	void onChangeTeam(PlayerSlotWidget::Ptr);
	void onChangeColour(PlayerSlotWidget::Ptr);

	void onChangeMap(ListBase::Ptr);
	void onChangeTileset(ListBase::Ptr);
	void onChangeTechtree(ListBase::Ptr);

	void onCheckChanged(Button::Ptr);

	void onButtonClick(Button::Ptr ptr);
	void onDismissDialog(BasicDialog::Ptr);
};

}}//end namespace

#endif
