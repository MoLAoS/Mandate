// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_WINDOW_H_
#define _SHARED_PLATFORM_WINDOW_H_

#include <map>
#include <string>
#include <SDL.h>

#include "types.h"

using std::map;
using std::string;

namespace Shared{ namespace Platform{

class Timer;
class PlatformContextGl;

enum MouseButton{
	mbLeft,
	mbRight,
	mbCenter,
	mbWheelUp,
	mbWheelDown
};

enum SizeState{
	ssMaximized,
	ssMinimized,
	ssRestored
};

// keycode constants (unfortunately designed after DirectInput and therefore not
// very specific)
// They also have to fit into a char. The positive numbers seem to be equal
// to ascii, for the rest we have to find sensefull mappings from SDL (which is
// alot more fine grained like left/right control instead of just control...)
const char vkAdd = -1;
const char vkSubtract = -2;
const char vkAlt = -3;
const char vkControl = -4;
const char vkShift = -5;
const char vkEscape = -6;
const char vkUp = -7;
const char vkLeft = -8;
const char vkRight = -9;
const char vkDown = -10;
const char vkReturn = -11;
const char vkBack = -12;
const char vkF1 = 0x70;
const char vkF2 = 0x71;
const char vkF3 = 0x72;
const char vkF4 = 0x73;
const char vkF5 = 0x74;
const char vkF6 = 0x75;
const char vkF7 = 0x76;
const char vkF8 = 0x77;
const char vkF9 = 0x78;
const char vkF10 = 0x79;
const char vkF11 = 0x7a;
const char vkF12 = 0x7b;
const char vkF13 = 0x7c;
const char vkF14 = 0x7d;
const char vkF15 = 0x7e;
const char vkF16 = 0x7f;
const char vkF17 = 0x80;
const char vkF18 = 0x81;
const char vkF19 = 0x82;
const char vkF20 = 0x83;
const char vkF21 = 0x84;
const char vkF22 = 0x85;
const char vkF23 = 0x86;
const char vkF24 = 0x87;

struct MouseState{
	bool leftMouse;
	bool rightMouse;
	bool centerMouse;
};

enum WindowStyle{
	wsFullscreen,
	wsWindowedFixed,
	wsWindowedResizable
};

// =====================================================
//	class Window
// =====================================================

class Window {
private:
	Uint32 lastMouseDown[3];
	int lastMouseX[3];
	int lastMouseY[3];

protected:
	int w, h;

public:
	static bool handleEvent();

	Window();
	virtual ~Window();

	WindowHandle getHandle()	{return 0;}
	string getText();
	int getX()					{ return 0; }
	int getY()					{ return 0; }
	int getW()					{ return w; }
	int getH()					{ return h; }

	//component state
	int getClientW()			{ return getW(); }
	int getClientH()			{ return getH(); }
	float getAspect();

	//object state
	void setText(string text);
	void setStyle(WindowStyle windowStyle);
	void setSize(int w, int h);
	void setPos(int x, int y);
	void setEnabled(bool enabled);
	void setVisible(bool visible);

	//misc
	void create();
	void destroy();
	void minimize();

	static void toggleFullscreen();

protected:
	virtual void eventCreate(){}
	virtual void eventMouseDown(int x, int y, MouseButton mouseButton){}
	virtual void eventMouseUp(int x, int y, MouseButton mouseButton){}
	virtual void eventMouseMove(int x, int y, const MouseState* mouseState){}
	virtual void eventMouseDoubleClick(int x, int y, MouseButton mouseButton){}
	virtual void eventMouseWheel(int x, int y, int zDelta){}
	virtual void eventKeyDown(char key){}
	virtual void eventKeyUp(char key){}
	virtual void eventKeyPress(char c){}
	virtual void eventResize(){};
	virtual void eventPaint(){}
	virtual void eventTimer(int timerId){}
	virtual void eventActivate(bool activated){};
	virtual void eventResize(SizeState sizeState){};
	virtual void eventMenu(int menuId){}
	virtual void eventClose(){};
	virtual void eventDestroy(){};

private:
	/// needed to detect double clicks
	void handleMouseDown(SDL_Event event);

	static MouseButton getMouseButton(int sdlButton);
	static char getKey(SDL_keysym keysym);
};

}}//end namespace

#endif
