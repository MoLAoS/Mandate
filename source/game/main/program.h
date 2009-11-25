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
#include "window_gl.h"
#include "socket.h"
#include "metrics.h"
#include "components.h"
#include "keymap.h"

/*
using Shared::Graphics::Context;
using Shared::Platform::WindowGl;
using Shared::Platform::SizeState;
using Shared::Platform::MouseState;
using Shared::Platform::PerformanceTimer;
using Shared::Platform::Ip;
*/
using namespace Shared::Platform;

namespace Glest { namespace Game {

class Program;
class MainWindow;

// =====================================================
// 	class ProgramState
//
///	Base class for all program states:
/// Intro, MainMenu, Game, BattleEnd (State Design pattern)
// =====================================================

class ProgramState {
protected:
	Program &program;

public:
	ProgramState(Program &program) : program(program) {}
	virtual ~ProgramState(){}

	virtual void render() = 0;
	virtual void update(){}
	virtual void updateCamera(){}
	virtual void tick(){}
	virtual void init(){}
	virtual void load(){}
	virtual void end(){}
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

class Program : public WindowGl {
private:
	class CrashProgramState : public ProgramState {
		GraphicMessageBox msgBox;
		int mouseX;
		int mouseY;
		int mouse2dAnim;
		const exception *e;

	public:
		CrashProgramState(Program &program, const exception *e);

		virtual void render();
		virtual void mouseDownLeft(int x, int y);
		virtual void mouseMove(int x, int y, const MouseState &mouseState);
		virtual void update();
	};

	static const int maxTimes;
	static Program *singleton;

	PerformanceTimer renderTimer;
	PerformanceTimer tickTimer;
	PerformanceTimer updateTimer;
	PerformanceTimer updateCameraTimer;

    ProgramState *programState;
	bool crashed;
	bool terminating;
	Keymap keymap;

private:
	Program(const Program &);
	const Program &operator =(const Program &);
	
public:
    Program(Config &config, int argc, char** argv);
    ~Program();
	static Program *getInstance()	{return singleton;}

	bool isTerminating() const		{ return terminating; }
	Keymap &getKeymap() 			{return keymap;}

	virtual void eventMouseDown(int x, int y, MouseButton mouseButton) {
		const Metrics &metrics = Metrics::getInstance();
		int vx = metrics.toVirtualX(x);
		int vy = metrics.toVirtualY(getH() - y);

		switch(mouseButton) {
		case mbLeft:
			programState->mouseDownLeft(vx, vy);
			break;
		case mbRight:
			programState->mouseDownRight(vx, vy);
			break;
		case mbCenter:
			programState->mouseDownCenter(vx, vy);
			break;
		default:
			break;
		}
	}

	virtual void eventMouseUp(int x, int y, MouseButton mouseButton) {
		const Metrics &metrics = Metrics::getInstance();
		int vx = metrics.toVirtualX(x);
		int vy = metrics.toVirtualY(getH() - y);

		switch(mouseButton) {
		case mbLeft:
			programState->mouseUpLeft(vx, vy);
			break;
		case mbRight:
			programState->mouseUpRight(vx, vy);
			break;
		case mbCenter:
			programState->mouseUpCenter(vx, vy);
			break;
		default:
			break;
		}
	}

	virtual void eventMouseMove(int x, int y, const MouseState &ms) {
		const Metrics &metrics= Metrics::getInstance();
		int vx = metrics.toVirtualX(x);
		int vy = metrics.toVirtualY(getH() - y);

		programState->mouseMove(vx, vy, ms);
	}

	virtual void eventMouseDoubleClick(int x, int y, MouseButton mouseButton) {
		const Metrics &metrics= Metrics::getInstance();
		int vx = metrics.toVirtualX(x);
		int vy = metrics.toVirtualY(getH() - y);

		switch(mouseButton){
		case mbLeft:
			programState->mouseDoubleClickLeft(vx, vy);
			break;
		case mbRight:
			programState->mouseDoubleClickRight(vx, vy);
			break;
		case mbCenter:
			programState->mouseDoubleClickCenter(vx, vy);
			break;
		default:
			break;
		}
	}

	virtual void eventMouseWheel(int x, int y, int zDelta) {
		const Metrics &metrics= Metrics::getInstance();
		int vx = metrics.toVirtualX(x);
		int vy = metrics.toVirtualY(getH() - y);

		programState->eventMouseWheel(vx, vy, zDelta);
	}

	// FIXME: using both left & right alt/control/shift at the same time will cause these to be
	// incorrect on some platforms (not sure that anybody cares though).
	virtual void eventKeyDown(const Key &key)	{programState->keyDown(key);}
	virtual void eventKeyUp(const Key &key)		{programState->keyUp(key);}
	virtual void eventKeyPress(char c)			{programState->keyPress(c);}

	virtual void eventActivate(bool active) {
		if (!active) {
			//minimize();
		}
	}

	virtual void eventResize(SizeState sizeState);

	/*
	// Unused events of Window
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

#endif
