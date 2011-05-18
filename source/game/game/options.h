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

#ifndef _GLEST_GAME_OPTIONS_H_
#define _GLEST_GAME_OPTIONS_H_

#include "compound_widgets.h"
#include "slider.h"
#include "sigslot.h"

namespace Glest { namespace Menu {
	class MenuStateOptions;
}}

namespace Glest { namespace Gui {
using namespace Widgets;

// =====================================================
// 	class SpinnerValueBox
// =====================================================

class SpinnerValueBox : public StaticText {
public:
	SpinnerValueBox(Container *parent) : StaticText(parent) {
		setWidgetStyle(WidgetType::TEXT_BOX);
	}
	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOLTIP); }
};

// =====================================================
// 	class Spinner
// =====================================================

class Spinner : public CellStrip,  public sigslot::has_slots {
private:
	SpinnerValueBox  *m_valueBox;
	ScrollBarButton  *m_upButton;
	ScrollBarButton  *m_downButton;
	int               m_minValue;
	int               m_maxValue;
	int               m_increment;
	int               m_value;

	void onButtonFired(Widget *btn);

public:
	Spinner(Container *parent);

	void setRanges(int min, int max) { m_minValue = min; m_maxValue = max; }
	void setIncrement(int inc) { m_increment = inc; }
	void setValue(int val) { 
		m_value = clamp(val, m_minValue, m_maxValue);
		m_valueBox->setText(intToStr(m_value));
	}

	int getValue() const { return m_value; }

	sigslot::signal<Widget*> ValueChanged;
};

// =====================================================
// 	class Options
//
//	Gui for changing options in the menu and game
// =====================================================

class Options : public TabWidget {
private:
	DropList	*m_langList,
				*m_shadowsList,
				*m_filterList,
				*m_lightsList,
				*m_modelShaderList;
							
	CheckBox	*m_3dTexCheckBox,
		        *m_debugModeCheckBox,
				*m_debugKeysCheckBox;
	
	Slider2		*m_volFxSlider,
				*m_volAmbientSlider,
				*m_volMusicSlider;

	Spinner     *m_minCamAltitudeSpinner,
		        *m_maxCamAltitudeSpinner;

	Spinner     *m_minRenderDistSpinner,
		        *m_maxRenderDistSpinner;

	map<string,string>  m_langMap;
	vector<string>      m_modelShaders;

	// can be null, some options are disabled if in game
	Glest::Menu::MenuStateOptions *m_optionsMenu;

public:
	Options(CellStrip *parent, Glest::Menu::MenuStateOptions *optionsMenu);

	void save();
	virtual string descType() const override { return "Options"; }

private:
	void disableWidgets();
	void setupListBoxLang();
	void initLabels();
	void initListBoxes();
	void setTexts();
	void buildOptionsPanel(CellStrip *container, int cell);
	void loadShaderList();

	// Build tabs
	void buildGameTab();
	void buildVideoTab();
	void buildAudioTab();
	void buildControlsTab();
	void buildNetworkTab();
	void buildDebugTab();

	// Event callbacks
	void on3dTexturesToggle(Widget *source);
	void onSliderValueChanged(Widget *source);
	void onSpinnerValueChanged(Widget *source);
	void onDropListSelectionChanged(Widget *source);
	void onPlayerNameChanged(Widget *source);
	void onToggleDebugMode(Widget*);
	void onToggleDebugKeys(Widget*);
	void onToggleShaders(Widget *source);
};

// =====================================================
// 	class OptionsFrame
//
//	Window for in-game options
// =====================================================

class OptionsFrame : public Frame {
private:
	Button		*m_saveButton;
	Options		*m_options;
	CellStrip	*m_optionsPanel;

protected:
	void init();
	void onButtonClicked(Widget*);

public:
	OptionsFrame(WidgetWindow* window);
	OptionsFrame(Container* parent);
	void init(Vec2i pos, Vec2i size, const string &title);

	virtual string descType() const override { return "OptionsFrame"; }
};

}}//end namespace

#endif