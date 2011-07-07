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
#include "timer.h"
#include "platform_util.h"

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
#	ifdef USE_SDL
		int64 lastMouseDown[MouseButton::COUNT];
		Vec2i lastMouse[MouseButton::COUNT];

#	elif defined(WIN32)
		typedef map<WindowHandle, Window*> WindowMap;

		static const DWORD fullscreenStyle;
		static const DWORD windowedFixedStyle;
		static const DWORD windowedResizeableStyle;

		static int nextClassName;
		static WindowMap createdWindows;
#	endif

protected:
	Input input;
	int x;
	int y;
	int w;
	int h;
	WindowHandle handle;
	VideoMode m_videoMode;
	bool m_resizing;

#	if defined(WIN32)
		string text;
		WindowStyle windowStyle;
		string className;
		DWORD style;

		void setMouseCapture(bool c);
#	else
		void setMouseCapture(bool c) __attribute__((unused)) {}
#	endif

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
	VideoMode getVideoMode() const { return m_videoMode; }

	//object state
	void setText(string text);
	void setStyle(WindowStyle windowStyle);
	void setSize(int w, int h);
	void setPos(int x, int y);
	void setEnabled(bool enabled);
	void setVisible(bool visible);
	void setVideoMode(VideoMode viedoMode) { m_videoMode = viedoMode; }
	void resize(PlatformContextGl *context, VideoMode mode);

	//misc
	void create();
	void minimize();
	void maximize();
	void restore();
	//void showPopupMenu(Menu *menu, int x, int y);
	void destroy(string &in_className, WindowHandle handle);
	void destroy();
	bool toggleFullscreen();
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
#	ifdef USE_SDL
		/// needed to detect double clicks
		void handleMouseDown(SDL_Event event);

#	elif defined(WIN32)
		static LRESULT CALLBACK eventRouter(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static int getNextClassName();
		void registerWindow(const string &className, WNDPROC wndProc = NULL);
		WindowHandle createWindow(const string &className, LPVOID creationData = NULL);
		void mouseEvent(int asdf, MouseButton mouseButton) {
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
#	endif
};

}}//end namespace

#endif // _SHARED_PLATFORM_WINDOW_H_
