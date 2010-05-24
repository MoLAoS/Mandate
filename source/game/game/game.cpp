// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Daniel Santos <daniel.santos@pobox.com>
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

const GameSettings &GameState::getGameSettings()	{return theSimInterface->getGameSettings();}

GameState::GameState(Program &program, XmlNode *savedGame)
		//main data
		: ProgramState(program)
		, simInterface(program.getSimulationInterface())
		//, savedGame(savedGame)
		, keymap(program.getKeymap())
		, input(program.getInput())
		, config(Config::getInstance())
		//, world(this)
		//, aiInterfaces()
		, gui(*this)
		, gameCamera()
		//, commander()
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
		//, paused(false)
		, noInput(false)
		//, gameOver(false)
		, netError(false)
		, scrollSpeed(config.getUiScrollSpeed())
		//, speed(GameSpeed::NORMAL)
		//, fUpdateLoops(1.f)
		//, lastUpdateLoopsFraction(0.f)
		, saveBox(NULL)
		, lastMousePos(0)
		, weatherParticleSystem(0) {
	assert(!singleton);
	singleton = this;
	
	simInterface->constructGameWorld(this);
	//gameSettings(simInterface->getGameSettings());

}

int GameState::getUpdateInterval() const {
	return simInterface->getUpdateInterval();
}

GameState::~GameState() {
	_TRACE_FUNCTION();
	Logger &logger= Logger::getInstance();
	Renderer &renderer= Renderer::getInstance();

	logger.setState(Lang::getInstance().get("Deleting"));
	logger.add("~GameState", !program.isTerminating());

	renderer.endGame();
	weatherParticleSystem = 0;
	SoundRenderer::getInstance().stopAllSounds();

	delete saveBox;
	saveBox = 0;

	gui.end(); //selection must be cleared before deleting units
	singleton = 0;
	logger.setLoading(true);

	// reset max update backlog, to prevent super-speed in menus
	program.setMaxUpdateBacklog(1);

	// delete the World
	simInterface->destroyGameWorld();
}


// ==================== init and load ====================

void GameState::load() {
	_TRACE_FUNCTION();
	GameSettings &gameSettings = theSimInterface->getGameSettings();
	Logger &logger= Logger::getInstance();
	const string &mapName = gameSettings.getMapPath();
	const string &tilesetName = gameSettings.getTilesetPath();
	const string &techName = gameSettings.getTechPath();
	const string &scenarioPath = gameSettings.getScenarioPath();
	string scenarioName = basename(scenarioPath);

	GraphicProgressBar progressBar;
	progressBar.init(345, 550, 300, 20);
	logger.setProgressBar(&progressBar);

	logger.setState(Lang::getInstance().get("Loading"));

	if(scenarioName.empty()) {
		logger.setSubtitle(formatString(mapName)+" - "+formatString(tilesetName)+" - "+formatString(techName));
	} else {
		logger.setSubtitle(formatString(scenarioName));
	}
	
	simInterface->loadWorld();

	// finished loading
	progressBar.setProgress(100);
	logger.setProgressBar(NULL);
}

void GameState::init() {
	_TRACE_FUNCTION();
	Lang &lang= Lang::getInstance();
	Logger &logger= Logger::getInstance();
	CoreData &coreData= CoreData::getInstance();
	Renderer &renderer= Renderer::getInstance();
	Map *map = simInterface->getWorld()->getMap();

	logger.setState(lang.get("Initializing"));

	//mesage box
	mainMessageBox.init("", lang.get("Yes"), lang.get("No"));
	mainMessageBox.setEnabled(false);

	// init world, and place camera
	simInterface->initWorld();
	gui.init();
	//REFACTOR: ThisTeamIndex belong in here, not the World
	chatManager.init(&console, simInterface->getWorld()->getThisTeamIndex());
	gameCamera.init(map->getW(), map->getH());
	/*const*/ Vec2i *v;// = new Vec2i(0, 0);
	if(simInterface->getWorld()->getThisFaction()){
		v = &map->getStartLocation(simInterface->getWorld()->getThisFaction()->getStartLocationIndex());  //FIXME: -loadmap has no players
	}else{
		v = &Vec2i(0, 0);
	}
	gameCamera.setPos(Vec2f((float)v->x, (float)v->y));
	if (simInterface->getSavedGame()) {
		gui.load(simInterface->getSavedGame()->getChild("gui"));
	}

	//wheather particle systems
	if(simInterface->getWorld()->getTileset()->getWeather() == Weather::RAINY){
		logger.add("Creating rain particle system", true);
		weatherParticleSystem= new RainParticleSystem();
		weatherParticleSystem->setSpeed(12.f / config.getGsWorldUpdateFps());
		weatherParticleSystem->setPos(gameCamera.getPos());
		renderer.manageParticleSystem(weatherParticleSystem, ResourceScope::GAME);
	} else if(simInterface->getWorld()->getTileset()->getWeather() == Weather::SNOWY){
		logger.add("Creating snow particle system", true);
		weatherParticleSystem= new SnowParticleSystem(1200);
		weatherParticleSystem->setSpeed(1.5f / config.getGsWorldUpdateFps());
		weatherParticleSystem->setPos(gameCamera.getPos());
		weatherParticleSystem->setTexture(coreData.getSnowTexture());
		renderer.manageParticleSystem(weatherParticleSystem, ResourceScope::GAME);
	}

	//init renderer state
	logger.add("Initializing renderer", true);
	renderer.initGame(this);

	//sounds
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	Tileset *tileset= simInterface->getWorld()->getTileset();
	const TechTree *techTree = simInterface->getWorld()->getTechTree();
	AmbientSounds *ambientSounds= tileset->getAmbientSounds();

	//rain
	if(tileset->getWeather()==Weather::RAINY && ambientSounds->isEnabledRain()){
		logger.add("Starting ambient stream", true);
		soundRenderer.playAmbient(ambientSounds->getRain());
	}
	//snow
	if(tileset->getWeather()==Weather::SNOWY && ambientSounds->isEnabledSnow()){
		logger.add("Starting ambient stream", true);
		soundRenderer.playAmbient(ambientSounds->getSnow());
	}

	int maxUpdtBacklog = simInterface->launchGame();
	program.setMaxUpdateBacklog(maxUpdtBacklog);

	logger.add("Starting music stream", true);
	if(simInterface->getWorld()->getThisFaction()){
		StrSound *gameMusic = simInterface->getWorld()->getThisFaction()->getType()->getMusic();
		soundRenderer.playMusic(gameMusic);
	}

	logger.add("Launching game");
	logger.setLoading(false);
	program.resetTimers();
}


// ==================== update ====================

//update
void GameState::update() {
	_TRACE_FUNCTION();
	// a) Updates non dependant on speed
	if (netError) {
		return;
	}

	//misc
	updateFps++;
	
	mouse2d = (mouse2d + 1) % Renderer::maxMouse2dAnim;

	//console
	console.update();

	// b) Updates depandant on speed

	///@todo clean-up, this should be done in SimulationInterface::updateWorld()
	// but the particle stuff still lives here, so we still need to process each loop here

	try {
		//update simulation
		simInterface->updateWorld();
			++worldFps;

		//Gui
		gui.update();

		//Particle systems
		if(weatherParticleSystem != NULL){
			weatherParticleSystem->setPos(gameCamera.getPos());
		}
		theRenderer.updateParticleManager(ResourceScope::GAME);

	} catch (Net::NetworkError &e) {
		LOG_NETWORK(e.what());
		displayError(e);
		netError = true;
		return;
		///@todo return to menu...
	} catch (std::runtime_error &e) {
		displayError(e);
		return;
	}

	//check for quiting status
	if (theSimInterface->getQuit()) {
		quitGame();
	}

	//update auto test
	if (Config::getInstance().getMiscAutoTest()) {
		AutoTest::getInstance().updateGame(this);
	}
}

void GameState::displayError(std::exception &e) {
	Lang &lang = Lang::getInstance();
	simInterface->pause();
	stringstream errmsg;
	errmsg << e.what() << endl;// << buf;
	mainMessageBox.init(errmsg.str(), lang.get("Ok"));
	mainMessageBox.setEnabled(true);
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
	Vec2i mmCell;

	const Metrics &metrics= Metrics::getInstance();
	bool messageBoxClick= false;

	//script message box, only if the exit box is not enabled
	if ( !mainMessageBox.getEnabled() && ScriptManager::getMessageBox()->getEnabled() ) {
		int button= 1;
		if ( ScriptManager::getMessageBox()->mouseClick(x, y, button) ) {
			ScriptManager::onMessageBoxOk();
			messageBoxClick= true;
		}
	}

	//exit message box
	if ( mainMessageBox.getEnabled() ) {
		int button= 1;
		if ( mainMessageBox.mouseClick(x, y, button) ) {
			if ( button == 1 ) {
				theSimInterface->doQuitGame(QuitSource::LOCAL);
				//quitGame();
			} else {
				//close message box
				mainMessageBox.setEnabled(false);
			}
		}
	//save box
	} else if ( saveBox ) {
		int button;
		if ( saveBox->mouseClick(x, y, button) ) {
			if (button == 1) {
				saveGame(saveBox->getEntry()->getText());
			}
			//close message box
			delete saveBox;
			saveBox = NULL;
		}

	} else if ( !noInput ) {
		gui.mouseDownLeft(x, y);
	}
}

void GameState::mouseDoubleClickLeft(int x, int y)
{
	if ( noInput ) return;
	if (!(mainMessageBox.getEnabled()  && mainMessageBox.isInBounds(x, y))
			&& !(saveBox && saveBox->isInBounds(x, y))) {
		gui.mouseDoubleClickLeft(x, y);
	}
}

void GameState::mouseMove(int x, int y, const MouseState &ms) {
	const Metrics &metrics = Metrics::getInstance();

	mouseX = x;
	mouseY = y;

	if (ms.get(MouseButton::MIDDLE)) {
		if ( !noInput ) {
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
		if ( !noInput ) {
			//main window
			if (y < 10) {
				gameCamera.setMoveZ(-scrollSpeed, true);
			} else if (y > metrics.getVirtualH() - 10) {
				gameCamera.setMoveZ(scrollSpeed, true);
			} else { //if(y < 20 || y > metrics.getVirtualH()-20){
				gameCamera.setMoveZ(0, true);
			}

			if (x < 10) {
				gameCamera.setMoveX(-scrollSpeed, true);
			} else if (x > metrics.getVirtualW() - 10) {
				gameCamera.setMoveX(scrollSpeed, true);
			} else { //if(x < 20 || x > metrics.getVirtualW()-20){
				gameCamera.setMoveX(0, true);
			}
		}

		if (mainMessageBox.getEnabled()) {
			mainMessageBox.mouseMove(x, y);
		} else if (ScriptManager::getMessageBox()->getEnabled()) {
			ScriptManager::getMessageBox()->mouseMove(x, y);
		} else if (saveBox) {
			saveBox->mouseMove(x, y);
		} else if ( !noInput ) {
			//graphics
			gui.mouseMoveGraphics(x, y);
		}
	}

	//display
	if (metrics.isInDisplay(x, y) && !gui.isSelecting() && !gui.isSelectingPos()) {
		if (!gui.isSelectingPos()) {
			gui.mouseMoveDisplay(x - metrics.getDisplayX(), y - metrics.getDisplayY());
		}
	}

	lastMousePos.x = x;
	lastMousePos.y = y;
}

void GameState::eventMouseWheel(int x, int y, int zDelta) {
	if (noInput) return;
	//gameCamera.transitionXYZ(0.0f, -(float)zDelta / 30.0f, 0.0f);
	gameCamera.zoom((float)zDelta / 30.0f);
}

void GameState::keyDown(const Key &key) {
	UserCommand cmd = keymap.getCommand(key);
	Lang &lang = Lang::getInstance();
	bool speedChangesAllowed = !theSimInterface->isNetworkInterface();

	if (config.getMiscDebugKeys()) {
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
				//intentional fall-through
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
				Renderer::getInstance().saveScreen(path);
				break;
			}
		}
		if (i > MAX_SCREENSHOTS) {
			console.addLine(lang.get("ScreenshotDirectoryFull"));
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
				console.addLine(lang.get("GamePaused"));
			} else if (curSpeed == GameSpeed::PAUSED) {
				console.addLine(lang.get("GameResumed"));
			} else {
				console.addLine(lang.get("GameSpeedSet") + " " + lang.get(GameSpeedNames[newSpeed]));
			}
			return;
		}
	}
	if (cmd == ucMenuQuit) { // exit
		if (!gui.cancelPending()) {
			showMessageBox(lang.get("ExitGame?"), "Quit...", true);
		}
	} else if (cmd == ucMenuSave) { // save
		if (!saveBox) {
			Shared::Platform::mkdir("savegames", true);
			saveBox = new GraphicTextEntryBox();
			saveBox->init(lang.get("Save"), lang.get("Cancel"), lang.get("SaveGame"), lang.get("Name"));
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
	_TRACE_FUNCTION();
	program.setState(new BattleEnd(program));
}

// ==================== PRIVATE ====================

// ==================== render ====================

void GameState::render3d(){
	_PROFILE_FUNCTION();
	Renderer &renderer= Renderer::getInstance();

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
	Renderer &renderer= Renderer::getInstance();
	Config &config= Config::getInstance();
	CoreData &coreData= CoreData::getInstance();

	//init
	renderer.reset2d();

	//display
	renderer.renderDisplay();

	//minimap
	if(!config.getUiPhotoMode()){
		renderer.renderMinimap();
	}

	//selection
	renderer.renderSelectionQuad();

	//exit message box
	if(mainMessageBox.getEnabled()){
		renderer.renderMessageBox(&mainMessageBox);
	}

	//script message box
	if(!mainMessageBox.getEnabled() && ScriptManager::getMessageBoxEnabled()){
		renderer.renderMessageBox(ScriptManager::getMessageBox());
	}

	//script display text
	if(!ScriptManager::getDisplayText().empty() && !ScriptManager::getMessageBoxEnabled()){
		renderer.renderText(
			ScriptManager::getDisplayText(), coreData.getMenuFontNormal(),
			gui.getDisplay()->getColor(), 200, 680, false);
	}

	//save box
	if (saveBox) {
		renderer.renderTextEntryBox(saveBox);
	}

	renderer.renderChatManager(&chatManager);

	//debug info
	if(config.getMiscDebugMode()){
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
			<< "Triangle count: " << renderer.getTriangleCount() << endl
			<< "Vertex count: " << renderer.getPointCount() << endl
			<< "Frame count: " << simInterface->getWorld()->getFrameCount() << endl
			<< "Camera VAng : " << gameCamera.getVAng() << endl;

		// resources
		for (int i=0; i<simInterface->getWorld()->getFactionCount(); ++i){
			str << "Player " << i << " res: ";
			for (int j=0; j < simInterface->getWorld()->getTechTree()->getResourceTypeCount(); ++j) {
				str << simInterface->getWorld()->getFaction(i)->getResource(j)->getAmount() << " ";
			}
			str << endl;
		}
		str << "ClusterMap Nodes = " << Search::Transition::NumTransitions(Field::LAND) << endl
			<< "ClusterMap Edges = " << Search::Edge::NumEdges(Field::LAND) << endl
			<< "GameRole::" << GameRoleNames[theSimInterface->getNetworkRole()] << endl;

		renderer.renderText(
			str.str(), coreData.getMenuFontNormal(),
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
	if(!config.getUiPhotoMode()){
		renderer.renderResourceStatus();
		renderer.renderConsole(&console);
		renderer.renderConsole(ScriptManager::getDialogConsole());
	}

	//2d mouse
	renderer.renderMouse2d(mouseX, mouseY, mouse2d, gui.isSelectingPos()? 1.f: 0.f);
}


// ==================== misc ====================


void GameState::showLoseMessageBox() {
	Lang &lang = Lang::getInstance();
	showMessageBox(lang.get("YouLose") + ", " + lang.get("ExitGame?"), lang.get("BattleOver"), false);
}

void GameState::showWinMessageBox() {
	Lang &lang = Lang::getInstance();
	showMessageBox(lang.get("YouWin") + ", " + lang.get("ExitGame?"), lang.get("BattleOver"), false);
}

void GameState::showMessageBox(const string &text, const string &header, bool toggle) {
	if (!toggle) {
		mainMessageBox.setEnabled(false);
	}

	if (!mainMessageBox.getEnabled()) {
		mainMessageBox.setText(text);
		mainMessageBox.setHeader(header);
		mainMessageBox.setEnabled(true);
	} else {
		mainMessageBox.setEnabled(false);
	}
}

void GameState::saveGame(string name) const {
	XmlNode root("saved-game");
	root.addAttribute("version", GameConstants::saveGameVersion);
	gui.save(root.addChild("gui"));
	theSimInterface->getGameSettings().save(root.addChild("settings"));
	simInterface->getWorld()->save(root.addChild("world"));
	XmlIo::getInstance().save("savegames/" + name + ".sav", &root);
}


void ShowMap::render2d(){
	Renderer &renderer= Renderer::getInstance();
	Config &config= Config::getInstance();
	CoreData &coreData= CoreData::getInstance();

	//init
	renderer.reset2d();

	//display
	renderer.renderDisplay();

	//minimap
	if(!config.getUiPhotoMode()){
		renderer.renderMinimap();
	}

	//exit message box
	if(mainMessageBox.getEnabled()){
		renderer.renderMessageBox(&mainMessageBox);
	}

	//2d mouse
	renderer.renderMouse2d(mouseX, mouseY, mouse2d, gui.isSelectingPos()? 1.f: 0.f);
}

void ShowMap::keyDown(const Key &key) {
	UserCommand cmd = keymap.getCommand(key);
	Lang &lang = Lang::getInstance();

	if (cmd == ucSaveScreenshot) {
		Shared::Platform::mkdir("screens", true);
		int i;
		const int MAX_SCREENSHOTS = 100;

		// Find a filename from 'screen(0 to MAX_SCREENHOTS).tga' and save the screenshot in one that doesn't
		//  already exist.
		for (i = 0; i < MAX_SCREENSHOTS; ++i) {
			string path = "screens/screen" + intToStr(i) + ".tga";

			if(fileExists(path)){
				Renderer::getInstance().saveScreen(path);
				break;
			}
		}

		if (i > MAX_SCREENSHOTS) {
			console.addLine(lang.get("ScreenshotDirectoryFull"));
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
		string stateString = gameCamera.getState() == GameCamera::sGame ? lang.get("GameCamera") : lang.get("FreeCamera");
		console.addLine(lang.get("CameraModeSet") + " " + stateString);

	//exit
	} else if (cmd == ucMenuQuit) {
		if (!gui.cancelPending()) {
			showMessageBox(lang.get("ExitGame?"), "Quit...", true);
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
