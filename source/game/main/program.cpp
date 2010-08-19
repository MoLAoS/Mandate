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

#include "pch.h"
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
#include "menu_state_new_game.h"
#include "menu_state_join_game.h"
#include "sim_interface.h"
#include "network_interface.h"

#include "leak_dumper.h"

#include "interpolation.h"

using namespace Glest::Net;

using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;

namespace Glest { namespace Main {

// =====================================================
// 	class Program::CrashProgramState
// =====================================================

Program::CrashProgramState::CrashProgramState(Program &program, const exception *e) :
		ProgramState(program) {
	try {
		Renderer::getInstance().saveScreen("glestadv-crash.tga");
	} catch(runtime_error &e) {
		printf("%s", e.what());
	}

	msgBox.init("", "Exit");
	if(e) {
		fprintf(stderr, "%s\n", e->what());
		msgBox.setText(string("Exception: ") + e->what());
	} else {
		msgBox.setText("Glest Advanced Engine has crashed."
					   "\nPlease help us improve GAE by emailing the file"
					   "\ngae-crash.txt to " + gaeMailString + ".");
	}
	mouse2dAnim = mouseY = mouseX = 0;
	this->e = e;
}

void Program::CrashProgramState::renderBg() {
	Renderer &renderer= Renderer::getInstance();
	renderer.clearBuffers();
	renderer.reset2d();
	renderer.renderMessageBox(&msgBox);
	renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);
}

void Program::CrashProgramState::renderFg() {
	Renderer &renderer= Renderer::getInstance();
	renderer.swapBuffers();
}

void Program::CrashProgramState::mouseDownLeft(int x, int y) {
	if(msgBox.mouseClick(x,y)) {
		program.exit();
	}
}

void Program::CrashProgramState::mouseMove(int x, int y, const MouseState &mouseState) {
	mouseX = x;
	mouseY = y;
	msgBox.mouseMove(x, y);
}

void Program::CrashProgramState::update() {
	mouse2dAnim = (mouse2dAnim +1) % Renderer::maxMouse2dAnim;
}

// =====================================================
// 	class Program
// =====================================================

Program *Program::singleton = NULL;

// ===================== PUBLIC ========================

Program::Program(CmdArgs &args)
		: cmdArgs(args)
		, tickTimer(1, maxTimes, -1)
		, updateTimer(GameConstants::updateFps, maxUpdateTimes, maxUpdateBackLog)
		, updateCameraTimer(GameConstants::cameraFps, maxTimes, 10)
		, simulationInterface(0)
		, programState(0)
		, crashed(false)
		, terminating(false)
		, visible(true)
		, keymap(getInput(), "keymap.ini") {
	// set video mode
	setDisplaySettings();

	// Window
	Window::setText("Glest Advanced Engine");
	Window::setStyle(g_config.getDisplayWindowed() ? wsWindowedFixed: wsFullscreen);
	Window::setPos(0, 0);
	Window::setSize(g_config.getDisplayWidth(), g_config.getDisplayHeight());
	Window::create();

	// log start
	Logger::getInstance().clear();
	Logger::getServerLog().clear();
	Logger::getClientLog().clear();
	g_errorLog.clear();

	// lang
	g_lang.setLocale(g_config.getUiLocale());

	Shared::Graphics::use_simd_interpolation = g_config.getRenderInterpolateWithSIMD();
	
	// render
	initGl(g_config.getRenderColorBits(), g_config.getRenderDepthBits(), g_config.getRenderStencilBits());
	makeCurrentGl();

	// load coreData, (needs renderer, but must load before renderer init) and init renderer
	if (!g_coreData.load() || !g_renderer.init()) {
		throw runtime_error("An error occurred loading core data.\nPlease see glestadv-error.log");
	}	

	//sound
	g_soundRenderer.init(this);

	if (!fileExists("keymap.ini")) {
		keymap.save("keymap.ini");
	}
	keymap.load("keymap.ini");

	simulationInterface = new SimulationInterface(*this);

	WidgetWindow::instance = this;
	singleton = this;

	init();

	if (cmdArgs.isTest("interpolation")) {
		Shared::Graphics::test_interpolate();
	}
}

Program::~Program() {
	Renderer::getInstance().end();

	if (programState) {
		delete programState;
	}
	delete simulationInterface;

	//restore video mode
	restoreDisplaySettings();
	singleton = 0;
}

void Program::init() {
	// startup and immediately host a game
	if(cmdArgs.isServer()) {
		MainMenu* mainMenu = new MainMenu(*this);
		setState(mainMenu);
		mainMenu->setState(new MenuStateNewGame(*this, mainMenu, true));
	// startup and immediately connect to server
	} else if(!cmdArgs.getClientIP().empty()) {
		MainMenu* mainMenu = new MainMenu(*this);
		setState(mainMenu);
		mainMenu->setState(new MenuStateJoinGame(*this, mainMenu, true, Ip(cmdArgs.getClientIP())));
	// load map and tileset without players
	} else if(!cmdArgs.getLoadmap().empty()) {
		GameSettings &gs = simulationInterface->getGameSettings();
		gs.clear();
		gs.setDefaultResources(false);
		gs.setDefaultUnits(false);
		gs.setDefaultVictoryConditions(false);
		gs.setThisFactionIndex(0);
		gs.setTeam(0, 0);
		gs.setMapPath(string("maps/") + cmdArgs.getLoadmap() + ".gbm");
		gs.setTilesetPath(string("tilesets/") + cmdArgs.getLoadTileset());
		gs.setTechPath(string("techs/magitech"));
		gs.setFogOfWar(false);
		gs.setFactionCount(0);

		setState(new ShowMap(*this));
	} else if(!cmdArgs.getScenario().empty()) {
		ScenarioInfo scenarioInfo;
		Scenario::loadScenarioInfo(cmdArgs.getScenario(), cmdArgs.getCategory(), &scenarioInfo);
		Scenario::loadGameSettings(cmdArgs.getScenario(), cmdArgs.getCategory(), &scenarioInfo);
		setState(new QuickScenario(*this));

	// normal startup
	} else {
		setState(new Intro(*this));
	}
}

void Program::loop() {
	int updateCounter = 0;
	int64 lastRender = 0;

	while (handleEvent()) {
		// render
		if (visible && Chrono::getCurMillis() - lastRender >= 1000 / g_config.getRenderFpsMax()) {
			_PROFILE_SCOPE("Program::loop() : Render");
			lastRender = Chrono::getCurMillis();
			programState->renderBg();
			Renderer::getInstance().reset2d(true);
			WidgetWindow::render();
			programState->renderFg();
		}

		size_t sleepTime = updateCameraTimer.timeToWait();
		sleepTime = sleepTime < updateTimer.timeToWait() ? sleepTime : updateTimer.timeToWait();
		sleepTime = sleepTime < tickTimer.timeToWait() ? sleepTime : tickTimer.timeToWait();

		if (sleepTime) {
			_PROFILE_SCOPE("Program::loop() : Sleeping");
			Shared::Platform::sleep(sleepTime);
		}

		//update camera
		while (updateCameraTimer.isTime()) {
			programState->updateCamera();
		}

		//update world
		while (updateTimer.isTime() && !terminating) {
			++updateCounter;
			if (updateCounter % 4 == 0) {
				_PROFILE_SCOPE("Program::loop() : Update Gui & Sound");
				WidgetWindow::update();
				SoundRenderer::getInstance().update();
				GraphicComponent::update();
			}
			const int &interval = programState->getUpdateInterval();
			if (interval && updateCounter % interval == 0) {
				_PROFILE_SCOPE("Program::loop() : Update World/Menu");
				programState->update();
			}
			if (simulationInterface->isNetworkInterface()) {
				_PROFILE_SCOPE("Program::loop() : Update Network");
				simulationInterface->asNetworkInterface()->update();
			}
		}

		//tick timer
		while (tickTimer.isTime() && !terminating) {
			programState->tick();
		}
	}
	terminating = true;
}

void Program::eventResize(SizeState sizeState) {
	switch (sizeState) {
		case ssMinimized:
			visible = false;
			//restoreVideoMode();
			break;
		case ssMaximized:
		case ssRestored:
			visible = true;
			//setDisplaySettings();
			//renderer.reloadResources();
			break;
	}
}

bool Program::mouseDown(MouseButton btn, Vec2i pos) {
	const Metrics &metrics = Metrics::getInstance();
	int vx = metrics.toVirtualX(pos.x);
	int vy = metrics.toVirtualY(pos.y);

	switch (btn) {
		case MouseButton::LEFT:
			programState->mouseDownLeft(vx, vy);
			break;
		case MouseButton::RIGHT:
			programState->mouseDownRight(vx, vy);
			break;
		case MouseButton::MIDDLE:
			programState->mouseDownCenter(vx, vy);
			break;
		default:
			break;
	}
	return true;
}

bool Program::mouseUp(MouseButton btn, Vec2i pos) {
	const Metrics &metrics = Metrics::getInstance();
	int vx = metrics.toVirtualX(pos.x);
	int vy = metrics.toVirtualY(pos.y);

	switch (btn) {
		case MouseButton::LEFT:
			programState->mouseUpLeft(vx, vy);
			break;
		case MouseButton::RIGHT:
			programState->mouseUpRight(vx, vy);
			break;
		case MouseButton::MIDDLE:
			programState->mouseUpCenter(vx, vy);
			break;
		default:
			break;
	}
	return true;
}

bool Program::mouseMove(Vec2i pos) {
	const Metrics &metrics = Metrics::getInstance();
	int vx = metrics.toVirtualX(pos.x);
	int vy = metrics.toVirtualY(pos.y);

	programState->mouseMove(vx, vy, input.getMouseState());
	return true;
}

bool Program::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	const Metrics &metrics = Metrics::getInstance();
	int vx = metrics.toVirtualX(pos.x);
	int vy = metrics.toVirtualY(pos.y);

	switch (btn){
		case MouseButton::LEFT:
			programState->mouseDoubleClickLeft(vx, vy);
			break;
		case MouseButton::RIGHT:
			programState->mouseDoubleClickRight(vx, vy);
			break;
		case MouseButton::MIDDLE:
			programState->mouseDoubleClickCenter(vx, vy);
			break;
		default:
			break;
	}
	return true;
}

bool Program::mouseWheel(Vec2i pos, int zDelta) {
	const Metrics &metrics = Metrics::getInstance();
	int vx = metrics.toVirtualX(pos.x);
	int vy = metrics.toVirtualY(pos.y);

	programState->eventMouseWheel(vx, vy, zDelta);
	return true;
}

bool Program::keyDown(Key key) {
	programState->keyDown(key);
	return true;
}

bool Program::keyUp(Key key) {
	programState->keyUp(key);
	return true;
}

bool Program::keyPress(char c) {
	programState->keyPress(c);
	return true;
}


// ==================== misc ====================

void Program::setSimInterface(SimulationInterface *si) {
	delete simulationInterface;
	simulationInterface = si;
}

void Program::setState(ProgramState *programState) {
	if (programState) {
		delete this->programState;
	}
	this->programState = programState;
	programState->load();
	programState->init();
	resetTimers();
}

void Program::exit() {
	destroy();
	terminating = true;
}

void Program::resetTimers() {
	tickTimer.reset();
	updateTimer.reset();
	updateCameraTimer.reset();
}

// ==================== PRIVATE ====================

void Program::setDisplaySettings() {
	if (!g_config.getDisplayWindowed()) {
		int freq= g_config.getDisplayRefreshFrequency();
		int colorBits= g_config.getRenderColorBits();
		int screenWidth= g_config.getDisplayWidth();
		int screenHeight= g_config.getDisplayHeight();

		if (!(changeVideoMode(screenWidth, screenHeight, colorBits, freq)
		|| changeVideoMode(screenWidth, screenHeight, colorBits, 0))) {
			throw runtime_error( "Error setting video mode: " +
				intToStr(screenWidth) + "x" + intToStr(screenHeight) + "x" + intToStr(colorBits));
		}
	}
}

void Program::restoreDisplaySettings(){
	if(!g_config.getDisplayWindowed()){
		restoreVideoMode();
	}
}

void Program::crash(const exception *e) {
	// if we've already crashed then we just try to exit
	if(!crashed) {
		crashed = true;

		if(programState) {
			delete programState;
		}

		programState = new CrashProgramState(*this, e);
		loop();
	} else {
		exit();
	}
}

}}//end namespace
