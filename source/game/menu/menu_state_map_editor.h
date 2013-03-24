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

#ifndef _GLEST_GAME_MENUSTATEMAPEDITOR_H_
#define _GLEST_GAME_MENUSTATEMAPEDITOR_H_

#include "main_menu.h"
#include "sim_interface.h"
#include "thread.h"

#include "player_slot_widget.h"

namespace Glest { namespace Menu {
using namespace Widgets;

// ===============================
// 	class MenuStateMapEditor
// ===============================

class MenuStateMapEditor: public MenuState {
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
						*m_tilesetLabel;
	DropList			*m_techTreeList,
						*m_tilesetList;
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
	vector<string>		 m_factionFiles;
	MapInfo				 m_mapInfo;
	float				 m_origMusicVolume;
	bool				 m_fadeMusicOut;

public:
	MenuStateMapEditor(Program &program, MainMenu *mainMenu, bool openNetworkSlots = false);
	~MenuStateMapEditor();

	virtual void update() override;

	MenuStates getIndex() const { return MenuStates::MAP_EDITOR; }

private:
	bool getMapList();
	bool loadGameSettings();
	void reloadFactions(bool setStagger);
	void updateControlers();
	void updateNetworkSlots();

	bool hasUnconnectedSlots();
	bool hasNetworkSlots();

	void randomiseStartLocs();
	//void randomiseMap();
	//void randomiseTileset();

    int getSlotIndex(PlayerSlotWidget* psw, PlayerSlotWidget* *slots);
    int getLowestFreeColourIndex(PlayerSlotWidget* *slots);

	void onChangeFaction(Widget*);
	void onChangeControl(Widget*);
	void onChangeTeam(Widget*);
	void onChangeColour(Widget*);

	void onChangeMap(Widget*);
	void onChangeTileset(Widget*);
	void onChangeTechtree(Widget*);

	void onCheckChanged(Widget*);

	void onButtonClick(Widget*);
	void onDismissDialog(Widget*);
	void onCloseUnconnectedSlots(Widget*);
};

}}//end namespace

#endif
