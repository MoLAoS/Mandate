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
#include "network_manager.h"
#include "checksum.h"

#include "leak_dumper.h"


using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest{ namespace Game{

const char *controlTypeNames[ctCount] = {
	"Closed",
	"Cpu",
	"CpuUltra",
	"Network",
	"Human"
};

// =====================================================
// 	class Game
// =====================================================

// ===================== PUBLIC ========================

const char *Game::SpeedDesc[sCount] = {
	"Slowest",
	"VerySlow",
	"Slow",
	"Normal",
	"Fast",
	"VeryFast",
	"Fastest"
};

void Game::_init() {
	mouseX = 0;
	mouseY = 0;
	mouse2d = 0;
	loadingText = "";
	weatherParticleSystem = NULL;
	updateFps = 0;
	renderFps = 0;
	lastUpdateFps = 0;
	lastRenderFps = 0;
	paused = false;
	gameOver = false;
	renderNetworkStatus = false;
	scrollSpeed = Config::getInstance().getScrollSpeed();
	speed = sNormal;
	lastMousePos = Vec2i(0);
	fUpdateLoops = 1.0f;
	lastUpdateLoopsFraction = 0.0f;
	exitMessageBox = NULL;
	saveBox = NULL;
}


Game::~Game(){
    Logger &logger= Logger::getInstance();
	Renderer &renderer= Renderer::getInstance();

	logger.setState(Lang::getInstance().get("Deleting"));
	logger.add("Game", true);

	renderer.endGame();
	SoundRenderer::getInstance().stopAllSounds();

	if(saveBox) {
		delete saveBox;
	}
	if(exitMessageBox) {
		delete exitMessageBox;
	}
	deleteValues(aiInterfaces.begin(), aiInterfaces.end());

	gui.end();		//selection must be cleared before deleting units
	world.end();	//must die before selection because of referencers
}


// ==================== init and load ====================

void Game::load(){
	Logger::getInstance().setState(Lang::getInstance().get("Loading"));

	//tileset
    world.loadTileset(checksum);

    //tech, load before map because of resources
    world.loadTech(checksum);

    //map
    world.loadMap(checksum);
}

void Game::init(){
	Logger &logger= Logger::getInstance();
	CoreData &coreData= CoreData::getInstance();
	Renderer &renderer= Renderer::getInstance();
	Config &config= Config::getInstance();
	Map *map= world.getMap();
	NetworkManager &networkManager= NetworkManager::getInstance();

	logger.setState(Lang::getInstance().get("Initializing"));

	//check fog of war
	if(!config.getFogOfWar() && networkManager.isNetworkGame() ){
		throw runtime_error("Can not play online games with for of war disabled");
	}

	//init world, and place camera
	commander.init(&world);

	world.init(savedGame ? savedGame->getChild("world") : NULL);
	gui.init(this);
	chatManager.init(&console, world.getThisTeamIndex());
	const Vec2i &v= map->getStartLocation(world.getThisFaction()->getStartLocationIndex());
	gameCamera.init(map->getW(), map->getH());
	gameCamera.setPos(Vec2f((float)v.x, (float)v.y));

	if(savedGame && (!networkManager.isNetworkGame() || networkManager.isServer())) {
		gui.load(savedGame->getChild("gui"));
	}

	//create IAs
	aiInterfaces.resize(world.getFactionCount());
	for(int i=0; i<world.getFactionCount(); ++i){
		Faction *faction= world.getFaction(i);
		if(faction->getCpuControl()){
			aiInterfaces[i]= new AiInterface(*this, i, faction->getTeam());
			logger.add("Creating AI for faction " + intToStr(i), true);
		}
		else{
			aiInterfaces[i]= NULL;
		}
	}

	//wheather particle systems
	if(world.getTileset()->getWeather() == wRainy){
		logger.add("Creating rain particle system", true);
		weatherParticleSystem= new RainParticleSystem();
		weatherParticleSystem->setSpeed(12.f / config.getWorldUpdateFPS());
		weatherParticleSystem->setPos(gameCamera.getPos());
		renderer.manageParticleSystem(weatherParticleSystem, rsGame);
	}
	else if(world.getTileset()->getWeather() == wSnowy){
		logger.add("Creating snow particle system", true);
		weatherParticleSystem= new SnowParticleSystem(1200);
		weatherParticleSystem->setSpeed(1.5f / config.getWorldUpdateFPS());
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
	AmbientSounds *ambientSounds= tileset->getAmbientSounds();

	//rain
	if(tileset->getWeather()==wRainy && ambientSounds->isEnabledRain()){
		logger.add("Starting ambient stream", true);
		soundRenderer.playAmbient(ambientSounds->getRain());
	}

	//snow
	if(tileset->getWeather()==wSnowy && ambientSounds->isEnabledSnow()){
		logger.add("Starting ambient stream", true);
		soundRenderer.playAmbient(ambientSounds->getSnow());
	}

	logger.add("Waiting for network", true);
	networkManager.getGameNetworkInterface()->waitUntilReady(checksum);
/*
	if(networkManager.isNetworkClient()) {
		program->setMaxUpdateBacklog(-1);
	} else {
		program->setMaxUpdateBacklog(2);
	}*/

	logger.add("Starting music stream", true);
	StrSound *gameMusic= world.getThisFaction()->getType()->getMusic();
	soundRenderer.playMusic(gameMusic);

	logger.add("Launching game");
	program->resetTimers();

	if(savedGame) {
		delete savedGame;
		savedGame = NULL;
	}
}


// ==================== update ====================

//update
void Game::update(){
	// a) Updates non dependant on speed

	//misc
	updateFps++;
	mouse2d= (mouse2d+1) % Renderer::maxMouse2dAnim;

	//console
	console.update();

	// b) Updates depandant on speed

	int updateLoops= getUpdateLoops();

	//update
	for(int i=0; i<updateLoops; ++i){
		Renderer &renderer= Renderer::getInstance();

		//AiInterface
		for(int i=0; i<world.getFactionCount(); ++i){
			if(world.getFaction(i)->getCpuControl()){
				aiInterfaces[i]->update();
			}
		}

		//World
		world.update();

		try {
			// Commander
			commander.updateNetwork();
		} catch (SocketException &e) {
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

	try {
		//call the chat manager
		chatManager.updateNetwork();
	} catch (SocketException &e) {
		displayError(e);
		return;
	}

	//check for quiting status
	if(NetworkManager::getInstance().getGameNetworkInterface()->getQuit()){
		program->setState(new BattleEnd(program, world.getStats()));
	}
}

void Game::displayError(SocketException &e) {
	Lang &lang = Lang::getInstance();
	paused = true;
	string errmsg = e.what();

	if(NetworkManager::getInstance().isServer()) {
		saveGame("network_auto_save");
		errmsg += "\n" + lang.get("YourGameWasSaved") + " network_auto_save.";
	}

	if(exitMessageBox) {
		delete exitMessageBox;
	}

	exitMessageBox = new GraphicMessageBox();
	exitMessageBox->init(errmsg, lang.get("Ok"));
}

void Game::updateCamera(){
	gameCamera.update();
}

// ==================== render ====================

//render
void Game::render(){
	renderFps++;
	render3d();
	render2d();
	Renderer::getInstance().swapBuffers();
}

// ==================== tick ====================

void Game::tick(){
	lastUpdateFps= updateFps;
	lastRenderFps= renderFps;
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

	//exit message box
	if(exitMessageBox){
		int button;
		if(exitMessageBox->mouseClick(x, y, button)){
			if(button==1){
				networkManager.getGameNetworkInterface()->quitGame();
				program->setState(new BattleEnd(program, world.getStats()));
			}
			else{
				//close message box
				delete exitMessageBox;
				exitMessageBox= NULL;
			}
		}

   //save box
	} else if(saveBox) {
		int button;
		if (saveBox->mouseClick(x, y, button)) {
			if (button == 1) {
				saveGame(saveBox->getEntry()->getText());
			}
			//close message box
			delete saveBox;
			saveBox = NULL;
		}

	} else {
		gui.mouseDownLeft(x, y);
	}
}

void Game::mouseDownRight(int x, int y){
	gui.mouseDownRight(x, y);
}

void Game::mouseUpLeft(int x, int y){
	gui.mouseUpLeft(x, y);
}

void Game::mouseUpRight(int x, int y){
	gui.mouseUpRight(x, y);
}

void Game::mouseDownCenter(int x, int y) {
	gameCamera.stop();
}

void Game::mouseUpCenter(int x, int y) {
}

void Game::mouseDoubleClickLeft(int x, int y){
	if(!(exitMessageBox  && exitMessageBox->isInBounds(x, y))
			&& !(saveBox && saveBox->isInBounds(x, y))) {
		gui.mouseDoubleClickLeft(x, y);
	}
}

void Game::mouseMove(int x, int y, const MouseState *ms){
	const Metrics &metrics= Metrics::getInstance();
	Vec2i mmCell;

    mouseX= x;
    mouseY= y;

	if(ms->centerMouse) {
		if(gui.isControlDown()) {
			float speed = gui.isShiftDown() ? 1.f : 0.125f;
			float response = gui.isShiftDown() ? 0.1875f : 0.0625f;
			gameCamera.moveForwardH((y - lastMousePos.y) * speed, response);
			gameCamera.moveSideH((x - lastMousePos.x) * speed, response);
		} else {
			float ymult = Config::getInstance().getCameraInvertYAxis() ? -0.2f : 0.2f;
			float xmult = Config::getInstance().getCameraInvertXAxis() ? -0.2f : 0.2f;
			gameCamera.transitionVH(-(y - lastMousePos.y) * ymult, (lastMousePos.x - x) * xmult);
		}
	} else if(ms->leftMouse && gui.getMinimapCell(x, y, mmCell)) {
		gameCamera.setPos(Vec2f(static_cast<float>(mmCell.x), static_cast<float>(mmCell.y)));
	} else {
		//main window
		if(y<10){
			gameCamera.setMoveZ(-scrollSpeed);
		}
		else if(y> metrics.getVirtualH()-10){
			gameCamera.setMoveZ(scrollSpeed);
		}
		else{
			gameCamera.setMoveZ(0);
		}

		if(x<10){
			gameCamera.setMoveX(-scrollSpeed);
		}
		else if(x> metrics.getVirtualW()-10){
			gameCamera.setMoveX(scrollSpeed);
		}
		else{
			gameCamera.setMoveX(0);
		}

		if(exitMessageBox){
			exitMessageBox->mouseMove(x, y);
		} else if (saveBox) {
			saveBox->mouseMove(x, y);
		} else {
			//graphics
			gui.mouseMoveGraphics(x, y);
		}
	}
    //display
    if(metrics.isInDisplay(x, y) && !gui.isSelecting() && !gui.isSelectingPos()){
        if(!gui.isSelectingPos()){
			gui.mouseMoveDisplay(x - metrics.getDisplayX(), y - metrics.getDisplayY());
	    }
    }

	lastMousePos.x = x;
	lastMousePos.y = y;
}

void Game::eventMouseWheel(int x, int y, int zDelta) {
	//gameCamera.transitionXYZ(0.0f, -(float)zDelta / 30.0f, 0.0f);
	gameCamera.zoom((float)zDelta / 30.0f);
}

void Game::keyDown(char key){

	Lang &lang= Lang::getInstance();
	bool speedChangesAllowed= !NetworkManager::getInstance().isNetworkGame();

	if(saveBox && saveBox->getEntry()->isActivated()) {
		switch(key) {
		case vkReturn:
			saveGame(saveBox->getEntry()->getText());
			//intentional fall-through
		case vkEscape:
			delete saveBox;
			saveBox = NULL;
			break;

		default:
			saveBox->keyDown(key);
		};
		return;
	} else {
		//send key to the chat manager
		chatManager.keyDown(key);
	}

	if(!chatManager.getEditEnabled()) {

		if(key=='N') {
			renderNetworkStatus= true;
		} else if(key == 'E') {
			Shared::Platform::mkdir("screens", true);

			for(int i = 0; i < 100; ++i) {
				string path= "screens/screen" + intToStr(i) + ".tga";

				FILE *f= fopen(path.c_str(), "rb");
				if(!f) {
					Renderer::getInstance().saveScreen(path);
					break;
				} else {
					fclose(f);
				}
			}
		}

		//move camera left
		else if(key==vkLeft){
			gameCamera.setMoveX(-scrollSpeed);
		}

		//move camera right
		else if(key==vkRight){
			gameCamera.setMoveX(scrollSpeed);
		}

		//move camera up
		else if(key==vkUp){
			gameCamera.setMoveZ(scrollSpeed);
		}

		//move camera down
		else if(key==vkDown){
			gameCamera.setMoveZ(-scrollSpeed);
		}

		//switch display color
		else if(key=='C'){
			gui.switchToNextDisplayColor();
		}

		//change camera mode
		else if(key=='F'){
			gameCamera.switchState();
			string stateString= gameCamera.getState()==GameCamera::sGame? lang.get("GameCamera"): lang.get("FreeCamera");
			console.addLine(lang.get("CameraModeSet")+" "+ stateString);
		}

		//pause
		else if(key=='P') {
			if(speedChangesAllowed){
				if(paused){
					console.addLine(lang.get("GameResumed"));
					paused= false;
				}
				else{
					console.addLine(lang.get("GamePaused"));
					paused= true;
				}
			}
		}

		//increment speed
		else if(key==vkAdd){
			if(speedChangesAllowed){
				incSpeed();
			}
		}

		//decrement speed
		else if(key==vkSubtract){
			if(speedChangesAllowed){
				decSpeed();
			}
		}

		//turn command queuing on
		else if(key==vkShift){
			gui.setShiftDown(true);
		}

		//control key
		else if(key==vkControl){
			gui.setControlDown(true);
		}

		//exit
		else if(key==vkEscape){
			if(!gui.cancelPending()) {
				showExitMessageBox(lang.get("ExitGame?"), true);
			}
		}

		//save
		else if(key == 'Z'){
			if(saveBox==NULL){
				Shared::Platform::mkdir("savegames", true);
				saveBox= new GraphicTextEntryBox();
				saveBox->init(lang.get("Save"), lang.get("Cancel"), lang.get("SaveGame"), lang.get("Name"));
			} else {
				delete saveBox;
				saveBox= NULL;
			}
		}

		//group
		else if(key>='0' && key<'0'+Selection::maxGroups){
			gui.groupKey(key-'0');
		}

		//hotkeys
		if(gameCamera.getState()==GameCamera::sGame){
			gui.hotKey(key);
		}
		else{
			//rotate camera leftt
			if(key=='A'){
				gameCamera.setRotate(-1);
			}

			//rotate camera right
			else if(key=='D'){
				gameCamera.setRotate(1);
			}

			//camera up
			else if(key=='S'){
				gameCamera.setMoveY(1);
			}

			//camera down
			else if(key=='W'){
				gameCamera.setMoveY(-1);
			}
		}
	}
}

void Game::keyUp(char key){

	if(!chatManager.getEditEnabled()){
		switch(key){
		case 'N':
			renderNetworkStatus= false;
			break;

		case vkShift:
			gui.setShiftDown(false);
			break;

		case vkControl:
			gui.setControlDown(false);
			break;

		case 'A':
		case 'D':
			gameCamera.setRotate(0);
			break;

		case 'W':
		case 'S':
			gameCamera.setMoveY(0);
			break;

		case vkUp:
		case vkDown:
			gameCamera.setMoveZ(0);
			break;

		case vkLeft:
		case vkRight:
			gameCamera.setMoveX(0);
			break;
		}
	}
}

void Game::keyPress(char c){
	chatManager.keyPress(c);
}

// ==================== PRIVATE ====================

// ==================== render ====================

void Game::render3d(){

	Renderer &renderer= Renderer::getInstance();

	//init
	renderer.reset3d();
	renderer.computeVisibleQuad();
	renderer.loadGameCameraMatrix();
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

	//init
	renderer.reset2d();

	//display
	renderer.renderDisplay();

	//minimap
	if(!config.getPhotoMode()){
        renderer.renderMinimap();
	}

    //selection
	renderer.renderSelectionQuad();

	//exit message box
	if(exitMessageBox){
		renderer.renderMessageBox(exitMessageBox);
	}

	//save box
	if (saveBox) {
		renderer.renderTextEntryBox(saveBox);
	}

	renderer.renderChatManager(&chatManager);

    //debug info
	if(config.getDebugMode()){
        string str;

		str += "MouseXY: " + intToStr(mouseX) + "," + intToStr(mouseY) + "\n";
		str += "PosObjWord: " + intToStr(gui.getPosObjWorld().x) + "," + intToStr(gui.getPosObjWorld().y) + "\n";
		str += "Render FPS: " + intToStr(lastRenderFps) + "\n";
		str += "Update FPS: " + intToStr(lastUpdateFps) + "\n";
		str += "GameCamera pos: " + floatToStr(gameCamera.getPos().x)
				+ "," + floatToStr(gameCamera.getPos().y)
				+ "," + floatToStr(gameCamera.getPos().z) + "\n";
		str += "Time: " + floatToStr(world.getTimeFlow()->getTime()) + "\n";
		str += "Triangle count: " + intToStr(renderer.getTriangleCount()) + "\n";
		str += "Vertex count: " + intToStr(renderer.getPointCount()) + "\n";
		str += "Frame count: " + intToStr(world.getFrameCount()) + "\n";

        for(int i=0; i<world.getFactionCount(); ++i){
            str+= "Player "+intToStr(i)+" res: ";
            for(int j=0; j<world.getTechTree()->getResourceTypeCount(); ++j){
                str+= intToStr(world.getFaction(i)->getResource(j)->getAmount());
                str+=" ";
            }
            str+="\n";
        }

		Renderer::getInstance().renderText(
			str, coreData.getMenuFontNormal(),
			gui.getDisplay()->getColor(), 10, 500, false);
	}

	//network status
	if(renderNetworkStatus){
		Renderer::getInstance().renderText(
			NetworkManager::getInstance().getGameNetworkInterface()->getNetworkStatus(),
			coreData.getMenuFontNormal(),
			gui.getDisplay()->getColor(), 20, 500, false);
	}

    //resource info
	if(!config.getPhotoMode()){
        renderer.renderResourceStatus();
		renderer.renderConsole(&console);
    }

    //2d mouse
	renderer.renderMouse2d(mouseX, mouseY, mouse2d, gui.isSelectingPos()? 1.f: 0.f);
}


// ==================== misc ====================

void Game::checkWinner(){
	if(!gameOver){
		Lang &lang= Lang::getInstance();

		//lose
		bool lose= false;
		if(!hasBuilding(world.getThisFaction())){
			lose= true;
			for(int i=0; i<world.getFactionCount(); ++i){
				if(!world.getFaction(i)->isAlly(world.getThisFaction())){
					world.getStats()->setVictorious(i);
				}
			}

			gameOver= true;
			showExitMessageBox(lang.get("YouLose")+", "+lang.get("ExitGame?"), false);
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
						world.getStats()->setVictorious(i);
					}
				}
				gameOver= true;
				showExitMessageBox(lang.get("YouWin")+", "+lang.get("ExitGame?"), false);
			}
		}
	}
}

bool Game::hasBuilding(const Faction *faction){
	for(int i=0; i<faction->getUnitCount(); ++i){
		Unit *unit = faction->getUnit(i);
		if(unit->getType()->hasSkillClass(scBeBuilt) && unit->isAlive()){
			return true;
		}
	}
	return false;
}

void Game::updateSpeed() {
	Lang &lang= Lang::getInstance();
	console.addLine(lang.get("GameSpeedSet")+" "+lang.get(SpeedDesc[speed]));
	if(speed == sNormal) {
		fUpdateLoops = 1.0f;
	} else if(speed > sNormal) {
		fUpdateLoops = 1.0f + (float)(speed - sNormal) / (float)(sFastest - sNormal)
				* (Config::getInstance().getFastestSpeed() - 1.0f);
	} else {
		fUpdateLoops = Config::getInstance().getSlowestSpeed() + (float)(speed) / (float)(sNormal)
				* (1.0f - Config::getInstance().getSlowestSpeed());
	}
}

void Game::incSpeed(){
	if(speed < sFastest) {
		speed = (Speed)(speed + 1);
		updateSpeed();
	}
}

void Game::decSpeed(){
	if(speed > sSlowest) {
		speed = (Speed)(speed - 1);
		updateSpeed();
	}
}

int Game::getUpdateLoops(){
	if(paused || (!NetworkManager::getInstance().isNetworkGame() && (saveBox || exitMessageBox))){
		return 0;
	} else {
		int updateLoops = (int)(fUpdateLoops + lastUpdateLoopsFraction);
		lastUpdateLoopsFraction = fUpdateLoops + lastUpdateLoopsFraction - (float)updateLoops;
		return updateLoops;
	}
}

void Game::showExitMessageBox(const string &text, bool toggle){
	Lang &lang= Lang::getInstance();

	if(exitMessageBox && toggle){
		delete exitMessageBox;
		exitMessageBox= NULL;
		return;
	}

	if(!exitMessageBox){
		exitMessageBox= new GraphicMessageBox();
		exitMessageBox->init(text, lang.get("Yes"), lang.get("No"));
	}
}

void Game::saveGame(string name) const {
	XmlNode root("saved-game");
	root.addAttribute("version", "1");
	gui.save(root.addChild("gui"));
	gameSettings.save(root.addChild("settings"));
	world.save(root.addChild("world"));
	XmlIo::getInstance().save("savegames/" + name + ".sav", &root);
}

}}//end namespace
