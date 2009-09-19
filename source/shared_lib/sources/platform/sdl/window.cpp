// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "window.h"

#include <iostream>
#include <stdexcept>
#include <cassert>
#include <cctype>

#include "conversion.h"
#include "platform_util.h"

#include "sdl_private.h"
#include "noimpl.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

namespace Shared { namespace Platform {

// =====================================================
// class Window
// =====================================================

// ========== PUBLIC ==========

Window::Window() : x(0), y(0), w(0), h(0), handle(0) {
	memset(lastMouseDown, 0, sizeof(lastMouseDown));
	memset(lastMouse, 0, sizeof(lastMouse));
}

Window::~Window() {}

bool Window::handleEvent() {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEMOTION:
				input.setLastMouseEvent(Chrono::getCurMillis());
				input.setMousePos(Vec2i(event.button.x, event.button.y));
		}

		switch(event.type) {
			case SDL_QUIT:
				return false;

			case SDL_MOUSEBUTTONDOWN:
				handleMouseDown(event);
				break;

			case SDL_MOUSEBUTTONUP: {
					MouseButton b = Input::getMouseButton(event.button.button);
					input.setMouseState(b, false);
					eventMouseUp(event.button.x, event.button.y, b);
					break;
				}

			case SDL_MOUSEMOTION:
				input.setMouseState(mbLeft, event.motion.state & SDL_BUTTON_LMASK);
				input.setMouseState(mbRight, event.motion.state & SDL_BUTTON_RMASK);
				input.setMouseState(mbCenter, event.motion.state & SDL_BUTTON_MMASK);
				eventMouseMove(event.motion.x, event.motion.y, input.getMouseState());
				break;

			case SDL_KEYDOWN:
				/* handle ALT+Return */
				if(event.key.keysym.sym == SDLK_RETURN
						&& (event.key.keysym.mod & (KMOD_LALT | KMOD_RALT))) {
					toggleFullscreen();
				} else {
					eventKeyDown(Key(event.key.keysym));
					eventKeyPress(static_cast<char>(event.key.keysym.unicode));
				}
				break;

			case SDL_KEYUP:
				eventKeyUp(Key(event.key.keysym));
				break;
		}
	}

	return true;
}

string Window::getText() {
	char* c = 0;
	SDL_WM_GetCaption(&c, 0);

	return string(c);
}

int Window::getClientW() {return getW();}
int Window::getClientH() {return getH();}

float Window::getAspect() {
	return static_cast<float>(getClientH())/getClientW();
}

void Window::setText(string text) {
	SDL_WM_SetCaption(text.c_str(), 0);
}

void Window::setSize(int w, int h) {
	this->w = w;
	this->h = h;
	Private::ScreenWidth = w;
	Private::ScreenHeight = h;
}

void Window::setPos(int x, int y)  {
	if(x != 0 || y != 0) {
		NOIMPL;
		return;
	}
}

void Window::setEnabled(bool enabled) {
	NOIMPL;
}

void Window::setVisible(bool visible) {
	 NOIMPL;
}

void Window::setStyle(WindowStyle windowStyle) {
	if(windowStyle == wsFullscreen)
		return;
	// NOIMPL;
}

void Window::create() {
	// nothing here
}

void Window::minimize() {
	NOIMPL;
}

void Window::maximize() {
	NOIMPL;
}

void Window::restore() {
	NOIMPL;
}

//void Window::showPopupMenu(Menu *menu, int x, int y) {
//	NOIMPL;
//}

void Window::destroy() {
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

void Window::toggleFullscreen() {
	SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
}

void Window::handleMouseDown(SDL_Event event) {
	static const Uint32 DOUBLECLICKTIME = 500;
	static const int DOUBLECLICKDELTA = 5;

	MouseButton button = Input::getMouseButton(event.button.button);

	// windows implementation uses 120 for the resolution of a standard mouse
	// wheel notch.  However, newer mice have finer resolutions.  I dunno if SDL
	// handles those, but for now we're going to say that each mouse wheel
	// movement is 120.
	if(button == mbWheelUp) {
		eventMouseWheel(event.button.x, event.button.y, 120);
		return;
	} else if(button == mbWheelDown) {
		eventMouseWheel(event.button.x, event.button.y, -120);
		return;
	}

	input.setMouseState(button, true);

	if(input.getLastMouseEvent() - lastMouseDown[button] < DOUBLECLICKTIME
			&& abs(lastMouse[button].x - event.button.x) < DOUBLECLICKDELTA
			&& abs(lastMouse[button].y - event.button.y) < DOUBLECLICKDELTA) {
		eventMouseDown(event.button.x, event.button.y, button);
		eventMouseDoubleClick(event.button.x, event.button.y, button);
	} else {
		eventMouseDown(event.button.x, event.button.y, button);
	}
	lastMouseDown[button] = input.getLastMouseEvent();
	lastMouse[button] = input.getMousePos();
}

}}//end namespace
