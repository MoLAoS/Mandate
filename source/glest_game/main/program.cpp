// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "program.h"

#include "sound.h"
#include "renderer.h"
#include "config.h"
#include "game.h"
#include "main_menu.h"
#include "intro.h"
#include "world.h"
#include "main.h"
#include "sound_renderer.h"
#include "logger.h"
#include "profiler.h"
#include "core_data.h"
#include "metrics.h"
#include "network_manager.h"
#include "menu_state_new_game.h"
#include "menu_state_join_game.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;

// =====================================================
// 	class Program
// =====================================================

namespace Glest{ namespace Game{

const int Program::maxTimes= 10;

// ===================== PUBLIC ========================

Program::Program(){
	programState= NULL;
}

void Program::initNormal(WindowGl *window){
	init(window);
	setState(new Intro(this));
}

void Program::initServer(WindowGl *window){
	MainMenu* mainMenu= NULL;

	init(window);
	mainMenu= new MainMenu(this);
	setState(mainMenu);
	mainMenu->setState(new MenuStateNewGame(this, mainMenu, true));
}

void Program::initClient(WindowGl *window, const Ip &serverIp){
	MainMenu* mainMenu= NULL;

	init(window);
	mainMenu= new MainMenu(this);
	setState(mainMenu);
	mainMenu->setState(new MenuStateJoinGame(this, mainMenu, true, serverIp));
}

Program::~Program(){
	delete programState;

	Renderer::getInstance().end();

	//restore video mode
	restoreDisplaySettings();
}

void Program::loop(){

	//render
	programState->render();

	//update camera
	while(updateCameraTimer.isTime()){
		programState->updateCamera();
	}

	//update world
	while(updateTimer.isTime()){
		GraphicComponent::update();
		programState->update();
		SoundRenderer::getInstance().update();
		NetworkManager::getInstance().update();
	}

	//fps timer
	while(fpsTimer.isTime()){
		programState->tick();
	}
}

void Program::resize(SizeState sizeState){

	switch(sizeState){
	case ssMinimized:
		//restoreVideoMode();
		break;
	case ssMaximized:
	case ssRestored:
		//setDisplaySettings();
		//renderer.reloadResources();
		break;
	}
}

// ==================== misc ====================

void Program::setState(ProgramState *programState){

	delete this->programState;

	this->programState= programState;
	programState->load();
	programState->init();

	updateTimer.reset();
	updateCameraTimer.reset();
	fpsTimer.reset();
}

void Program::exit(){
	window->destroy();
}

// ==================== PRIVATE ====================

void Program::init(WindowGl *window){

	this->window= window;
	Config &config= Config::getInstance();

    //set video mode
	setDisplaySettings();

	//window
	window->setText("Glest");
	window->setStyle(config.getWindowed()? wsWindowedFixed: wsFullscreen);
	window->setPos(0, 0);
	window->setSize(config.getScreenWidth(), config.getScreenHeight());
	window->create();

	//timers
	fpsTimer.init(1, maxTimes);
	updateTimer.init(GameConstants::updateFps, maxTimes);
	updateCameraTimer.init(GameConstants::cameraFps, maxTimes);

    //log start
	Logger &logger= Logger::getInstance();
	logger.setFile("glest.log");
	logger.clear();

	//lang
	Lang &lang= Lang::getInstance();
	lang.load("data/lang/"+ config.getLang());

	//render
	Renderer &renderer= Renderer::getInstance();

	window->initGl(config.getColorBits(), config.getDepthBits(), config.getStencilBits());
	window->makeCurrentGl();

	//coreData, needs renderer, but must load before renderer init
	CoreData &coreData= CoreData::getInstance();
    coreData.load();

	//init renderer (load global textures)
	renderer.init();

	//sound
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.init(window);
}

void Program::setDisplaySettings(){

	Config &config= Config::getInstance();

	if(!config.getWindowed()){

		int freq= config.getRefreshFrequency();
		int colorBits= config.getColorBits();
		int screenWidth= config.getScreenWidth();
		int screenHeight= config.getScreenHeight();

		if(!(changeVideoMode(screenWidth, screenHeight, colorBits, freq) ||
			changeVideoMode(screenWidth, screenHeight, colorBits, 0)))
		{
			throw runtime_error(
				"Error setting video mode: " +
				intToStr(screenWidth) + "x" + intToStr(screenHeight) + "x" + intToStr(colorBits));
		}
	}
}

void Program::restoreDisplaySettings(){
	Config &config= Config::getInstance();

	if(!config.getWindowed()){
		restoreVideoMode();
	}
}

}}//end namespace
