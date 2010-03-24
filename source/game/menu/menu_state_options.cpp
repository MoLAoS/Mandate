// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
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

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateOptions
// =====================================================

MenuStateOptions::MenuStateOptions(Program &program, MainMenu *mainMenu) :
	MenuState(program, mainMenu, "config")
{
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();

	//buttons
	buttonReturn.init(200, 150, 100);
	buttonAutoConfig.init(320, 150, 100);
	buttonOpenglInfo.init(440, 150, 100);

	//labels
	initLabels();

	//list boxes
	initListBoxes();

	//set text for the above components
	setTexts();

	//sound

	//lang
	setupListBoxLang();

	//shadows
	for(int i= 0; i<Renderer::sCount; ++i){
		listBoxShadows.pushBackItem(lang.get(Renderer::shadowsToStr(static_cast<Renderer::Shadows>(i))));
	}

	string str= config.getRenderShadows();
	listBoxShadows.setSelectedItemIndex(clamp(Renderer::strToShadows(str), 0, Renderer::sCount-1));

	//filter
	listBoxFilter.pushBackItem("Bilinear");
	listBoxFilter.pushBackItem("Trilinear");
	listBoxFilter.setSelectedItem(config.getRenderFilter());

	//textures 3d
	listBoxTextures3D.pushBackItem(lang.get("No"));
	listBoxTextures3D.pushBackItem(lang.get("Yes"));
	listBoxTextures3D.setSelectedItemIndex(clamp(config.getRenderTextures3D(), 0, 1));

	//lights
	for(int i= 1; i<=8; ++i){
		listBoxLights.pushBackItem(intToStr(i));
	}
	listBoxLights.setSelectedItemIndex(clamp(config.getRenderLightsMax()-1, 0, 7));

	//sound
	for(int i=0; i<=100; i+=5){
		listBoxVolumeFx.pushBackItem(intToStr(i));
		listBoxVolumeAmbient.pushBackItem(intToStr(i));
		listBoxVolumeMusic.pushBackItem(intToStr(i));
	}
	listBoxVolumeFx.setSelectedItem(intToStr(config.getSoundVolumeFx()/5*5));
	listBoxVolumeAmbient.setSelectedItem(intToStr(config.getSoundVolumeAmbient()/5*5));
	listBoxVolumeMusic.setSelectedItem(intToStr(config.getSoundVolumeMusic()/5*5));
}

void MenuStateOptions::mouseClick(int x, int y, MouseButton mouseButton){

	Config &config= Config::getInstance();
	Lang &lang= Lang::getInstance();
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(buttonReturn.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }
	else if(buttonAutoConfig.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		Renderer::getInstance().autoConfig();
		saveConfig();
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
	}
	else if(buttonOpenglInfo.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateGraphicInfo(program, mainMenu));
	}
	else if(listBoxLang.mouseClick(x, y)){
		map<string,string>::iterator it = langMap.find(listBoxLang.getSelectedItem());
		string lng;
		if ( it != langMap.end() ) {
			lng = it->second;
		} else {
			lng = listBoxLang.getSelectedItem();
		}
		config.setUiLocale(lng);
		lang.setLocale(config.getUiLocale());
		saveConfig();
		mainMenu->setState(new MenuStateOptions(program, mainMenu));

	}
	else if(listBoxShadows.mouseClick(x, y)){
		int index= listBoxShadows.getSelectedItemIndex();
		config.setRenderShadows(Renderer::shadowsToStr(static_cast<Renderer::Shadows>(index)));
		saveConfig();
	}
	else if(listBoxFilter.mouseClick(x, y)){
		config.setRenderFilter(listBoxFilter.getSelectedItem());
		saveConfig();
	}
	else if(listBoxTextures3D.mouseClick(x, y)){
		config.setRenderTextures3D(listBoxTextures3D.getSelectedItemIndex());
		saveConfig();
	}
	else if(listBoxLights.mouseClick(x, y)){
		config.setRenderLightsMax(listBoxLights.getSelectedItemIndex()+1);
		saveConfig();
	}
	else if(listBoxVolumeFx.mouseClick(x, y)){
		config.setSoundVolumeFx(atoi(listBoxVolumeFx.getSelectedItem().c_str()));
		saveConfig();
	}
	else if(listBoxVolumeAmbient.mouseClick(x, y)){
		config.setSoundVolumeAmbient(atoi(listBoxVolumeAmbient.getSelectedItem().c_str()));
		saveConfig();
	}
	else if(listBoxVolumeMusic.mouseClick(x, y)){
		CoreData::getInstance().getMenuMusic()->setVolume(Conversion::strToInt(listBoxVolumeMusic.getSelectedItem())/100.f);
		config.setSoundVolumeMusic(atoi(listBoxVolumeMusic.getSelectedItem().c_str()));
		saveConfig();
	}
}

void MenuStateOptions::mouseMove(int x, int y, const MouseState &ms){
	buttonReturn.mouseMove(x, y);
	buttonAutoConfig.mouseMove(x, y);
	buttonOpenglInfo.mouseMove(x, y);
	listBoxLang.mouseMove(x, y);
	listBoxVolumeFx.mouseMove(x, y);
	listBoxVolumeAmbient.mouseMove(x, y);
	listBoxVolumeMusic.mouseMove(x, y);
	listBoxLang.mouseMove(x, y);
	listBoxFilter.mouseMove(x, y);
	listBoxShadows.mouseMove(x, y);
	listBoxTextures3D.mouseMove(x, y);
	listBoxLights.mouseMove(x, y);
}

void MenuStateOptions::render(){
	Renderer &renderer= Renderer::getInstance();

	renderer.renderButton(&buttonReturn);
	renderer.renderButton(&buttonAutoConfig);
	renderer.renderButton(&buttonOpenglInfo);
	renderer.renderListBox(&listBoxLang);
	renderer.renderListBox(&listBoxShadows);
	renderer.renderListBox(&listBoxTextures3D);
	renderer.renderListBox(&listBoxLights);
	renderer.renderListBox(&listBoxFilter);
	renderer.renderListBox(&listBoxVolumeFx);
	renderer.renderListBox(&listBoxVolumeAmbient);
	renderer.renderListBox(&listBoxVolumeMusic);
	renderer.renderLabel(&labelLang);
	renderer.renderLabel(&labelShadows);
	renderer.renderLabel(&labelTextures3D);
	renderer.renderLabel(&labelLights);
	renderer.renderLabel(&labelFilter);
	renderer.renderLabel(&labelVolumeFx);
	renderer.renderLabel(&labelVolumeAmbient);
	renderer.renderLabel(&labelVolumeMusic);
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
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();

	const string langDir = "gae/data/lang/";
	vector<string> langResults;
	findAll(langDir + "*.lng", langResults, true);
	if(langResults.empty()){
        throw runtime_error("No lang files in " + langDir);
	}

	const string langListPath = langDir + "langlist.txt";
	istream *fp = FSFactory::getInstance()->getIStream(langListPath.c_str());
	
	// insert values into table from file
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

	// insert the values for langNames
	vector<string> langNames;
	for ( vector<string>::iterator it = langResults.begin(); it != langResults.end(); ++it ) {
		map<string,string>::iterator lcit = langTable.find(*it);
		if ( lcit != langTable.end() ) {
			langNames.push_back(lcit->second);
			langMap[lcit->second] = *it;
		} else {
			langNames.push_back(*it);
		}
	}

	// insert values and initial value for listBoxLang
    listBoxLang.setItems(langNames);
	const string &loc = config.getUiLocale();
	if ( langTable.find(loc) != langTable.end() ) {
		listBoxLang.setSelectedItem(langTable[loc]);
	} else {
		listBoxLang.setSelectedItem(loc);
	}
}

void MenuStateOptions::initLabels() {
	labelVolumeFx.init(200, 530);
	labelVolumeAmbient.init(200, 500);
	labelVolumeMusic.init(200, 470);

	labelLang.init(200, 400);

	labelFilter.init(200, 340);
	labelShadows.init(200, 310);
	labelTextures3D.init(200, 280);
	labelLights.init(200, 250);
}

void MenuStateOptions::initListBoxes() {
	listBoxVolumeFx.init(350, 530, 80);
	listBoxVolumeAmbient.init(350, 500, 80);
	listBoxVolumeMusic.init(350, 470, 80);
	listBoxMusicSelect.init(350, 440, 150);

	listBoxLang.init(350, 400, 170);

	listBoxFilter.init(350, 340, 170);
	listBoxShadows.init(350, 310, 170);
	listBoxTextures3D.init(350, 280, 80);
	listBoxLights.init(350, 250, 80);
}

void MenuStateOptions::setTexts() {
	Lang &lang= Lang::getInstance();

	buttonReturn.setText(lang.get("Return"));
	buttonAutoConfig.setText(lang.get("AutoConfig"));
	buttonOpenglInfo.setText(lang.get("GraphicInfo"));

	labelLang.setText(lang.get("Language"));
	labelShadows.setText(lang.get("Shadows"));
	labelFilter.setText(lang.get("Filter"));
	labelTextures3D.setText(lang.get("Textures3D"));
	labelLights.setText(lang.get("MaxLights"));
	labelVolumeFx.setText(lang.get("FxVolume"));
	labelVolumeAmbient.setText(lang.get("AmbientVolume"));
	labelVolumeMusic.setText(lang.get("MusicVolume"));
}

}}//end namespace
