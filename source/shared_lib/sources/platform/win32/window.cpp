// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa,
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
	exStyle = 0;
	ownDc = false;
	x = 0;
	y = 0;
	w = 100;
	h = 100;
}

Window::~Window() {
	if (handle != 0) {
		DestroyWindow(handle);
		handle = 0;
		BOOL b = UnregisterClass(className.c_str(), GetModuleHandle(NULL));
		assert(b);
	}
}

//static
bool Window::handleEvent() {
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			return false;
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
		rect.left = 0;
		rect.top = 0;
		rect.bottom = h;
		rect.right = w;

		AdjustWindowRect(&rect, style, FALSE);

		w = rect.right - rect.left;
		h = rect.bottom - rect.top;
	}

	this->w = w;
	this->h = h;

	if (handle != 0) {
		MoveWindow(handle, x, y, w, h, FALSE);
		UpdateWindow(handle);
	}
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
		exStyle = WS_EX_APPWINDOW;
		ownDc = true;
		break;
	case wsWindowedFixed:
		style = windowedFixedStyle;
		exStyle = 0;
		ownDc = false;
		break;
	case wsWindowedResizeable:
		style = windowedResizeableStyle;
		exStyle = 0;
		ownDc = false;
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

void Window::toggleFullscreen() {
	if (windowStyle == wsFullscreen) {
		setStyle(wsWindowedFixed);
	} else {
		setStyle(wsFullscreen);
	}
}

void Window::create() {
	registerWindow();
	createWindow();
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

void Window::destroy() {
	DestroyWindow(handle);
	BOOL b = UnregisterClass(className.c_str(), GetModuleHandle(NULL));
	assert(b);
	handle = 0;
}

// ===================== PRIVATE =======================

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
			ClientToScreen(eventWindow->getHandle(), &mousePos);

			eventWindow->input.setLastMouseEvent(Chrono::getCurMillis());
			eventWindow->input.setMousePos(Vec2i(mousePos.x, mousePos.y));
			switch(msg) {
			case WM_LBUTTONDOWN:
				eventWindow->mouseyVent(0, mbLeft);
				return 0;

			case WM_LBUTTONUP:
				eventWindow->mouseyVent(1, mbLeft);
				return 0;

			case WM_LBUTTONDBLCLK:
				eventWindow->mouseyVent(2, mbLeft);
				return 0;

			case WM_RBUTTONDOWN:
				eventWindow->mouseyVent(0, mbRight);
				return 0;

			case WM_RBUTTONUP:
				eventWindow->mouseyVent(1, mbRight);
				return 0;

			case WM_RBUTTONDBLCLK:
				eventWindow->mouseyVent(2, mbRight);
				return 0;

			case WM_MBUTTONDOWN:
				eventWindow->mouseyVent(0, mbCenter);
				return 0;

			case WM_MBUTTONUP:
				eventWindow->mouseyVent(1, mbCenter);
				return 0;

			case WM_MBUTTONDBLCLK:
				eventWindow->mouseyVent(2, mbCenter);
				return 0;

			case WM_XBUTTONDOWN:
				// we only know about XBUTTON1 and XBUTTON2, but there may be more later, let
				// DefWindowProc take these.
				switch(HIWORD(wParam)) {
				case XBUTTON1:
					eventWindow->mouseyVent(0, mbButtonX1);
					return TRUE;// don't ask me why it wants TRUE instead of zero like
								// everything else
				case XBUTTON2:
					eventWindow->mouseyVent(0, mbButtonX2);
					return TRUE;
				}
				break;

			case WM_XBUTTONUP:
				switch(HIWORD(wParam)) {
				case XBUTTON1:
					eventWindow->mouseyVent(1, mbButtonX1);
					return TRUE;// don't ask me why it wants TRUE instead of zero like
								// everything else
				case XBUTTON2:
					eventWindow->mouseyVent(1, mbButtonX2);
					return TRUE;
				}
				break;

			case WM_XBUTTONDBLCLK:
				switch(HIWORD(wParam)) {
				case XBUTTON1:
					eventWindow->mouseyVent(2, mbButtonX1);
					return TRUE;// don't ask me why it wants TRUE instead of zero like
								// everything else
				case XBUTTON2:
					eventWindow->mouseyVent(2, mbButtonX2);
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
				eventWindow->input.setMouseState(mbLeft, wParam & MK_LBUTTON);
				eventWindow->input.setMouseState(mbRight, wParam & MK_RBUTTON);
				eventWindow->input.setMouseState(mbCenter, wParam & MK_MBUTTON);
				eventWindow->input.setMouseState(mbButtonX1, wParam & MK_XBUTTON1);
				eventWindow->input.setMouseState(mbButtonX2, wParam & MK_XBUTTON2);
				eventWindow->eventMouseMove(mousePos.x, mousePos.y, eventWindow->input.getMouseState());
				return 0;
			}
			break;
		}

	case WM_KEYDOWN: {
			Key key(Input::getKeyCode(wParam), static_cast<char>(wParam));
			// bottom 16 bits is repeat acount, and I only care if it's zero or non-zero
			bool isRepeat = (lParam << 16);
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

void Window::registerWindow(WNDPROC wndProc) {
	WNDCLASSEX wc;

	this->className = "Window" + intToStr(Window::getNextClassName());

	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_DBLCLKS | (ownDc ? CS_OWNDC : 0);
	wc.lpfnWndProc   = wndProc == NULL ? eventRouter : wndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle(NULL);
	wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = this->className.c_str();
	wc.hIconSm       = NULL;

	int registerClassErr = RegisterClassEx(&wc);
	assert(registerClassErr);

}

void Window::createWindow(LPVOID creationData) {

	handle = CreateWindowEx(
				 exStyle,
				 className.c_str(),
				 text.c_str(),
				 style,
				 x, y, w, h,
				 NULL, NULL, GetModuleHandle(NULL), creationData);

	createdWindows.insert(pair<WindowHandle, Window*>(handle, this));
	eventRouter(handle, WM_CREATE, 0, 0);

	assert(handle != NULL);

	ShowWindow(handle, SW_SHOW);
	UpdateWindow(handle);
}

}}//end namespace
