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

#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif

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

const GameSettings &GameState::getGameSettings()	{return g_simInterface->getGameSettings();}

GameState::GameState(Program &program)
		//main data
		: ProgramState(program)
		, simInterface(program.getSimulationInterface())
		, keymap(program.getKeymap())
		, input(program.getInput())
		, config(Config::getInstance())
		, gui(*this)
		, gameCamera()
		, console()
		, chatManager(simInterface, keymap)
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
		, scrollSpeed(config.getUiScrollSpeed())
		, m_msgBox(0)
		, m_scriptMsgBox(0)
		, saveBox(NULL)
		, lastMousePos(0)
		, weatherParticleSystem(0) {
	assert(!singleton);
	singleton = this;
	simInterface->constructGameWorld(this);
}

int GameState::getUpdateInterval() const {
	return simInterface->getUpdateInterval();
}

GameState::~GameState() {
	g_logger.setState(g_lang.get("Deleting"));
	g_logger.add("~GameState", !program.isTerminating());

	g_renderer.endGame();
	weatherParticleSystem = 0;
	g_soundRenderer.stopAllSounds();

	delete saveBox;
	saveBox = 0;

	gui.end(); //selection must be cleared before deleting units
	singleton = 0;
	g_logger.setLoading(true);

	// reset max update backlog, to prevent super-speed in menus
	program.setMaxUpdateBacklog(12);

	// delete the World
	simInterface->destroyGameWorld();
}


// ==================== init and load ====================

void GameState::load() {
	GameSettings &gameSettings = g_simInterface->getGameSettings();
	const string &mapName = gameSettings.getMapPath();
	const string &tilesetName = gameSettings.getTilesetPath();
	const string &techName = gameSettings.getTechPath();
	const string &scenarioPath = gameSettings.getScenarioPath();
	string scenarioName = basename(scenarioPath);

	GraphicProgressBar progressBar;
	progressBar.init(345, 550, 300, 20);
	g_logger.setProgressBar(&progressBar);

	g_logger.setState(Lang::getInstance().get("Loading"));

	if (scenarioName.empty()) {
		g_logger.setSubtitle(formatString(mapName) + " - " + 
			formatString(tilesetName) + " - " + formatString(techName));
	} else {
		g_logger.setSubtitle(formatString(scenarioName));
	}

	simInterface->loadWorld();

	// finished loading
	progressBar.setProgress(100);
	g_logger.setProgressBar(NULL);
}

void GameState::init() {
	g_logger.setState(g_lang.get("Initializing"));

	IF_DEBUG_EDITION( g_debugRenderer.reset(); )

	// init world, and place camera
	simInterface->initWorld();
	gui.init();
	//REFACTOR: ThisTeamIndex belongs in here, not the World
	chatManager.init(&g_console, g_world.getThisTeamIndex());
	gameCamera.init(g_map.getW(), g_map.getH());
	Vec2i v(g_map.getW() / 2, g_map.getH() / 2);
	if (g_world.getThisFaction()) {  //e.g. -loadmap has no players
		v = g_map.getStartLocation(g_world.getThisFaction()->getStartLocationIndex());
	}
	gameCamera.setPos(Vec2f((float)v.x, (float)v.y));
	if (simInterface->getSavedGame()) {
		gui.load(simInterface->getSavedGame()->getChild("gui"));
	}

	ScriptManager::initGame();

	// weather particle systems
	if (g_world.getTileset()->getWeather() == Weather::RAINY) {
		g_logger.add("Creating rain particle system", true);
		weatherParticleSystem= new RainParticleSystem();
		weatherParticleSystem->setSpeed(12.f / WORLD_FPS);
		weatherParticleSystem->setPos(gameCamera.getPos());
		g_renderer.manageParticleSystem(weatherParticleSystem, ResourceScope::GAME);
	} else if (g_world.getTileset()->getWeather() == Weather::SNOWY) {
		g_logger.add("Creating snow particle system", true);
		weatherParticleSystem= new SnowParticleSystem(1200);
		weatherParticleSystem->setSpeed(1.5f / WORLD_FPS);
		weatherParticleSystem->setPos(gameCamera.getPos());
		weatherParticleSystem->setTexture(g_coreData.getSnowTexture());
		g_renderer.manageParticleSystem(weatherParticleSystem, ResourceScope::GAME);
	}

	// init renderer state
	g_logger.add("Initializing renderer", true);
	g_renderer.initGame(this);

	//IF_DEBUG_EDITION( simInterface->getGaia()->showSpawnPoints(); );

	// sounds
	Tileset *tileset = g_world.getTileset();
	const TechTree *techTree = g_world.getTechTree();
	AmbientSounds *ambientSounds = tileset->getAmbientSounds();

	// rain
	if (tileset->getWeather() == Weather::RAINY && ambientSounds->isEnabledRain()) {
		g_logger.add("Starting ambient stream", true);
		g_soundRenderer.playAmbient(ambientSounds->getRain());
	}
	// snow
	if (tileset->getWeather() == Weather::SNOWY && ambientSounds->isEnabledSnow()) {
		g_logger.add("Starting ambient stream", true);
		g_soundRenderer.playAmbient(ambientSounds->getSnow());
	}

	// Launch
	int maxUpdtBacklog = simInterface->launchGame();
	program.setMaxUpdateBacklog(maxUpdtBacklog);

	g_logger.add("Starting music stream", true);
	if (g_world.getThisFaction()) {
		StrSound *gameMusic = g_world.getThisFaction()->getType()->getMusic();
		if (gameMusic) {
			g_soundRenderer.playMusic(gameMusic);
		}
	}
	g_logger.add("Launching game");
	g_logger.setLoading(false);
	program.resetTimers();
	program.setFade(1.f);
}


// ==================== update ====================

//update
void GameState::update() {
	if (gotoMenu) {
		program.setState(new BattleEnd(program));
		//program.setState(new MainMenu(program));
		return;
	}
	if (exitGame) {
		g_simInterface->doQuitGame(QuitSource::LOCAL);
	}
	if (netError) {
		return;
	}
	++updateFps;
	mouse2d = (mouse2d + 1) % Renderer::maxMouse2dAnim;

	//console
	console.update();

	///@todo clean-up, this should be done in SimulationInterface::updateWorld()
	// but the particle stuff still lives here, so we still need to process each loop here

	try {
		// update simulation
		simInterface->updateWorld();
		++worldFps;

		// Gui
		gui.update();

		// Particle systems
		if (weatherParticleSystem) {
			weatherParticleSystem->setPos(gameCamera.getPos());
		}
		g_renderer.updateParticleManager(ResourceScope::GAME);

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
	if (g_simInterface->getQuit()) {
		quitGame();
	}

	// update auto test
	if (g_config.getMiscAutoTest()) {
		AutoTest::getInstance().updateGame(this);
	}
}

void GameState::displayError(std::exception &e) {
	simInterface->pause();
	string errMsg(e.what());

	if (m_msgBox) {
		program.removeFloatingWidget(m_msgBox);
		m_msgBox = 0;
	}
	if (m_scriptMsgBox) {
		program.removeFloatingWidget(m_scriptMsgBox);
		m_scriptMsgBox = 0;
	}
	m_msgBox = new MessageDialog(&program);
	program.setFloatingWidget(m_msgBox, true);
	Vec2i pos, size(320, 200);
	pos = g_metrics.getScreenDims() / 2 - size / 2;
	m_msgBox->setPos(pos);
	m_msgBox->setSize(size);
	m_msgBox->setTitleText("Error...");///@todo localise
	m_msgBox->setMessageText("An error has occurred.\n" + errMsg);
	m_msgBox->setButtonText(g_lang.get("Ok"));
	m_msgBox->Button1Clicked.connect(this, &GameState::onErrorDismissed);
}

void GameState::onErrorDismissed(MessageDialog::Ptr) {
	simInterface->resume();
	program.removeFloatingWidget(m_msgBox);
	m_msgBox = 0;
	gotoMenu = true;
}

void GameState::doExitMessage(const string &msg) {
	assert(!m_msgBox);
	if (m_scriptMsgBox) {
		program.removeFloatingWidget(m_scriptMsgBox);
		m_scriptMessages.push_front(
			ScriptMessage(m_scriptMsgBox->getTitleText(), m_scriptMsgBox->getMessageText()));
		m_scriptMsgBox = 0;
	}
	m_msgBox = new MessageDialog(&program);
	program.setFloatingWidget(m_msgBox, true);
	Vec2i pos, size(320, 200);
	pos = g_metrics.getScreenDims() / 2 - size / 2;
	m_msgBox->setPos(pos);
	m_msgBox->setSize(size);
	m_msgBox->setTitleText(g_lang.get("ExitGame?"));
	m_msgBox->setMessageText(msg);
	m_msgBox->setButtonText(g_lang.get("Ok"), g_lang.get("Cancel"));
	m_msgBox->Button1Clicked.connect(this, &GameState::onExitSelected);
	m_msgBox->Button2Clicked.connect(this, &GameState::onExitCancel);
}

void GameState::onExitSelected(MessageDialog::Ptr) {
	exitGame = true;
	program.removeFloatingWidget(m_msgBox);
	m_msgBox = 0;
}

void GameState::onExitCancel(MessageDialog::Ptr) {
	program.removeFloatingWidget(m_msgBox);
	m_msgBox = 0;
	if (!m_scriptMessages.empty()) {
		doScriptMessage();
	}
}

void GameState::addScriptMessage(const string &header, const string &msg) {
	m_scriptMessages.push_back(ScriptMessage(header, msg));
	if (!m_scriptMsgBox && !m_msgBox) {
		doScriptMessage();
	}
}

void GameState::doScriptMessage() {
	assert(!m_scriptMessages.empty());
	m_scriptMsgBox = new MessageDialog(&program);
	program.setFloatingWidget(m_scriptMsgBox, true);
	Vec2i pos, size(320, 200);
	pos = g_metrics.getScreenDims() / 2 - size / 2;
	m_scriptMsgBox->setPos(pos);
	m_scriptMsgBox->setSize(size);
	m_scriptMsgBox->setTitleText(g_lang.getScenarioString(m_scriptMessages.front().header));
	m_scriptMsgBox->setMessageText(g_lang.getScenarioString(m_scriptMessages.front().text));
	m_scriptMsgBox->setButtonText(g_lang.get("Ok"));
	m_scriptMsgBox->Button1Clicked.connect(this, &GameState::onScriptMessageDismissed);
	m_scriptMessages.pop_front();
}

void GameState::onScriptMessageDismissed(MessageDialog::Ptr) {
	program.removeFloatingWidget(m_scriptMsgBox);
	m_scriptMsgBox = 0;
	if (!m_scriptMessages.empty() && !m_msgBox) {
		doScriptMessage();
	}
}

void GameState::updateCamera() {
	gameCamera.update();
}

// ==================== render ====================

//render
void GameState::renderBg(){
	renderFps++;
	render3d();
}

void GameState::renderFg(){
	render2d();
	Renderer::getInstance().swapBuffers();
}

// ==================== tick ====================

void GameState::tick(){
	lastWorldFps = worldFps;
	lastUpdateFps= updateFps;
	lastRenderFps= renderFps;
	worldFps = 0;
	updateFps= 0;
	renderFps= 0;

	if (netError) return;

	//Win/lose check
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
	bool messageBoxClick = false;
	//save box
	if (saveBox) {
		int button;
		if (saveBox->mouseClick(x, y, button)) {
			if (button == 1) {
				saveGame(saveBox->getEntry()->getText());
			}
			//close message box
			delete saveBox;
			saveBox = NULL;
		}

	} else if (!noInput) {
		gui.mouseDownLeft(x, y);
	}
}

void GameState::mouseDoubleClickLeft(int x, int y) {
	if ( noInput ) return;
	if (!(saveBox && saveBox->isInBounds(x, y))) {
		gui.mouseDoubleClickLeft(x, y);
	}
}

void GameState::mouseMove(int x, int y, const MouseState &ms) {
	mouseX = x;
	mouseY = y;

	if (ms.get(MouseButton::MIDDLE)) {
		if (!noInput) {
			if (input.isCtrlDown()) {
				float speed = input.isShiftDown() ? 1.f : 0.125f;
				float response = input.isShiftDown() ? 0.1875f : 0.0625f;
				gameCamera.moveForwardH((y - lastMousePos.y) * speed, response);
				gameCamera.moveSideH((x - lastMousePos.x) * speed, response);
			} else {
				float ymult = Config::getInstance().getCameraInvertYAxis() ? -0.2f : 0.2f;
				float xmult = Config::getInstance().getCameraInvertXAxis() ? -0.2f : 0.2f;
				gameCamera.transitionVH(-(y - lastMousePos.y) * ymult, (lastMousePos.x - x) * xmult);
			}
		}
	} else {
		if (!noInput) {
			//main window
			if (y < 10) {
				gameCamera.setMoveZ(-scrollSpeed, true);
			} else if (y > g_metrics.getVirtualH() - 10) {
				gameCamera.setMoveZ(scrollSpeed, true);
			} else { //if(y < 20 || y > metrics.getVirtualH()-20){
				gameCamera.setMoveZ(0, true);
			}

			if (x < 10) {
				gameCamera.setMoveX(-scrollSpeed, true);
			} else if (x > g_metrics.getVirtualW() - 10) {
				gameCamera.setMoveX(scrollSpeed, true);
			} else { //if(x < 20 || x > metrics.getVirtualW()-20){
				gameCamera.setMoveX(0, true);
			}
		}

		if (saveBox) {
			saveBox->mouseMove(x, y);
		} else if (!noInput) {
			//graphics
			gui.mouseMove(x, y);
		}
	}
	lastMousePos.x = x;
	lastMousePos.y = y;
}

void GameState::eventMouseWheel(int x, int y, int zDelta) {
	if (noInput) return;
	gameCamera.zoom(zDelta / 30.f);
}

void GameState::keyDown(const Key &key) {
	UserCommand cmd = keymap.getCommand(key);
	bool speedChangesAllowed = !g_simInterface->isNetworkInterface();

	if (g_config.getMiscDebugKeys()) {
		stringstream str;
		Keymap::Entry e(key.getCode(), keymap.getCurrentMods());
		str << e.toString();
		if (cmd != ucNone) {
			str << " = " << Keymap::getCommandName(cmd);
		}
		console.addLine(str.str());
	}
	if (saveBox && saveBox->getEntry()->isActivated()) {
		switch (key.getCode()) {
			case KeyCode::RETURN:
				saveGame(saveBox->getEntry()->getText());
				// intentional fall-through
			case KeyCode::ESCAPE:
				delete saveBox;
				saveBox = NULL;
				break;

			default:
				saveBox->keyDown(key);
		};
		return;

	}
	if (chatManager.keyDown(key)) {
		return; // key consumed, we're done here
	}
	if (cmd == ucSaveScreenshot) {
		Shared::Platform::mkdir("screens", true);
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
			console.addLine(g_lang.get("ScreenshotDirectoryFull"));
		}
	} else if (cmd == ucCameraPosLeft) { // move camera left
		gameCamera.setMoveX(-scrollSpeed, false);
	} else if (cmd == ucCameraPosRight) { // move camera right
		gameCamera.setMoveX(scrollSpeed, false);
	} else if (cmd == ucCameraPosUp) { // move camera up
		gameCamera.setMoveZ(scrollSpeed, false);
	} else if (cmd == ucCameraPosDown) { // move camera down
		gameCamera.setMoveZ(-scrollSpeed, false);
	} else if (cmd == ucCameraRotateLeft) { // rotate camera left
		gameCamera.setRotate(-1);
	} else if (cmd == ucCameraRotateRight ) { // rotate camera right
		gameCamera.setRotate(1);
	} else if (cmd == ucCameraPitchUp) { // camera pitch up
		gameCamera.setMoveY(1);
	} else if ( cmd == ucCameraPitchDown) { // camera pitch down
		gameCamera.setMoveY(-1);
	} else if (cmd == ucCycleDisplayColor) { // switch display color
		gui.switchToNextDisplayColor();
	} else if (cmd == ucCameraCycleMode) { // reset camera pos & angle
		gameCamera.switchState();
	} else if (speedChangesAllowed) { // pause
		GameSpeed curSpeed = simInterface->getSpeed();
		GameSpeed newSpeed = curSpeed;
		if (cmd == ucPauseOn) { // on
			newSpeed = simInterface->pause();
			//paused = true;
		} else if (cmd == ucPauseOff) { // off
			newSpeed = simInterface->resume();
			//paused = false;
		} else if (cmd == ucPauseToggle) { // toggle
			if (curSpeed == GameSpeed::PAUSED) {
				newSpeed = simInterface->resume();
			} else {
				newSpeed = simInterface->pause();
			}
		//increment speed
		} else if (cmd == ucSpeedInc) {
			newSpeed = simInterface->incSpeed();
		} else if (cmd == ucSpeedDec) { // decrement speed
			newSpeed = simInterface->decSpeed();
		// reset speed
		} else if (cmd == ucSpeedReset) {
			newSpeed = simInterface->resetSpeed();
		}
		if (curSpeed != newSpeed) {
			if (newSpeed == GameSpeed::PAUSED) {
				console.addLine(g_lang.get("GamePaused"));
			} else if (curSpeed == GameSpeed::PAUSED) {
				console.addLine(g_lang.get("GameResumed"));
			} else {
				console.addLine(g_lang.get("GameSpeedSet") + " " + g_lang.get(GameSpeedNames[newSpeed]));
			}
			return;
		}
	}
	if (cmd == ucMenuQuit) { // exit
		if (!gui.cancelPending()) {
			doExitMessage(g_lang.get("ExitGame?"));
		}
	} else if (cmd == ucMenuSave) { // save
		if (!saveBox) {
			Shared::Platform::mkdir("savegames", true);
			saveBox = new GraphicTextEntryBox();
			saveBox->init(g_lang.get("Save"), g_lang.get("Cancel"), g_lang.get("SaveGame"), g_lang.get("Name"));
		}
	} else if (key.getCode() >= KeyCode::ZERO && key.getCode() < KeyCode::ZERO + Selection::maxGroups) { // group
		gui.groupKey(key.getCode() - KeyCode::ZERO);
	} else { // hotkeys
		gui.hotKey(cmd);
	}
}

void GameState::keyUp(const Key &key) {
	if (!chatManager.getEditEnabled()) {
		switch (keymap.getCommand(key)) {
			case ucCameraRotateLeft:
			case ucCameraRotateRight:
				gameCamera.setRotate(0);
				break;

			case ucCameraPitchUp:
			case ucCameraPitchDown:
				gameCamera.setMoveY(0);
				break;

			case ucCameraPosUp:
			case ucCameraPosDown:
				gameCamera.setMoveZ(0, false);
				break;

			case ucCameraPosLeft:
			case ucCameraPosRight:
				gameCamera.setMoveX(0, false);
				break;

			default:
				break;
		}
	}
}

void GameState::keyPress(char c) {
	if (saveBox) {
		if (saveBox->getEntry()->isActivated()) {
			switch (c) {
				case '/':
				case '\\':
				case ':':
				case ';':
				case ',':
				case '\'':
				case '"':
					break;
				default:
					saveBox->keyPress(c);
			}
		} else {
			// hacky... :(
			saveBox->setFocus();
		}
	} else {
		chatManager.keyPress(c);
	}
}

void GameState::quitGame() {
	program.setState(new BattleEnd(program));
}

// ==================== PRIVATE ====================

// ==================== render ====================

void GameState::render3d(){
	_PROFILE_FUNCTION();

	//init
	g_renderer.reset3d();

	g_renderer.loadGameCameraMatrix();
	g_renderer.computeVisibleArea();
	g_renderer.setupLighting();

	//shadow map
	g_renderer.renderShadowsToTexture();

	//clear buffers
	g_renderer.clearBuffers();

	//surface
	g_renderer.renderSurface();

	//selection circles
	g_renderer.renderSelectionEffects();

	//units
	g_renderer.renderUnits();

	//objects
	g_renderer.renderObjects();

	//water
	g_renderer.renderWater();
	g_renderer.renderWaterEffects();

	//particles
	g_renderer.renderParticleManager(ResourceScope::GAME);

	//mouse 3d
	g_renderer.renderMouse3d();
}

void GameState::render2d(){
	_PROFILE_FUNCTION();

	//init
	g_renderer.reset2d();

	//selection
	g_renderer.renderSelectionQuad();

	//script display text
	if (!ScriptManager::getDisplayText().empty() && !m_scriptMsgBox) {
		g_renderer.renderText(
			ScriptManager::getDisplayText(), g_coreData.getMenuFontNormal(),
			gui.getDisplay()->getColor(), 200, 680, false);
	}

	//save box
	if (saveBox) {
		g_renderer.renderTextEntryBox(saveBox);
	}

	g_renderer.renderChatManager(&chatManager);

	//debug info
	if (g_config.getMiscDebugMode()) {
		stringstream str;

		str	<< "MouseXY: " << mouseX << "," << mouseY << endl
			<< "PosObjWord: " << gui.getPosObjWorld().x << "," << gui.getPosObjWorld().y << endl
			<< "Render FPS: " << lastRenderFps << endl
			<< "Update FPS: " << lastUpdateFps << endl
			<< "World FPS: " << lastWorldFps << endl
			<< "GameCamera pos: " << gameCamera.getPos().x
				<< "," << gameCamera.getPos().y
				<< "," << gameCamera.getPos().z << endl
			<< "Time: " << simInterface->getWorld()->getTimeFlow()->getTime() << endl
			<< "Triangle count: " << g_renderer.getTriangleCount() << endl
			<< "Vertex count: " << g_renderer.getPointCount() << endl
			<< "Frame count: " << simInterface->getWorld()->getFrameCount() << endl
			<< "Camera VAng : " << gameCamera.getVAng() << endl;

		// resources
		for (int i=0; i<simInterface->getWorld()->getFactionCount(); ++i){
			str << "Player " << i << " res: ";
			for (int j=0; j < g_world.getTechTree()->getResourceTypeCount(); ++j) {
				str << g_world.getFaction(i)->getResource(j)->getAmount() << " ";
			}
			str << endl;
		}
		str << "ClusterMap Nodes = " << Search::Transition::NumTransitions(Field::LAND) << endl
			<< "ClusterMap Edges = " << Search::Edge::NumEdges(Field::LAND) << endl
			<< "GameRole::" << GameRoleNames[g_simInterface->getNetworkRole()] << endl;

		g_renderer.renderText(
			str.str(), g_coreData.getMenuFontNormal(),
			gui.getDisplay()->getColor(), 10, 500, false);
	}

	//network status
	/* NETWORK:
	if(renderNetworkStatus && networkManager.isNetworkGame()) {
		renderer.renderText(
			networkManager.getGameInterface()->getStatus(),
			coreData.getMenuFontNormal(),
			gui.getDisplay()->getColor(), 750, 75, false);
	}
	*/

	// resource info & consoles
	if (!g_config.getUiPhotoMode()) {
		g_renderer.renderResourceStatus();
		g_renderer.renderConsole(&console);
		g_renderer.renderConsole(ScriptManager::getDialogConsole());
	}

	//2d mouse
	//renderer.renderMouse2d(mouseX, mouseY, mouse2d, gui.isSelectingPos()? 1.f: 0.f);
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
	g_simInterface->getGameSettings().save(root.addChild("settings"));
	simInterface->getWorld()->save(root.addChild("world"));
	XmlIo::getInstance().save("savegames/" + name + ".sav", &root);
}

void ShowMap::render2d(){
	//init
	g_renderer.reset2d();

	//2d mouse
	g_renderer.renderMouse2d(mouseX, mouseY, mouse2d, gui.isSelectingPos() ? 1.f : 0.f);
}

void ShowMap::keyDown(const Key &key) {
	UserCommand cmd = keymap.getCommand(key);

	if (cmd == ucSaveScreenshot) {
		Shared::Platform::mkdir("screens", true);
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
			console.addLine(g_lang.get("ScreenshotDirectoryFull"));
		}

	//move camera left
	} else if (cmd == ucCameraPosLeft) {
		gameCamera.setMoveX(-scrollSpeed, false);

	//move camera right
	} else if (cmd == ucCameraPosRight) {
		gameCamera.setMoveX(scrollSpeed, false);

	//move camera up
	} else if (cmd == ucCameraPosUp) {
		gameCamera.setMoveZ(scrollSpeed, false);

	//move camera down
	} else if (cmd == ucCameraPosDown) {
		gameCamera.setMoveZ(-scrollSpeed, false);

	//switch display color
	} else if (cmd == ucCycleDisplayColor) {
		gui.switchToNextDisplayColor();

	//change camera mode
	} else if (cmd == ucCameraCycleMode) {
		gameCamera.switchState();
		string stateString = gameCamera.getState() == GameCamera::sGame 
			? g_lang.get("GameCamera") : g_lang.get("FreeCamera");
		console.addLine(g_lang.get("CameraModeSet") + " " + stateString);

	//exit
	} else if (cmd == ucMenuQuit) {
		if (!gui.cancelPending()) {
			doExitMessage(g_lang.get("ExitGame?"));
		}

	} else {
		switch (cmd) {
			//rotate camera left
			case ucCameraRotateLeft:
				gameCamera.setRotate(-1);
				break;

			//rotate camera right
			case ucCameraRotateRight:
				gameCamera.setRotate(1);
				break;

			//camera up
			case ucCameraPitchUp:
				gameCamera.setMoveY(1);
				break;

			//camera down
			case ucCameraPitchDown:
				gameCamera.setMoveY(-1);
				break;

			default:
				break;
		}
	}
}


}}//end namespace
