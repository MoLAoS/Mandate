// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008-2009 Daniel Santos <daniel.santos@pobox.com>
//				  2009-2010 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "game.h"
#include "config.h"
#include "renderer.h"
#include "particle_renderer.h"
#include "commander.h"
#include "battle_end.h"
#include "sound_renderer.h"
#include "profiler.h"
#include "core_data.h"
#include "metrics.h"
#include "faction.h"
#include "checksum.h"
#include "auto_test.h"
#include "profiler.h"
#include "cluster_map.h"
#include "sim_interface.h"
#include "network_interface.h"
#include "game_menu.h"
#include "resource_bar.h"
#include "mouse_cursor.h"
#include "options.h"

#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif
#include "ai.h"

#include "leak_dumper.h"
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

using Glest::Util::Logger;

using namespace Glest::Net;
using namespace Glest::Sim;
using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest { namespace Gui {

// =====================================================
// 	class GameState
// =====================================================

// ===================== PUBLIC ========================

GameState *GameState::singleton = NULL;

const GameSettings &GameState::getGameSettings()	{return g_simInterface.getGameSettings();}

GameState::GameState(Program &program)
		//main data
		: ProgramState(program)
		, simInterface(program.getSimulationInterface())
		, keymap(program.getKeymap())
		, input(program.getInput())
		, config(Config::getInstance())
		, gui(*this)
		, gameCamera()
		//, m_teamChat(false)
		//misc
		, checksum()
		, loadingText("")
		, mouse2d(0)
		, worldFps(0)
		, lastWorldFps(0)
		, updateFps(0)
		, lastUpdateFps(0)
		, renderFps(0)
		, lastRenderFps(0)
		, noInput(false)
		, netError(false)
		, gotoMenu(false)
		, exitGame(false)
		, exitProgram(false)
		, scrollSpeed(config.getUiScrollSpeed())
		, m_modalDialog(0)
		, m_chatDialog(0)
		, m_debugPanel(0)
		, lastMousePos(0)
		, weatherParticleSystem(0)
		, m_options(0) {
	assert(!singleton);
	singleton = this;
	simInterface->constructGameWorld(this);
}

GameState::~GameState() {
	g_logger.getProgramLog().setState(g_lang.get("Deleting"));
	g_logger.logProgramEvent("~GameState", !program.isTerminating());

	g_renderer.endGame();
	weatherParticleSystem = 0;
	g_soundRenderer.stopAllSounds();

	singleton = 0;
	g_logger.getProgramLog().setLoading(true);

	// reset max update backlog, to prevent super-speed in menus
	program.setMaxUpdateBacklog(2);

	// delete the World
	simInterface->destroyGameWorld();
}

// ==================== init and load ====================

void GameState::load() {
	GameSettings &gameSettings = g_simInterface.getGameSettings();
	const string &mapName = gameSettings.getMapPath();
	const string &tilesetName = gameSettings.getTilesetPath();
	const string &techName = gameSettings.getTechPath();
	const string &scenarioPath = gameSettings.getScenarioPath();
	string scenarioName = basename(scenarioPath);
	const string &thisFactionName = gameSettings.getFactionTypeName(gameSettings.getThisFactionIndex());

	// determine loading screen settings:
	// 1. check sceneraio if applicable
	// 2. check faction
	// 3. check tech
	// 4. use defaults
	ProgramLog &log = g_logger.getProgramLog();

	if (!scenarioName.empty() 
		&& log.setupLoadingScreen(scenarioPath)) {
	} else if (log.setupLoadingScreen(techName + "/factions/" + thisFactionName)) {
	} else if (log.setupLoadingScreen(techName)) {
	} else {
		log.useLoadingScreenDefaults();
	}

	// maybe use config option instead - hailstone 01May2011
	if (g_program.getMouseCursor().descType() == "ImageSetMouseCursor") {
		ImageSetMouseCursor &mouse = static_cast<ImageSetMouseCursor&>(g_program.getMouseCursor());
		// check for custom mouse (faction then tech)
		if (mouse.loadMouse(techName + "/factions/" + thisFactionName)) {
		} else if (mouse.loadMouse(techName)) {
		} else {
			// already using default
		}
	}

	g_logger.getProgramLog().setProgressBar(true);
	g_logger.getProgramLog().setState(Lang::getInstance().get("Loading"));

	if (scenarioName.empty()) {
		log.setSubtitle(formatString(mapName) + " - " +
			formatString(tilesetName) + " - " + formatString(techName));
	} else {
		log.setSubtitle(formatString(scenarioName));
	}

	simInterface->loadWorld();

	// finished loading
	g_logger.getProgramLog().setProgressBar(false);
}

void GameState::init() {
	g_logger.getProgramLog().setState(g_lang.get("Initializing"));

	IF_DEBUG_EDITION( g_debugRenderer.reset(); )

	Vec2i size(420, 200), pos = g_metrics.getScreenDims() / 2 - size / 2;
	m_chatDialog = new ChatDialog(&g_program);
	m_chatDialog->setPos(pos);
	m_chatDialog->setSize(size);
	m_chatDialog->Button1Clicked.connect(this, &GameState::onChatEntered);
	m_chatDialog->Button2Clicked.connect(this, &GameState::onChatCancel);
	m_chatDialog->Escaped.connect(this, &GameState::onChatCancel);
	m_chatDialog->Close.connect(this, &GameState::onChatCancel);
	m_chatDialog->setVisible(false);

	m_debugPanel = new DebugPanel(static_cast<Container*>(&g_program));
	m_debugPanel->setSize(Vec2i(400, 400));
	m_debugPanel->setPos(Vec2i(25, 250));
	m_debugPanel->setVisible(g_config.getMiscDebugMode());
	m_debugPanel->setButtonText("");
	m_debugPanel->setTitleText("Debug");
	m_debugPanel->setDebugText("");
	m_debugPanel->Close.connect(this, &GameState::toggleDebug);

	int wh = g_widgetConfig.getDefaultItemHeight();

	m_gameMenu = new GameMenu(); 
	m_gameMenu->setVisible(false);

	///@todo StaticText (?) for script message
	m_scriptDisplayPos = Vec2i(175, g_metrics.getScreenH() - 64);

	// init world, and place camera
	simInterface->initWorld();
	gui.init();
	gameCamera.init(g_map.getW(), g_map.getH());
	Vec2i v(g_map.getW() / 2, g_map.getH() / 2);
	if (g_world.getThisFaction()) {  // e.g. -loadmap has no players
		v = g_map.getStartLocation(g_world.getThisFaction()->getStartLocationIndex());
	}
	gameCamera.setPos(Vec2f(float(v.x), float(v.y + 12)));
	if (simInterface->getSavedGame()) {
		gui.load(simInterface->getSavedGame()->getChild("gui"));
	}

	ScriptManager::initGame();

	// weather particle systems
	if (g_world.getTileset()->getWeather() == Weather::RAINY) {
		g_logger.logProgramEvent("Creating rain particle system", true);
		weatherParticleSystem= new RainParticleSystem();
		weatherParticleSystem->setSpeed(12.f / WORLD_FPS);
		weatherParticleSystem->setPos(gameCamera.getPos());
		g_renderer.manageParticleSystem(weatherParticleSystem, ResourceScope::GAME);
	} else if (g_world.getTileset()->getWeather() == Weather::SNOWY) {
		g_logger.logProgramEvent("Creating snow particle system", true);
		weatherParticleSystem= new SnowParticleSystem(1200);
		weatherParticleSystem->setSpeed(1.5f / WORLD_FPS);
		weatherParticleSystem->setPos(gameCamera.getPos());
		weatherParticleSystem->setTexture(g_coreData.getSnowTexture());
		g_renderer.manageParticleSystem(weatherParticleSystem, ResourceScope::GAME);
	}

	// init renderer state
	g_logger.logProgramEvent("Initializing renderer", true);
	g_renderer.initGame(this);

	//IF_DEBUG_EDITION( simInterface->getGaia()->showSpawnPoints(); );

	// sounds
	Tileset *tileset = g_world.getTileset();
	AmbientSounds *ambientSounds = tileset->getAmbientSounds();

	// rain
	if (tileset->getWeather() == Weather::RAINY && ambientSounds->isEnabledRain()) {
		g_logger.logProgramEvent("Starting ambient stream", true);
		g_soundRenderer.playAmbient(ambientSounds->getRain());
	}
	// snow
	if (tileset->getWeather() == Weather::SNOWY && ambientSounds->isEnabledSnow()) {
		g_logger.logProgramEvent("Starting ambient stream", true);
		g_soundRenderer.playAmbient(ambientSounds->getSnow());
	}

	// Launch
	int maxUpdtBacklog = simInterface->launchGame();
	program.setMaxUpdateBacklog(maxUpdtBacklog);

	g_logger.logProgramEvent("Starting music stream", true);
	if (g_world.getThisFaction()) {
		StrSound *gameMusic = g_world.getThisFaction()->getType()->getMusic();
		if (gameMusic) {
			g_soundRenderer.playMusic(gameMusic);
		}
	}
	delete simInterface->getSavedGame();
	g_logger.logProgramEvent("Launching game");
	g_logger.getProgramLog().setLoading(false);
	program.resetTimers();
	program.setFade(1.f);
	m_debugStats.init();
	Debug::g_debugStats = &m_debugStats;
}

// ==================== update ====================

//update
void GameState::update() {
	if (gotoMenu) {
		if (m_modalDialog) {
			program.removeFloatingWidget(m_modalDialog);
			m_modalDialog = 0;
		}
		program.clear();
		program.setState(new BattleEnd(program));
		return;
	}
	if (exitGame) {
		g_simInterface.doQuitGame(QuitSource::LOCAL);
		program.getMouseCursor().initMouse(); // reset to default cursor images
	}
	if (exitProgram) {
		program.clear();
		program.exit();
		return;
	}
	if (netError) {
		return;
	}
	++updateFps;
	mouse2d = (mouse2d + 1) % Renderer::maxMouse2dAnim;

	///@todo clean-up, this should be done in SimulationInterface::updateWorld()
	// but the particle stuff still lives here, so we still need to process each loop here

	try {
		// update simulation
		if (simInterface->updateWorld()) {
			++worldFps;

			// Particle systems
			if (weatherParticleSystem) {
				weatherParticleSystem->setPos(gameCamera.getPos());
			}
			g_renderer.updateParticleManager(ResourceScope::GAME);
		}

		// Gui
		gui.update();

	} catch (Net::NetworkError &e) {
		LOG_NETWORK(e.what());
		displayError(e);
		netError = true;
		return;
	} catch (std::exception &e) {
		displayError(e);
		return;
	}

	// check for quiting status
	if (g_simInterface.getQuit()) {
		quitGame();
	}
}

void GameState::displayError(std::exception &e) {
	simInterface->pause();
	string errMsg(e.what());

	if (m_modalDialog) {
		program.removeFloatingWidget(m_modalDialog);
		m_modalDialog = 0;
	}
	if (m_chatDialog->isVisible()) {
		m_chatDialog->setVisible(false);
	}
	if (m_debugPanel->isVisible()) {
		toggleDebug();
	}
	if (m_gameMenu->isVisible()) {
		toggleGameMenu();
	}
	gui.resetState();
	Vec2i screenDims = g_metrics.getScreenDims();
	Vec2i size(screenDims.x - 200, screenDims.y / 2);
	Vec2i pos = screenDims / 2 - size / 2;
	MessageDialog* dialog = MessageDialog::showDialog(pos, size,
		g_lang.get("Error"), "An error has occurred.\n" + errMsg, g_lang.get("Ok"), "");
	m_modalDialog = dialog;
	dialog->Button1Clicked.connect(this, &GameState::onErrorDismissed);
	dialog->Escaped.connect(this, &GameState::onErrorDismissed);
}

void GameState::onErrorDismissed(Widget*) {
	simInterface->resume();
	program.removeFloatingWidget(m_modalDialog);
	m_modalDialog = 0;
	gotoMenu = true;
}

void GameState::doGameMenu() {
	if (m_modalDialog) {
		g_widgetWindow.removeFloatingWidget(m_modalDialog);
		m_modalDialog = 0;
		return;
	}
	gui.resetState();
	if (m_chatDialog->isVisible()) {
		m_chatDialog->setVisible(false);
	}
	toggleGameMenu();
}

void GameState::doExitMessage(const string &msg) {
	if (m_modalDialog) {
		g_widgetWindow.removeFloatingWidget(m_modalDialog);
		m_modalDialog = 0;
		return;
	}
	if (m_chatDialog->isVisible()) {
		m_chatDialog->setVisible(false);
	}
	gui.resetState();

	Vec2i size = g_widgetConfig.getDefaultDialogSize();
	Vec2i pos = g_metrics.getScreenDims() / 2 - size / 2;
	BasicDialog *dialog = MessageDialog::showDialog(pos, size, g_lang.get("ExitGame?"),
		msg, g_lang.get("Ok"), g_lang.get("Cancel"));
	dialog->Button1Clicked.connect(this, &GameState::onConfirmQuitGame);
	dialog->Button2Clicked.connect(this, &GameState::destroyDialog);
	dialog->Escaped.connect(this, &GameState::destroyDialog);
	dialog->Close.connect(this, &GameState::destroyDialog);
	m_modalDialog = dialog;
}

void GameState::confirmQuitGame() {
	doExitMessage(g_lang.get("ExitGame?"));
}

void GameState::confirmExitProgram() {
	if (m_modalDialog) {
		g_widgetWindow.removeFloatingWidget(m_modalDialog);
		m_modalDialog = 0;
	}
	if (m_chatDialog->isVisible()) {
		m_chatDialog->setVisible(false);
	}
	Vec2i size = g_widgetConfig.getDefaultDialogSize();
	Vec2i pos = g_metrics.getScreenDims() / 2 - size / 2;
	BasicDialog *dialog = MessageDialog::showDialog(pos, size, g_lang.get("ExitProgram?"),
		g_lang.get("ExitProgram?"), g_lang.get("Ok"), g_lang.get("Cancel"));
	dialog->Button1Clicked.connect(this, &GameState::onConfirmExitProgram);
	dialog->Button2Clicked.connect(this, &GameState::destroyDialog);
	dialog->Escaped.connect(this, &GameState::destroyDialog);
	dialog->Close.connect(this, &GameState::destroyDialog);
	m_modalDialog = dialog;
}

void GameState::onConfirmQuitGame(Widget*) {
	exitGame = true;
	program.removeFloatingWidget(m_modalDialog);
	m_modalDialog = 0;
}

void GameState::onConfirmExitProgram(Widget*) {
	exitProgram = true;
	program.removeFloatingWidget(m_modalDialog);
	m_modalDialog = 0;
}

const string allowMask = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";

void GameState::doSaveBox() {
	if (!simInterface->getGameSettings().getScenarioPath().empty()) {
		gui.getRegularConsole()->addLine(g_lang.get("CantSaveScenarios"));
		// msg ?
		return;
	}
	if (m_chatDialog->isVisible()) {
		m_chatDialog->setVisible(false);
	}
	if (m_gameMenu->isVisible()) {
		toggleGameMenu();
	}
	if (m_modalDialog) {
		g_widgetWindow.removeFloatingWidget(m_modalDialog);
		m_modalDialog = 0;
	}
	gui.resetState();
	Vec2i size = g_widgetConfig.getDefaultDialogSize();
	Vec2i pos = g_metrics.getScreenDims() / 2 - size / 2;
	InputDialog* dialog = InputDialog::showDialog(pos, size, g_lang.get("SaveGame"),
		g_lang.get("SelectSaveGame"), g_lang.get("Save"), g_lang.get("Cancel"));
	m_modalDialog = dialog;
	dialog->setInputMask(allowMask);
	dialog->Button1Clicked.connect(this, &GameState::onSaveSelected);
	dialog->Button2Clicked.connect(this, &GameState::destroyDialog);
	dialog->Escaped.connect(this, &GameState::destroyDialog);
	dialog->Close.connect(this, &GameState::destroyDialog);
}

void GameState::onSaveSelected(Widget*) {
	InputDialog* in  = static_cast<InputDialog*>(m_modalDialog);
	string name = in->getInput();

	if (name.empty()) {
		g_console.addLine(g_lang.get("EnterFilename"));
		return;
	}
	saveGame(name);
	program.removeFloatingWidget(m_modalDialog);
	m_modalDialog = 0;
	
	string msg = g_lang.get("YourGameWasSaved");
	string::size_type pos = msg.find("%s");
	if (pos != string::npos) {
		msg.replace(pos, 2, name + ".sav");
	}
	gui.getRegularConsole()->addLine(msg);
}

void GameState::toggleDebug(Widget*) {
	bool v = !m_debugPanel->isVisible();
	m_debugPanel->setVisible(v);
	g_config.setMiscDebugMode(v);
}

void GameState::toggleGameMenu(Widget*) {
	m_gameMenu->setVisible(!m_gameMenu->isVisible());
}

void GameState::toggleOptions(Widget*) {
	if (!m_options) {
		// just-in-time instantiation
		m_options = new OptionsFrame(static_cast<Container*>(&g_program));

		// init
		Vec2i screenDims = g_metrics.getScreenDims();
		Vec2i size(screenDims.x - 200, screenDims.y / 2);
		Vec2i pos = screenDims / 2 - size / 2;
		m_options->init(pos, size, g_lang.get("Options"));

		m_options->setVisible(true);
		m_options->Close.connect(this, &GameState::toggleOptions);
	} else {
		m_options->setVisible(!m_options->isVisible());
	}
}

void GameState::togglePinWidgets(Widget*) {
	bool pin = !g_config.getUiPinWidgets();
	g_config.setUiPinWidgets(pin);
	static_cast<MinimapFrame*>(gui.getMinimap()->getParent())->setPinned(pin);
	static_cast<ResourceBarFrame*>(gui.getResourceBar()->getParent())->setPinned(pin);
	static_cast<DisplayFrame*>(gui.getDisplay()->getParent())->setPinned(pin);
}

void GameState::rejigWidgets() {
	m_gameMenu->init();
	m_options->layoutCells();
}

void GameState::addScriptMessage(const string &header, const string &msg) {
	m_scriptMessages.push_back(ScriptMessage(header, msg));
	if (!m_modalDialog) {
		doScriptMessage();
	}
}

void GameState::setScriptDisplay(const string &msg) {
	m_scriptDisplay = msg;
	if (!msg.empty()) {
		const FontMetrics *fm = g_widgetConfig.getMenuFont()->getMetrics();
		int space = g_metrics.getScreenW() - 175 - 320;
		fm->wrapText(m_scriptDisplay, space);
		int lines = 1;
		foreach (string, c, m_scriptDisplay) {
			if (*c == '\n') {
				++lines;
			}
		}
		m_scriptDisplayPos.y = g_metrics.getScreenH() - 64 - int(fm->getHeight() * (lines - 1));
	}
}

void GameState::doScriptMessage() {
	assert(!m_scriptMessages.empty());
	Vec2i size = g_widgetConfig.getDefaultDialogSize();
	Vec2i pos = g_metrics.getScreenDims() / 2 - size / 2;
	MessageDialog* dialog = MessageDialog::showDialog(pos, size,
		g_lang.getScenarioString(m_scriptMessages.front().header),
		g_lang.getScenarioString(m_scriptMessages.front().text), g_lang.get("Ok"), "");
	m_modalDialog = dialog;
	dialog->Button1Clicked.connect(this, &GameState::destroyDialog);
	dialog->Escaped.connect(this, &GameState::destroyDialog);
	m_scriptMessages.pop_front();
}

void GameState::doChatDialog() {
	if (m_gameMenu->isVisible()) {
		toggleGameMenu();
	}
	gui.resetState();
	if (m_chatDialog->isVisible()) {
		m_chatDialog->setVisible(false);
	} else {
		m_chatDialog->setVisible(true);
	}
}

//void GameState::updateChatDialog() {
//	if (m_modalDialog) {
//		m_modalDialog->setTeamChat(m_teamChat);
//	}
//}

void GameState::onChatEntered(Widget* ptr) {
	string txt = m_chatDialog->getInput();
	bool isTeamOnly = m_chatDialog->isTeamOnly();
	int team = (isTeamOnly ? g_world.getThisFaction()->getTeam() : -1);
	NetworkInterface *netInterface = simInterface->asNetworkInterface();
	if (netInterface) {
		netInterface->sendTextMessage(txt, team);
	} else {
		int ndx = g_world.getThisFactionIndex();
		const GameSettings &gs = simInterface->getGameSettings();
		string player = gs.getPlayerName(ndx);
		Colour colour = factionColours[gs.getColourIndex(ndx)];
		gui.getDialogConsole()->addDialog(player + ": ", colour, txt);
	}
	m_chatDialog->clearText();
	m_chatDialog->setVisible(false);
}

void GameState::onChatCancel(Widget*) {
	m_chatDialog->setVisible(false);
}

void GameState::destroyDialog(Widget*) {
	if (m_modalDialog) {
		program.removeFloatingWidget(m_modalDialog);
		m_modalDialog = 0;
		if (!m_scriptMessages.empty()) {
			doScriptMessage();
		}
	}
}

void GameState::doDefeatedMessage(Faction *f) {
	string player = "[" + f->getName() + "] ";
	string msg = g_lang.getDefeatedMessage();

	string::size_type n = msg.find("%s");
	while (n != string::npos) {
		string start = msg.substr(0, n);
		string end = msg.substr(n + 2);
		msg = start + f->getName() + end;
		n = msg.find("%s");
	}
	n = msg.find("%i");
	while (n != string::npos) {
		string start = msg.substr(0, n);
		string end = msg.substr(n + 2);
		msg = start + intToStr(f->getTeam()) + end;
		n = msg.find("%i");
	}

	gui.getDialogConsole()->addDialog(player, factionColours[f->getColourIndex()], msg, false);
}

void GameState::updateCamera() {
	gameCamera.update();
}

// ==================== render ====================

//render
void GameState::renderBg(){
	SECTION_TIMER(RENDER_3D);
	renderFps++;
	render3d();
}

void GameState::renderFg(){
	SECTION_TIMER(RENDER_SWAP_BUFFERS);
	render2d();
	Renderer::getInstance().swapBuffers();
}

// ==================== tick ====================

void GameState::tick() {
	lastWorldFps = worldFps;
	lastUpdateFps = updateFps;
	lastRenderFps = renderFps;
	worldFps = 0;
	updateFps = 0;
	renderFps = 0;

	if (netError) return;

	m_debugStats.tick(lastRenderFps, lastWorldFps);
	stringstream ss;
	m_debugStats.report(ss);
	m_debugPanel->setDebugText(ss.str());

	// Win/lose check
	GameStatus status = simInterface->checkWinner();
	if (status == GameStatus::WON) {
		showWinMessageBox();
	} else if (status == GameStatus::LOST) {
		showLoseMessageBox();
	}
	gui.tick();
}

// ==================== events ====================

void GameState::mouseDownLeft(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");
	if (!noInput) {
		gui.mouseDownLeft(x, y);
	}
}

void GameState::mouseDownRight(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");
	if (!noInput) {
		gui.mouseDownRight(x, y);
		m_cameraDragCenter.x = float(x);
		m_cameraDragCenter.y = float(y);
	}
}

void GameState::mouseUpLeft(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");
	if (!noInput) {
		gui.mouseUpLeft(x, y);
	}
}
void GameState::mouseUpRight(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");
	g_program.getMouseCursor().setAppearance(MouseAppearance::DEFAULT);
	if (!noInput) {
		gui.mouseUpRight(x, y);
	}
	//if (gameCamera.isMoving()) {
	//	// stop moving if button is released
	//	gameCamera.setMoveZ(0, true);
	//	gameCamera.setMoveX(0, true);
	//}
}

void GameState::mouseDoubleClickLeft(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");
	if (!noInput) {
		gui.mouseDoubleClickLeft(x, y);
	}
}

void GameState::mouseMove(int x, int y, const MouseState &ms) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");
	mouseX = x;
	mouseY = y;

	if (ms.get(MouseButton::MIDDLE)) {
		if (!noInput) {
			if (input.isCtrlDown()) {
				float speed = input.isShiftDown() ? 1.f : 0.125f;
				float response = input.isShiftDown() ? 0.1875f : 0.0625f;
				gameCamera.moveForwardH(-(y - lastMousePos.y) * speed, response);
				gameCamera.moveSideH((x - lastMousePos.x) * speed, response);
			} else {
				float ymult = g_config.getCameraInvertYAxis() ? -0.2f : 0.2f;
				float xmult = g_config.getCameraInvertXAxis() ? -0.2f : 0.2f;
				gameCamera.transitionVH((y - lastMousePos.y) * ymult, (lastMousePos.x - x) * xmult);
			}
		}
	}/* Disabled for now because people don't like it - hailstone 08April2011
	 http://glest.org/glest_board/index.php?topic=6757.msg69999#msg69999
	 else if (ms.get(MouseButton::RIGHT)) {
		if (!noInput) {
			Vec2f moveVec = Vec2f(Vec2i(x, y)) - m_cameraDragCenter;
			if (moveVec.length() > 50) { /// @todo add to config if this is staying - hailstone 11Feb2011
				moveVec.normalize();
				g_program.getMouseCursor().setAppearance(MouseAppearance::MOVE_FREE);
				gameCamera.setMoveZ(moveVec.y * -scrollSpeed, true);
				gameCamera.setMoveX(moveVec.x * scrollSpeed, true);
			}
		}
	}*/ else {
		if (!noInput) {
			// main window
			if (g_config.getUiMoveCameraAtScreenEdge()) { // move camera at screen edges ?
				if (y < 10) {
					gameCamera.setMoveZ(scrollSpeed, true);
				} else if (y > g_metrics.getScreenH() - 10) {
					gameCamera.setMoveZ(-scrollSpeed, true);
				} else {
					gameCamera.setMoveZ(0, true);
				}

				if (x < 10) {
					gameCamera.setMoveX(-scrollSpeed, true);
				} else if (x > g_metrics.getScreenW() - 10) {
					gameCamera.setMoveX(scrollSpeed, true);
				} else {
					gameCamera.setMoveX(0, true);
				}
			}
		}
		if (!noInput) { // graphics
			gui.mouseMove(x, y);
		}
	}
	lastMousePos.x = x;
	lastMousePos.y = y;
}

void GameState::eventMouseWheel(int x, int y, int zDelta) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << ", " << zDelta << " )");
	if (!noInput) {
		gameCamera.zoom(zDelta / 30.f);
	}
}

void GameState::keyDown(const Key &key) {
	WIDGET_LOG( __FUNCTION__ << "( " << Key::getName(KeyCode(key)) << " )");
	UserCommand cmd = keymap.getCommand(key);
	bool speedChangesAllowed = !g_simInterface.isNetworkInterface();

	if (g_config.getMiscDebugKeys()) {
		stringstream str;
		HotKey e(key.getCode(), keymap.getCurrentMods());
		str << e.toString();
		if (cmd != UserCommand::NONE) {
			str << " = " << Keymap::getCommandName(cmd);
		}
		gui.getRegularConsole()->addLine(str.str());
	}
	if (cmd == UserCommand::CYCLE_SHADERS) {
		g_renderer.cycleShaders();
	} else if (cmd == UserCommand::CHAT_AUDIENCE_ALL) {
		m_chatDialog->setTeamOnly(false);
	} else if (cmd == UserCommand::CHAT_AUDIENCE_TEAM) {
		m_chatDialog->setTeamOnly(true);
	} else if (cmd == UserCommand::CHAT_AUDIENCE_TOGGLE) {
		m_chatDialog->toggleTeamOnly();
	} else if (cmd == UserCommand::SHOW_CHAT_DIALOG) {
		doChatDialog();
	} else if (cmd == UserCommand::SAVE_SCREENSHOT) {
		int i;
		const int MAX_SCREENSHOTS = 100;

		// Find a filename from 'screen(0 to MAX_SCREENHOTS).tga' and save the screenshot in one that doesn't
		//  already exist.
		for (i = 0; i < MAX_SCREENSHOTS; ++i) {
			string path = "screens/screen" + intToStr(i) + ".tga";

			if (!fileExists(path)) {
				g_renderer.saveScreen(path);
				break;
			}
		}
		if (i > MAX_SCREENSHOTS) {
			gui.getRegularConsole()->addLine(g_lang.get("ScreenshotDirectoryFull"));
		}
	} else if (cmd == UserCommand::MOVE_CAMERA_LEFT) { // move camera left
		gameCamera.setMoveX(-scrollSpeed, false);
	} else if (cmd == UserCommand::MOVE_CAMERA_RIGHT) { // move camera right
		gameCamera.setMoveX(scrollSpeed, false);
	} else if (cmd == UserCommand::MOVE_CAMERA_UP) { // move camera up
		gameCamera.setMoveZ(scrollSpeed, false);
	} else if (cmd == UserCommand::MOVE_CAMERA_DOWN) { // move camera down
		gameCamera.setMoveZ(-scrollSpeed, false);
	} else if (cmd == UserCommand::ROTATE_CAMERA_LEFT) { // rotate camera left
		gameCamera.setRotate(-1);
	} else if (cmd == UserCommand::ROTATE_CAMERA_RIGHT) { // rotate camera right
		gameCamera.setRotate(1);
	} else if (cmd == UserCommand::ZOOM_CAMERA_IN) { // camera zoom in
		gameCamera.zoom(15.f);
	} else if (cmd == UserCommand::ZOOM_CAMERA_OUT) { // camera zoom out
		gameCamera.zoom(-15.f);
	} else if (cmd == UserCommand::PITCH_CAMERA_UP) { // camera pitch up
		gameCamera.setMoveY(1);
	} else if ( cmd == UserCommand::PITCH_CAMERA_DOWN) { // camera pitch down
		gameCamera.setMoveY(-1);
	} else if (cmd == UserCommand::CAMERA_RESET) { // reset camera height & angle
		gameCamera.reset();
	} else if (cmd == UserCommand::CAMERA_RESET_ZOOM) { // reset camera height
		gameCamera.reset(false, true);
	} else if (cmd == UserCommand::CAMERA_RESET_ANGLE) { // reset camera angle
		gameCamera.reset(true, false);
	} else if (speedChangesAllowed) {
		GameSpeed curSpeed = simInterface->getSpeed();
		GameSpeed newSpeed = curSpeed;
		if (cmd == UserCommand::PAUSE_GAME) { // pause on
			newSpeed = simInterface->pause();
		} else if (cmd == UserCommand::RESUME_GAME) { // pause off
			newSpeed = simInterface->resume();
		} else if (cmd == UserCommand::TOGGLE_PAUSE) { // toggle
			if (curSpeed == GameSpeed::PAUSED) {
				newSpeed = simInterface->resume();
			} else {
				newSpeed = simInterface->pause();
			}
		} else if (cmd == UserCommand::INC_SPEED) { // increment speed
			newSpeed = simInterface->incSpeed();
		} else if (cmd == UserCommand::DEC_SPEED) { // decrement speed
			newSpeed = simInterface->decSpeed();
		} else if (cmd == UserCommand::RESET_SPEED) { // reset speed
			newSpeed = simInterface->resetSpeed();
		}
		if (curSpeed != newSpeed) {
			if (newSpeed == GameSpeed::PAUSED) {
				gui.getRegularConsole()->addLine(g_lang.get("GamePaused"));
			} else if (curSpeed == GameSpeed::PAUSED) {
				gui.getRegularConsole()->addLine(g_lang.get("GameResumed"));
			} else {
				gui.getRegularConsole()->addLine(g_lang.get("GameSpeedSet") + " " + g_lang.get(GameSpeedNames[newSpeed]));
			}
			return;
		}
	}
	if (cmd == UserCommand::QUIT_GAME) { // exit
		if (!gui.cancelPending()) {
			doGameMenu();
		}
	} else if (cmd == UserCommand::SAVE_GAME) { // save
		if (!m_modalDialog) {
			doSaveBox();
		}
	} else if (key.getCode() >= KeyCode::ZERO && key.getCode() < KeyCode::ZERO + Selection::maxGroups) { // group
		gui.groupKey(key.getCode() - KeyCode::ZERO);
	} else { // hotkeys
		gui.hotKey(cmd);
	}
}

void GameState::keyUp(const Key &key) {
	WIDGET_LOG( __FUNCTION__ << "( " << Key::getName(KeyCode(key)) << " )");
	if (key.isModifier()) {
		gameCamera.setRotate(0.f);
		gameCamera.setMoveX(0.f, false);
		gameCamera.setMoveY(0.f);
		gameCamera.setMoveZ(0.f, false);
	} else {
		switch (keymap.getCommand(key)) {
			case UserCommand::ROTATE_CAMERA_LEFT: case UserCommand::ROTATE_CAMERA_RIGHT:
				gameCamera.setRotate(0.f);
				break;
			case UserCommand::PITCH_CAMERA_UP: case UserCommand::PITCH_CAMERA_DOWN:
				gameCamera.setMoveY(0.f);
				break;
			case UserCommand::MOVE_CAMERA_UP: case UserCommand::MOVE_CAMERA_DOWN:
				gameCamera.setMoveZ(0.f, false);
				break;
			case UserCommand::MOVE_CAMERA_LEFT: case UserCommand::MOVE_CAMERA_RIGHT:
				gameCamera.setMoveX(0.f, false);
				break;
		}
	}
}

void GameState::keyPress(char c) {
	WIDGET_LOG( __FUNCTION__ << "( '" << c << "' )");
}

void GameState::quitGame() {
	gotoMenu = true;
}

// ==================== PRIVATE ====================

// ==================== render ====================

void GameState::render3d(){
	_PROFILE_FUNCTION();
	Renderer &renderer = g_renderer;

	//init
	renderer.reset3d();

	renderer.loadGameCameraMatrix();
	renderer.computeVisibleArea();
	renderer.setupLighting();

	//shadow map
	renderer.renderShadowsToTexture();

	//clear buffers
	renderer.clearBuffers();

	//surface
	renderer.renderSurface();

	//selection circles
	renderer.renderSelectionEffects();

	//units
	renderer.renderUnits();

	//objects
	renderer.renderObjects();

	//water
	renderer.renderWater();
	renderer.renderWaterEffects();

	//particles
	renderer.renderParticleManager(ResourceScope::GAME);

	//mouse 3d
	renderer.renderMouse3d();
}

void GameState::render2d(){
	_PROFILE_FUNCTION();

	// init
	g_renderer.reset2d();

	// selection box
	g_renderer.renderSelectionQuad();

	// script display text
	if (!m_scriptDisplay.empty() && !m_modalDialog) {
		///@todo put on widget
		//g_renderer.renderText(m_scriptDisplay, g_widgetConfig.getMenuFont()[FontSize::NORMAL];,
		//	gui.getDisplay()->getColor(), m_scriptDisplayPos.x, m_scriptDisplayPos.y, false);
	}
}


// ==================== misc ====================


void GameState::showLoseMessageBox() {
	doExitMessage(g_lang.get("YouLose") + ", " + g_lang.get("ExitGame?"));
}

void GameState::showWinMessageBox() {
	doExitMessage(g_lang.get("YouWin") + ", " + g_lang.get("ExitGame?"));
}

void GameState::saveGame(string name) const {
	XmlNode root("saved-game");
	root.addAttribute("version", GameConstants::saveGameVersion);
	gui.save(root.addChild("gui"));
	g_simInterface.getGameSettings().save(root.addChild("settings"));
	simInterface->getWorld()->save(root.addChild("world"));
	XmlIo::getInstance().save("savegames/" + name + ".sav", &root);
}

// =====================================================
//  class ShowMap
// =====================================================

void ShowMap::render2d(){
}

void ShowMap::keyDown(const Key &key) {
	UserCommand cmd = keymap.getCommand(key);

	if (cmd == UserCommand::SAVE_SCREENSHOT) {
		int i;
		const int MAX_SCREENSHOTS = 100;

		// Find a filename from 'screen(0 to MAX_SCREENHOTS).tga' and save the screenshot in one that doesn't
		//  already exist.
		for (i = 0; i < MAX_SCREENSHOTS; ++i) {
			string path = "screens/screen" + intToStr(i) + ".tga";

			if(fileExists(path)){
				g_renderer.saveScreen(path);
				break;
			}
		}
		if (i > MAX_SCREENSHOTS) {
			gui.getRegularConsole()->addLine(g_lang.get("ScreenshotDirectoryFull"));
		}
	} else if (cmd == UserCommand::MOVE_CAMERA_LEFT) { // move camera left
		gameCamera.setMoveX(-scrollSpeed, false);
	} else if (cmd == UserCommand::MOVE_CAMERA_RIGHT) { // move camera right
		gameCamera.setMoveX(scrollSpeed, false);
	} else if (cmd == UserCommand::MOVE_CAMERA_UP) { // move camera up
		gameCamera.setMoveZ(scrollSpeed, false);
	} else if (cmd == UserCommand::MOVE_CAMERA_DOWN) { // move camera down
		gameCamera.setMoveZ(-scrollSpeed, false);
	} else if (cmd == UserCommand::CAMERA_RESET) { // reset camera height & angle
		gameCamera.reset();
	} else if (cmd == UserCommand::CAMERA_RESET_ZOOM) { // reset camera height
		gameCamera.reset(false, true);
	} else if (cmd == UserCommand::CAMERA_RESET_ANGLE) { // reset camera angle
		gameCamera.reset(true, false);
	} else if (cmd == UserCommand::QUIT_GAME) {
		if (!gui.cancelPending()) {
			doExitMessage(g_lang.get("ExitGame?"));
		}
	} else {
		switch (cmd) {
			// rotate camera left
			case UserCommand::ROTATE_CAMERA_LEFT:
				gameCamera.setRotate(-1);
				break;
			// rotate camera right
			case UserCommand::ROTATE_CAMERA_RIGHT:
				gameCamera.setRotate(1);
				break;
			// camera up
			case UserCommand::PITCH_CAMERA_UP:
				gameCamera.setMoveY(1);
				break;
			// camera down
			case UserCommand::PITCH_CAMERA_DOWN:
				gameCamera.setMoveY(-1);
				break;
			default:
				break;
		}
	}
}


}}//end namespace
