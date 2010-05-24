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

#ifndef _GLEST_GAME_PROGRAM_H_
#define _GLEST_GAME_PROGRAM_H_

#include "context.h"
#include "timer.h"
#include "platform_util.h"
#include "socket.h"
#include "metrics.h"
#include "components.h"
#include "keymap.h"
#include "CmdArgs.h"
#include "forward_decs.h"
#include "game_constants.h"
#include "widget_window.h"

using namespace Shared::Platform;
using namespace Glest::Graphics;
using namespace Glest::Global;
using Glest::Sim::SimulationInterface;
using Glest::Gui::Keymap;

namespace Glest { namespace Main {

using Widgets::WidgetWindow;

class Program;
class MainWindow;

// =====================================================
// 	class ProgramState
//
///	Base class for all program states:
/// Intro, MainMenu, GameState, BattleEnd (State Design pattern)
// =====================================================

class ProgramState {
protected:
	Program &program;

public:
	ProgramState(Program &program) : program(program) {}
	virtual ~ProgramState(){}

    virtual void renderBg() = 0;
    virtual void renderFg() = 0;
	virtual void update(){}
	virtual void updateCamera(){}
	virtual void tick(){}
	virtual void init(){}
	virtual void load(){}
	virtual void end(){}

	virtual int getUpdateInterval() const { return GameConstants::defaultUpdateInterval; }

	virtual void mouseDownLeft(int x, int y){}
	virtual void mouseDownRight(int x, int y){}
	virtual void mouseDownCenter(int x, int y){}
	virtual void mouseUpLeft(int x, int y){}
	virtual void mouseUpRight(int x, int y){}
	virtual void mouseUpCenter(int x, int y){}
	virtual void mouseDoubleClickLeft(int x, int y){}
	virtual void mouseDoubleClickRight(int x, int y){}
	virtual void mouseDoubleClickCenter(int x, int y){}
	virtual void eventMouseWheel(int x, int y, int zDelta){}
	virtual void mouseMove(int x, int y, const MouseState &mouseState){}
	virtual void keyDown(const Key &key){}
	virtual void keyUp(const Key &key){}
	virtual void keyPress(char c){}
};

// ===============================
// 	class Program
// ===============================

class Program : public WidgetWindow {
	friend class Glest::Sim::SimulationInterface;
private:
	class CrashProgramState : public ProgramState {
		GraphicMessageBox msgBox;
		int mouseX;
		int mouseY;
		int mouse2dAnim;
		const exception *e;

	public:
		CrashProgramState(Program &program, const exception *e);

		virtual void renderBg();
		virtual void renderFg();
		virtual void mouseDownLeft(int x, int y);
		virtual void mouseMove(int x, int y, const MouseState &mouseState);
		virtual void update();
	};

	static const int maxUpdateTimes = 5 * 6;
	static const int maxUpdateBackLog = 12; ///@todo should be speed dependant
	static const int maxTimes = 5;

	static Program *singleton;
	CmdArgs cmdArgs;

	PerformanceTimer renderTimer;
	PerformanceTimer tickTimer;
	PerformanceTimer updateTimer;
	PerformanceTimer updateCameraTimer;

	SimulationInterface *simulationInterface;

	ProgramState *programState;
	bool crashed;
	bool terminating;
	bool visible;
	Keymap keymap;

	void init();

public:
	Program(Config &config, CmdArgs &args);
	~Program();
	static Program *getInstance()	{return singleton;}
	const CmdArgs& getCmdArgs()	const { return cmdArgs; }

	bool isTerminating() const		{ return terminating;	}
	bool isVisible() const			{ return visible;		}
	Keymap &getKeymap() 			{ return keymap;		}

	SimulationInterface* getSimulationInterface() { return simulationInterface; }
	void setSimInterface(SimulationInterface *si);

	// Widget virtuals
	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos);
	virtual bool mouseWheel(Vec2i pos, int zDelta);
	virtual bool keyDown(Key key);
	virtual bool keyUp(Key key);
	virtual bool keyPress(char c);

		// Window virtuals
	virtual void eventActivate(bool active) {}
	virtual void eventResize(SizeState sizeState);
	// Unused events of Window
	/*
	virtual void eventCreate(){}
	virtual void eventClose(){}
	virtual void eventResize(){};
	virtual void eventPaint(){}
	virtual void eventTimer(int timerId){}
	virtual void eventMenu(int menuId){}
	virtual void eventClose(){};
	virtual void eventDestroy(){};
	*/

	//misc
	void setState(ProgramState *programState);
	void crash(const exception *e);
	void loop();
	void exit();
	void setMaxUpdateBacklog(int maxBacklog)	{updateTimer.setMaxBacklog(maxBacklog);}
	void resetTimers();

private:
	void setDisplaySettings();
	void restoreDisplaySettings();
};

}} //end namespace

#define theSimInterface (Program::getInstance()->getSimulationInterface())

#endif
