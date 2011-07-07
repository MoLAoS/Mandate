// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "../window.h"

#include <cassert>

#include "control.h"
#include "conversion.h"
#include "platform_util.h"
#include "gl_wrap.h"

#include "leak_dumper.h"


using namespace Shared::Util;

namespace Shared {
namespace Platform {

// =====================================================
// class Window
// =====================================================

const DWORD Window::fullscreenStyle = WS_POPUP | WS_OVERLAPPED;
const DWORD Window::windowedFixedStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_SYSMENU;
const DWORD Window::windowedResizeableStyle = WS_SIZEBOX | WS_CAPTION | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_SYSMENU;

int Window::nextClassName = 0;
Window::WindowMap Window::createdWindows;

// ===================== PUBLIC ========================

Window::Window() {
	handle = 0;
	style =  windowedFixedStyle;
	x = 0;
	y = 0;
	w = 100;
	h = 100;
	m_resizing = false;
}

Window::~Window() {
	destroy(className, handle);
}

//static
bool Window::handleEvent() {
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			// when the window is destroyed for resizing it will
			// send a WM_QUIT message that should be ignored
			if (m_resizing) {
				m_resizing = false; // recreated window
				continue;
			} else {
				return false;
			}
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}

string Window::getText() {
	if (handle == 0) {
		return text;
	}

	char c[255];
	SendMessage(handle, WM_GETTEXT, 255, (LPARAM) c);
	return string(c);
}

int Window::getClientW() {
	RECT r;

	GetClientRect(handle, &r);
	return r.right;
}

int Window::getClientH() {
	RECT r;

	GetClientRect(handle, &r);
	return r.bottom;
}

float Window::getAspect() {
	return static_cast<float>(getClientH()) / getClientW();
}

void Window::setText(string text) {
	this->text = text;
	if (handle != 0) {
		SendMessage(handle, WM_SETTEXT, 0, (LPARAM) text.c_str());
	}
}

void Window::setSize(int w, int h) {

	if (windowStyle != wsFullscreen) {
		RECT rect;
		rect.left = x;
		rect.top = y;
		rect.bottom = y + h;
		rect.right = x + w;

		AdjustWindowRectEx(&rect, style, FALSE, 0);

		w = rect.right - rect.left;
		h = rect.bottom - rect.top;
	} else {
		RECT rect;
		rect.left = 0;
		rect.top = 0;
		rect.bottom = h;
		rect.right = w;

		AdjustWindowRectEx(&rect, style, FALSE, WS_EX_APPWINDOW);
	}

	this->w = w;
	this->h = h;

	if (handle != 0) {
		MoveWindow(handle, x, y, w, h, FALSE);
		UpdateWindow(handle);
	}
}

void Window::resize(PlatformContextGl *context, VideoMode mode) {
	m_resizing = true;
	setVisible(false);

	// store the context in a temporary window
	registerWindow("temp");
	WindowHandle temp = createWindow("temp");
	ShowWindow(temp, SW_HIDE);
	UpdateWindow(temp);
	context->changeWindow(temp);
	
	// remove the current window
	destroy();

	// change viedo mode if fullscreen
	if (windowStyle == wsFullscreen) {
		if (changeVideoMode(mode)) {
			m_videoMode = mode;
		} else {
			cout << "Error setting viedo mode " << mode.w << "x" << mode.h << " " << mode.bpp
				<< "bpp @ " << mode.freq << " Hz.\n";
			// error...
		}
	} else {
		m_videoMode = mode;
	}

	// create the new window with the changed size
	setSize(m_videoMode.w, m_videoMode.h);
	create();
	context->changeWindow(handle);
	setVisible(true);
	
	// remove the temp window now the context has been moved
	destroy(string("temp"), temp);
}

void Window::setPos(int x, int y) {
	this->x = x;
	this->y = y;
	if (handle != 0) {
		MoveWindow(handle, x, y, w, h, FALSE);
	}
}

void Window::setEnabled(bool enabled) {
	EnableWindow(handle, static_cast<BOOL>(enabled));
}

void Window::setVisible(bool visible) {

	if (visible) {
		ShowWindow(handle, SW_SHOW);
		UpdateWindow(handle);
	} else {
		ShowWindow(handle, SW_HIDE);
		UpdateWindow(handle);
	}
}

void Window::setStyle(WindowStyle windowStyle) {
	this->windowStyle = windowStyle;
	switch (windowStyle) {
	case wsFullscreen:
		style = fullscreenStyle;
		break;
	case wsWindowedFixed:
		style = windowedFixedStyle;
		break;
	case wsWindowedResizeable:
		style = windowedResizeableStyle;
		break;
	}

	if (handle != 0) {
		setVisible(false);
		int err = SetWindowLong(handle,  GWL_STYLE, style);
		assert(err);

		setVisible(true);
		UpdateWindow(handle);
	}
}

void Window::setMouseCapture(bool c) {
	if (c) {
		SetCapture(handle);
	} else {
		ReleaseCapture();
	}
}

Vec2i centreWindowPos(Vec2i wndSize) {
	RECT rect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	Vec2i desktopPos(rect.left, rect.top);
	Vec2i desktopSize(rect.right - rect.left, rect.bottom - rect.top);
	Vec2i pos = (desktopSize - wndSize) / 2 + desktopPos;
	return Vec2i(max(pos.x, desktopPos.x), max(pos.y, desktopPos.y));
}

bool Window::toggleFullscreen() {
	if (windowStyle == wsFullscreen) {
		setStyle(wsWindowedFixed);
		restoreVideoMode();
	} else {
		if (!changeVideoMode(m_videoMode)) {
			return false;
		}
		setStyle(wsFullscreen);
	}
	Vec2i pos = (windowStyle != wsFullscreen) ? centreWindowPos(Vec2i(m_videoMode.w, m_videoMode.h)) :  Vec2i(0, 0);
	setPos(pos.x, pos.y);
	setSize(m_videoMode.w, m_videoMode.h);
	return true;
}

void Window::create() {
	className = "Window" + intToStr(Window::getNextClassName());
	registerWindow(className);
	handle = createWindow(className);
}

void Window::minimize() {
	ShowWindow(getHandle(), SW_MINIMIZE);
}

void Window::maximize() {
	ShowWindow(getHandle(), SW_MAXIMIZE);
}

void Window::restore() {
	ShowWindow(getHandle(), SW_RESTORE);
}

/*
void Window::showPopupMenu(Menu *menu, int x, int y) {
	RECT rect;

	GetWindowRect(handle, &rect);

	TrackPopupMenu(menu->getHandle(), TPM_LEFTALIGN | TPM_TOPALIGN, rect.left + x, rect.top + y, 0, handle, NULL);
}*/

void Window::destroy(string &in_className, WindowHandle handle) {
	if (handle != 0) {
		ShowWindow(handle, SW_HIDE);
		DestroyWindow(handle);
	}
	if (in_className != "") {
		BOOL b = UnregisterClass(in_className.c_str(), GetModuleHandle(NULL));
		in_className = "";
		assert(b);
	}
}

void Window::destroy() {
	destroy(className, handle);
	handle = 0;
}

// ===================== PRIVATE =======================

// debug hook
void nop() {}

LRESULT CALLBACK Window::eventRouter(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	Window *eventWindow;
	WindowMap::iterator it;

	it = createdWindows.find(hwnd);
	if (it == createdWindows.end()) {
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	eventWindow = it->second;

	switch (msg) {
	case WM_CREATE:
		eventWindow->eventCreate();
		break;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
	case WM_MOUSEMOVE: {
			RECT windowRect;
			POINT mousePos;

			GetWindowRect(eventWindow->getHandle(), &windowRect);
			mousePos.x = LOWORD(lParam) - windowRect.left;
			mousePos.y = HIWORD(lParam) - windowRect.top;
			if (eventWindow->windowStyle == wsWindowedFixed) {
				int titleBarHeight = GetSystemMetrics(SM_CYCAPTION);
				int borderWidth = GetSystemMetrics(SM_CXFIXEDFRAME);
				mousePos.x -= borderWidth;
				mousePos.y -= titleBarHeight;
			}
			if (msg != WM_MOUSEWHEEL) { // don't ask me... I just work here
				ClientToScreen(eventWindow->getHandle(), &mousePos);
			}

			eventWindow->input.setLastMouseEvent(Chrono::getCurMillis());
			eventWindow->input.setMousePos(Vec2i(mousePos.x, mousePos.y));
			switch(msg) {
			case WM_LBUTTONDOWN:
				eventWindow->mouseEvent(0, MouseButton::LEFT);
				return 0;

			case WM_LBUTTONUP:
				eventWindow->mouseEvent(1, MouseButton::LEFT);
				return 0;

			case WM_LBUTTONDBLCLK:
				eventWindow->mouseEvent(2, MouseButton::LEFT);
				return 0;

			case WM_RBUTTONDOWN:
				eventWindow->mouseEvent(0, MouseButton::RIGHT);
				return 0;

			case WM_RBUTTONUP:
				eventWindow->mouseEvent(1, MouseButton::RIGHT);
				return 0;

			case WM_RBUTTONDBLCLK:
				eventWindow->mouseEvent(2, MouseButton::RIGHT);
				return 0;

			case WM_MBUTTONDOWN:
				eventWindow->mouseEvent(0, MouseButton::MIDDLE);
				return 0;

			case WM_MBUTTONUP:
				eventWindow->mouseEvent(1, MouseButton::MIDDLE);
				return 0;

			case WM_MBUTTONDBLCLK:
				eventWindow->mouseEvent(2, MouseButton::MIDDLE);
				return 0;

			case WM_XBUTTONDOWN:
				// we only know about XBUTTON1 and XBUTTON2, but there may be more later, let
				// DefWindowProc take these.
				switch(HIWORD(wParam)) {
				case XBUTTON1:
					eventWindow->mouseEvent(0, MouseButton::BUTTON_X1);
					return TRUE;// don't ask me why it wants TRUE instead of zero like
								// everything else
				case XBUTTON2:
					eventWindow->mouseEvent(0, MouseButton::BUTTON_X2);
					return TRUE;
				}
				break;

			case WM_XBUTTONUP:
				switch(HIWORD(wParam)) {
				case XBUTTON1:
					eventWindow->mouseEvent(1, MouseButton::BUTTON_X1);
					return TRUE;// don't ask me why it wants TRUE instead of zero like
								// everything else
				case XBUTTON2:
					eventWindow->mouseEvent(1, MouseButton::BUTTON_X2);
					return TRUE;
				}
				break;

			case WM_XBUTTONDBLCLK:
				switch(HIWORD(wParam)) {
				case XBUTTON1:
					eventWindow->mouseEvent(2, MouseButton::BUTTON_X1);
					return TRUE;// don't ask me why it wants TRUE instead of zero like
								// everything else
				case XBUTTON2:
					eventWindow->mouseEvent(2, MouseButton::BUTTON_X2);
					return TRUE;
				}
				break;

			case WM_MOUSEWHEEL:
				eventWindow->eventMouseWheel(mousePos.x, mousePos.y, GET_WHEEL_DELTA_WPARAM(wParam));
				return 0;

			case WM_MOUSEHWHEEL:
				//eventWindow->eventMouseHWheel(mousePos.x, mousePos.y, GET_WHEEL_DELTA_WPARAM(wParam));
				break; // not handled, send to DefWindowProc

			case WM_MOUSEMOVE:
				eventWindow->input.setMouseState(MouseButton::LEFT, wParam & MK_LBUTTON);
				eventWindow->input.setMouseState(MouseButton::RIGHT, wParam & MK_RBUTTON);
				eventWindow->input.setMouseState(MouseButton::MIDDLE, wParam & MK_MBUTTON);
				eventWindow->input.setMouseState(MouseButton::BUTTON_X1, wParam & MK_XBUTTON1);
				eventWindow->input.setMouseState(MouseButton::BUTTON_X2, wParam & MK_XBUTTON2);
				eventWindow->eventMouseMove(mousePos.x, mousePos.y, eventWindow->input.getMouseState());
				return 0;
			}
			break;
		}

	case WM_KEYDOWN: {
			Key key(Input::getKeyCode(wParam), static_cast<char>(wParam));
			// bottom 16 bits is repeat acount
			int rCount = (lParam & 0xFFFF);
			bool isRepeat = rCount != 1;
			// I don't want repeats of modifier keys posting
			if(!isRepeat || !key.isModifier()) {
				eventWindow->input.updateKeyModifiers(key.getCode(), true);
				eventWindow->eventKeyDown(key);
			}
			break;
		}

	case WM_KEYUP: {
			Key key(Input::getKeyCode(wParam), static_cast<char>(wParam));
			eventWindow->input.updateKeyModifiers(key.getCode(), false);
			eventWindow->eventKeyUp(key);
			break;
		}

	case WM_CHAR:
		eventWindow->eventKeyPress(static_cast<char>(wParam));
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == 0) {
			eventWindow->eventMenu(LOWORD(wParam));
		}
		break;

	case WM_ACTIVATE:
		eventWindow->eventActivate(wParam != WA_INACTIVE);
		break;

	case WM_MOVE: {
			RECT rect;
	
			GetWindowRect(eventWindow->getHandle(), &rect);
			eventWindow->x = rect.left;
			eventWindow->y = rect.top;
			eventWindow->w = rect.right - rect.left;
			eventWindow->h = rect.bottom - rect.top;
		}
		break;

	case WM_SIZE: {
			RECT rect;
	
			GetWindowRect(eventWindow->getHandle(), &rect);
			eventWindow->x = rect.left;
			eventWindow->y = rect.top;
			eventWindow->w = rect.right - rect.left;
			eventWindow->h = rect.bottom - rect.top;
	
			eventWindow->eventResize(static_cast<SizeState>(wParam));
		}
		break;

	case WM_SIZING:
		eventWindow->eventResize();
		break;

	case WM_PAINT:
		eventWindow->eventPaint();
		break;

	case WM_CLOSE:
		eventWindow->eventClose();
		break;

	case WM_DESTROY:
		eventWindow->eventDestroy();
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int Window::getNextClassName() {
	return ++nextClassName;
}

void Window::registerWindow(const string &className, WNDPROC wndProc) {
	WNDCLASSEX wc;

	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_DBLCLKS | CS_OWNDC;
	wc.lpfnWndProc   = wndProc == NULL ? eventRouter : wndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle(NULL);
	wc.hIcon         = LoadIcon(wc.hInstance, "GlestAdvIcon");
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = className.c_str();
	wc.hIconSm       = NULL;

	int registerClassErr = RegisterClassEx(&wc);
	assert(registerClassErr);

}

WindowHandle Window::createWindow(const string &className, LPVOID creationData) {
	Vec2i pos = (windowStyle != wsFullscreen) ? centreWindowPos(Vec2i(w, h)) : Vec2i(0, 0);
	setPos(pos.x, pos.y);

	WindowHandle handle = CreateWindowEx(WS_EX_APPWINDOW, className.c_str(), text.c_str(),
				 style, x, y, w, h, NULL, NULL, GetModuleHandle(NULL), creationData);

	createdWindows.insert(std::pair<WindowHandle, Window*>(handle, this));
	eventRouter(handle, WM_CREATE, 0, 0);

	assert(handle != NULL);

	ShowWindow(handle, SW_SHOW);
	UpdateWindow(handle);

	return handle;
}

}}//end namespace
