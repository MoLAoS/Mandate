// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2005 Matthias Braun <matze@braunis.de>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
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

#include "types.h"
#include "input.h"
#if defined(WIN32)  || defined(WIN64)
//#	include "platform_menu.h"
#endif
#include "timer.h"

using std::map;
using std::string;

namespace Shared { namespace Platform {

class Timer;
class PlatformContextGl;

enum SizeState {
	ssMaximized,
	ssMinimized,
	ssRestored
};

enum WindowStyle {
	wsFullscreen,
	wsWindowedFixed,
	wsWindowedResizeable
};

// =====================================================
//	class Window
// =====================================================

class Window {
private:
#ifdef USE_SDL
	int64 lastMouseDown[mbCount];
	Vec2i lastMouse[mbCount];
#elif defined(WIN32)  || defined(WIN64)
	typedef map<WindowHandle, Window*> WindowMap;

	static const DWORD fullscreenStyle;
	static const DWORD windowedFixedStyle;
	static const DWORD windowedResizeableStyle;

	static int nextClassName;
	static WindowMap createdWindows;
#endif

protected:
	Input input;
	int x;
	int y;
	int w;
	int h;
	WindowHandle handle;
#if defined(WIN32)  || defined(WIN64)
	string text;
	WindowStyle windowStyle;
	string className;
	DWORD style;
	DWORD exStyle;
	bool ownDc;
#endif

public:
	Window();
	virtual ~Window();

	WindowHandle getHandle()		{return handle;}
	const Input &getInput() const	{return input;}

	string getText();
	int getX() const			{return x;}
	int getY() const			{return y;}
	int getW() const			{return w;}
	int getH() const			{return h;}

	//component state
	int getClientW();//			{return getW();}
	int getClientH();//			{return getH();}
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
	void minimize();
	void maximize();
	void restore();
	//void showPopupMenu(Menu *menu, int x, int y);
	void destroy();
	void toggleFullscreen();
	bool handleEvent();

protected:
	virtual void eventCreate() {}
	virtual void eventMouseDown(int x, int y, MouseButton mouseButton) {}
	virtual void eventMouseUp(int x, int y, MouseButton mouseButton) {}
	virtual void eventMouseMove(int x, int y, const MouseState &mouseState) {}
	virtual void eventMouseDoubleClick(int x, int y, MouseButton mouseButton) {}
	virtual void eventMouseWheel(int x, int y, int zDelta) {}
	virtual void eventKeyDown(const Key &key) {}
	virtual void eventKeyUp(const Key &key) {}
	virtual void eventKeyPress(char c) {}
	virtual void eventResize() {}
	virtual void eventPaint() {}
	virtual void eventTimer(int timerId) {}
	virtual void eventActivate(bool activated) {}
	virtual void eventResize(SizeState sizeState) {}
	virtual void eventMenu(int menuId) {}
	virtual void eventClose() {}
	virtual void eventDestroy() {}

private:
#ifdef USE_SDL
	/// needed to detect double clicks
	void handleMouseDown(SDL_Event event);
#elif defined(WIN32)  || defined(WIN64)
	static LRESULT CALLBACK eventRouter(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static int getNextClassName();
	void registerWindow(WNDPROC wndProc = NULL);
	void createWindow(LPVOID creationData = NULL);
	void mouseyVent(int asdf, MouseButton mouseButton) {
		const Vec2i &mousePos = input.getMousePos();
		switch(asdf) {
		case 0:
			input.setMouseState(mouseButton, true);
			eventMouseDown(mousePos.x, mousePos.y, mouseButton);
			break;
		case 1:
			input.setMouseState(mouseButton, false);
			eventMouseUp(mousePos.x, mousePos.y, mouseButton);
			break;
		case 2:
			eventMouseDoubleClick(mousePos.x, mousePos.y, mouseButton);
			break;
		}
	}
#endif
};

}}//end namespace

#endif // _SHARED_PLATFORM_WINDOW_H_
