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
#include "test_pane.h"
#include "texture_gl.h"
#include "leak_dumper.h"

#include "interpolation.h"

using namespace Glest::Net;

using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;

namespace Glest { namespace Main {

// Program widget event logging...
#define ENABLE_WIDGET_LOGGING 0
// widget logging still needs to be turned on (in logger.h), this just disables 
// the logging macros in this file.
#if !ENABLE_WIDGET_LOGGING
#	undef WIDGET_LOG
#	define WIDGET_LOG(x)
#endif

// =====================================================
// 	class Program::CrashProgramState
// =====================================================

Program::CrashProgramState::CrashProgramState(Program &program, const exception *e)
		: ProgramState(program)
		, done(false) {
	program.setFade(1.f);
//	program.initMouse();
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

void Program::CrashProgramState::onExit(Widget*) {
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
		, updateTimer(GameConstants::updateFps, maxUpdateTimes, maxUpdateBackLog)
		, renderTimer(g_config.getRenderFpsMax(), 1, 0)
		, updateCameraTimer(GameConstants::cameraFps, maxTimes, 10)
		, guiUpdateTimer(GameConstants::guiUpdatesPerSec, maxTimes, 10)
		, simulationInterface(0)
		, m_programState(0)
		, crashed(false)
		, terminating(false)
		, visible(true)
		, keymap(getInput(), "keymap.ini") {
	// lang
	g_lang.setLocale(g_config.getUiLocale());
	
	if (!fileExists("keymap.ini")) {
		keymap.save("keymap.ini");
	}

	{	ONE_TIME_TIMER(Init_Sound, cout);
		// sound
		g_soundRenderer.init(this);
	}

	simulationInterface = new SimulationInterface(*this);

	WidgetWindow::instance = this;
	singleton = this;

	lastFps = fps = 0;
	m_fpsLabel = new StaticText(this);
	m_fpsLabel->setText("FPS: " + intToStr(000));
	Vec2i sz = m_fpsLabel->getTextDimensions();
	m_fpsLabel->setPos(Vec2i(g_metrics.getScreenW() - sz.w - 5,  5));
	m_fpsLabel->setSize(sz);
	m_fpsLabel->setText("FPS: 0");	
	m_fpsLabel->setAlignment(Alignment::FLUSH_RIGHT);
	m_fpsLabel->setPermanent();
	if (!g_config.getMiscDebugMode()) {
		m_fpsLabel->setVisible(false);
	}

	if (cmdArgs.isTest("interpolation")) {
		Shared::Graphics::test_interpolate();
	}
	g_logger.logProgramEvent("Program object successfully constructed.");
}

Program::~Program() {
	Renderer::getInstance().end();
	delete m_programState;
	delete simulationInterface;
	singleton = 0;
}

bool Program::init() {

	// startup and immediately host a game
	if (cmdArgs.isServer()) {
		MainMenu* mainMenu = new MainMenu(*this, false);
		setState(mainMenu);
		mainMenu->setState(new MenuStateNewGame(*this, mainMenu, true));

	// startup and immediately connect to server
	} else if (!cmdArgs.getClientIP().empty()) {
		MainMenu* mainMenu = new MainMenu(*this, false);
		setState(mainMenu);
		mainMenu->setState(new MenuStateJoinGame(*this, mainMenu, true, Ip(cmdArgs.getClientIP())));
	
	// load map and tileset without players
	} else if (!cmdArgs.getLoadmap().empty()) {
		GameSettings &gs = simulationInterface->getGameSettings();
		gs.setPreviewSettings();
		string map = cmdArgs.getLoadmap();
		if (map[0]=='/') {      // supporting absolute paths with extension,
			gs.setMapPath(map); // e.g. showmap in map editor, FIXME: a bit hacky
		} else {
			gs.setMapPath(string("maps/") + map);
		}
		gs.setTilesetPath(string("tilesets/") + cmdArgs.getLoadTileset());
		gs.setTechPath(string("techs/magitech"));
		try {
			setState(new ShowMap(*this));
		} catch (runtime_error &e) {
			// e.g. map path wrong
			std::stringstream ss;
			ss << "Error trying to load map '" << map << "' with tileset '" <<
				cmdArgs.getLoadTileset() << "'\nException: " << e.what();
			cout << ss.str();
			g_logger.logError(ss.str());
			return false;
		}

	// start scenario
	} else if (!cmdArgs.getScenario().empty()) {
		ScenarioInfo scenarioInfo;
		try {
			Scenario::loadScenarioInfo(cmdArgs.getScenario(), cmdArgs.getCategory(), &scenarioInfo);
			Scenario::loadGameSettings(cmdArgs.getScenario(), cmdArgs.getCategory(), &scenarioInfo);
			setState(new QuickScenario(*this));
		} catch (runtime_error &e) {
			std::stringstream ss;
			ss << "Error trying to load scenario '" << cmdArgs.getScenario() << "' from category '" <<
				cmdArgs.getCategory() << "'\nException: " << e.what();
			cout << ss.str();
			g_logger.logError(ss.str());
			return false;
		}

	// load last game settings
	} else if (cmdArgs.isLoadLastGame()) {
		try {
			Shared::Xml::XmlTree doc("game-settings");
			doc.load("last_gamesettings.gs");
			GameSettings &gs = simulationInterface->getGameSettings();
			gs = GameSettings(doc.getRootNode());
			if (gs.hasNetworkSlots()) {
				g_logger.logError("Error: option -lastgame: last game-settings has network slot(s).");
				cout << "Error: option -lastgame: last game-settings has network slot(s).";
				return false;
			}
			// randomise factions that need it
			vector<string> factionNames;
			findAll(gs.getTechPath() + "/factions/*.", factionNames);
			gs.randomiseFactions(factionNames);
		} catch (runtime_error &e) {
			std::stringstream ss;
			ss << "Error trying to load last game-settings\nException: " << e.what();
			cout << ss.str();
			g_logger.logError(ss.str());
			return false;
		}
		setState(new GameState(*this));
	
	} else if (cmdArgs.isTest("gui")) {
		setState(new TestPane(*this));

	// normal startup
	} else {
		// make sure to delete the music line so it doesn't play multiple times if uncommenting
		//setState(new Intro(*this));
		g_soundRenderer.playMusic(g_coreData.getMenuMusic());
		setState(new MainMenu(*this));
	}
	return true;
}

void Program::setFpsCounterVisible(bool v) {
	m_fpsLabel->setVisible(v);
}

void Program::loop() {
	size_t sleepTime;

	while (handleEvent() && !terminating) {
		{
			_PROFILE_SCOPE("Program::loop() : Determine sleep time");
			int64 cameraTime = updateCameraTimer.timeToWait();
			int64 updateTime = updateTimer.timeToWait();
			int64 renderTime = renderTimer.timeToWait();
			int64 tickTime   = tickTimer.timeToWait();
			int64 guiTime    = guiUpdateTimer.timeToWait();
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
				_PROFILE_SCOPE("Program::loop() : Update Sound");
				SoundRenderer::getInstance().update();
			}
			if (visible) {
				_PROFILE_SCOPE("Program::loop() : Render");
				m_programState->renderBg();
				g_renderer.reset2d();
				if (m_programState->isGameState()) {
					static_cast<GameState*>(m_programState)->getDebugStats()->enterSection(TimerSection::RENDER_2D);
				}
				WidgetWindow::render();
				if (m_programState->isGameState()) {
					static_cast<GameState*>(m_programState)->getDebugStats()->exitSection(TimerSection::RENDER_2D);
				}
				m_programState->renderFg();
				++fps;
			}
		}

		// update camera
		while (updateCameraTimer.isTime()) {
			_PROFILE_SCOPE("Program::loop() : Update Camera");
			m_programState->updateCamera();
		}

		// update gui
		while (guiUpdateTimer.isTime()) {
			_PROFILE_SCOPE("Program::loop() : Update Gui");
			WidgetWindow::update();
		}

		// tick timer
		while (tickTimer.isTime()) {
			_PROFILE_SCOPE("Program::loop() : tick");
			m_programState->tick();
			lastFps = fps;
			fps = 0;
			m_fpsLabel->setText("FPS: " + intToStr(lastFps));
		}

		// update world
		while (updateTimer.isTime() && !terminating) {
			_PROFILE_SCOPE("Program::loop() : Update World/Menu");
			if (simulationInterface->isNetworkInterface()) {
				simulationInterface->asNetworkInterface()->update();
			}
			m_programState->update();
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
			m_programState->mouseDownLeft(pos.x, pos.y);
			break;
		case MouseButton::RIGHT:
			m_programState->mouseDownRight(pos.x, pos.y);
			break;
		case MouseButton::MIDDLE:
			m_programState->mouseDownCenter(pos.x, pos.y);
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
			m_programState->mouseUpLeft(pos.x, pos.y);
			break;
		case MouseButton::RIGHT:
			m_programState->mouseUpRight(pos.x, pos.y);
			break;
		case MouseButton::MIDDLE:
			m_programState->mouseUpCenter(pos.x, pos.y);
			break;
		default:
			break;
	}
	return true;
}

bool Program::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	m_programState->mouseMove(pos.x, pos.y, input.getMouseState());
	return true;
}

bool Program::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	switch (btn){
		case MouseButton::LEFT:
			m_programState->mouseDoubleClickLeft(pos.x, pos.y);
			break;
		case MouseButton::RIGHT:
			m_programState->mouseDoubleClickRight(pos.x, pos.y);
			break;
		case MouseButton::MIDDLE:
			m_programState->mouseDoubleClickCenter(pos.x, pos.y);
			break;
		default:
			break;
	}
	return true;
}

bool Program::mouseWheel(Vec2i pos, int zDelta) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << ", " << zDelta << " )");
	m_programState->eventMouseWheel(pos.x, pos.y, zDelta);
	return true;
}

bool Program::keyDown(Key key) {
	WIDGET_LOG( __FUNCTION__ << "( " << Key::getName(KeyCode(key)) << " )");
	m_programState->keyDown(key);
	return true;
}

bool Program::keyUp(Key key) {
	WIDGET_LOG( __FUNCTION__ << "( " << Key::getName(KeyCode(key)) << " )");
	m_programState->keyUp(key);
	return true;
}

bool Program::keyPress(char c) {
	WIDGET_LOG( __FUNCTION__ << "( '" << c << "' )");
	m_programState->keyPress(c);
	return true;
}


// ==================== misc ====================

void Program::setSimInterface(SimulationInterface *si) {
	delete simulationInterface;
	simulationInterface = si;
}

void Program::setState(ProgramState *programState) try {
	//cout << "setting state to ProgramState object @ 0x" << intToHex(int(programState)) << endl;
	assert(programState != m_programState);
	delete m_programState;
	m_programState = programState;
	m_programState->load();
	m_programState->init();
	resetTimers();
} catch (exception &e) {
	crash(&e);
	///@bug I think this causes a crash in Renderer::swapBuffers() on exit - hailstone 17March2011
	///@todo has been fixed by reworking Program::loop() so ProgramState::update() is the last thing.
	/// The whole CrashProgramState thing, and ending up with Program::loop on the call stack twice, is 
	/// not very nice... Should seek alt solution. - silnarm 10May2011
}

void Program::exit() {
	destroy();
	terminating = true;
}

void Program::resetTimers() {
	tickTimer.reset();
	updateTimer.setFps(WORLD_FPS);
	updateTimer.reset();
	updateCameraTimer.reset();
	guiUpdateTimer.reset();
}

void Program::setUpdateFps(int updateFps) {
	updateTimer.setFps(updateFps);
}

// ==================== PRIVATE ====================

void Program::crash(const exception *e) {
	// if we've already crashed then we just try to exit
	if(!crashed) {
		try {
			g_renderer.saveScreen("glestadv-crash_" + Logger::fileTimestamp() + ".png");
		} catch(runtime_error &e) {
			printf("Exception: %s\n", e.what());
		}
		crashed = true;
		delete m_programState;
		clear();
		m_programState = new CrashProgramState(*this, e);
		loop();
	} else {
		exit();
	}
}

}}//end namespace
