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
#include "network_util.h"
#include "checksum.h"
#include "auto_test.h"
#include "profiler.h"
#include "cluster_map.h"

#include "leak_dumper.h"
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest{ namespace Game{

// =====================================================
// 	class Game
// =====================================================

// ===================== PUBLIC ========================

Game *Game::singleton = NULL;

Game::Game(Program &program, const GameSettings &gs, XmlNode *savedGame)
		//main data
		: ProgramState(program)
		, gameSettings(gs)
		, savedGame(savedGame)
		, keymap(program.getKeymap())
		, input(program.getInput())
		, config(Config::getInstance())
		, world(this)
		, aiInterfaces()
		, gui(*this)
		, gameCamera()
		, commander()
		, console()
		, chatManager(keymap/*, console, world.getThisTeamIndex()*/)
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
		, paused(false)
		, noInput(false)
		, gameOver(false)
		, scrollSpeed(config.getUiScrollSpeed())
		, speed(GameSpeed::NORMAL)
		, fUpdateLoops(1.f)
		, lastUpdateLoopsFraction(0.f)
		, saveBox(NULL)
		, lastMousePos(0)
		, weatherParticleSystem(NULL) {
	assert(!singleton);
	singleton = this;
}

const char *Game::SpeedDesc[GameSpeed::COUNT] = {
	"Slowest",
	"VerySlow",
	"Slow",
	"Normal",
	"Fast",
	"VeryFast",
	"Fastest"
};

Game::~Game() {
	Logger &logger= Logger::getInstance();
	Renderer &renderer= Renderer::getInstance();

	logger.setState(Lang::getInstance().get("Deleting"));
	logger.add("Game", !Program::getInstance()->isTerminating());

	renderer.endGame();
	SoundRenderer::getInstance().stopAllSounds();

	delete saveBox;
	deleteValues(aiInterfaces.begin(), aiInterfaces.end());

	gui.end();		//selection must be cleared before deleting units
	world.end();	//must die before selection because of referencers
	singleton = NULL;
	logger.setLoading(true);
	program.setMaxUpdateBacklog(1);
}


// ==================== init and load ====================

void Game::load(){
	Logger &logger= Logger::getInstance();
	string mapName= gameSettings.getMapPath();
	string tilesetName= gameSettings.getTilesetPath();
	string techName= gameSettings.getTechPath();
	string scenarioPath= gameSettings.getScenarioPath();
	string scenarioName= basename(scenarioPath);

	GraphicProgressBar progressBar;
	progressBar.init(345, 550, 300, 20);
	logger.setProgressBar(&progressBar);

	logger.setState(Lang::getInstance().get("Loading"));

	if(scenarioName.empty()) {
		logger.setSubtitle(formatString(mapName)+" - "+formatString(tilesetName)+" - "+formatString(techName));
	} else {
		logger.setSubtitle(formatString(scenarioName));
	}
	
	//preload
	world.preload();
	//tileset
	if (!world.loadTileset()) {
		throw runtime_error ( "The tileset could not be loaded. See glestadv-error.log" );
	}
	//tech, load before map because of resources
	if (!world.loadTech()) {
		throw runtime_error ( "The techtree could not be loaded. See glestadv-error.log" );
	}
	//map
	world.loadMap();

	//scenario
	if (!scenarioName.empty()) {
		Lang::getInstance().loadScenarioStrings(scenarioPath, scenarioName);
		world.loadScenario(scenarioPath + "/" + scenarioName + ".xml");
	}

	// finished loading
	progressBar.setProgress(100);
	logger.setProgressBar(NULL);
}

void Game::init() {
	Lang &lang= Lang::getInstance();
	Logger &logger= Logger::getInstance();
	CoreData &coreData= CoreData::getInstance();
	Renderer &renderer= Renderer::getInstance();
	Map *map= world.getMap();
	NetworkManager &networkManager= NetworkManager::getInstance();

	logger.setState(lang.get("Initializing"));

	//mesage box
	mainMessageBox.init("", lang.get("Yes"), lang.get("No"));
	mainMessageBox.setEnabled(false);

	// init world, and place camera
	commander.init(&world);

	// setup progress bar (used for ClusterMap init)
	GraphicProgressBar progressBar;
	progressBar.init(345, 550, 300, 20);
	logger.setProgressBar(&progressBar);

	world.init();
	logger.setProgressBar(NULL);

	gui.init();
	chatManager.init(&console, world.getThisTeamIndex());
	const Vec2i &v= map->getStartLocation(world.getThisFaction()->getStartLocationIndex());
	gameCamera.init(map->getW(), map->getH());
	gameCamera.setPos(Vec2f((float)v.x, (float)v.y));

	ScriptManager::init(this);

	// create (or receive) random number seeds for AIs
	int aiCount = 0;
	for (int i=0; i < world.getFactionCount(); ++i) {
		if (world.getFaction(i)->getCpuControl()) {
			++aiCount;
		}
	}
	int32 *seeds = (aiCount ? new int32[aiCount] : NULL);

	if (aiCount) {
		if (!isNetworkClient()) { // if not client, make seeds
			Random r;
			r.init(Chrono::getCurMillis());
			for (int i=0; i < aiCount; ++i) {
				seeds[i] = r.rand();
			}
		}
		if (isNetworkGame()) { // if net game, send or receive seeds
			logger.add("Syncing AIs", true);
			networkManager.getGameNetworkInterface()->doSyncAiSeeds(aiCount, seeds);
		}
	}
	int seedCount = 0;

	//create AIs
	aiInterfaces.resize(world.getFactionCount());
	for (int i=0; i < world.getFactionCount(); ++i) {
		Faction *faction = world.getFaction(i);
		if (faction->getCpuControl() && ScriptManager::getPlayerModifiers(i)->getAiEnabled()) {
			aiInterfaces[i] = new AiInterface(*this, i, faction->getTeam(), seeds[seedCount++]);
			logger.add("Creating AI for faction " + intToStr(i), true);
		} else {
			aiInterfaces[i]= NULL;
		}
	}
	delete seeds;

	//wheather particle systems
	if(world.getTileset()->getWeather() == Weather::RAINY){
		logger.add("Creating rain particle system", true);
		weatherParticleSystem= new RainParticleSystem();
		weatherParticleSystem->setSpeed(12.f / config.getGsWorldUpdateFps());
		weatherParticleSystem->setPos(gameCamera.getPos());
		renderer.manageParticleSystem(weatherParticleSystem, rsGame);
	} else if(world.getTileset()->getWeather() == Weather::SNOWY){
		logger.add("Creating snow particle system", true);
		weatherParticleSystem= new SnowParticleSystem(1200);
		weatherParticleSystem->setSpeed(1.5f / config.getGsWorldUpdateFps());
		weatherParticleSystem->setPos(gameCamera.getPos());
		weatherParticleSystem->setTexture(coreData.getSnowTexture());
		renderer.manageParticleSystem(weatherParticleSystem, rsGame);
	}

	//init renderer state
	logger.add("Initializing renderer", true);
	renderer.initGame(this);

	//sounds
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	Tileset *tileset= world.getTileset();
	const TechTree *techTree = world.getTechTree();
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
	// ready ?
	if (isNetworkGame()) {
		logger.add("Waiting for network", true);
		tileset->doChecksum(checksum);
		techTree->doChecksum(checksum);
		map->doChecksum(checksum);
		networkManager.getGameNetworkInterface()->doWaitUntilReady(checksum);
	}
	// set maximum update timer back log
	if (!isNetworkGame()) {
		program.setMaxUpdateBacklog(2); // non-network game, may drop frames
	} else {
		program.setMaxUpdateBacklog(-1); // network games must always catch up
	}

	logger.add("Starting music stream", true);
	StrSound *gameMusic= world.getThisFaction()->getType()->getMusic();
	soundRenderer.playMusic(gameMusic);

	logger.add("Launching game");
	logger.setLoading(false);
	program.resetTimers();
}


// ==================== update ====================

//update
void Game::update() {
	// a) Updates non dependant on speed

	//misc
	updateFps++;
	
	mouse2d = (mouse2d + 1) % Renderer::maxMouse2dAnim;

	//console
	console.update();

	// b) Updates depandant on speed

	int updateLoops= getUpdateLoops();

	//update
	for (int i = 0; i < updateLoops; ++i) {
		Renderer &renderer = Renderer::getInstance();

		//AiInterface
		for (int i = 0; i < world.getFactionCount(); ++i)
			if ( world.getFaction(i)->getCpuControl()
			&&   ScriptManager::getPlayerModifiers(i)->getAiEnabled() ) {
				aiInterfaces[i]->update();
			}

		//World
		world.update();
		++worldFps;

		try { // Commander
			commander.updateNetwork();
		} catch (runtime_error e) {
			LOG_NETWORK(e.what());
			displayError(e);
			return;
		}

		//Gui
		gui.update();

		//Particle systems
		if(weatherParticleSystem != NULL){
			weatherParticleSystem->setPos(gameCamera.getPos());
		}
		renderer.updateParticleManager(rsGame);
	}

	try { //call the chat manager		
		chatManager.updateNetwork();
	} catch (SocketException e) {
		LOG_NETWORK(e.what());
		displayError(e);
		return;
	}

	//check for quiting status
	if(NetworkManager::getInstance().getGameNetworkInterface()->getQuit()) {
		quitGame();
	}

	//update auto test
	if (Config::getInstance().getMiscAutoTest()) {
		AutoTest::getInstance().updateGame(this);
	}
}

void Game::displayError(runtime_error &e) {
	Lang &lang = Lang::getInstance();
	paused = true;
	stringstream errmsg;
	char buf[512];
	/* NETWORK:
	const char* saveName = NetworkManager::getInstance().isServer()
			? "network_server_auto_save" : "network_client_auto_save";
	saveGame(saveName);
	snprintf(buf, sizeof(buf) - 1, lang.get("YourGameWasSaved").c_str(), saveName);
	*/
	errmsg << e.what() << endl << buf;

	mainMessageBox.init ( errmsg.str(), lang.get("Ok") );
	mainMessageBox.setEnabled ( true );
}

void Game::updateCamera() {
	gameCamera.update();
}

// ==================== render ====================

//render
void Game::render(){
	renderFps++;
	profileBegin("game-render3d");
	render3d();
	profileEnd("game-render3d");
	profileBegin("game-render2d");
	render2d();
	profileEnd("game-render2d");
	profileBegin("game-swapBuffers");
	Renderer::getInstance().swapBuffers();
	profileEnd("game-swapBuffers");
}

// ==================== tick ====================

void Game::tick(){
	lastWorldFps = worldFps;
	lastUpdateFps= updateFps;
	lastRenderFps= renderFps;
	worldFps = 0;
	updateFps= 0;
	renderFps= 0;

	//Win/lose check
	checkWinner();
	gui.tick();
}

// ==================== events ====================

void Game::mouseDownLeft(int x, int y){
	NetworkManager &networkManager= NetworkManager::getInstance();
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

	//exit message box, has to be the last thing to do in this function
	if ( mainMessageBox.getEnabled() ) {
		int button= 1;
		if ( mainMessageBox.mouseClick(x, y, button) ) {
			if ( button == 1 ) {
				networkManager.getGameNetworkInterface()->doQuitGame();
				quitGame();
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

void Game::mouseDoubleClickLeft(int x, int y)
{
	if ( noInput ) return;
	if (!(mainMessageBox.getEnabled()  && mainMessageBox.isInBounds(x, y))
			&& !(saveBox && saveBox->isInBounds(x, y))) {
		gui.mouseDoubleClickLeft(x, y);
	}
}

void Game::mouseMove(int x, int y, const MouseState &ms) {
	const Metrics &metrics = Metrics::getInstance();

	mouseX = x;
	mouseY = y;

	if (ms.get(mbCenter)) {
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

void Game::eventMouseWheel(int x, int y, int zDelta) {
	if ( noInput ) return;
	//gameCamera.transitionXYZ(0.0f, -(float)zDelta / 30.0f, 0.0f);
	gameCamera.zoom((float)zDelta / 30.0f);
}

void Game::keyDown(const Key &key) {
	UserCommand cmd = keymap.getCommand(key);
	Lang &lang = Lang::getInstance();
	bool isNetworkGame = NetworkManager::getInstance().isNetworkGame();
	bool speedChangesAllowed = !isNetworkGame;

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
			case keyReturn:
				saveGame(saveBox->getEntry()->getText());
				//intentional fall-through
			case keyEscape:
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
	// if ChatManger does not use this key, we keep processing

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

	//pause
	} else if (speedChangesAllowed) {
		bool prevPausedValue = paused;
		// on
		if (cmd == ucPauseOn) {
			paused = true;
		// off
		} else if (cmd == ucPauseOff) {
			paused = false;
		// toggle
		} else if (cmd == ucPauseToggle) {
			paused = !paused;
		}
		if (prevPausedValue != paused) {
			if (paused) {
				console.addLine(lang.get("GamePaused"));
			} else {
				console.addLine(lang.get("GameResumed"));
			}
			return;
		}

		//increment speed
		if (cmd == ucSpeedInc) {
			incSpeed();
			return;

		//decrement speed
		} else if (cmd == ucSpeedDec) {
			decSpeed();
			return;

		// reset speed
		} else if (cmd == ucSpeedReset) {
			resetSpeed();
			return;
		}
	}

	//exit
	if (cmd == ucMenuQuit) {
		if (!gui.cancelPending()) {
			showMessageBox(lang.get("ExitGame?"), "Quit...", true);
		}

	//save
	} else if (cmd == ucMenuSave) {
		/*if (!saveBox) {
			Shared::Platform::mkdir("savegames", true);
			saveBox = new GraphicTextEntryBox();
			saveBox->init(lang.get("Save"), lang.get("Cancel"), lang.get("SaveGame"), lang.get("Name"));
		}*/
		console.addLine("Save feature disabled: to be enabled in 0.3");

	//group
	} else if (key.getCode() >= key0 && key.getCode() < key0 + Selection::maxGroups) {
		gui.groupKey(key.getCode() - key0);

	//hotkeys
	} else if (gameCamera.getState() == GameCamera::sGame) {
		gui.hotKey(cmd);
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
	/*
	ucCameraZoomIn,
	ucCameraZoomOut,
	ucCameraZoomReset,
	ucCameraAngleReset,
	ucCameraZoomAndAngleReset,
	*/
}

void Game::keyUp(const Key &key) {
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

void Game::keyPress(char c) {
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

void Game::quitGame(){
	program.setState(new BattleEnd(program, world.getStats()));
}

// ==================== PRIVATE ====================

// ==================== render ====================

void Game::render3d(){

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
	renderer.renderParticleManager(rsGame);

	//mouse 3d
	renderer.renderMouse3d();
}

void Game::render2d(){
	Renderer &renderer= Renderer::getInstance();
	Config &config= Config::getInstance();
	CoreData &coreData= CoreData::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();

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
			<< "Time: " << world.getTimeFlow()->getTime() << endl
			<< "Triangle count: " << renderer.getTriangleCount() << endl
			<< "Vertex count: " << renderer.getPointCount() << endl
			<< "Frame count: " << world.getFrameCount() << endl;

		str << "Camera VAng : " << gameCamera.getVAng() << endl;

		// resources
		for(int i=0; i<world.getFactionCount(); ++i){
			str << "Player " << i << " res: ";
			for(int j=0; j<world.getTechTree()->getResourceTypeCount(); ++j){
				str << world.getFaction(i)->getResource(j)->getAmount() << " ";
			}
			str << endl;
		}

		// cluster map
		str << "ClusterMap Nodes = " << Search::Transition::NumTransitions(Field::LAND) << endl;
		str << "ClusterMap Edges = " << Search::Edge::NumEdges(Field::LAND) << endl;

		renderer.renderText(
			str.str(), coreData.getMenuFontNormal(),
			gui.getDisplay()->getColor(), 10, 500, false);
	}

	//network status
	/* NETWORK:
	if(renderNetworkStatus && networkManager.isNetworkGame()) {
		renderer.renderText(
			networkManager.getGameNetworkInterface()->getStatus(),
			coreData.getMenuFontNormal(),
			gui.getDisplay()->getColor(), 750, 75, false);
	}
	*/

	//resource info
	if(!config.getUiPhotoMode()){
		renderer.renderResourceStatus();
		renderer.renderConsole(&console);
	}

	//2d mouse
	renderer.renderMouse2d(mouseX, mouseY, mouse2d, gui.isSelectingPos()? 1.f: 0.f);
}


// ==================== misc ====================


void Game::checkWinner(){
	if(!gameOver){
		if(gameSettings.getDefaultVictoryConditions()){
			checkWinnerStandard();
		}
		else
		{
			checkWinnerScripted();
		}
	}
}

void Game::checkWinnerStandard(){
	//lose
	bool lose= false;
	if(!hasBuilding(world.getThisFaction())){
		lose= true;
		for(int i=0; i<world.getFactionCount(); ++i){
			if(!world.getFaction(i)->isAlly(world.getThisFaction())){
				world.getStats().setVictorious(i);
			}
		}
		gameOver= true;
		showLoseMessageBox();
	}

	//win
	if(!lose){
		bool win= true;
		for(int i=0; i<world.getFactionCount(); ++i){
			if(i!=world.getThisFactionIndex()){
				if(hasBuilding(world.getFaction(i)) && !world.getFaction(i)->isAlly(world.getThisFaction())){
					win= false;
				}
			}
		}

		//if win
		if(win){
			for(int i=0; i< world.getFactionCount(); ++i){
				if(world.getFaction(i)->isAlly(world.getThisFaction())){
					world.getStats().setVictorious(i);
				}
			}
			gameOver= true;
			showWinMessageBox();
		}
	}
}

void Game::checkWinnerScripted(){
	if(ScriptManager::getGameOver()){
		gameOver= true;
		for(int i= 0; i<world.getFactionCount(); ++i){
			if(ScriptManager::getPlayerModifiers(i)->getWinner()){
				world.getStats().setVictorious(i);
			}
		}
		if(ScriptManager::getPlayerModifiers(world.getThisFactionIndex())->getWinner()){
			showWinMessageBox();
		}
		else{
			showLoseMessageBox();
		}
	}
}


bool Game::hasBuilding(const Faction *faction){
	for(int i=0; i<faction->getUnitCount(); ++i){
		Unit *unit = faction->getUnit(i);
		if(unit->getType()->hasSkillClass(SkillClass::BE_BUILT) && unit->isAlive()){
			return true;
		}
	}
	return false;
}

void Game::updateSpeed() {
	Lang &lang= Lang::getInstance();
	console.addLine(lang.get("GameSpeedSet") + " " + lang.get(SpeedDesc[speed]));
	
	if (speed == GameSpeed::NORMAL) {
		fUpdateLoops = 1.0f;
	} else if (speed > GameSpeed::NORMAL) {
		fUpdateLoops = 1.0f + float(speed - GameSpeed::NORMAL) / float(GameSpeed::FASTEST - GameSpeed::NORMAL)
			* (theConfig.getGsSpeedFastest() - 1.0f);
	} else {
		fUpdateLoops = theConfig.getGsSpeedSlowest() + float(speed) / float(GameSpeed::NORMAL)
			* (1.0f - theConfig.getGsSpeedSlowest());
	}
}

void Game::incSpeed() {
	if (speed < GameSpeed::FASTEST) {
		++speed;
		updateSpeed();
	}
}

void Game::decSpeed() {
	if (speed > GameSpeed::SLOWEST) {
		--speed;
		updateSpeed();
	}
}

void Game::resetSpeed() {
	speed = GameSpeed::NORMAL;
	updateSpeed();
}

int Game::getUpdateLoops(){
	if (paused || (!NetworkManager::getInstance().isNetworkGame() && (saveBox || mainMessageBox.getEnabled()))) {
		return 0;
	} else {
		int updateLoops = (int)(fUpdateLoops + lastUpdateLoopsFraction);
		lastUpdateLoopsFraction = fUpdateLoops + lastUpdateLoopsFraction - (float)updateLoops;
		return updateLoops;
	}
}

void Game::showLoseMessageBox() {
	Lang &lang = Lang::getInstance();
	showMessageBox(lang.get("YouLose") + ", " + lang.get("ExitGame?"), lang.get("BattleOver"), false);
}

void Game::showWinMessageBox() {
	Lang &lang = Lang::getInstance();
	showMessageBox(lang.get("YouWin") + ", " + lang.get("ExitGame?"), lang.get("BattleOver"), false);
}

void Game::showMessageBox(const string &text, const string &header, bool toggle) {
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

void Game::saveGame(string name) const {
	/*XmlNode root("saved-game");
	root.addAttribute("version", "1");
	gui.save(root.addChild("gui"));
	gameSettings.save(root.addChild("settings"));
	world.save(root.addChild("world"));
	XmlIo::getInstance().save("savegames/" + name + ".sav", &root);*/
}



void ShowMap::init() {
	Lang &lang= Lang::getInstance();
	Logger &logger= Logger::getInstance();
	CoreData &coreData= CoreData::getInstance();
	Renderer &renderer= Renderer::getInstance();
	Map *map= world.getMap();

	logger.setState(lang.get("Initializing"));

	//mesage box
	mainMessageBox.init("", lang.get("Yes"), lang.get("No"));
	mainMessageBox.setEnabled(false);

	// init world, and place camera
	commander.init(&world);

	// setup progress bar (used for ClusterMap init)
	GraphicProgressBar progressBar;
	progressBar.init(345, 550, 300, 20);
	logger.setProgressBar(&progressBar);

	world.init();
	logger.setProgressBar(NULL);

	gui.init();
	const Vec2i &v= map->getStartLocation(0);
	gameCamera.init(map->getW(), map->getH());
	gameCamera.setPos(Vec2f((float)v.x, (float)v.y));

	//wheather particle systems
	if(world.getTileset()->getWeather() == Weather::RAINY){
		logger.add("Creating rain particle system", true);
		weatherParticleSystem= new RainParticleSystem();
		weatherParticleSystem->setSpeed(12.f / config.getGsWorldUpdateFps());
		weatherParticleSystem->setPos(gameCamera.getPos());
		renderer.manageParticleSystem(weatherParticleSystem, rsGame);
	} else if(world.getTileset()->getWeather() == Weather::SNOWY){
		logger.add("Creating snow particle system", true);
		weatherParticleSystem= new SnowParticleSystem(1200);
		weatherParticleSystem->setSpeed(1.5f / config.getGsWorldUpdateFps());
		weatherParticleSystem->setPos(gameCamera.getPos());
		weatherParticleSystem->setTexture(coreData.getSnowTexture());
		renderer.manageParticleSystem(weatherParticleSystem, rsGame);
	}

	//init renderer state
	logger.add("Initializing renderer", true);
	renderer.initGame(this);

	//sounds
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	Tileset *tileset= world.getTileset();
	const TechTree *techTree = world.getTechTree();
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
	// set maximum update timer back log
	program.setMaxUpdateBacklog(2); // non-network game, may drop frames

	logger.add("Launching game");
	logger.setLoading(false);
	program.resetTimers();
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
