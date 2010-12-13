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

// =====================================================
// 	class MenuStateOptions
// =====================================================

MenuStateOptions::MenuStateOptions(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu)
		, m_transitionTarget(Transition::INVALID) {
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();
	const Metrics &metrics = Metrics::getInstance();
	CoreData &coreData = CoreData::getInstance();

	Font *font = coreData.getFTMenuFontNormal();
	// create
	int gap = (metrics.getScreenW() - 450) / 4;
	int x = gap, w = 150, y = 50, h = 30;
	m_returnButton = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	m_returnButton->setTextParams(lang.get("Return"), Vec4f(1.f), font);
	m_returnButton->Clicked.connect(this, &MenuStateOptions::onButtonClick);

	x += w + gap;
	m_autoConfigButton = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	m_autoConfigButton->setTextParams(lang.get("AutoConfig"), Vec4f(1.f), font);
	m_autoConfigButton->Clicked.connect(this, &MenuStateOptions::onButtonClick);
	
	x += w + gap;
	m_openGlInfoButton = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	m_openGlInfoButton->setTextParams(lang.get("GraphicInfo"), Vec4f(1.f), font);
	m_openGlInfoButton->Clicked.connect(this, &MenuStateOptions::onButtonClick);
	
	int y_space = metrics.getScreenH() - 150; // -100 bottom, -50 top
	const int nItems = 10; // 2 spacers
	int itemHeight = int(font->getMetrics()->getHeight() + 5.f);
	int y_gap = (y_space - itemHeight * nItems) / 11;

	x = metrics.getScreenW() / 2 - 300;
	y = 100 + y_gap;
	w = 600;
	h = itemHeight;
	int yInc = h + y_gap;
	
	OptionContainer* ocPtr;

	// lights
	ocPtr = new OptionContainer(&program,  Vec2i(x, y), Vec2i(w, h), lang.get("MaxLights"));
	m_lightsList = new DropList(ocPtr);
	ocPtr->setWidget(m_lightsList );
	m_lightsList->setSize(Vec2i(150, m_lightsList ->getHeight()));
	for (int i = 1; i <= 8; ++i) {
		m_lightsList->addItem(intToStr(i));
	}
	m_lightsList->setSelected(clamp(config.getRenderLightsMax()-1, 0, 7));
	m_lightsList->SelectionChanged.connect(this, &MenuStateOptions::onDropListSelectionChanged);
	m_lightsList->setDropBoxHeight(std::max(y - 100, 150));

	// 3D Textures
	y += yInc;
	ocPtr = new OptionContainer(&program,  Vec2i(x, y), Vec2i(w, h), lang.get("Textures3D"));
	m_3dTexCheckBox = new CheckBox(ocPtr);
	ocPtr->setWidget(m_3dTexCheckBox);
	m_3dTexCheckBox->setChecked(config.getRenderTextures3D());
	m_3dTexCheckBox->Clicked.connect(this, &MenuStateOptions::on3dTexturesToggle);

	// Texture filter
	y += yInc;
	ocPtr = new OptionContainer(&program,  Vec2i(x, y), Vec2i(w, h), lang.get("Filter"));
	m_filterList = new DropList(ocPtr);
	ocPtr->setWidget(m_filterList);

	m_filterList->setSize(Vec2i(350, m_filterList->getHeight()));
	m_filterList->addItem("Bilinear");
	m_filterList->addItem("Trilinear");
	m_filterList->setSelected(config.getRenderFilter());
	m_filterList->SelectionChanged.connect(this, &MenuStateOptions::onDropListSelectionChanged);

	// Shadows
	y += yInc;
	ocPtr = new OptionContainer(&program,  Vec2i(x, y), Vec2i(w, h), lang.get("Shadows"));
	m_shadowsList = new DropList(ocPtr);
	ocPtr->setWidget(m_shadowsList);

	m_shadowsList->setSize(Vec2i(350, m_shadowsList->getHeight()));
	for(int i= 0; i < ShadowMode::COUNT; ++i){
		m_shadowsList->addItem(lang.get(Renderer::shadowsToStr(ShadowMode(i))));
	}
	string str= config.getRenderShadows();
	m_shadowsList->setSelected(clamp(int(Renderer::strToShadows(str)), 0, ShadowMode::COUNT - 1));
	m_shadowsList->SelectionChanged.connect(this, &MenuStateOptions::onDropListSelectionChanged);

	// Language
	y += yInc * 2;
	ocPtr = new OptionContainer(&program, Vec2i(x, y), Vec2i(w, h), "Language");
	m_langList = new DropList(ocPtr);
	ocPtr->setWidget(m_langList);
	m_langList->setSize(Vec2i(350, m_langList->getHeight()));
	setupListBoxLang();
	m_langList->SelectionChanged.connect(this, &MenuStateOptions::onDropListSelectionChanged);
	m_langList->setDropBoxHeight(std::max(y - 150, 200));

	y += yInc * 2;

	m_volMusicSlider = new Slider(&program, Vec2i(x, y), Vec2i(w, h), lang.get("MusicVolume"));
	float val = clamp(float(config.getSoundVolumeMusic()) / 100.f, 0.f, 1.f);
	m_volMusicSlider->setValue(val);
	m_volMusicSlider->ValueChanged.connect(this, &MenuStateOptions::onSliderValueChanged);

	y += yInc;
	m_volAmbientSlider = new Slider(&program, Vec2i(x, y), Vec2i(w, h), lang.get("AmbientVolume"));
	val = clamp(float(config.getSoundVolumeAmbient()) / 100.f, 0.f, 1.f);
	m_volAmbientSlider->setValue(val);
	m_volAmbientSlider->ValueChanged.connect(this, &MenuStateOptions::onSliderValueChanged);

	y += yInc;
	m_volFxSlider = new Slider(&program, Vec2i(x, y), Vec2i(w, h), lang.get("FxVolume"));
	val = clamp(float(config.getSoundVolumeFx()) / 100.f, 0.f, 1.f);
	m_volFxSlider->setValue(val);
	m_volFxSlider->ValueChanged.connect(this, &MenuStateOptions::onSliderValueChanged);
}

void MenuStateOptions::onButtonClick(Button* btn) {
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if (btn == m_autoConfigButton) {
		soundRenderer.playFx(coreData.getClickSoundA());
		Renderer::getInstance().autoConfig();
		saveConfig();
		m_transitionTarget = Transition::RE_LOAD;
	} else if (btn == m_returnButton) {
		soundRenderer.playFx(coreData.getClickSoundA());
		saveConfig();
		m_transitionTarget = Transition::RETURN;
		mainMenu->setCameraTarget(MenuStates::ROOT);
	} else if (btn == m_openGlInfoButton) {
		soundRenderer.playFx(coreData.getClickSoundB());
		m_transitionTarget = Transition::GL_INFO;
		mainMenu->setCameraTarget(MenuStates::GFX_INFO);
	}
	doFadeOut();
}

void MenuStateOptions::on3dTexturesToggle(Button* cb) {
	Config &config= Config::getInstance();
	config.setRenderTextures3D(m_3dTexCheckBox->isChecked());
// 	saveConfig();
}


void MenuStateOptions::onDropListSelectionChanged(ListBase* list) {
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

void MenuStateOptions::onSliderValueChanged(Slider* slider) {
	Config &config= Config::getInstance();
	if (slider == m_volFxSlider) {
		config.setSoundVolumeFx(int(slider->getValue() * 100));
	} else if (slider == m_volAmbientSlider) {
		config.setSoundVolumeAmbient(int(slider->getValue() * 100));
	} else if (slider == m_volMusicSlider) {
		config.setSoundVolumeMusic(int(slider->getValue() * 100));
		CoreData::getInstance().getMenuMusic()->setVolume(slider->getValue());
	}
}

// private

void MenuStateOptions::saveConfig(){
	Config &config= Config::getInstance();

	config.save();
	Renderer::getInstance().loadConfig();
	SoundRenderer::getInstance().loadConfig();
}

//TODO: could use some more cleanup - hailstone 1/DEC/2009
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
