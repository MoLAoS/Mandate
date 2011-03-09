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

Program::CrashProgramState::CrashProgramState(Program &program, const exception *e)
		: ProgramState(program)
		, done(false) {
	program.setFade(1.f);
	program.initMouse();
	string msg;
	if(e) {
		msg = string("Exception: ") + e->what();
		fprintf(stderr, "%s\n", e->what());
	} else {
		msg = string("Glest Advanced Engine has crashed. Please help us improve GAE by emailing the file")
			+ " gae-crash.txt to " + gaeMailString + ".";
	}
	Vec2i screenDims = g_metrics.getScreenDims();
	Vec2i size(screenDims.x - 200, screenDims.y / 2);
	Vec2i pos = screenDims / 2 - size / 2;
	msgBox = MessageDialog::showDialog(pos, size, "Crash!", msg, g_lang.get("Exit"), "");
	msgBox->Button1Clicked.connect(this, &Program::CrashProgramState::onExit);
	this->e = e;
}

void Program::CrashProgramState::onExit(BasicDialog*) {
	done = true;
}

void Program::CrashProgramState::update() {
	if (done) {
		program.exit();
	}
}

void Program::CrashProgramState::renderBg() {
	g_renderer.clearBuffers();
}

void Program::CrashProgramState::renderFg() {
	g_renderer.swapBuffers();
}

// =====================================================
// 	class Program
// =====================================================

Program *Program::singleton = NULL;

// ===================== PUBLIC ========================

Program::Program(CmdArgs &args)
		: cmdArgs(args)
		, tickTimer(1, maxTimes, -1)
		, updateTimer(40/*GameConstants::updateFps*/, maxUpdateTimes, maxUpdateBackLog)
		, renderTimer(g_config.getRenderFpsMax(), 1, 0)
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

//	// clear logs
//	g_logger.clear();
//	g_serverLog.clear();
//	g_clientLog.clear();
//	g_errorLog.clear();
//#	if LOG_WIDGET_EVENTS
//		g_widgetLog.clear();
//#	endif
//#	if AI_LOGGING
//		Logger::getAiLog().clear();
//#	endif
//	Logger::getWorldLog().clear();

	// lang
	g_lang.setLocale(g_config.getUiLocale());

	Shared::Graphics::use_simd_interpolation = g_config.getRenderInterpolateWithSIMD();
	
	// render
	g_logger.logProgramEvent("Initialising OpenGL");
	initGl(g_config.getRenderColorBits(), g_config.getRenderDepthBits(), g_config.getRenderStencilBits());
	makeCurrentGl();

	g_logger.logProgramEvent("Loading default texture.");
	Texture2D::defaultTexture = g_renderer.getTexture2D(ResourceScope::GLOBAL, "data/core/misc_textures/default.tga");

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

	if (cmdArgs.isTest("interpolation")) {
		Shared::Graphics::test_interpolate();
	}
	g_logger.logProgramEvent("Program object successfully constructed.");
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

bool Program::init() {
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
		string map = cmdArgs.getLoadmap();
		// supporting absolute paths with extension, e.g. showmap in map editor, FIXME: a bit hacky
		if(map[0]=='/'){
			gs.setMapPath(map);
		}else{
			gs.setMapPath(string("maps/") + map);
		}
		gs.setTilesetPath(string("tilesets/") + cmdArgs.getLoadTileset());
		gs.setTechPath(string("techs/magitech"));
		gs.setFogOfWar(false);
		gs.setShroudOfDarkness(false);
		gs.setFactionCount(0);

		try{
			setState(new ShowMap(*this));
		}catch(runtime_error &e){
			// e.g. map path wrong
			cout << "caught exception:" << endl << e.what() << endl;
			return false;
		}
	} else if(!cmdArgs.getScenario().empty()) {
		ScenarioInfo scenarioInfo;
		try{
			Scenario::loadScenarioInfo(cmdArgs.getScenario(), cmdArgs.getCategory(), &scenarioInfo);
			Scenario::loadGameSettings(cmdArgs.getScenario(), cmdArgs.getCategory(), &scenarioInfo);
			setState(new QuickScenario(*this));
		}catch(runtime_error &e){
			cout << "exception caught:" << endl << e.what() << endl;
			return false;
		}
	} else if (cmdArgs.isLoadLastGame()) {
		Shared::Xml::XmlTree doc("game-settings");
		doc.load("last_gamesettings.gs");
		GameSettings &gs = simulationInterface->getGameSettings();
		gs = GameSettings(doc.getRootNode());
		setState(new GameState(*this));

	// normal startup
	} else {
		// make sure to fix up menu music in battle end and intro if uncommenting
		//setState(new Intro(*this));
		setState(new MainMenu(*this));
	}
	return true;
}

void Program::loop() {
	size_t sleepTime;

	while (handleEvent()) {
		{
			_PROFILE_SCOPE("Program::loop() : Determine sleep time");
			int64 cameraTime = updateCameraTimer.timeToWait();
			int64 updateTime = updateTimer.timeToWait();
			int64 renderTime = renderTimer.timeToWait();
			int64 tickTime   = tickTimer.timeToWait();
			sleepTime = std::min(std::min(cameraTime, updateTime), std::min(renderTime, tickTime));
		}

		// Zzzz...
		if (sleepTime) {
			_PROFILE_SCOPE("Program::loop() : Sleeping");
			Shared::Platform::sleep(sleepTime);
		}

		// render
		if (renderTimer.isTime()) {
			{
				_PROFILE_SCOPE("Program::loop() : Update Sound & Gui");
				SoundRenderer::getInstance().update();
				WidgetWindow::update();
			}
			if (visible) {
				_PROFILE_SCOPE("Program::loop() : Render");
				programState->renderBg();
				g_renderer.reset2d(true);
				WidgetWindow::render();
				programState->renderFg();
			}
		}

		// update camera
		while (updateCameraTimer.isTime()) {
			_PROFILE_SCOPE("Program::loop() : update camera");
			programState->updateCamera();
		}

		// update world
		while (updateTimer.isTime() && !terminating) {
			_PROFILE_SCOPE("Program::loop() : Update World/Menu");
			programState->update();
			if (simulationInterface->isNetworkInterface()) {
				simulationInterface->asNetworkInterface()->update();
			}
		}

		// tick timer
		while (tickTimer.isTime() && !terminating) {
			_PROFILE_SCOPE("Program::loop() : tick");
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
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	switch (btn) {
		case MouseButton::LEFT:
			programState->mouseDownLeft(pos.x, pos.y);
			break;
		case MouseButton::RIGHT:
			programState->mouseDownRight(pos.x, pos.y);
			break;
		case MouseButton::MIDDLE:
			programState->mouseDownCenter(pos.x, pos.y);
			break;
		default:
			break;
	}
	return true;
}

bool Program::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	switch (btn) {
		case MouseButton::LEFT:
			programState->mouseUpLeft(pos.x, pos.y);
			break;
		case MouseButton::RIGHT:
			programState->mouseUpRight(pos.x, pos.y);
			break;
		case MouseButton::MIDDLE:
			programState->mouseUpCenter(pos.x, pos.y);
			break;
		default:
			break;
	}
	return true;
}

bool Program::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	programState->mouseMove(pos.x, pos.y, input.getMouseState());
	return true;
}

bool Program::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	switch (btn){
		case MouseButton::LEFT:
			programState->mouseDoubleClickLeft(pos.x, pos.y);
			break;
		case MouseButton::RIGHT:
			programState->mouseDoubleClickRight(pos.x, pos.y);
			break;
		case MouseButton::MIDDLE:
			programState->mouseDoubleClickCenter(pos.x, pos.y);
			break;
		default:
			break;
	}
	return true;
}

bool Program::mouseWheel(Vec2i pos, int zDelta) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << ", " << zDelta << " )");
	programState->eventMouseWheel(pos.x, pos.y, zDelta);
	return true;
}

bool Program::keyDown(Key key) {
	WIDGET_LOG( __FUNCTION__ << "( " << Key::getName(KeyCode(key)) << " )");
	programState->keyDown(key);
	return true;
}

bool Program::keyUp(Key key) {
	WIDGET_LOG( __FUNCTION__ << "( " << Key::getName(KeyCode(key)) << " )");
	programState->keyUp(key);
	return true;
}

bool Program::keyPress(char c) {
	WIDGET_LOG( __FUNCTION__ << "( '" << c << "' )");
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
	initMouse();
	resetTimers(programState->getUpdateFps());
}

void Program::exit() {
	destroy();
	terminating = true;
}

void Program::resetTimers(int updateFps) {
	tickTimer.reset();
	updateTimer.setFps(updateFps);
	updateTimer.reset();
	updateCameraTimer.reset();
}

void Program::setUpdateFps(int updateFps) {
	updateTimer.setFps(updateFps);
}

// ==================== PRIVATE ====================

void Program::setDisplaySettings() {
	if (!g_config.getDisplayWindowed()) {
		int freq= g_config.getDisplayRefreshFrequency();
		int colorBits= g_config.getRenderColorBits();
		int screenWidth= g_config.getDisplayWidth();
		int screenHeight= g_config.getDisplayHeight();

		string modeString = intToStr(screenWidth) + "x" + intToStr(screenHeight) 
			+ " : " + intToStr(colorBits) + "bpp";

		g_logger.logProgramEvent("Setting video mode: " + modeString + " @" + intToStr(freq) + " Hz");
		if (!changeVideoMode(screenWidth, screenHeight, colorBits, freq)) {
			g_logger.logProgramEvent("Error setting video mode: " + modeString + " @" + intToStr(freq) + " Hz");
			g_logger.logProgramEvent("Attempting to set video mode " + modeString + " @ default freq.");
			if (!changeVideoMode(screenWidth, screenHeight, colorBits, 0)) {
				string msg = "Error setting video mode: " + modeString;
				g_logger.logProgramEvent(msg);
				g_logger.logError(msg);
				cout << msg << endl;
				throw runtime_error(msg);
			}
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
		try {
			g_renderer.saveScreen("glestadv-crash_" + Logger::fileTimestamp() + ".png");
		} catch(runtime_error &e) {
			printf("Exception: %s\n", e.what());
		}
		crashed = true;
		if (programState) {
			delete programState;
		}
		clear();
		programState = new CrashProgramState(*this, e);
		loop();
	} else {
		exit();
	}
}

}}//end namespace
