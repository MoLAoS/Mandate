// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//				  2008-2009 Daniel Santos <daniel.santos@pobox.com>
//				  2009-2010 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_GAME_H_
#define _GLEST_GAME_GAME_H_

#include <vector>

#include "user_interface.h"
#include "game_camera.h"
#include "world.h"
#include "ai_interface.h"
#include "program.h"
#include "chat_dialog.h"
#include "script_manager.h"
#include "game_settings.h"
#include "config.h"
#include "keymap.h"
#include "battle_end.h"
#include "compound_widgets.h"

// weather system not yet ready
//#include "../physics/weather.h"

using std::vector;
using std::ifstream;

namespace Glest { 
namespace Sim {
	class SimulationInterface;
}
namespace Graphics {
	class GraphicMessageBox;
	class GraphicTextEntryBox;
}
using namespace Graphics;
using namespace Main;

namespace Gui {

using Widgets::MessageDialog;

struct ScriptMessage {
	string header;
	string text;
	ScriptMessage(const string &header, const string &text) : header(header), text(text) {}
};

// =====================================================
// 	class GameState
//
//	Main game class
// =====================================================

class GameState: public ProgramState, public sigslot::has_slots {
private:
	//REMOVE? GameState does not own or know anything about AIs
	typedef vector<Plan::Ai*> Ais;
	typedef vector<Plan::AiInterface*> AiInterfaces;

	typedef deque<ScriptMessage> MessageQueue;

protected:
	static GameState *singleton;

	//main data
	SimulationInterface *simInterface;
	Keymap &keymap;
	const Input &input;
	const Config &config;
	UserInterface gui;
	GameCamera gameCamera;

	//misc
	Checksum checksum;
	string loadingText;
	int mouse2d;
	int mouseX, mouseY; //coords win32Api
	int worldFps, lastWorldFps;
	int updateFps, lastUpdateFps;
	int renderFps, lastRenderFps;
	bool noInput;
	bool netError, gotoMenu, exitGame, exitProgram;
	float scrollSpeed;

	MessageQueue m_scriptMessages;

	Frame*			m_modalDialog;
	ChatDialog*		m_chatDialog;

	Vec2i lastMousePos;

	//misc ptr
	ParticleSystem *weatherParticleSystem;

	//UnitVector lastPickUnits;
	//const Object *lastPickObject;
	//vector<string> rawPick;
	//vector<string> unitPickHits;

public:
	GameState(Program &program);
	virtual ~GameState();
	static GameState *getInstance()				{return singleton;}

	//get
	const GameSettings &getGameSettings();
	const Keymap &getKeymap() const			{return keymap;}
	const Input &getInput() const			{return input;}

	const GameCamera *getGameCamera() const	{return &gameCamera;}
	GameCamera *getGameCamera()				{return &gameCamera;}
	const UserInterface *getGui() const		{return &gui;}
	UserInterface *getGui()					{return &gui;}
	
	// ProgramState implementation

	//void resetRawPick() { rawPick.clear(); }
	//void addRawPick(const string &str) {
	//	rawPick.push_back(str);
	//}

	//void resetUnitPickHits() { unitPickHits.clear(); }
	//void addUnitPickHit(const string &str) {
	//	unitPickHits.push_back(str);
	//}

	//void lastPick(UnitVector &units, const Object *obj) {
	//	lastPickUnits = units;
	//	lastPickObject = obj;
	//}

	// init
	virtual void load();
	virtual void init();
	virtual int getUpdateFps() const {return 40;}
	virtual void update();
	virtual void updateCamera();
	virtual void renderBg();
	virtual void renderFg();
	virtual void tick();

	// event handlers
	virtual void keyDown(const Key &key);
	virtual void keyUp(const Key &key);
	virtual void keyPress(char c);
	virtual void mouseDownLeft(int x, int y);
	virtual void mouseDownRight(int x, int y);
	virtual void mouseUpLeft(int x, int y);
	virtual void mouseUpRight(int x, int y);
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

	void addScriptMessage(const string &header, const string &msg);
	//void onScriptMessageDismissed(BasicDialog*);

	virtual void quitGame();

	virtual bool isGameState() const	{ return true; }

	void confirmQuitGame();
	void confirmExitProgram();

	void onConfirmQuitGame(BasicDialog*);
	void onConfirmExitProgram(BasicDialog*);

	void destroyDialog(BasicDialog* ptr = 0);

	void doDefeatedMessage(Faction *f);

protected:
	// render
	void render3d();
	virtual void render2d();
	
	// show messages
	void showLoseMessageBox();
	void showWinMessageBox();
	string controllerTypeToStr(ControlType ct);
	
	//char getStringFromFile(ifstream *fileStream, string *str);
	void saveGame(string name) const;

	void doSaveBox();
	void onSaveSelected(BasicDialog*);

	void displayError(std::exception &e);
	void onErrorDismissed(BasicDialog*);

	void doGameMenu();
	void doExitMessage(const string &msg);
	void onExitSelected(BasicDialog*);

	void doScriptMessage();

	void doChatDialog();
	//void updateChatDialog();

	void onChatEntered(BasicDialog*);
	void onChatCancel(BasicDialog*);
};


//TODO: better integrate with GameState, much duplicated code
class QuickScenario : public GameState {
public:
	QuickScenario(Program &program) : GameState(program) {}
	~QuickScenario() {}
	void quitGame() {
		if (m_modalDialog) {  //FIXME: shouldn't BattleEnd remove all widgets?
			g_widgetWindow.removeFloatingWidget(m_modalDialog);
			m_modalDialog = 0;
		}
		program.setState(new BattleEnd(program, true));
	}
};

class ShowMap : public GameState {
public:
	ShowMap(Program &program) : GameState(program) { }
	~ShowMap(){}
	void quitGame() { program.exit(); }
	void render2d();
	void keyDown(const Key &key);
};

}}//end namespace

#endif
