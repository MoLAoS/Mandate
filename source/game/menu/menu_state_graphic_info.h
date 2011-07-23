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

#ifndef _GLEST_GAME_MENUSTATEGRAPHICINFO_H_
#define _GLEST_GAME_MENUSTATEGRAPHICINFO_H_

#include "main_menu.h"

namespace Glest { namespace Menu {

// ===============================
// 	class MenuStateGraphicInfo  
// ===============================

class MenuStateGraphicInfo: public MenuState {
public:
	MenuStateGraphicInfo(Program &program, MainMenu *mainMenu);

	void onButtonClick(Widget*);

	virtual void update() override;

	MenuStates getIndex() const { return MenuStates::GFX_INFO; }

};

}}//end namespace

#endif
