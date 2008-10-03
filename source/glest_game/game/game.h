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
#include "game_settings.h"
#include "config.h"

using std::vector;

namespace Glest{ namespace Game{

class GraphicMessageBox;
class GraphicTextEntryBox;

// =====================================================
// 	class Game
//
//	Main game class
// =====================================================

class Game: public ProgramState{
public:
	enum Speed{
		sSlowest,
		sVerySlow,
		sSlow,
		sNormal,
		sFast,
		sVeryFast,
		sFastest,

  		sCount
	};

	static const char*SpeedDesc[sCount];

private:
	typedef vector<Ai*> Ais;
	typedef vector<AiInterface*> AiInterfaces;

private:
	//main data
	World world;
    AiInterfaces aiInterfaces;
    Gui gui;
    GameCamera gameCamera;
    Commander commander;
    Console console;
	ChatManager chatManager;

	//misc
	Checksum checksum;
    string loadingText;
    int mouse2d;
    int mouseX, mouseY; //coords win32Api
	int updateFps, lastUpdateFps;
	int renderFps, lastRenderFps;
	bool paused;
	bool gameOver;
	bool renderNetworkStatus;
	float scrollSpeed;
	Speed speed;
	float fUpdateLoops;
	float lastUpdateLoopsFraction;
	GraphicMessageBox *exitMessageBox;
	GraphicTextEntryBox *saveBox;
	Vec2i lastMousePos;

	//misc ptr
	ParticleSystem *weatherParticleSystem;
	GameSettings gameSettings;
	XmlNode *savedGame;

public:
	/** Constructor to start a new game or join a new network game */
	Game(Program *program, const GameSettings *gs) :
			ProgramState(program),
			world(),
			gameSettings(*gs) {
		_init();
		this->savedGame = NULL;
	}

	/**
	 * Constructor to load a saved game or join a network game that is resumed
	 * resumes from a previous session
	 */
	Game(Program *program, const GameSettings *gs, XmlNode *savedGame) :
			ProgramState(program),
			world(),
			gameSettings(*gs) {
		_init();
		this->savedGame = savedGame;
	}

    ~Game();

    //get
	GameSettings *getGameSettings()			{return &gameSettings;}

	const GameCamera *getGameCamera() const	{return &gameCamera;}
	GameCamera *getGameCamera()				{return &gameCamera;}
	const Commander *getCommander() const	{return &commander;}
	Gui *getGui()							{return &gui;}
	const Gui *getGui() const				{return &gui;}
	Commander *getCommander()				{return &commander;}
	Console *getConsole()					{return &console;}
	World *getWorld()						{return &world;}
	const World *getWorld() const			{return &world;}

    //init
    virtual void load();
    virtual void init();
	virtual void update();
	virtual void updateCamera();
	virtual void render();
	virtual void tick();

    //Event managing
    virtual void keyDown(char key);
    virtual void keyUp(char key);
    virtual void keyPress(char c);
    virtual void mouseDownLeft(int x, int y);
    virtual void mouseDownRight(int x, int y);
    virtual void mouseUpLeft(int x, int y);
    virtual void mouseUpRight(int x, int y);
	virtual void mouseDownCenter(int x, int y);
	virtual void mouseUpCenter(int x, int y);
    virtual void mouseDoubleClickLeft(int x, int y);
	virtual void eventMouseWheel(int x, int y, int zDelta);
    virtual void mouseMove(int x, int y, const MouseState *mouseState);

	void setCameraCell(int x, int y)	{
		gameCamera.setPos(Vec2f(static_cast<float>(x), static_cast<float>(y)));
	}

private:
	//render
    void render3d();
    void render2d();

	//misc
	void _init();
	void checkWinner();
	bool hasBuilding(const Faction *faction);
	void incSpeed();
	void decSpeed();
	void updateSpeed();
	int getUpdateLoops();
	void showExitMessageBox(const string &text, bool toggle);
	string controllerTypeToStr(ControlType ct);
	Unit *findUnit(int id);
	char getStringFromFile(ifstream *fileStream, string *str);
	void saveGame(string name) const;
};

}}//end namespace

#endif
