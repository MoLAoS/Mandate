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

#ifndef _GLEST_GAME_MAIN_H_
#define _GLEST_GAME_MAIN_H_

#include <ctime>

#include "program.h"
#include "window_gl.h"

using Shared::Platform::MouseButton;
using Shared::Platform::MouseState;

namespace Glest{ namespace Game{

// =====================================================
// 	class MainWindow
//
///	Main program window
// =====================================================

class MainWindow: public WindowGl{
private:
    Program* program;

public:
	MainWindow(Program *program);
	~MainWindow();

	void setProgram(Program *program)	{this->program= program;}

	virtual void eventMouseDown(int x, int y, MouseButton mouseButton);
	virtual void eventMouseUp(int x, int y, MouseButton mouseButton);
	virtual void eventMouseDoubleClick(int x, int y, MouseButton mouseButton);
	virtual void eventMouseMove(int x, int y, const MouseState *mouseState);
	virtual void eventMouseWheel(int x, int y, int zDelta);
	virtual void eventKeyDown(char key);
	virtual void eventKeyUp(char key);
	virtual void eventKeyPress(char c);
	virtual void eventActivate(bool active);
	virtual void eventResize(SizeState sizeState);
	virtual void eventClose();
};

}}//end namespace

#endif
