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

	//create
	buttonReturn.init(200, 150, 100);
	buttonAutoConfig.init(320, 150, 100);
	buttonOpenglInfo.init(440, 150, 100);

	//labels
	labelVolumeFx.init(200, 530);
	labelVolumeAmbient.init(200, 500);
	labelVolumeMusic.init(200, 470);

	labelLang.init(200, 400);

	labelFilter.init(200, 340);
	labelShadows.init(200, 310);
	labelTextures3D.init(200, 280);
	labelLights.init(200, 250);

   labelMaxPathNodes.init ( 500, 560 );
   labelPFAlgorithm.init ( 500, 530 );
#  ifdef _GAE_DEBUG_EDITION_
      labelPFTexturesOn.init ( 500, 500 );
      labelPFTextureMode.init ( 500, 470 );
#  endif
	//list boxes
	listBoxVolumeFx.init(350, 530, 80);
	listBoxVolumeAmbient.init(350, 500, 80);
	listBoxVolumeMusic.init(350, 470, 80);
	listBoxMusicSelect.init(350, 440, 150);

	listBoxLang.init(350, 400, 170);

	listBoxFilter.init(350, 340, 170);
	listBoxShadows.init(350, 310, 170);
	listBoxTextures3D.init(350, 280, 80);
	listBoxLights.init(350, 250, 80);

   listBoxMaxPathNodes.init ( 650, 560, 80 );
   listBoxPFAlgorithm.init ( 650, 530, 180 );
#  ifdef _GAE_DEBUG_EDITION_
      listBoxPFTexturesOn.init ( 650, 500, 180 );
      listBoxPFTextureMode.init ( 650, 470, 180 );
#  endif

	//set text
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

   labelMaxPathNodes.setText ( lang.get("MaxPathNodes") );
   labelPFAlgorithm.setText ( "SearchAlgorithm" );
#  ifdef _GAE_DEBUG_EDITION_
      labelPFTexturesOn.setText ( "DebugTextures" );
      labelPFTextureMode.setText ( "TextureMode" );
#  endif
	//sound

	//lang
	vector<string> langResults;
	findAll("data/lang/*.lng", langResults);
	if(langResults.empty()){
        throw runtime_error("There is no lang file");
	}
    listBoxLang.setItems(langResults);
	listBoxLang.setSelectedItem(config.getUiLang());

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

   // path finder node limit
   listBoxMaxPathNodes.pushBackItem ( intToStr ( 512 ) );
   listBoxMaxPathNodes.pushBackItem ( intToStr ( 1024 ) );
   listBoxMaxPathNodes.pushBackItem ( intToStr ( 2048 ) );
   listBoxMaxPathNodes.pushBackItem ( intToStr ( 4096 ) );
   listBoxMaxPathNodes.setSelectedItem ( intToStr ( config.getPathFinderMaxNodes() ) );

   listBoxPFAlgorithm.pushBackItem ( "Admissable A*" );
   //listBoxPFAlgorithm.pushBackItem ( "Greedy 'PingPong'" );
   listBoxPFAlgorithm.pushBackItem ( "Greedy Best First" );
   listBoxPFAlgorithm.setSelectedItemIndex ( config.getPathFinderUseAStar() ? 0 : 1 );

#  ifdef _GAE_DEBUG_EDITION_
      listBoxPFTexturesOn.pushBackItem ( "On" );
      listBoxPFTexturesOn.pushBackItem ( "Off" );
      listBoxPFTexturesOn.setSelectedItemIndex ( config.getMiscDebugTextures() ? 0 : 1 );
      listBoxPFTextureMode.pushBackItem ( "Path Only" );
      listBoxPFTextureMode.pushBackItem ( "Open/Closed Sets" );
      listBoxPFTextureMode.pushBackItem ( "Local Annotations" );
      listBoxPFTextureMode.setSelectedItemIndex ( config.getMiscDebugTextureMode () );
#  endif

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
		config.setUiLang(listBoxLang.getSelectedItem());
		lang.loadStrings(config.getUiLang());
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
		CoreData::getInstance().getMenuMusic()->setVolume(strToInt(listBoxVolumeMusic.getSelectedItem())/100.f);
		config.setSoundVolumeMusic(atoi(listBoxVolumeMusic.getSelectedItem().c_str()));
		saveConfig();
	}
   else if ( listBoxMaxPathNodes.mouseClick ( x,y ) )
   {
      config.setPathFinderMaxNodes ( strToInt ( this->listBoxMaxPathNodes.getSelectedItem () ) );
      saveConfig ();
   }
   else if ( listBoxPFAlgorithm.mouseClick ( x,y ) )
   {
      config.setPathFinderUseAStar( listBoxPFAlgorithm.getSelectedItemIndex () ? false : true );
      saveConfig ();
   }
#  ifdef _GAE_DEBUG_EDITION_
   else if ( listBoxPFTexturesOn.mouseClick ( x,y ) )
   {
      config.setMiscDebugTextures ( listBoxPFTexturesOn.getSelectedItemIndex () == 0 ? true : false );
      saveConfig ();
   }
   else if ( listBoxPFTextureMode.mouseClick ( x,y ) )
   {
      config.setMiscDebugTextureMode( listBoxPFTextureMode.getSelectedItemIndex () );
      saveConfig ();
   }
#  endif
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
   listBoxMaxPathNodes.mouseMove (x, y);
   listBoxPFAlgorithm.mouseMove (x, y);
#  ifdef _GAE_DEBUG_EDITION_      
      listBoxPFTexturesOn.mouseMove (x, y);
      listBoxPFTextureMode.mouseMove (x, y);
#  endif
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
   renderer.renderListBox ( &listBoxMaxPathNodes );
   renderer.renderListBox ( &listBoxPFAlgorithm );
#  ifdef _GAE_DEBUG_EDITION_
      renderer.renderListBox ( &listBoxPFTexturesOn );
      renderer.renderListBox ( &listBoxPFTextureMode );
#  endif
   renderer.renderLabel(&labelLang);
	renderer.renderLabel(&labelShadows);
	renderer.renderLabel(&labelTextures3D);
	renderer.renderLabel(&labelLights);
	renderer.renderLabel(&labelFilter);
	renderer.renderLabel(&labelVolumeFx);
	renderer.renderLabel(&labelVolumeAmbient);
	renderer.renderLabel(&labelVolumeMusic);
   renderer.renderLabel( & labelMaxPathNodes );
   renderer.renderLabel( & labelPFAlgorithm );
#  ifdef _GAE_DEBUG_EDITION_
      renderer.renderLabel( & labelPFTexturesOn );
      renderer.renderLabel( & labelPFTextureMode );
#  endif
}

void MenuStateOptions::saveConfig(){
	Config &config= Config::getInstance();

	config.save();
	Renderer::getInstance().loadConfig();
	SoundRenderer::getInstance().loadConfig();
}

}}//end namespace
