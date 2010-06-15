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

#ifndef _GLEST_GAME_MENUSTATEROOT_H_
#define _GLEST_GAME_MENUSTATEROOT_H_

#include "main_menu.h"

namespace Glest { namespace Menu {

// ===============================
// 	class MenuStateRoot  
// ===============================

STRINGY_ENUM( RootMenuItem,
	NEWGAME,
	JOINGAME,
	SCENARIO,
	LOADGAME,
	OPTIONS,
	ABOUT,
	EXIT
);

class MenuStateRoot: public MenuState {
private:
	Widgets::Button *buttons[RootMenuItem::COUNT];
	RootMenuItem selectedItem;

private:
	MenuStateRoot(const MenuStateRoot &);
	const MenuStateRoot &operator =(const MenuStateRoot &);

public:
	MenuStateRoot(Program &program, MainMenu *mainMenu);

	// MenuState::update()
	void update();

	// Event handler
	void onButtonClick(Widgets::Button::Ptr);
	void onTextChanged(Widgets::TextBox::Ptr);
	void onListBoxChanged(Widgets::ListBase::Ptr);

	void onComboBoxExpanded(Widgets::DropList::Ptr);
	void onComboBoxCollapsed(Widgets::DropList::Ptr);

	MenuStates getIndex() const { return MenuStates::ROOT; }
};

}}//end namespace

#endif
