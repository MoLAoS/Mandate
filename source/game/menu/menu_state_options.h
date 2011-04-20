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
#include "slider.h"

namespace Glest { namespace Menu {
using namespace Widgets;

class SpinnerValueBox : public StaticText {
public:
	SpinnerValueBox(Container *parent) : StaticText(parent) {
		setWidgetStyle(WidgetType::TEXT_BOX);
	}
	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOLTIP); }
};

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

	map<string,string>  langMap;
	vector<string>      m_modelShaders;

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
	void buildOptionsPanel(CellStrip *container, int cell);
	void loadShaderList();

	void onButtonClick(Widget *source);
	void on3dTexturesToggle(Widget *source);
	void onSliderValueChanged(Widget *source);
	void onSpinnerValueChanged(Widget *source);
	void onDropListSelectionChanged(Widget *source);
	void onPlayerNameChanged(Widget *source);
	void onToggleDebugMode(Widget*);
	void onToggleDebugKeys(Widget*);
	void onToggleShaders(Widget *source);

};

}}//end namespace

#endif
