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
#include "platform_util.h"
#include "socket.h"
#include "metrics.h"
#include "components.h"
#include "keymap.h"
#include "CmdArgs.h"
#include "forward_decs.h"
#include "game_constants.h"
#include "widget_window.h"
#include "framed_widgets.h"

using namespace Shared::Platform;
using namespace Glest::Graphics;
using namespace Glest::Global;
using Glest::Sim::SimulationInterface;
using Glest::Gui::Keymap;

namespace Glest { namespace Main {

using namespace Widgets;

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

	virtual bool isGameState() const	{ return false; }
};

// ===============================
// 	class Program
// ===============================

class Program : public WidgetWindow {
	friend class Glest::Sim::SimulationInterface;
private:
	class CrashProgramState : public ProgramState, public sigslot::has_slots {
		MessageDialog* msgBox;
		const exception *e;
		bool done;

	public:
		CrashProgramState(Program &program, const exception *e);
		virtual int getUpdateFps() const {return 10;}

		void onExit(Widget*);
		virtual void update() override;

		virtual void renderBg() override;
		virtual void renderFg() override;
	};

	static const int maxUpdateTimes = 5;
	static const int maxUpdateBackLog = 12; ///@todo should be speed dependant
	static const int maxTimes = 5;

	static Program *singleton;
	CmdArgs cmdArgs;

	PerformanceTimer tickTimer;
	PerformanceTimer updateTimer;
	PerformanceTimer renderTimer;
	PerformanceTimer updateCameraTimer;
	PerformanceTimer guiUpdateTimer;

	SimulationInterface *simulationInterface;

	int fps, lastFps;
	StaticText *m_fpsLabel;

	ProgramState *m_programState;
	bool crashed;
	bool terminating;
	bool visible;
	Keymap keymap;

public:
	Program(CmdArgs &args);
	~Program();
	static Program *getInstance()	{return singleton;}
	bool init();
	const CmdArgs& getCmdArgs()	const { return cmdArgs; }

	bool isTerminating() const		{ return terminating;	}
	bool isVisible() const			{ return visible;		}
	Keymap &getKeymap() 			{ return keymap;		}

	SimulationInterface* getSimulationInterface() { return simulationInterface; }
	void setSimInterface(SimulationInterface *si);

	void setFpsCounterVisible(bool v);

	// InputWidget virtuals
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;
	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos) override;
	virtual bool mouseWheel(Vec2i pos, int zDelta) override;
	virtual bool keyDown(Key key) override;
	virtual bool keyUp(Key key) override;
	virtual bool keyPress(char c) override;

	// Window virtuals
	virtual void eventActivate(bool active) override {}
	virtual void eventResize(SizeState sizeState) override;
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

	// misc
	void setState(ProgramState *programState);
	void crash(const exception *e);
	void loop();
	void exit();
	void setMaxUpdateBacklog(int maxBacklog)	{updateTimer.setMaxBacklog(maxBacklog);}
	void resetTimers();
	void setUpdateFps(int updateFps);
	void setTechTitle(const string &title) {
		if (title.empty()) {
			Window::setText("Glest Advanced Engine");
		} else {
			Window::setText(title + " - Glest Advanced Engine");
		}
	}
};

}} //end namespace

#endif
