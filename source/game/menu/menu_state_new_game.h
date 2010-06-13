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

#include "main_menu.h"
#include "sim_interface.h"

#include "compound_widgets.h"

namespace Glest { namespace Menu {
using namespace Widgets;

WRAPPED_ENUM( NewGameTransition, RETURN, PLAY );

// ===============================
// 	class MenuStateNewGame
// ===============================

class MenuStateNewGame: public MenuState/*StartGameBase*/, public sigslot::has_slots {
private:
	NewGameTransition targetTransition;
	int humanSlot;

	Button::Ptr btnReturn, btnPlayNow;

	StaticText::Ptr stControl, stFaction, stTeam;
	PlayerSlotWidget::Ptr psWidgets[GameConstants::maxPlayers];

	StaticText::Ptr stMap, stMapInfo;
	DropList::Ptr dlMaps;

	StaticText::Ptr stTechtree, stTileset;
	DropList::Ptr dlTechtree, dlTileset;

	StaticText::Ptr stFogOfWar;
	CheckBox::Ptr cbFogOfWar;

	StaticText::Ptr stRandomLocs;
	CheckBox::Ptr cbRandomLocs;

	vector<string> mapFiles;
	vector<string> techTreeFiles;
	vector<string> tilesetFiles;
	vector<string> factionFiles;

	MapInfo mapInfo;
	GraphicMessageBox *msgBox;

public:
	MenuStateNewGame(Program &program, MainMenu *mainMenu, bool openNetworkSlots = false);

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

	void onChangeMap(ListBase::Ptr);
	void onChangeTileset(ListBase::Ptr);
	void onChangeTechtree(ListBase::Ptr);

	void onButtonClick(Button::Ptr ptr);
};

}}//end namespace

#endif
