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

#include "framed_widgets.h"
#include "slider.h"
#include "sigslot.h"
#include "platform_util.h"

namespace Glest { namespace Menu {
	class MenuStateOptions;
}}

namespace Glest { namespace Gui {
using namespace Widgets;
using Shared::Platform::VideoMode;

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
				*m_terrainRendererList,
				*m_waterRendererList,
				*m_resolutionList,
				*m_shadowTextureSizeList;

	CheckBox	*m_useShadersCheckBox,
				*m_fullscreenCheckBox,
				*m_autoRepairCheckBox,
				*m_autoReturnCheckBox,
				*m_bumpMappingCheckBox,
				*m_specularMappingCheckBox,
				*m_cameraInvertXAxisCheckBox,
				*m_cameraInvertYAxisCheckBox,
				*m_cameraMoveAtEdgesCheckBox,
				*m_resoureNamesCheckBox,
				*m_focusArrowsCheckBox;
	
	Slider2		*m_volFxSlider,
				*m_volAmbientSlider,
				*m_volMusicSlider;

	Spinner     *m_minCamAltitudeSpinner,
		        *m_maxCamAltitudeSpinner;

	Spinner     *m_minRenderDistSpinner,
		        *m_maxRenderDistSpinner;

	Spinner		*m_consoleMaxLinesSpinner,
				*m_shadowFrameSkipSpinner,
				*m_scrollSpeedSpinner;

	MessageDialog *m_messageDialog;

	map<string,string>  m_langMap;
	vector<string>      m_modelShaders;
	vector<VideoMode>	m_resolutions;

	// can be null, some options are disabled if in game
	Glest::Menu::MenuStateOptions *m_optionsMenu;

public:
	Options(CellStrip *parent, Glest::Menu::MenuStateOptions *optionsMenu);
	virtual ~Options();

	void save();
	virtual string descType() const override { return "Options"; }

private:
	void disableWidgets();
	void setupListBoxLang();
	void syncVideoModeList(VideoMode mode);

	// Build tabs
	void buildGameTab();
	void buildVideoTab();
	void buildAudioTab();
	void buildControlsTab();
	void buildNetworkTab();
	void buildDebugTab();

	// Event callbacks
	void onCheckBoxCahnged(Widget *source);
	void onSliderValueChanged(Widget *source);
	void onSpinnerValueChanged(Widget *source);
	void onDropListSelectionChanged(Widget *source);
	void onPlayerNameChanged(Widget *source);

private:
	bool haveSpecShaders;
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