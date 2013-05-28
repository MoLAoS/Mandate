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

#ifndef _GLEST_GAME_MENUSTATECHARACTERCREATOR_H_
#define _GLEST_GAME_MENUSTATECHARACTERCREATOR_H_

#include "forward_decs.h"
#include "main_menu.h"
#include "character_creator.h"

namespace Glest { namespace Menu {
using namespace Gui;
// =================================
// 	class MenuStateCharacterCreator
// =================================

class MenuStateCharacterCreator: public MenuState {
private:
	WRAPPED_ENUM( Transition, RETURN, RE_LOAD, SAVE );

private:
	Transition m_transitionTarget;
	Button		        *m_returnButton,
                        *m_saveButton;
	CharacterCreator	*m_characterCreator;

private:
	MenuStateCharacterCreator(const MenuStateCharacterCreator &);
	const MenuStateCharacterCreator &operator =(const MenuStateCharacterCreator &);

public:
	MenuStateCharacterCreator(Program &program, MainMenu *mainMenu);

	virtual void update() override;

	MenuStates getIndex() const { return MenuStates::CHARACTER_CREATOR; }
	void reload();

private:
	void onButtonClick(Widget *source);
};

}}//end namespace

#endif
