
#include "pch.h"
#include "options.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "util.h"
#include "FSFactory.hpp"
#include "menu_state_options.h"
#include "game.h"
#include "keymap_widget.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Glest::Graphics;

namespace Glest { namespace Gui {

// =====================================================
// 	class Options
// =====================================================

Options::Options(CellStrip *parent, MenuStateOptions *optionsMenu)
		: TabWidget(parent)
		, m_optionsMenu(optionsMenu) {
	setSizeHint(0, SizeHint(-1, g_widgetConfig.getDefaultItemHeight()));
	buildOptionsPanel(parent, 0);
	// add each tab
	buildGameTab();
	buildVideoTab();
	buildAudioTab();
	buildControlsTab();
	buildNetworkTab();
	buildDebugTab();

	if (!m_optionsMenu) {
		disableWidgets();
	}
	int page = g_config.getUiLastOptionsPage();
	if (page < 0 || page > 4) {
		page = 0;
	}
	setActivePage(page);
}

Options::~Options() {
	g_config.setUiLastOptionsPage(getActivePage());
}

void Options::buildGameTab() {
	//CellStrip *panel = new CellStrip(this, Orientation::HORIZONTAL, 2);
	//TabWidget::add(g_lang.get("Game"), panel);
}

void Options::buildVideoTab() {
	Lang &lang = Lang::getInstance();
	Config &config= Config::getInstance();

	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));

	CellStrip *panel = new CellStrip(this, Orientation::HORIZONTAL, 2);
	panel->setAnchors(fillAnchors);
	panel->setPos(Vec2i(0,0));
	panel->anchor();
	panel->setSize(Vec2i(g_config.getDisplayWidth(), g_widgetConfig.getDefaultItemHeight()));

	TabWidget::add(lang.get("Video"), panel);
	
	Anchors padAnchors(Anchor(AnchorType::RIGID, 10), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::RIGID, 10), Anchor(AnchorType::RIGID, 0));
	Anchors squashAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::SPRINGY, 15));

	Anchors centreAnchors;
	centreAnchors.setCentre(true, false);

	const int rows = 7;

	// Column 1

	CellStrip *col1 = new CellStrip(panel, Orientation::VERTICAL, Origin::FROM_TOP, rows);
	col1->setCell(0);
	col1->setAnchors(padAnchors);

	OptionWidget *dw = new OptionWidget(col1, lang.get("Resolution"));
	dw->setCell(0);

	m_resolutionList = new DropList(dw);
	m_resolutionList->setCell(1);
	m_resolutionList->setDropBoxHeight(280);
	m_resolutionList->setAnchors(squashAnchors);


	// add the possible resoultions to the list
	vector<VideoMode> modes;
	getPossibleScreenModes(modes);
	for (int i = 0; i < modes.size(); ++i) {
		m_resolutions.push_back(modes[i]);
		m_resolutionList->addItem(videoModeToString(modes[i]));
	}
	VideoMode mode = VideoMode(g_config.getDisplayWidth(), g_config.getDisplayHeight(),
		g_config.getRenderColorBits(), g_config.getDisplayRefreshFrequency());
	syncVideoModeList(mode);
	m_resolutionList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);

	m_fullscreenCheckBox = createStandardCheckBox(col1, 1, lang.get("Fullscreen"));
	m_fullscreenCheckBox->setChecked(!config.getDisplayWindowed());
	m_fullscreenCheckBox->Clicked.connect(this, &Options::onToggleFullscreen);
	if (m_resolutionList->getSelectedIndex() == -1) {
		// current settings do not match an acceptable vid mode, disable
		m_fullscreenCheckBox->setEnabled(false);
	}

	m_bumpMappingCheckBox = createStandardCheckBox(col1, 2, lang.get("BumpMapping"));
	m_bumpMappingCheckBox->setChecked(config.getRenderEnableBumpMapping());
	m_bumpMappingCheckBox->Clicked.connect(this, &Options::onToggleBumpMapping);

	m_specularMappingCheckBox = createStandardCheckBox(col1, 3, lang.get("SpecularMapping"));
	m_specularMappingCheckBox->setChecked(config.getRenderEnableSpecMapping());
	m_specularMappingCheckBox->Clicked.connect(this, &Options::onToggleSpecularMapping);

	dw = new OptionWidget(col1, lang.get("ShadowTextureSize"));
	dw->setCell(4);

	m_shadowTextureSizeList = new DropList(dw);
	m_shadowTextureSizeList->setCell(1);
	m_shadowTextureSizeList->setDropBoxHeight(280);
	m_shadowTextureSizeList->setAnchors(squashAnchors);

	// sizes must be powers of 2 and less than the resolution
	///@todo only add the sizes less than resolution (both width and height values) - hailstone 17July2011
	m_shadowTextureSizeList->addItem("64");
	m_shadowTextureSizeList->addItem("128");
	m_shadowTextureSizeList->addItem("256");
	m_shadowTextureSizeList->addItem("512");
	m_shadowTextureSizeList->addItem("1024");
	m_shadowTextureSizeList->setSelected(toStr(config.getRenderShadowTextureSize()));
	m_shadowTextureSizeList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);

	dw = new OptionWidget(col1, lang.get("ShadowFrameSkip"));
	dw->setCell(5);
	
	m_shadowFrameSkipSpinner = new Spinner(dw);
	m_shadowFrameSkipSpinner->setCell(1);
	m_shadowFrameSkipSpinner->setAnchors(squashAnchors);
	m_shadowFrameSkipSpinner->setRanges(0, 5); ///@todo work out the details - hailstone 17July2011
	m_shadowFrameSkipSpinner->setIncrement(1);
	m_shadowFrameSkipSpinner->setValue(config.getRenderShadowFrameSkip());
	m_shadowFrameSkipSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	// Column 2

	CellStrip *col2 = new CellStrip(panel, Orientation::VERTICAL, Origin::FROM_TOP, rows);
	col2->setCell(1);
	col2->setAnchors(padAnchors);

	// Shadows
	dw = new OptionWidget(col2, lang.get("Shadows"));
	dw->setCell(0);
	m_shadowsList = new DropList(dw);
	m_shadowsList->setCell(1);
	m_shadowsList->setAnchors(squashAnchors);
	for(int i= 0; i < ShadowMode::COUNT; ++i){
		m_shadowsList->addItem(lang.get(Renderer::shadowsToStr(ShadowMode(i))));
	}
	string str= config.getRenderShadows();
	m_shadowsList->setSelected(clamp(int(Renderer::strToShadows(str)), 0, ShadowMode::COUNT - 1));
	m_shadowsList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);

	// Texture filter
	dw = new OptionWidget(col2, lang.get("TextureFilter"));
	dw->setCell(1);
	m_filterList = new DropList(dw);
	m_filterList->setCell(1);
	m_filterList->setAnchors(squashAnchors);
	m_filterList->addItem("Bilinear");
	m_filterList->addItem("Trilinear");
	m_filterList->setSelected(config.getRenderFilter());
	m_filterList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);

	// lights
	DoubleOption *qw = new DoubleOption(col2, lang.get("MaxLights"), lang.get("Textures3D"));
	qw->setCustomSplit(true, 35);
	qw->setCell(2);
	m_lightsList = new DropList(qw);
	m_lightsList->setCell(1);
	m_lightsList->setAnchors(squashAnchors);
	for (int i = 1; i <= 8; ++i) {
		m_lightsList->addItem(intToStr(i));
	}
	m_lightsList->setSelected(clamp(config.getRenderLightsMax()-1, 0, 7));
	m_lightsList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);
	m_lightsList->setDropBoxHeight(200);

	// 3D Textures
	CheckBoxHolder *cbh = new CheckBoxHolder(qw);
	cbh->setCell(3);
	m_3dTexCheckBox = new CheckBox(cbh);
	m_3dTexCheckBox->setCell(1);
	m_3dTexCheckBox->setAnchors(squashAnchors);
	m_3dTexCheckBox->setChecked(config.getRenderTextures3D());
	m_3dTexCheckBox->Clicked.connect(this, &Options::on3dTexturesToggle);

	// render min / max distance
	RelatedDoubleOption *rdo = new RelatedDoubleOption(col2,
		lang.get("RenderDistance"), lang.get("Min"), lang.get("Max"));
	rdo->setCell(3);
	rdo->setCustomSplits(13, 14, 20);
	m_minRenderDistSpinner = new Spinner(rdo);
	m_minRenderDistSpinner->setCell(2);
	m_minRenderDistSpinner->setAnchors(squashAnchors);
	m_minRenderDistSpinner->setRanges(0, 20);
	m_minRenderDistSpinner->setIncrement(1);
	m_minRenderDistSpinner->setValue(int(config.getRenderDistanceMin()));
	m_minRenderDistSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	m_maxRenderDistSpinner = new Spinner(rdo);
	m_maxRenderDistSpinner->setCell(4);
	m_maxRenderDistSpinner->setAnchors(squashAnchors);
	m_maxRenderDistSpinner->setRanges(32, 4096);
	m_maxRenderDistSpinner->setIncrement(32);
	m_maxRenderDistSpinner->setValue(int(config.getRenderDistanceMax()));
	m_maxRenderDistSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	// Field of View
	//dw = new OptionWidget(col2, lang.get("RenderFoV"));
	//dw->setCell(4);

	// Enable Shaders
	dw = new OptionWidget(col2, lang.get("UseShaders"));
	dw->setCell(4);
	cbh = new CheckBoxHolder(dw);
	cbh->setCell(1);
	cbh->setAnchors(fillAnchors);
	CheckBox *checkBox  = new CheckBox(cbh);
	checkBox->setCell(1);
	checkBox->setAnchors(squashAnchors);
	checkBox->setChecked(config.getRenderUseShaders());
	checkBox->Clicked.connect(this, &Options::onToggleShaders);
	if (g_config.getRenderTestingShaders()) {
		checkBox->setEnabled(false);
	}

	// Terrain Shader
	dw = new OptionWidget(col2, lang.get("TerrainShader"));
	dw->setCell(5);
	m_terrainRendererList = new DropList(dw);
	m_terrainRendererList->setCell(1);
	m_terrainRendererList->setAnchors(squashAnchors);
	m_terrainRendererList->addItem("Original Terrain Renderer");
	m_terrainRendererList->addItem("Terrain Renderer 2");
	m_terrainRendererList->setSelected(config.getRenderTerrainRenderer() - 1);
	m_terrainRendererList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);
	
	// Water Shader
	//dw = new OptionWidget(col2, lang.get("WaterShader"));
	//dw->setCell(7);

	// Model Shader
	dw = new OptionWidget(col2, lang.get("ModelShader"));
	dw->setCell(6);

	m_modelShaderList = new DropList(dw);
	m_modelShaderList->setCell(1);
	m_modelShaderList->setAnchors(squashAnchors);
	m_modelShaderList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);

	loadShaderList();

	SizeHint hint(-1, int(g_widgetConfig.getDefaultItemHeight() * 1.5f));
	for (int i=0; i < rows; ++i) {
		col1->setSizeHint(i, hint);
		col2->setSizeHint(i, hint);
	}
}

void Options::syncVideoModeList(VideoMode mode) {
	int currentIndex = -1;
	for (int i = 0; i < m_resolutions.size(); ++i) {
		if (m_resolutions[i] == mode) {
			currentIndex = i;
		}
	}
	m_resolutionList->setSelected(currentIndex);
}

void Options::buildAudioTab() {
	Lang &lang = g_lang;
	Config &config= g_config;

	CellStrip *panel = new CellStrip(this, Orientation::VERTICAL, Origin::FROM_TOP, 3);
	TabWidget::add(lang.get("Audio"), panel);
	SizeHint hint(-1, int(g_widgetConfig.getDefaultItemHeight() * 1.5f));
	panel->setSizeHint(0, hint);
	panel->setSizeHint(1, hint);
	panel->setSizeHint(2, hint);

	Anchors squashAnchors(Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::SPRINGY, 15));

	OptionWidget *ow = new OptionWidget(panel, lang.get("FxVolume"));
	ow->setCell(0);
	ow->setAbsoluteSplit(200, true); // OptionWidget was 'geared' to live in a two column layout...
	m_volFxSlider = new Slider2(ow, false);
	m_volFxSlider->setCell(1);
	m_volFxSlider->setAnchors(squashAnchors);
	m_volFxSlider->setRange(100);
	m_volFxSlider->setValue(clamp(config.getSoundVolumeFx(), 0, 100));
	m_volFxSlider->ValueChanged.connect(this, &Options::onSliderValueChanged);

	ow = new OptionWidget(panel, lang.get("AmbientVolume"));
	ow->setCell(1);
	ow->setAbsoluteSplit(200, true);
	m_volAmbientSlider = new Slider2(ow, false);
	m_volAmbientSlider->setCell(1);
	m_volAmbientSlider->setAnchors(squashAnchors);
	m_volAmbientSlider->setRange(100);
	m_volAmbientSlider->setValue(clamp(config.getSoundVolumeAmbient(), 0, 100));
	m_volAmbientSlider->ValueChanged.connect(this, &Options::onSliderValueChanged);

	ow = new OptionWidget(panel, lang.get("MusicVolume"));
	ow->setCell(2);
	ow->setAbsoluteSplit(200, true);
	m_volMusicSlider = new Slider2(ow, false);
	m_volMusicSlider->setCell(1);
	m_volMusicSlider->setAnchors(squashAnchors);
	m_volMusicSlider->setRange(100);
	m_volMusicSlider->setValue(clamp(config.getSoundVolumeMusic(), 0, 100));
	m_volMusicSlider->ValueChanged.connect(this, &Options::onSliderValueChanged);
}

void Options::buildControlsTab() {
	CellStrip *panel = new CellStrip(this, Orientation::VERTICAL, 2);
	TabWidget::add(g_lang.get("Controls"), panel);

	StaticText *label = new StaticText(panel);
	label->setCell(0);
	label->setAnchors(Anchors::getFillAnchors());
	label->setAlignment(Alignment::FLUSH_LEFT);
	label->setText(g_lang.get("HotKeys"));
	panel->setSizeHint(0, SizeHint(-1, g_widgetConfig.getDefaultItemHeight() * 5 / 4));

	KeymapWidget *keymapWidget = new KeymapWidget(panel);
	keymapWidget->setCell(1);
	keymapWidget->setAnchors(Anchors::getFillAnchors());
}

void Options::buildNetworkTab() {
	//CellStrip *panel = new CellStrip(this, Orientation::HORIZONTAL, 2);
	//TabWidget::add(g_lang.get("Network"), panel);
}

void Options::buildDebugTab() {
	CellStrip *panel = new CellStrip(this, Orientation::VERTICAL, Origin::FROM_TOP, 2);
	TabWidget::add(g_lang.get("Debug"), panel);
	SizeHint hint(-1, int(g_widgetConfig.getDefaultItemHeight() * 1.5f));
	panel->setSizeHint(0, hint);
	panel->setSizeHint(1, SizeHint());

	// centre check-boxes vetically in cell
	Anchors centreAnchors;
	centreAnchors.setCentre(true, false);

	// using centering anchors, must size widgets ourselves
	Vec2i cbSize(g_widgetConfig.getDefaultItemHeight());

	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();

	// Debug mode/keys container
	DoubleOption *qw = new DoubleOption(panel, lang.get("DebugMode"), lang.get("DebugKeys"));
	qw->setCell(0);
	qw->setCustomSplit(true, 30);  // DoubleOption was 'geared' to live in a two column layout...
	qw->setCustomSplit(false, 30);

	// Debug mode
	m_debugModeCheckBox = new CheckBox(qw);
	m_debugModeCheckBox->setCell(1);
	m_debugModeCheckBox->setSize(cbSize);
	m_debugModeCheckBox->setAnchors(centreAnchors);
	m_debugModeCheckBox->setChecked(config.getMiscDebugMode());
	m_debugModeCheckBox->Clicked.connect(this, &Options::onToggleDebugMode);

	// Debug keys
	m_debugKeysCheckBox = new CheckBox(qw);
	m_debugKeysCheckBox->setCell(3);
	m_debugKeysCheckBox->setSize(cbSize);
	m_debugKeysCheckBox->setAnchors(centreAnchors);
	m_debugKeysCheckBox->setChecked(config.getMiscDebugKeys());
	m_debugKeysCheckBox->Clicked.connect(this, &Options::onToggleDebugKeys);

	//panel->borderStyle().setSolid(g_widgetConfig.getColourIndex(Vec3f(1.f, 0.f, 1.f)));
	//panel->borderStyle().setSizes(2);

	DebugOptions *dbgOptions = new DebugOptions(panel, m_optionsMenu != 0);
	dbgOptions->setCell(1);
	dbgOptions->setAnchors(Anchors::getFillAnchors());
}

void Options::disableWidgets() {
	// disable all the widgets until they're known to be ready for ingame - hailstone 14May2011
	m_langList->setEnabled(false);
	m_shadowsList->setEnabled(false);
	m_filterList->setEnabled(false);
	m_lightsList->setEnabled(false);
	m_modelShaderList->setEnabled(false);

	m_3dTexCheckBox->setEnabled(false);
	m_debugModeCheckBox->setEnabled(false);
	m_debugKeysCheckBox->setEnabled(false);
	
	//m_volFxSlider->setEnabled(false);
	//m_volAmbientSlider->setEnabled(false);
	//m_volMusicSlider->setEnabled(false);

	m_minCamAltitudeSpinner->setEnabled(false);
	m_maxCamAltitudeSpinner->setEnabled(false);

	m_minRenderDistSpinner->setEnabled(false);
	m_maxRenderDistSpinner->setEnabled(false);
}

void Options::buildOptionsPanel(CellStrip *container, int cell) {
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();

	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));
	Anchors padAnchors(Anchor(AnchorType::RIGID, 10), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::RIGID, 10), Anchor(AnchorType::RIGID, 0));

	Anchors centreAnchors;
	centreAnchors.setCentre(true, false);

	CellStrip *pnl = new CellStrip(this, Orientation::HORIZONTAL, 2);
	pnl->setAnchors(fillAnchors);
	pnl->setPos(Vec2i(0,0));
	pnl->anchor();
	pnl->setSize(Vec2i(g_config.getDisplayWidth(), g_widgetConfig.getDefaultItemHeight()));
	//pnl->borderStyle().setSolid(g_widgetConfig.getColourIndex(Vec3f(1.f, 0.f, 1.f)));
	//pnl->borderStyle().setSizes(2);
	TabWidget::add(g_lang.get("Game"), pnl);
	//borderStyle().setSolid(g_widgetConfig.getColourIndex(Vec3f(1.f, 0.f, 1.f)));
	//borderStyle().setSizes(2);

	const int rows = 10;

	CellStrip *col1 = new CellStrip(pnl, Orientation::VERTICAL, Origin::FROM_TOP, rows);
	col1->setCell(0);
	col1->setAnchors(padAnchors);

	CellStrip *col2 = new CellStrip(pnl, Orientation::VERTICAL, Origin::FROM_TOP, rows);
	col2->setCell(1);
	col2->setAnchors(padAnchors);

	SizeHint hint(-1, int(g_widgetConfig.getDefaultItemHeight() * 1.5f));
	for (int i=0; i < rows; ++i) {
		col1->setSizeHint(i, hint);
		col2->setSizeHint(i, hint);
	}

	Anchors squashAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::SPRINGY, 15));

	// Player Name
	OptionWidget *dw = new OptionWidget(col1, lang.get("PlayerName"));
	dw->setCell(0);
	TextBox *tb = new TextBox(dw);
	tb->setText(g_config.getNetPlayerName());
	tb->setCell(1);
	tb->setAnchors(squashAnchors);
	tb->TextChanged.connect(this, &Options::onPlayerNameChanged);

	// Language
	dw = new OptionWidget(col1, lang.get("Language"));
	dw->setCell(1);
	m_langList = new DropList(dw);
	m_langList->setCell(1);
	m_langList->setAnchors(squashAnchors);
	setupListBoxLang();
	m_langList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);
	m_langList->setDropBoxHeight(200);

	// Camera min / max altitude
	RelatedDoubleOption *rdo = new RelatedDoubleOption(col1,
		lang.get("CameraAltitude"), lang.get("Min"), lang.get("Max"));
	rdo->setCell(2);
	rdo->setCustomSplits(13, 14, 20);
	m_minCamAltitudeSpinner = new Spinner(rdo);
	m_minCamAltitudeSpinner->setCell(2);
	m_minCamAltitudeSpinner->setAnchors(squashAnchors);
	m_minCamAltitudeSpinner->setRanges(0, 20);
	m_minCamAltitudeSpinner->setIncrement(1);
	m_minCamAltitudeSpinner->setValue(int(config.getCameraMinDistance()));
	m_minCamAltitudeSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	m_maxCamAltitudeSpinner = new Spinner(rdo);
	m_maxCamAltitudeSpinner->setCell(4);
	m_maxCamAltitudeSpinner->setAnchors(squashAnchors);
	m_maxCamAltitudeSpinner->setRanges(32, 2048);
	m_maxCamAltitudeSpinner->setIncrement(32);
	m_maxCamAltitudeSpinner->setValue(int(config.getCameraMaxDistance()));
	m_maxCamAltitudeSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	m_cameraInvertXAxisCheckBox = createStandardCheckBox(col1, 3, lang.get("CameraInvertXAxis"));
	m_cameraInvertXAxisCheckBox->setChecked(config.getCameraInvertXAxis());
	m_cameraInvertXAxisCheckBox->Clicked.connect(this, &Options::onToggleCameraInvertXAxis);
	
	m_cameraInvertYAxisCheckBox = createStandardCheckBox(col1, 4, lang.get("CameraInvertYAxis"));
	m_cameraInvertYAxisCheckBox->setChecked(config.getCameraInvertYAxis());
	m_cameraInvertYAxisCheckBox->Clicked.connect(this, &Options::onToggleCameraInvertYAxis);

	// Column 2
	m_autoRepairCheckBox = createStandardCheckBox(col2, 0, lang.get("AutoRepair"));
	m_autoRepairCheckBox->setChecked(config.getGsAutoRepairEnabled());
	m_autoRepairCheckBox->Clicked.connect(this, &Options::onToggleAutoRepair);

	m_autoReturnCheckBox = createStandardCheckBox(col2, 1, lang.get("AutoReturn"));
	m_autoReturnCheckBox->setChecked(config.getGsAutoReturnEnabled());
	m_autoReturnCheckBox->Clicked.connect(this, &Options::onToggleAutoReturn);

	m_focusArrowsCheckBox = createStandardCheckBox(col2, 2, lang.get("FocusArrows"));
	m_focusArrowsCheckBox->setChecked(config.getUiFocusArrows());
	m_focusArrowsCheckBox->Clicked.connect(this, &Options::onToggleFocusArrows);

	dw = new OptionWidget(col2, lang.get("ConsoleMaxLines"));
	dw->setCell(3);
	
	m_consoleMaxLinesSpinner = new Spinner(dw);
	m_consoleMaxLinesSpinner->setCell(1);
	m_consoleMaxLinesSpinner->setAnchors(squashAnchors);
	m_consoleMaxLinesSpinner->setRanges(1, 100);
	m_consoleMaxLinesSpinner->setIncrement(1);
	m_consoleMaxLinesSpinner->setValue(config.getUiConsoleMaxLines());
	m_consoleMaxLinesSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	dw = new OptionWidget(col2, lang.get("ScrollSpeed"));
	dw->setCell(4);
	
	m_scrollSpeedSpinner = new Spinner(dw);
	m_scrollSpeedSpinner->setCell(1);
	m_scrollSpeedSpinner->setAnchors(squashAnchors);
	m_scrollSpeedSpinner->setRanges(1, 100);
	m_scrollSpeedSpinner->setIncrement(1);
	m_scrollSpeedSpinner->setValue((int)config.getUiScrollSpeed());
	m_scrollSpeedSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);
}

void Options::loadShaderList() {
	m_modelShaderList->clearItems();
	if (g_config.getRenderUseShaders() && !g_config.getRenderTestingShaders()) {
		findAll("/gae/shaders/*.xml", m_modelShaders, true, false);
		string currentName = g_config.getRenderModelShader();
		int currentNdx = -1, i = 0;
		foreach (vector<string>, it, m_modelShaders) {
			if (currentName == *it) {
				currentNdx = i;
			}
			m_modelShaderList->addItem(formatString(*it));
			++i;
		}
		m_modelShaderList->SelectionChanged.disconnect(this);
		m_modelShaderList->setSelected(currentNdx);
		m_modelShaderList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);
		m_modelShaderList->setEnabled(true);
	} else {
		m_modelShaderList->setEnabled(false);
	}
}

CheckBox *Options::createStandardCheckBox(CellStrip *container, int cell, const string &text) {
	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));
	Anchors squashAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::SPRINGY, 15));

	Anchors centreAnchors;
	centreAnchors.setCentre(true, false);

	OptionWidget *dw = new OptionWidget(container, text);
	dw->setCell(cell);
	
	CheckBoxHolder *cbh = new CheckBoxHolder(dw);
	cbh->setCell(1);
	cbh->setAnchors(fillAnchors);
	
	Vec2i cbSize(g_widgetConfig.getDefaultItemHeight());

	CheckBox *checkBox = new CheckBox(cbh);
	checkBox->setCell(1);
	checkBox->setSize(cbSize);
	checkBox->setAnchors(squashAnchors);//centreAnchors);
	
	///@todo fix the anchors. The size is weird. - hailstone 17Jully2011
	
	return checkBox;
}

void Options::onToggleShaders(Widget*) {
	g_config.setRenderUseShaders(!g_config.getRenderUseShaders());
	loadShaderList();
	if (!g_config.getRenderUseShaders()) {
		g_renderer.changeShader("");
	}
}

void Options::onToggleDebugMode(Widget*) {
	g_config.setMiscDebugMode(m_debugModeCheckBox->isChecked());
	if (m_optionsMenu) {
		m_optionsMenu->showDebugText(m_debugModeCheckBox->isChecked());
	}
	save();
}

void Options::onToggleDebugKeys(Widget*) {
	g_config.setMiscDebugKeys(m_debugKeysCheckBox->isChecked());
}

void Options::onToggleFullscreen(Widget*) {
	if (g_program.toggleFullscreen()) {
		g_config.setDisplayWindowed(!m_fullscreenCheckBox->isChecked());
	} else {
		m_fullscreenCheckBox->setChecked(false);
	}
}

void Options::onToggleAutoRepair(Widget*) {
	g_config.setGsAutoRepairEnabled(m_autoRepairCheckBox->isChecked());
}

void Options::onToggleAutoReturn(Widget*) {
	g_config.setGsAutoReturnEnabled(m_autoReturnCheckBox->isChecked());
}

void Options::onToggleBumpMapping(Widget*) {
	g_config.setRenderEnableBumpMapping(m_bumpMappingCheckBox->isChecked());
}

void Options::onToggleSpecularMapping(Widget*) {
	g_config.setRenderEnableSpecMapping(m_specularMappingCheckBox->isChecked());
}

void Options::onToggleCameraInvertXAxis(Widget*) {
	g_config.setCameraInvertXAxis(m_cameraInvertXAxisCheckBox->isChecked());
}

void Options::onToggleCameraInvertYAxis(Widget*) {
	g_config.setCameraInvertYAxis(m_cameraInvertYAxisCheckBox->isChecked());
}

void Options::onToggleFocusArrows(Widget*) {
	g_config.setUiFocusArrows(m_focusArrowsCheckBox->isChecked());
}

void Options::on3dTexturesToggle(Widget*) {
	Config &config= Config::getInstance();
	config.setRenderTextures3D(m_3dTexCheckBox->isChecked());
// 	saveConfig();
}

void Options::onSpinnerValueChanged(Widget *source) {
	Spinner *spinner = static_cast<Spinner*>(source);
	if (spinner == m_minCamAltitudeSpinner) {
		g_config.setCameraMinDistance(float(spinner->getValue()));
	} else if (spinner == m_maxCamAltitudeSpinner) {
		g_config.setCameraMaxDistance(float(spinner->getValue()));
	} else if (spinner == m_minRenderDistSpinner) {
		g_config.setRenderDistanceMin(float(spinner->getValue()));
	} else if (spinner == m_maxRenderDistSpinner) {
		g_config.setRenderDistanceMax(float(spinner->getValue()));
	} else if (spinner == m_shadowFrameSkipSpinner) {
		g_config.setRenderShadowFrameSkip(spinner->getValue());
	} else if (spinner == m_consoleMaxLinesSpinner) {
		g_config.setUiConsoleMaxLines(spinner->getValue());
	} else if (spinner == m_scrollSpeedSpinner) {
		g_config.setUiScrollSpeed(float(spinner->getValue()));
	}
}

void Options::onPlayerNameChanged(Widget *source) {
	TextBox *tb = static_cast<TextBox*>(source);
	g_config.setNetPlayerName(tb->getText());
}

void Options::onDropListSelectionChanged(Widget *source) {
	ListBase *list = static_cast<ListBase*>(source);
	Config &config= Config::getInstance();

	if (list == m_langList) {
		map<string,string>::iterator it = m_langMap.find(list->getSelectedItem()->getText());
		string lng;
		if ( it != m_langMap.end() ) {
			lng = it->second;
		} else {
			lng = list->getSelectedItem()->getText();
		}
		config.setUiLocale(lng);
		if (m_optionsMenu) {
			m_optionsMenu->reload();
		} else {
			///@todo reload user_interface? - hailstone 14May2011
		}
	} else if (list == m_filterList) {
		config.setRenderFilter(m_filterList->getSelectedItem()->getText());
// 		saveConfig();
	} else if (list == m_shadowsList) {
		int index = m_shadowsList->getSelectedIndex();
		config.setRenderShadows(Renderer::shadowsToStr(ShadowMode(index)));
// 		saveConfig();
	} else if (list == m_lightsList) {
		config.setRenderLightsMax(list->getSelectedIndex() + 1);
// 		saveConfig();
	} else if (list == m_modelShaderList) {
		string shader = m_modelShaders[list->getSelectedIndex()];
		g_renderer.changeShader(shader);
		g_config.setRenderModelShader(shader);
	} else if (list == m_terrainRendererList) {
		g_config.setRenderTerrainRenderer(list->getSelectedIndex() + 1);
	} else if (list == m_shadowTextureSizeList) {
		g_config.setRenderShadowTextureSize(Conversion::strToInt(list->getSelectedItem()->getText()));
	} else if (list == m_resolutionList) {
		// change res
		VideoMode mode = m_resolutions[m_resolutionList->getSelectedIndex()];
		if (mode != g_program.getVideoMode()) {
			//m_previousVidMode = g_program.getVideoMode();
			Vec2i sz(400, 240);
			Vec2i screenDims = g_metrics.getScreenDims();
			Vec2i pos = (screenDims - sz) / 2;
			/*m_messageDialog = MessageDialog::showDialog(pos, sz, g_lang.get("Confirm"),
				g_lang.get("ConfirmResChange"), g_lang.get("Ok"), g_lang.get("Cancel"));
			m_messageDialog->Button1Clicked.connect(this, &Options::onConfirmResolutionChange);
			m_messageDialog->Button2Clicked.connect(this, &Options::onCancelResolutionChange);
			m_messageDialog->Escaped.connect(this, &Options::onCancelResolutionChange);
			m_messageDialog->Close.connect(this, &Options::onCancelResolutionChange);
			*/
		}
		// recreate window
		/*
		if (changeVideoMode(res.width, res.height, colorBits, refresh)) {
			// show timed dialog
		} else {
			// show error dialog
		}
		*/
		//g_program.resize(g_config.getDisplayWidth(), g_config.getDisplayHeight());
		/*g_program.setVideoMode(mode);*/
		g_program.resize(mode);

		Metrics &metrics = Metrics::getInstance();
		metrics.setScreenW(mode.w);
		metrics.setScreenH(mode.h);
		g_config.setDisplayWidth(mode.w);
		g_config.setDisplayHeight(mode.h);
		g_renderer.resetGlLists();
		g_widgetConfig.reloadFonts();

		if (m_optionsMenu) {
			m_optionsMenu->reload();
		} else {
			g_gameState.rejigWidgets();
		}
		g_config.save();
		
		//((Widget*)&g_program)->setSize(Vec2i(mode.w, mode.h));
		///@todo update shadow texture size if larger than res -hailstone 22June2011
	}
}

//void Options::onCancelResolutionChange(Widget*) {
//	m_rootWindow->removeFloatingWidget(m_messageDialog);
//	m_messageDialog = 0;
//	syncVideoModeList(m_previousVidMode);
//}
//
//void Options::onConfirmResolutionChange(Widget*) {
//	m_rootWindow->removeFloatingWidget(m_messageDialog);
//	m_messageDialog = 0;
//	VideoMode mode = m_resolutions[m_resolutionList->getSelectedIndex()];
//	g_config.setDisplayWidth(mode.w);
//	g_config.setDisplayHeight(mode.h);
//	g_config.setDisplayRefreshFrequency(mode.freq);
//	g_config.setRenderColorBits(mode.bpp);
//	g_config.save();
//	g_program.exit();
//}

void Options::onSliderValueChanged(Widget *source) {
	Slider2 *slider = static_cast<Slider2*>(source);
	if (slider == m_volFxSlider) {
		g_config.setSoundVolumeFx(slider->getValue());
		g_soundRenderer.setFxVolume(slider->getValue() / 100.f);
	} else if (slider == m_volAmbientSlider) {
		g_config.setSoundVolumeAmbient(slider->getValue());
		g_soundRenderer.setAmbientVolume(slider->getValue() / 100.f);
	} else if (slider == m_volMusicSlider) {
		g_config.setSoundVolumeMusic(slider->getValue());
		g_soundRenderer.setMusicVolume(slider->getValue() / 100.f);
	}
}

// private

void Options::save() {
	Config &config= Config::getInstance();

	config.save();
	Renderer::getInstance().loadConfig();
	SoundRenderer::getInstance().loadConfig();
}

void Options::setupListBoxLang() {
	Config &config= Config::getInstance();

	const string langDir = "gae/data/lang/";
	vector<string> langResults;
	findAll(langDir + "*.lng", langResults, true);
	if(langResults.empty()){
        throw runtime_error("No lang files in " + langDir);
	}

	const string langListPath = langDir + "langlist.txt";
	istream *fp = FSFactory::getInstance()->getIStream(langListPath.c_str());
	
	// insert values into table from file (all possible lang codes)
	map<string,string> langTable;
	char buf[128];
	//while ( fgets(buf, 128, fp) ) {
	while(fp->getline(buf, 128)){
		char *code = strtok(buf, "=");
		char *lang = strtok(NULL, "=");
		if ( code && lang ) {
			langTable[string(code)] = string(lang);
		}
	}
	delete fp;

	// insert the values for langNames (the locales we care about (have lang files for))
	vector<string> langNames;
	for ( vector<string>::iterator it = langResults.begin(); it != langResults.end(); ++it ) {
		map<string,string>::iterator lcit = langTable.find(*it);
		if ( lcit != langTable.end() ) {
			if (lcit->second[lcit->second.size() - 1] == 13) {
				lcit->second[lcit->second.size() - 1] = '\0';
			}
			langNames.push_back(lcit->second);
			m_langMap[lcit->second] = *it;
		} else {
			langNames.push_back(*it);
		}
	}

	// insert values and initial value for listBoxLang
	m_langList->addItems(langNames);
	const string &loc = config.getUiLocale();
	if (langTable.find(loc) != langTable.end()) {
		m_langList->setSelected(langTable[loc]);
	} else {
		m_langList->setSelected(loc);
	}
}

// =====================================================
// 	class OptionsFrame
// =====================================================

OptionsFrame::OptionsFrame(WidgetWindow* window)
		: Frame(window, ButtonFlags::CLOSE)
		, m_saveButton(0)
		, m_options(0) {
	init();
}

OptionsFrame::OptionsFrame(Container* parent)
		: Frame(parent, ButtonFlags::CLOSE)
		, m_saveButton(0)
		, m_options(0) {
	init();
}

void OptionsFrame::init() {
	
	delete m_options;

	// apply/cancel buttons

	// options panel
	setSizeHint(1, SizeHint());
	m_optionsPanel = new CellStrip(this, Orientation::HORIZONTAL, 1);
	m_optionsPanel->setCell(1);
	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	m_optionsPanel->setAnchors(anchors);

	m_options = new Options(m_optionsPanel, 0);
	m_options->setCell(0);
	m_options->setAnchors(anchors);

	setDirty();
}

void OptionsFrame::init(Vec2i pos, Vec2i size, const string &title) {
	setPos(pos);
	setSize(size);
	setTitleText(title);
	layoutCells();
}

void OptionsFrame::onButtonClicked(Widget *source) {
	/*Button *btn = static_cast<Button*>(source);
	if (btn == m_button1) {
		Button1Clicked(this);
	} else {
		Button2Clicked(this);
	}*/
}

}}//end namespace