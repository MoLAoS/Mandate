// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_PROGRAM_H_
#define _GLEST_GAME_PROGRAM_H_

#include "context.h"
#include "platform_util.h"
#include "window_gl.h"
#include "socket.h"
#include "metrics.h"

using Shared::Graphics::Context;
using Shared::Platform::WindowGl;
using Shared::Platform::SizeState;
using Shared::Platform::MouseState;
using Shared::Platform::PerformanceTimer;
using Shared::Platform::Ip;

namespace Glest{ namespace Game{

class Program;
class MainWindow;

// =====================================================
// 	class ProgramState
//
///	Base class for all program states:
/// Intro, MainMenu, Game, BattleEnd (State Design pattern)
// =====================================================

class ProgramState{
protected:
	Program *program;

public:
	ProgramState(Program *program)	{this->program= program;}
	virtual ~ProgramState(){};

	virtual void render()=0;
	virtual void update(){};
	virtual void updateCamera(){};
	virtual void tick(){};
	virtual void init(){};
	virtual void load(){};
	virtual void end(){};
	virtual void mouseDownLeft(int x, int y){};
	virtual void mouseUpLeft(int x, int y){};
	virtual void mouseDownRight(int x, int y){};
	virtual void mouseDoubleClickLeft(int x, int y){};
	virtual void mouseDownCenter(int x, int y){};
	virtual void mouseUpCenter(int x, int y){};
	virtual void eventMouseWheel(int x, int y, int zDelta){};
	virtual void mouseMove(int x, int y, const MouseState *mouseState){};
	virtual void keyDown(char key){};
	virtual void keyUp(char key){};
	virtual void keyPress(char c){};
};

// ===============================
// 	class Program
// ===============================

class Program{
private:
	static const int maxTimes;

private:
    ProgramState *programState;

	PerformanceTimer fpsTimer;
	PerformanceTimer updateTimer;
	PerformanceTimer updateCameraTimer;

    WindowGl *window;

public:
    Program();
    ~Program();

	void initNormal(WindowGl *window);
	void initServer(WindowGl *window);
	void initClient(WindowGl *window, const Ip &serverIp);

	//main
	void mouseDownLeft(int x, int y){
		const Metrics &metrics= Metrics::getInstance();
		programState->mouseDownLeft(metrics.toVirtualX(x), metrics.toVirtualY(y));
	}

	void mouseUpLeft(int x, int y){
		const Metrics &metrics= Metrics::getInstance();
		programState->mouseUpLeft(metrics.toVirtualX(x), metrics.toVirtualY(y));
	}

	void mouseDownRight(int x, int y){
		const Metrics &metrics= Metrics::getInstance();
		programState->mouseDownRight(metrics.toVirtualX(x), metrics.toVirtualY(y));
	}

	void mouseDoubleClickLeft(int x, int y){
		const Metrics &metrics= Metrics::getInstance();
		programState->mouseDoubleClickLeft(metrics.toVirtualX(x), metrics.toVirtualY(y));
	}

	void mouseDownCenter(int x, int y){
		const Metrics &metrics= Metrics::getInstance();
		programState->mouseDownCenter(metrics.toVirtualX(x), metrics.toVirtualY(y));
	}

	void mouseUpCenter(int x, int y){
		const Metrics &metrics= Metrics::getInstance();
		programState->mouseUpCenter(metrics.toVirtualX(x), metrics.toVirtualY(y));
	}

	void eventMouseWheel(int x, int y, int zDelta) {
		const Metrics &metrics= Metrics::getInstance();
		programState->eventMouseWheel(metrics.toVirtualX(x), metrics.toVirtualY(y), zDelta);
	}

	void mouseMove(int x, int y, const MouseState *ms){
		const Metrics &metrics= Metrics::getInstance();
		programState->mouseMove(metrics.toVirtualX(x), metrics.toVirtualY(y), ms);
	}

	void keyDown(char key){
		//delegate event
		programState->keyDown(key);
	}

	void keyUp(char key){
		programState->keyUp(key);
	}

	void keyPress(char c){
		programState->keyPress(c);
	}

	void loop();
	void resize(SizeState sizeState);

	//misc
	void setState(ProgramState *programState);
	void exit();

private:
	void init(WindowGl *window);
	void setDisplaySettings();
	void restoreDisplaySettings();
};

}} //end namespace

#endif
