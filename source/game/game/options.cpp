
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
#include "user_interface.h"
#include "resource_bar.h"

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
	haveSpecShaders = fileExists("gae/shaders/spec.xml") && fileExists("gae/shaders/bump_spec.xml");
	setSizeHint(0, SizeHint(-1, g_widgetConfig.getDefaultItemHeight()));
	//buildOptionsPanel(parent, 0);
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
	Config &config = g_config;
	Lang &lang = g_lang;

	CellStrip *container = new CellStrip(this, Orientation::HORIZONTAL, 2);
	container->setSizeHint(0, SizeHint(35, -1));
	container->setSizeHint(1, SizeHint(65, -1));

	OptionPanel *leftPnl = new OptionPanel(container, 0);
	OptionPanel *rightPnl = new OptionPanel(container, 1);

	//leftPnl->setSplitDistance(30);
	//rightPnl->setSplitDistance(30);

	rightPnl->addHeading(leftPnl, g_lang.get("General"));
	// Player Name
	TextBox *tb = rightPnl->addTextBox(lang.get("PlayerName"), g_config.getNetPlayerName());
	tb->TextChanged.connect(this, &Options::onPlayerNameChanged);

	// Language
	m_langList = rightPnl->addDropList(lang.get("Language"));
	setupListBoxLang();
	m_langList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);
	m_langList->setDropBoxHeight(200);

	rightPnl->addHeading(leftPnl, g_lang.get("Camera"));

	// Camera min / max altitude
	SpinnerPair sp = rightPnl->addSpinnerPair(lang.get("CameraAltitude"), lang.get("Min"), lang.get("Max"));

	m_minCamAltitudeSpinner = sp.first;
	m_minCamAltitudeSpinner->setRanges(0, 20);
	m_minCamAltitudeSpinner->setIncrement(1);
	m_minCamAltitudeSpinner->setValue(int(config.getCameraMinDistance()));
	m_minCamAltitudeSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	m_maxCamAltitudeSpinner = sp.second;
	m_maxCamAltitudeSpinner->setRanges(32, 2048);
	m_maxCamAltitudeSpinner->setIncrement(32);
	m_maxCamAltitudeSpinner->setValue(int(config.getCameraMaxDistance()));
	m_maxCamAltitudeSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	// Move camera at screen edges
	m_cameraMoveAtEdgesCheckBox = rightPnl->addCheckBox(lang.get("MoveCameraAtScreenEdges"), g_config.getUiMoveCameraAtScreenEdge());
	m_cameraMoveAtEdgesCheckBox->Clicked.connect(this, &Options::onCheckBoxCahnged);

	// Invert axis
	m_cameraInvertXAxisCheckBox = rightPnl->addCheckBox(lang.get("CameraInvertXAxis"), config.getCameraInvertXAxis());
	m_cameraInvertXAxisCheckBox->Clicked.connect(this, &Options::onCheckBoxCahnged);
	
	m_cameraInvertYAxisCheckBox = rightPnl->addCheckBox(lang.get("CameraInvertYAxis"), config.getCameraInvertYAxis());
	m_cameraInvertYAxisCheckBox->Clicked.connect(this, &Options::onCheckBoxCahnged);

	rightPnl->addHeading(leftPnl, g_lang.get("Behavior"));

	// Auto repair / return
	m_autoRepairCheckBox = rightPnl->addCheckBox(lang.get("AutoRepair"), config.getGsAutoRepairEnabled());
	m_autoRepairCheckBox->Clicked.connect(this, &Options::onCheckBoxCahnged);

	m_autoReturnCheckBox = rightPnl->addCheckBox(lang.get("AutoReturn"), config.getGsAutoReturnEnabled());
	m_autoReturnCheckBox->Clicked.connect(this, &Options::onCheckBoxCahnged);

	// focus arrows
	m_focusArrowsCheckBox = rightPnl->addCheckBox(lang.get("FocusArrows"), config.getUiFocusArrows());
	m_focusArrowsCheckBox->Clicked.connect(this, &Options::onCheckBoxCahnged);

	rightPnl->addHeading(leftPnl, g_lang.get("Interface"));

	m_resoureNamesCheckBox = rightPnl->addCheckBox(lang.get("ResourceNames"), g_config.getUiResourceNames());
	m_resoureNamesCheckBox->Clicked.connect(this, &Options::onCheckBoxCahnged);

	// max console lines
	m_consoleMaxLinesSpinner = rightPnl->addSpinner(lang.get("ConsoleMaxLines"));
	m_consoleMaxLinesSpinner->setRanges(1, 100);
	m_consoleMaxLinesSpinner->setIncrement(1);
	m_consoleMaxLinesSpinner->setValue(config.getUiConsoleMaxLines());
	m_consoleMaxLinesSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	// scroll speed
	m_scrollSpeedSpinner = rightPnl->addSpinner(lang.get("ScrollSpeed"));
	m_scrollSpeedSpinner->setRanges(1, 100);
	m_scrollSpeedSpinner->setIncrement(1);
	m_scrollSpeedSpinner->setValue(int(config.getUiScrollSpeed()));
	m_scrollSpeedSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	TabWidget::add(g_lang.get("Game"), container);
}

void Options::buildVideoTab() {
	Config &config = g_config;
	Lang &lang = g_lang;

	CellStrip *container = new CellStrip(this, Orientation::HORIZONTAL, 2);
	container->setSizeHint(0, SizeHint(35, -1));
	container->setSizeHint(1, SizeHint(65, -1));

	OptionPanel *leftPnl = new OptionPanel(container, 0);
	OptionPanel *rightPnl = new OptionPanel(container, 1);

	/*leftPnl->setSplitDistance(40);
	rightPnl->setSplitDistance(40);*/

	rightPnl->addHeading(leftPnl, g_lang.get("General"));
	
	// Video Mode
	m_resolutionList = rightPnl->addDropList(lang.get("Resolution"));
	m_resolutionList->setDropBoxHeight(280);

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

	// Fullscreen
	m_fullscreenCheckBox = rightPnl->addCheckBox(lang.get("Fullscreen"), !config.getDisplayWindowed());
	m_fullscreenCheckBox->Clicked.connect(this, &Options::onCheckBoxCahnged);
	if (m_resolutionList->getSelectedIndex() == -1) {
		// current settings do not match an acceptable vid mode, disable
		m_fullscreenCheckBox->setEnabled(false);
	}

	rightPnl->addHeading(leftPnl, g_lang.get("Shaders"));

	// Enable Shaders
	m_useShadersCheckBox = rightPnl->addCheckBox(lang.get("UseShaders"), config.getRenderUseShaders());
	m_useShadersCheckBox->Clicked.connect(this, &Options::onCheckBoxCahnged);
	if (g_config.getRenderTestingShaders()) {
		m_useShadersCheckBox->setEnabled(false);
	}

	if (config.getRenderEnableSpecMapping() && !haveSpecShaders) {
		config.setRenderEnableSpecMapping(false);
	}

	// Enable bump mapping
	m_bumpMappingCheckBox = rightPnl->addCheckBox(lang.get("BumpMapping"), config.getRenderEnableBumpMapping());
	m_bumpMappingCheckBox->Clicked.connect(this, &Options::onCheckBoxCahnged);
	m_bumpMappingCheckBox->setEnabled(config.getRenderUseShaders());

	// Enable specular mapping
	m_specularMappingCheckBox = rightPnl->addCheckBox(lang.get("SpecularMapping"), config.getRenderEnableSpecMapping());
	m_specularMappingCheckBox->Clicked.connect(this, &Options::onCheckBoxCahnged);
	m_specularMappingCheckBox->setEnabled(haveSpecShaders);

	// Terrain Shader
	m_terrainRendererList = rightPnl->addDropList(lang.get("TerrainShader"));
	m_terrainRendererList->addItem("Original Terrain Renderer");
	m_terrainRendererList->addItem("Terrain Renderer 2");
	m_terrainRendererList->setSelected(config.getRenderTerrainRenderer() - 1);
	m_terrainRendererList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);
	
	// Water Shader
	m_waterRendererList = rightPnl->addDropList(lang.get("WaterShader"));
	m_waterRendererList->addItem(lang.get("Opaque"));
	m_waterRendererList->addItem(lang.get("3D Textures"));
	m_waterRendererList->setSelected(int(config.getRenderTextures3D()));
	m_waterRendererList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);

	rightPnl->addHeading(leftPnl, g_lang.get("Shadows"));

	// Shadow texture size
	m_shadowTextureSizeList = rightPnl->addDropList(lang.get("ShadowTextureSize"), true);
	m_shadowTextureSizeList->setDropBoxHeight(280);

	// sizes must be powers of 2 and less than the resolution
	///@todo only add the sizes less than resolution (both width and height values) - hailstone 17July2011
	m_shadowTextureSizeList->addItem("64");
	m_shadowTextureSizeList->addItem("128");
	m_shadowTextureSizeList->addItem("256");
	m_shadowTextureSizeList->addItem("512");
	m_shadowTextureSizeList->addItem("1024");
	m_shadowTextureSizeList->setSelected(toStr(config.getRenderShadowTextureSize()));
	m_shadowTextureSizeList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);

	// Shadow frame skip
	m_shadowFrameSkipSpinner = rightPnl->addSpinner(lang.get("ShadowFrameSkip"));
	m_shadowFrameSkipSpinner->setRanges(0, 5);
	m_shadowFrameSkipSpinner->setIncrement(1);
	m_shadowFrameSkipSpinner->setValue(config.getRenderShadowFrameSkip());
	m_shadowFrameSkipSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	// Shadows
	m_shadowsList = rightPnl->addDropList(lang.get("Shadows"));
	for(int i= 0; i < ShadowMode::COUNT; ++i){
		m_shadowsList->addItem(lang.get(Renderer::shadowsToStr(ShadowMode(i))));
	}
	string str= config.getRenderShadows();
	m_shadowsList->setSelected(clamp(int(Renderer::strToShadows(str)), 0, ShadowMode::COUNT - 1));
	m_shadowsList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);

	rightPnl->addHeading(leftPnl, g_lang.get("Misc"));

	// Texture filter
	m_filterList = rightPnl->addDropList(lang.get("TextureFilter"));
	m_filterList->addItem("Bilinear");
	m_filterList->addItem("Trilinear");
	m_filterList->setSelected(config.getRenderFilter());
	m_filterList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);

	// lights
	m_lightsList = rightPnl->addDropList(lang.get("MaxLights"), true);
	for (int i = 1; i <= 8; ++i) {
		m_lightsList->addItem(intToStr(i));
	}
	m_lightsList->setSelected(clamp(config.getRenderLightsMax()-1, 0, 7));
	m_lightsList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);
	m_lightsList->setDropBoxHeight(200);

	// render min / max distance
	SpinnerPair sp = rightPnl->addSpinnerPair(lang.get("RenderDistance"), lang.get("Min"), lang.get("Max"));
	m_minRenderDistSpinner = sp.first;
	m_minRenderDistSpinner->setRanges(0, 20);
	m_minRenderDistSpinner->setIncrement(1);
	m_minRenderDistSpinner->setValue(int(config.getRenderDistanceMin()));
	m_minRenderDistSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	m_maxRenderDistSpinner = sp.second;
	m_maxRenderDistSpinner->setRanges(32, 4096);
	m_maxRenderDistSpinner->setIncrement(32);
	m_maxRenderDistSpinner->setValue(int(config.getRenderDistanceMax()));
	m_maxRenderDistSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	// Field of View
	//dw = new OptionWidget(col2, lang.get("RenderFoV"));
	//dw->setCell(4);

	TabWidget::add(lang.get("Video"), container);
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

	StaticText *label = new StaticText(panel);
	label->setCell(0);
	label->setAnchors(Anchors::getFillAnchors());
	label->setAlignment(Alignment::FLUSH_LEFT);
	label->setText(g_lang.get("HotKeys"));
	panel->setSizeHint(0, SizeHint(-1, g_widgetConfig.getDefaultItemHeight() * 5 / 4));

	KeymapWidget *keymapWidget = new KeymapWidget(panel);
	keymapWidget->setCell(1);
	keymapWidget->setAnchors(Anchors::getFillAnchors());

	TabWidget::add(g_lang.get("Controls"), panel);
}

void Options::buildNetworkTab() {
	//CellStrip *panel = new CellStrip(this, Orientation::HORIZONTAL, 2);
	//TabWidget::add(g_lang.get("Network"), panel);
}

void Options::buildDebugTab() {
	DebugOptions *dbgOptions = new DebugOptions(this, m_optionsMenu != 0);
	TabWidget::add(g_lang.get("Debug"), dbgOptions);
}

void Options::disableWidgets() {
	// disable all the widgets until they're known to be ready for ingame - hailstone 14May2011
	m_langList->setEnabled(false);
	m_shadowsList->setEnabled(false);
	m_filterList->setEnabled(false);
	m_lightsList->setEnabled(false);
	m_terrainRendererList->setEnabled(false);
	m_waterRendererList->setEnabled(false);

	m_minCamAltitudeSpinner->setEnabled(false);
	m_maxCamAltitudeSpinner->setEnabled(false);

	m_minRenderDistSpinner->setEnabled(false);
	m_maxRenderDistSpinner->setEnabled(false);
}

void setShader() {
	if (!g_config.getRenderUseShaders()) {
		g_renderer.changeShader("");
	} else {
		if (g_config.getRenderEnableBumpMapping() && g_config.getRenderEnableSpecMapping())  {
			g_renderer.changeShader("bump_spec");
		} else if (g_config.getRenderEnableBumpMapping()) {
			g_renderer.changeShader("bump");
		} else if (g_config.getRenderEnableSpecMapping()) {
			g_renderer.changeShader("spec");
		} else {
			g_renderer.changeShader("basic");
		}
	}
}

void Options::onCheckBoxCahnged(Widget *src) {
	CheckBox *cb = static_cast<CheckBox*>(src);
	if (cb == m_fullscreenCheckBox) {
		if (g_program.toggleFullscreen()) {
			g_config.setDisplayWindowed(!m_fullscreenCheckBox->isChecked());
		} else {
			m_fullscreenCheckBox->setChecked(false);
		}
	} else if (cb == m_useShadersCheckBox) {
		g_config.setRenderUseShaders(m_useShadersCheckBox->isChecked());
		setShader();
		m_bumpMappingCheckBox->setEnabled(g_config.getRenderUseShaders());
		if (haveSpecShaders) {
			m_specularMappingCheckBox->setEnabled(g_config.getRenderUseShaders());
		}
	} else if (cb == m_autoRepairCheckBox) {
		g_config.setGsAutoRepairEnabled(m_autoRepairCheckBox->isChecked());
	} else if (cb == m_autoReturnCheckBox) {
		g_config.setGsAutoReturnEnabled(m_autoReturnCheckBox->isChecked());
	} else if (cb == m_bumpMappingCheckBox) {
		g_config.setRenderEnableBumpMapping(m_bumpMappingCheckBox->isChecked());
		setShader();
	} else if (cb == m_specularMappingCheckBox) {
		g_config.setRenderEnableSpecMapping(m_specularMappingCheckBox->isChecked());
		setShader();
	} else if (cb == m_cameraInvertXAxisCheckBox) {
		g_config.setCameraInvertXAxis(m_cameraInvertXAxisCheckBox->isChecked());
	} else if (cb == m_cameraInvertYAxisCheckBox) {
		g_config.setCameraInvertYAxis(m_cameraInvertYAxisCheckBox->isChecked());
	} else if (cb == m_cameraMoveAtEdgesCheckBox) {
		g_config.setUiMoveCameraAtScreenEdge(m_cameraMoveAtEdgesCheckBox->isChecked());
	} else if (cb == m_focusArrowsCheckBox) {
		g_config.setUiFocusArrows(m_focusArrowsCheckBox->isChecked());
	} else if (cb == m_resoureNamesCheckBox) {
		g_config.setUiResourceNames(m_resoureNamesCheckBox->isChecked());
		if (!m_optionsMenu) {
			g_userInterface.getResourceBar()->reInit(-1);
		}
	}
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
	Config &config = g_config;

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
	} else if (list == m_shadowsList) {
		int index = m_shadowsList->getSelectedIndex();
		config.setRenderShadows(Renderer::shadowsToStr(ShadowMode(index)));
	} else if (list == m_lightsList) {
		config.setRenderLightsMax(list->getSelectedIndex() + 1);
	} else if (list == m_terrainRendererList) {
		config.setRenderTerrainRenderer(list->getSelectedIndex() + 1);
	} else if (list == m_waterRendererList) {
		config.setRenderTextures3D(bool(list->getSelectedIndex()));
	} else if (list == m_shadowTextureSizeList) {
		config.setRenderShadowTextureSize(Conversion::strToInt(list->getSelectedItem()->getText()));
	} else if (list == m_resolutionList) {
		// change res
		VideoMode mode = m_resolutions[m_resolutionList->getSelectedIndex()];
		g_program.resize(mode);

		Metrics &metrics = Metrics::getInstance();
		metrics.setScreenW(mode.w);
		metrics.setScreenH(mode.h);
		config.setDisplayWidth(mode.w);
		config.setDisplayHeight(mode.h);
		g_renderer.resetGlLists();
		g_widgetConfig.reloadFonts();

		if (m_optionsMenu) {
			m_optionsMenu->reload();
		} else {
			g_gameState.rejigWidgets();
		}
		///@todo update shadow texture size if larger than res -hailstone 22June2011
	}
	save();
}

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
	g_config.save();
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