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

#include "pch.h"
#include "menu_state_options.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "menu_state_graphic_info.h"
#include "util.h"
#include "FSFactory.hpp"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Menu {

class RelatedDoubleOption : public CellStrip {
public:
	RelatedDoubleOption(Container *parent, const string &title, const string &txt1, const string &txt2)
			: CellStrip(parent, Orientation::HORIZONTAL, Origin::FROM_LEFT, 5) {
		setSizeHint(0, SizeHint(40));
		setSizeHint(1, SizeHint(10));
		setSizeHint(2, SizeHint(20));
		setSizeHint(3, SizeHint(10));
		setSizeHint(4, SizeHint(20));

		Anchors dwAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 2),
			Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 2));
		setAnchors(dwAnchors);

		StaticText *label = new StaticText(this);
		label->setText(title);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		label->setCell(0);
		label->setAnchors(Anchor(AnchorType::RIGID, 0));
		label->setAlignment(Alignment::FLUSH_RIGHT);
		label->borderStyle().setSizes(0, 0, 10, 0);

		label = new StaticText(this);
		label->setText(txt1);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		label->setCell(1);
		label->setAnchors(Anchor(AnchorType::RIGID, 0));
		label->setAlignment(Alignment::FLUSH_RIGHT);
		label->borderStyle().setSizes(0, 0, 10, 0);

		label = new StaticText(this);
		label->setText(txt2);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		label->setCell(3);
		label->setAnchors(Anchor(AnchorType::RIGID, 0));
		label->setAlignment(Alignment::FLUSH_RIGHT);
		label->borderStyle().setSizes(0, 0, 10, 0);
	}

	void setCustomSplits(int label, int val1, int val2) {
		setSizeHint(1, SizeHint(label));
		setSizeHint(2, SizeHint(val1));
		setSizeHint(3, SizeHint(label));
		setSizeHint(4, SizeHint(val2));
	}
};

class CheckBoxHolder : public CellStrip {
public:
	CheckBoxHolder(Container *parent)
			: CellStrip(parent, Orientation::HORIZONTAL, 3) {
		setSizeHint(0, SizeHint());
		setSizeHint(1, SizeHint(-1, g_widgetConfig.getDefaultItemHeight()));
		setSizeHint(2, SizeHint());
		setAnchors(Anchors(Anchor(AnchorType::RIGID, 0)));
	}
};

Spinner::Spinner(Container *parent)
		: CellStrip(parent, Orientation::HORIZONTAL, 2)
		, m_minValue(0), m_maxValue(0), m_increment(1), m_value(0) {
	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	setAnchors(anchors);
	m_valueBox = new SpinnerValueBox(this);
	m_valueBox->setCell(0);
	m_valueBox->setAnchors(anchors);
	m_valueBox->setText("0");

	setSizeHint(1, SizeHint(-1, g_widgetConfig.getDefaultItemHeight()));
	m_upButton = new ScrollBarButton(this, Direction::UP);
	m_upButton->setCell(1);
	Anchors a = anchors;
	a.set(Edge::BOTTOM, 50, true);
	m_upButton->setAnchors(a);
	m_upButton->Fire.connect(this, &Spinner::onButtonFired);

	m_downButton = new ScrollBarButton(this, Direction::DOWN);
	m_downButton->setCell(1);
	a = anchors;
	a.set(Edge::TOP, 50, true);
	m_downButton->setAnchors(a);
	m_downButton->Fire.connect(this, &Spinner::onButtonFired);
}

void Spinner::onButtonFired(Widget *source) {
	ScrollBarButton *btn = static_cast<ScrollBarButton*>(source);
	int val = m_value + (btn == m_upButton ? m_increment : -m_increment);
	val = clamp(val, m_minValue, m_maxValue);
	if (val != m_value) {
		m_value = val;
		m_valueBox->setText(intToStr(m_value));
		ValueChanged(this);
	}
}

// =====================================================
// 	class MenuStateOptions
// =====================================================

MenuStateOptions::MenuStateOptions(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu)
		, m_transitionTarget(Transition::INVALID) {
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();
	const Metrics &metrics = Metrics::getInstance();

	int s = g_widgetConfig.getDefaultItemHeight();

	CellStrip *rootStrip = new CellStrip((Container*)&program, Orientation::VERTICAL, 2);
	Vec2i pad(15, 25);
	rootStrip->setPos(pad);
	rootStrip->setSize(Vec2i(g_config.getDisplayWidth() - pad.w * 2, g_config.getDisplayHeight() - pad.h * 2));

	// Option panel
	rootStrip->setSizeHint(0, SizeHint());
	buildOptionsPanel(rootStrip, 0);

	Anchors anchors(Anchor(AnchorType::RIGID, 0));

	// Buttons panel
	rootStrip->setSizeHint(1, SizeHint(-1, s * 3));
	CellStrip *btnPanel = new CellStrip(rootStrip, Orientation::HORIZONTAL, 3);
	btnPanel->setCell(1);
	btnPanel->setAnchors(anchors);

	anchors.setCentre(true, true);
	Vec2i sz(s * 7, s);

	// create buttons
	m_returnButton = new Button(btnPanel, Vec2i(0), sz);
	m_returnButton->setCell(0);
	m_returnButton->setAnchors(anchors);
	m_returnButton->setText(lang.get("Return"));
	m_returnButton->Clicked.connect(this, &MenuStateOptions::onButtonClick);

	m_autoConfigButton = new Button(btnPanel, Vec2i(0), sz);
	m_autoConfigButton->setCell(1);
	m_autoConfigButton->setAnchors(anchors);
	m_autoConfigButton->setText(lang.get("AutoConfig"));
	m_autoConfigButton->Clicked.connect(this, &MenuStateOptions::onButtonClick);
	
	m_openGlInfoButton = new Button(btnPanel, Vec2i(0), sz);
	m_openGlInfoButton->setCell(2);
	m_openGlInfoButton->setAnchors(anchors);
	m_openGlInfoButton->setText(lang.get("GraphicInfo"));
	m_openGlInfoButton->Clicked.connect(this, &MenuStateOptions::onButtonClick);
}

void MenuStateOptions::buildOptionsPanel(CellStrip *container, int cell) {
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();

	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));
	Anchors padAnchors(Anchor(AnchorType::RIGID, 10), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::RIGID, 10), Anchor(AnchorType::RIGID, 0));

	CellStrip *pnl = new CellStrip(container, Orientation::HORIZONTAL, 2);
	pnl->setCell(cell);
	pnl->setAnchors(fillAnchors);
	//pnl->borderStyle().setSolid(g_widgetConfig.getColourIndex(Vec3f(1.f, 0.f, 1.f)));
	//pnl->borderStyle().setSizes(2);

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

	OptionWidget *dw = new OptionWidget(col1, lang.get("FxVolume"));
	dw->setCell(0);
	m_volFxSlider = new Slider2(dw, false);
	m_volFxSlider->setCell(1);
	m_volFxSlider->setAnchors(squashAnchors);
	m_volFxSlider->setRange(100);
	m_volFxSlider->setValue(clamp(config.getSoundVolumeFx(), 0, 100));
	m_volFxSlider->ValueChanged.connect(this, &MenuStateOptions::onSliderValueChanged);

	dw = new OptionWidget(col1, lang.get("AmbientVolume"));
	dw->setCell(1);
	m_volAmbientSlider = new Slider2(dw, false);
	m_volAmbientSlider->setCell(1);
	m_volAmbientSlider->setAnchors(squashAnchors);
	m_volAmbientSlider->setRange(100);
	m_volAmbientSlider->setValue(clamp(config.getSoundVolumeAmbient(), 0, 100));
	m_volAmbientSlider->ValueChanged.connect(this, &MenuStateOptions::onSliderValueChanged);

	dw = new OptionWidget(col1, lang.get("MusicVolume"));
	dw->setCell(2);
	m_volMusicSlider = new Slider2(dw, false);
	m_volMusicSlider->setCell(1);
	m_volMusicSlider->setAnchors(squashAnchors);
	m_volMusicSlider->setRange(100);
	m_volMusicSlider->setValue(clamp(config.getSoundVolumeMusic(), 0, 100));
	m_volMusicSlider->ValueChanged.connect(this, &MenuStateOptions::onSliderValueChanged);

	// Player Name
	dw = new OptionWidget(col1, lang.get("PlayerName"));
	dw->setCell(3);
	TextBox *tb = new TextBox(dw);
	tb->setText("");
	tb->setCell(1);
	tb->setAnchors(squashAnchors);

	// Language
	dw = new OptionWidget(col1, lang.get("Language"));
	dw->setCell(4);
	m_langList = new DropList(dw);
	m_langList->setCell(1);
	m_langList->setAnchors(squashAnchors);
	setupListBoxLang();
	m_langList->SelectionChanged.connect(this, &MenuStateOptions::onDropListSelectionChanged);
	m_langList->setDropBoxHeight(200);

	// Debug mode/keys container
	DoubleOption *qw = new DoubleOption(col1, lang.get("DebugMode"), lang.get("DebugKeys"));
	qw->setCell(5);

	// Debug mode
	CheckBoxHolder *cbh = new CheckBoxHolder(qw);
	cbh->setCell(1);
	cbh->setAnchors(fillAnchors);
	m_debugModeCheckBox = new CheckBox(cbh);
	m_debugModeCheckBox->setCell(1);
	m_debugModeCheckBox->setAnchors(squashAnchors);
	m_debugModeCheckBox->setChecked(config.getMiscDebugMode());
	m_debugModeCheckBox->Clicked.connect(this, &MenuStateOptions::onToggleDebugMode);

	// Debug keys
	cbh = new CheckBoxHolder(qw);
	cbh->setCell(3);
	cbh->setAnchors(fillAnchors);
	m_debugKeysCheckBox = new CheckBox(cbh);
	m_debugKeysCheckBox->setCell(1);
	m_debugKeysCheckBox->setAnchors(squashAnchors);
	m_debugKeysCheckBox->setChecked(config.getMiscDebugKeys());
	m_debugKeysCheckBox->Clicked.connect(this, &MenuStateOptions::onToggleDebugKeys);

	// Camera min / max altitude
	RelatedDoubleOption *rdo = new RelatedDoubleOption(col1,
		lang.get("CameraAltitude"), lang.get("Min"), lang.get("Max"));
	rdo->setCell(6);
	rdo->setCustomSplits(13, 14, 20);
	m_minCamAltitudeSpinner = new Spinner(rdo);
	m_minCamAltitudeSpinner->setCell(2);
	m_minCamAltitudeSpinner->setAnchors(squashAnchors);
	m_minCamAltitudeSpinner->setRanges(0, 20);
	m_minCamAltitudeSpinner->setIncrement(1);
	m_minCamAltitudeSpinner->setValue(int(config.getCameraMinDistance()));
	m_minCamAltitudeSpinner->ValueChanged.connect(this, &MenuStateOptions::onSpinnerValueChanged);

	m_maxCamAltitudeSpinner = new Spinner(rdo);
	m_maxCamAltitudeSpinner->setCell(4);
	m_maxCamAltitudeSpinner->setAnchors(squashAnchors);
	m_maxCamAltitudeSpinner->setRanges(32, 2048);
	m_maxCamAltitudeSpinner->setIncrement(32);
	m_maxCamAltitudeSpinner->setValue(int(config.getCameraMaxDistance()));
	m_maxCamAltitudeSpinner->ValueChanged.connect(this, &MenuStateOptions::onSpinnerValueChanged);

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
	m_shadowsList->SelectionChanged.connect(this, &MenuStateOptions::onDropListSelectionChanged);

	// Texture filter
	dw = new OptionWidget(col2, lang.get("TextureFilter"));
	dw->setCell(1);
	m_filterList = new DropList(dw);
	m_filterList->setCell(1);
	m_filterList->setAnchors(squashAnchors);
	m_filterList->addItem("Bilinear");
	m_filterList->addItem("Trilinear");
	m_filterList->setSelected(config.getRenderFilter());
	m_filterList->SelectionChanged.connect(this, &MenuStateOptions::onDropListSelectionChanged);

	// lights
	qw = new DoubleOption(col2, lang.get("MaxLights"), lang.get("Textures3D"));
	qw->setCustomSplit(true, 35);
	qw->setCell(2);
	m_lightsList = new DropList(qw);
	m_lightsList->setCell(1);
	m_lightsList->setAnchors(squashAnchors);
	for (int i = 1; i <= 8; ++i) {
		m_lightsList->addItem(intToStr(i));
	}
	m_lightsList->setSelected(clamp(config.getRenderLightsMax()-1, 0, 7));
	m_lightsList->SelectionChanged.connect(this, &MenuStateOptions::onDropListSelectionChanged);
	m_lightsList->setDropBoxHeight(200);

	// 3D Textures
	cbh = new CheckBoxHolder(qw);
	cbh->setCell(3);
	m_3dTexCheckBox = new CheckBox(cbh);
	m_3dTexCheckBox->setCell(1);
	m_3dTexCheckBox->setAnchors(squashAnchors);
	m_3dTexCheckBox->setChecked(config.getRenderTextures3D());
	m_3dTexCheckBox->Clicked.connect(this, &MenuStateOptions::on3dTexturesToggle);

	// Camera min / max altitude
	rdo = new RelatedDoubleOption(col2,
		lang.get("RenderDistance"), lang.get("Min"), lang.get("Max"));
	rdo->setCell(3);
	rdo->setCustomSplits(13, 14, 20);
	m_minRenderDistSpinner = new Spinner(rdo);
	m_minRenderDistSpinner->setCell(2);
	m_minRenderDistSpinner->setAnchors(squashAnchors);
	m_minRenderDistSpinner->setRanges(0, 20);
	m_minRenderDistSpinner->setIncrement(1);
	m_minRenderDistSpinner->setValue(int(config.getRenderDistanceMin()));
	m_minRenderDistSpinner->ValueChanged.connect(this, &MenuStateOptions::onSpinnerValueChanged);

	m_maxRenderDistSpinner = new Spinner(rdo);
	m_maxRenderDistSpinner->setCell(4);
	m_maxRenderDistSpinner->setAnchors(squashAnchors);
	m_maxRenderDistSpinner->setRanges(32, 4096);
	m_maxRenderDistSpinner->setIncrement(32);
	m_maxRenderDistSpinner->setValue(int(config.getRenderDistanceMax()));
	m_maxRenderDistSpinner->ValueChanged.connect(this, &MenuStateOptions::onSpinnerValueChanged);

	// Field of View
	dw = new OptionWidget(col2, lang.get("RenderFoV"));
	dw->setCell(4);

	// Enable Shaders
	dw = new OptionWidget(col2, lang.get("UseShaders"));
	dw->setCell(5);

	// Terrain Shader
	dw = new OptionWidget(col2, lang.get("TerrainShader"));
	dw->setCell(6);
	
	// Water Shader
	dw = new OptionWidget(col2, lang.get("WaterShader"));
	dw->setCell(7);

	// Model Shader
	dw = new OptionWidget(col2, lang.get("ModelShader"));
	dw->setCell(8);
}

void MenuStateOptions::onToggleDebugMode(Widget*) {
	g_config.setMiscDebugMode(m_debugModeCheckBox->isChecked());
	m_debugText->setVisible(m_debugModeCheckBox->isChecked());
	saveConfig();
}

void MenuStateOptions::onToggleDebugKeys(Widget*) {
	g_config.setMiscDebugKeys(m_debugKeysCheckBox->isChecked());
}

void MenuStateOptions::onButtonClick(Widget *source) {
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if (source == m_autoConfigButton) {
		soundRenderer.playFx(coreData.getClickSoundA());
		Renderer::getInstance().autoConfig();
		saveConfig();
		m_transitionTarget = Transition::RE_LOAD;
	} else if (source == m_returnButton) {
		soundRenderer.playFx(coreData.getClickSoundA());
		saveConfig();
		m_transitionTarget = Transition::RETURN;
		mainMenu->setCameraTarget(MenuStates::ROOT);
	} else if (source == m_openGlInfoButton) {
		soundRenderer.playFx(coreData.getClickSoundB());
		m_transitionTarget = Transition::GL_INFO;
		mainMenu->setCameraTarget(MenuStates::GFX_INFO);
	}
	doFadeOut();
}

void MenuStateOptions::on3dTexturesToggle(Widget*) {
	Config &config= Config::getInstance();
	config.setRenderTextures3D(m_3dTexCheckBox->isChecked());
// 	saveConfig();
}

void MenuStateOptions::onSpinnerValueChanged(Widget *source) {
	Spinner *spinner = static_cast<Spinner*>(source);
	if (spinner == m_minCamAltitudeSpinner) {
		g_config.setCameraMinDistance(float(spinner->getValue()));
	} else if (spinner == m_maxCamAltitudeSpinner) {
		g_config.setCameraMaxDistance(float(spinner->getValue()));
	} else if (spinner == m_minRenderDistSpinner) {
		g_config.setRenderDistanceMin(float(spinner->getValue()));
	} else if (spinner == m_maxRenderDistSpinner) {
		g_config.setRenderDistanceMax(float(spinner->getValue()));
	}
}

void MenuStateOptions::onDropListSelectionChanged(Widget *source) {
	ListBase *list = static_cast<ListBase*>(source);
	Config &config= Config::getInstance();

	if (list == m_langList) {
		map<string,string>::iterator it = langMap.find(list->getSelectedItem()->getText());
		string lng;
		if ( it != langMap.end() ) {
			lng = it->second;
		} else {
			lng = list->getSelectedItem()->getText();
		}
		config.setUiLocale(lng);
		m_transitionTarget = Transition::RE_LOAD;
		doFadeOut();
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
	}
}

void MenuStateOptions::update() {
	MenuState::update();
	if (m_transition) {
		program.clear();
		switch (m_transitionTarget) {
			case Transition::RETURN:
				mainMenu->setState(new MenuStateRoot(program, mainMenu));
				break;
			case Transition::GL_INFO:
				mainMenu->setState(new MenuStateGraphicInfo(program, mainMenu));
				break;
			case Transition::RE_LOAD:
				g_lang.setLocale(g_config.getUiLocale());
				saveConfig();
				mainMenu->setState(new MenuStateOptions(program, mainMenu));
				break;
		}
	}
}

void MenuStateOptions::onSliderValueChanged(Widget *source) {
	Slider2 *slider = static_cast<Slider2*>(source);
	if (slider == m_volFxSlider) {
		g_config.setSoundVolumeFx(slider->getValue());
	} else if (slider == m_volAmbientSlider) {
		g_config.setSoundVolumeAmbient(slider->getValue());
	} else if (slider == m_volMusicSlider) {
		g_config.setSoundVolumeMusic(slider->getValue());
		g_coreData.getMenuMusic()->setVolume(float(slider->getValue()));
	}
}

// private

void MenuStateOptions::saveConfig(){
	Config &config= Config::getInstance();

	config.save();
	Renderer::getInstance().loadConfig();
	SoundRenderer::getInstance().loadConfig();
}

///@todo could use some more cleanup - hailstone 1/DEC/2009
void MenuStateOptions::setupListBoxLang() {
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
			langMap[lcit->second] = *it;
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

}}//end namespace
