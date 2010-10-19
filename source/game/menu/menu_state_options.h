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
#include "compound_widgets.h"

namespace Glest { namespace Menu {
using namespace Widgets;

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

	DropList	*m_langList,
				*m_shadowsList,
				*m_filterList,
				*m_lightsList;
							
	CheckBox	*m_3dTexCheckBox;
	
	Slider		*m_volFxSlider,
				*m_volAmbientSlider,
				*m_volMusicSlider;

	map<string,string> langMap;

private:
	MenuStateOptions(const MenuStateOptions &);
	const MenuStateOptions &operator =(const MenuStateOptions &);

public:
	MenuStateOptions(Program &program, MainMenu *mainMenu);

	void update();

	MenuStates getIndex() const { return MenuStates::OPTIONS; }

private:
	void saveConfig();
	void setupListBoxLang();
	void initLabels();
	void initListBoxes();
	void setTexts();

	void onButtonClick(Button* btn);
	void on3dTexturesToggle(Button* cb);
	void onSliderValueChanged(Slider* slider);
	void onDropListSelectionChanged(ListBase* list);
};

}}//end namespace

#endif
