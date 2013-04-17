// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//				  2010 James McCulloch <silnarm at gmail>
//				  2011 Nathan Turner <hailstone3 at sourceforge>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_CHARACTERCREATOR_H_
#define _GLEST_GAME_CHARACTERCREATOR_H_

#include "framed_widgets.h"
#include "slider.h"
#include "sigslot.h"
#include "platform_util.h"

namespace Glest { namespace Menu {
	class MenuStateCharacterCreator;
}}

namespace Glest { namespace Gui {
using namespace Widgets;

// =====================================================
// 	class Character Creator
//
//	Gui for creating a sovereign character
//  in the menu and game
// =====================================================

class CharacterCreator : public TabWidget {
private:
	DropList	*m_langList;

	CheckBox	*m_fullscreenCheckBox;

	Slider2		*m_volFxSlider;

	Spinner     *m_minCamAltitudeSpinner;

	MessageDialog *m_messageDialog;

	map<string,string>  m_langMap;

	// can be null, some options are disabled if in game
	Glest::Menu::MenuStateCharacterCreator *m_characterCreatorMenu;

public:
	CharacterCreator(CellStrip *parent, Glest::Menu::MenuStateCharacterCreator *characterCreatorMenu);
	virtual ~CharacterCreator();

	void save();
	virtual string descType() const override { return "CharacterCreator"; }

private:
	void disableWidgets();
	void setupListBoxLang();

	// Build tabs
	void buildSovereignTab();
	void buildHeroTab();
	void buildLeaderTab();
	void buildMageTab();
	void buildUnitTab();

	// Event callbacks
	void onCheckBoxCahnged(Widget *source);
	void onSliderValueChanged(Widget *source);
	void onSpinnerValueChanged(Widget *source);
	void onDropListSelectionChanged(Widget *source);
	void onSovereignNameChanged(Widget *source);
};

// =====================================================
// 	class CharacterCreatorFrame
//
//	Window for in-game sovereign creation
// =====================================================

class CharacterCreatorFrame : public Frame {
private:
	Button		        *m_saveButton;
	CharacterCreator	*m_characterCreator;
	CellStrip	        *m_characterCreatorPanel;

protected:
	void init();
	void onButtonClicked(Widget*);

public:
	CharacterCreatorFrame(WidgetWindow* window);
	CharacterCreatorFrame(Container* parent);
	void init(Vec2i pos, Vec2i size, const string &title);

	virtual string descType() const override { return "CharacterCreatorFrame"; }
};

}}//end namespace

#endif
