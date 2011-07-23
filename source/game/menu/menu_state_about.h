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

#ifndef _GLEST_GAME_MENUSTATEABOUT_H_
#define _GLEST_GAME_MENUSTATEABOUT_H_

#include "main_menu.h"

namespace Glest { namespace Menu {
using namespace Widgets;

// ===============================
// 	class MenuStateAbout  
// ===============================

class MenuStateAbout: public MenuState {
private:
	Button* m_returnButton;

public:
	MenuStateAbout(Program &program, MainMenu *mainMenu);

	MenuStates getIndex() const { return MenuStates::ABOUT; }

	void onReturn(Widget*);

	virtual void update() override;
};

}}//end namespace

#endif
