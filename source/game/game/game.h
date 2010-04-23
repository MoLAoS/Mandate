// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_GAME_H_
#define _GLEST_GAME_GAME_H_

#include <vector>

#include "gui.h"
#include "game_camera.h"
#include "world.h"
#include "ai_interface.h"
#include "program.h"
#include "chat_manager.h"
#include "script_manager.h"
#include "game_settings.h"
#include "config.h"
#include "keymap.h"

// weather system not yet ready
//#include "../physics/weather.h"

using std::vector;
using std::ifstream;

namespace Glest { namespace Sim {
class SimulationInterface;
}}


namespace Glest { namespace Game {

class GraphicMessageBox;
class GraphicTextEntryBox;

// =====================================================
// 	class GameState
//
//	Main game class
// =====================================================

class GameState: public ProgramState {
public:
	/*WRAPPED_ENUM( GameSpeed,
		SLOWEST,
		VERY_SLOW,
		SLOW,
		NORMAL,
		FAST,
		VERY_FAST,
		FASTEST
	)*/
	
private:
	typedef vector<Ai*> Ais;
	typedef vector<AiInterface*> AiInterfaces;

protected:
	static GameState *singleton;

	//main data
	SimulationInterface *simInterface;
	//const GameSettings &gameSettings;
	//XmlNode *savedGame;
	Keymap &keymap;
	const Input &input;
	const Config &config;
	//World world;
	//AiInterfaces aiInterfaces;
	Gui gui;
	GameCamera gameCamera;
	//Commander commander;
	Console console;
	ChatManager chatManager;

	//misc
	Checksum checksum;
	string loadingText;
	int mouse2d;
	int mouseX, mouseY; //coords win32Api
	int worldFps, lastWorldFps;
	int updateFps, lastUpdateFps;
	int renderFps, lastRenderFps;
	//bool paused;
	bool noInput;
	//bool gameOver;
	bool netError;
	float scrollSpeed;
	//GameSpeed speed;
	//float fUpdateLoops;
	//float lastUpdateLoopsFraction;
	GraphicMessageBox mainMessageBox;

	GraphicTextEntryBox *saveBox;
	Vec2i lastMousePos;

	//misc ptr
	ParticleSystem *weatherParticleSystem;

public:
	GameState(Program &program, XmlNode *savedGame = NULL);
	//GameState(Program &program, const GameSettings &gs, XmlNode *savedGame = NULL);
	virtual ~GameState();
	static GameState *getInstance()				{return singleton;}

	//get
	const GameSettings &getGameSettings();
	const Keymap &getKeymap() const			{return keymap;}
	const Input &getInput() const			{return input;}

	const GameCamera *getGameCamera() const	{return &gameCamera;}
	GameCamera *getGameCamera()				{return &gameCamera;}
	//const Commander *getCommander() const	{return &commander;}
	//Commander *getCommander()				{return &commander;}
	const Gui *getGui() const				{return &gui;}
	Gui *getGui()							{return &gui;}
	Console *getConsole()					{return &console;}
//	const World *getWorld() const			{return &world;}
//	World *getWorld()						{return &world;}
	
	// ProgramState implementation

	// init
	virtual void load();
	virtual void init();
	virtual void update();
	virtual void updateCamera();
	virtual void render();
	virtual void tick();

	// event handlers
	virtual void keyDown(const Key &key);
	virtual void keyUp(const Key &key);
	virtual void keyPress(char c);
	virtual void mouseDownLeft(int x, int y);
	virtual void mouseDownRight(int x, int y)			{gui.mouseDownRight(x, y);}
	virtual void mouseUpLeft(int x, int y)				{gui.mouseUpLeft(x, y);}
	virtual void mouseUpRight(int x, int y)				{gui.mouseUpRight(x, y);}
	virtual void mouseDownCenter(int x, int y)			{gameCamera.stop();}
	virtual void mouseUpCenter(int x, int y)			{}
	virtual void mouseDoubleClickLeft(int x, int y);
	virtual void eventMouseWheel(int x, int y, int zDelta);
	virtual void mouseMove(int x, int y, const MouseState &mouseState);


	// game/camera control (used by ScriptManager)
	void lockInput()	{ noInput = true;	}
	void unlockInput()	{ noInput = false;	}
	void setCameraCell(int x, int y) {
		gameCamera.setPos(Vec2f(static_cast<float>(x), static_cast<float>(y)));
	}
	virtual void quitGame();
	//void pause()							{paused = true;}
	//void resume()							{paused = false;}

protected:
	// render
	void render3d();
	virtual void render2d();
	
	// game over checks
	//void checkWinner();
	//void checkWinnerStandard();
	//void checkWinnerScripted();
	//bool hasBuilding(const Faction *faction);

	// game speed control
	//void incSpeed();
	//void decSpeed();
	//void resetSpeed();
	//void updateSpeed();
	//int getUpdateLoops();

	// show messages
	void showLoseMessageBox();
	void showWinMessageBox();
	void showMessageBox(const string &text, const string &header, bool toggle);

//	void showExitMessageBox(const string &text, bool toggle);
	string controllerTypeToStr(ControlType ct);
	
	//char getStringFromFile(ifstream *fileStream, string *str);
	void saveGame(string name) const;
	void displayError(std::exception &e);
};


//TODO: better integrate with GameState, much duplicated code
class ShowMap : public GameState {
public:
	ShowMap(Program &program) : GameState(program) { }
	~ShowMap(){}
	void quitGame() { program.exit(); }
	void init();
	void render2d();
	void keyDown(const Key &key);
};

}}//end namespace

#endif
