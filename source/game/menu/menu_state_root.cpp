// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "menu_state_root.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_new_game.h"
#include "menu_state_join_game.h"
#include "menu_state_scenario.h"
#include "menu_state_load_game.h"
#include "menu_state_options.h"
#include "menu_state_about.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "socket.h"
#include "auto_test.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateRoot
// =====================================================

MenuStateRoot::MenuStateRoot(Program &program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "root")
{
	Lang &lang= Lang::getInstance();

	buttonNewGame.init(425, 370, 150);
    buttonJoinGame.init(425, 330, 150);
    buttonScenario.init(425, 290, 150);
	buttonLoadGame.init(425, 250, 150);
    buttonOptions.init(425, 210, 150);
    buttonAbout.init(425, 170, 150);
    buttonExit.init(425, 130, 150);
	labelVersion.init(520, 460);

	buttonNewGame.setText(lang.get("NewGame"));
	buttonJoinGame.setText(lang.get("JoinGame"));
	buttonScenario.setText(lang.get("Scenario"));
	buttonLoadGame.setText(lang.get("LoadGame"));
	buttonOptions.setText(lang.get("Options"));
	buttonAbout.setText(lang.get("About"));
	buttonExit.setText(lang.get("Exit"));
	labelVersion.setText("Advanced Engine " + gaeVersionString);

	// end network interface
	NetworkManager::getInstance().end();
}

void MenuStateRoot::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData=  CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(buttonNewGame.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
    }
	else if(buttonJoinGame.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateJoinGame(program, mainMenu));
    }
	else if(buttonScenario.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateScenario(program, mainMenu));
    }
    else if(buttonOptions.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
    }
	else if(buttonLoadGame.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateLoadGame(program, mainMenu));
	}
    else if(buttonAbout.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateAbout(program, mainMenu));
    }
    else if(buttonExit.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		program.exit();
    }
}

void MenuStateRoot::mouseMove(int x, int y, const MouseState &ms){
	buttonNewGame.mouseMove(x, y);
    buttonJoinGame.mouseMove(x, y);
    buttonScenario.mouseMove(x, y);
	buttonLoadGame.mouseMove(x, y);
    buttonOptions.mouseMove(x, y);
    buttonAbout.mouseMove(x, y);
    buttonExit.mouseMove(x,y);
}

void MenuStateRoot::render(){
	Renderer &renderer= Renderer::getInstance();
	CoreData &coreData= CoreData::getInstance();
	const Metrics &metrics= Metrics::getInstance();

	int w= 300;
	int h= 150;

	renderer.renderTextureQuad(
		(metrics.getVirtualW()-w)/2, 495-h/2, w, h,
		coreData.getLogoTexture(), GraphicComponent::getFade());
	renderer.renderButton(&buttonNewGame);
	renderer.renderButton(&buttonJoinGame);
	renderer.renderButton(&buttonScenario);
	renderer.renderButton(&buttonLoadGame);
	renderer.renderButton(&buttonOptions);
	renderer.renderButton(&buttonAbout);
	renderer.renderButton(&buttonExit);
	renderer.renderLabel(&labelVersion);
}

void MenuStateRoot::update(){
	if (Config::getInstance().getMiscAutoTest()) {
		AutoTest::getInstance().updateRoot(program, mainMenu);
	}
}

}}//end namespace
