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

#ifndef _GLEST_GAME_MENUSTATEOPTIONS_H_
#define _GLEST_GAME_MENUSTATEOPTIONS_H_

#include "main_menu.h"

namespace Glest { namespace Gui {
	class Options;
}}

namespace Glest { namespace Menu {

// ===============================
// 	class MenuStateOptions  
// ===============================

class MenuStateOptions: public MenuState {
private:
	WRAPPED_ENUM( Transition, RETURN, GL_INFO, RE_LOAD );

private:
	Transition m_transitionTarget;

	Button		*m_returnButton,
				*m_autoConfigButton,
				*m_openGlInfoButton;

	Options		*m_options;

private:
	MenuStateOptions(const MenuStateOptions &);
	const MenuStateOptions &operator =(const MenuStateOptions &);

public:
	MenuStateOptions(Program &program, MainMenu *mainMenu);

	virtual void update() override;

	MenuStates getIndex() const { return MenuStates::OPTIONS; }
	void reload();

private:
	void saveConfig();
	void onButtonClick(Widget *source);
};

}}//end namespace

#endif
