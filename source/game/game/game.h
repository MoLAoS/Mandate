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
#include "framed_widgets.h"
#include "debug_stats.h"
#include "debug_widgets.h"
#include "game_menu.h"

// weather system not yet ready
//#include "../physics/weather.h"

using std::vector;
using std::ifstream;

namespace Glest { namespace Gui {

using namespace Graphics;
using namespace Main;
using namespace Debug;
using Widgets::MessageDialog;

class OptionsFrame;

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
	Vec2f m_cameraDragCenter;

	DebugStats		m_debugStats;

	Vec2i			m_scriptDisplayPos;
	string			m_scriptDisplay;
	MessageQueue m_scriptMessages;

	Frame*			m_modalDialog;
	ChatDialog*		m_chatDialog;
	DebugPanel*     m_debugPanel;
	GameMenu*       m_gameMenu;
	OptionsFrame*   m_options;

	Vec2i lastMousePos;

	//misc ptr
	ParticleSystem *weatherParticleSystem;

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
	DebugStats* getDebugStats()             {return &m_debugStats;}
	Vec2i getMousePos() const				{return Vec2i(mouseX, mouseY);}
	
	// ProgramState implementation

	// init
	virtual void load();
	virtual void init();
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
	void setScriptDisplay(const string &msg);
	//void onScriptMessageDismissed(BasicDialog*);

	virtual void quitGame();

	virtual bool isGameState() const	{ return true; }

	void confirmQuitGame();
	void confirmExitProgram();

	void onConfirmQuitGame(Widget*);
	void onConfirmExitProgram(Widget*);

	void destroyDialog(Widget* ptr = 0);

	void doDefeatedMessage(Faction *f);
	void doSaveBox();

	void toggleDebug(Widget* ptr = 0);
	void toggleGameMenu(Widget* ptr = 0);
	void toggleOptions(Widget* ptr = 0);
	void togglePinWidgets(Widget* ptr = 0);

	void rejigWidgets();

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
	void onSaveSelected(Widget*);

	void displayError(std::exception &e);
	void onErrorDismissed(Widget*);

	void doGameMenu();
	void doExitMessage(const string &msg);
	void onExitSelected(Widget*);

	void doScriptMessage();

	void doChatDialog();

	void onChatEntered(Widget*);
	void onChatCancel(Widget*);
};


///@todo: better integrate with GameState, much duplicated code
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
