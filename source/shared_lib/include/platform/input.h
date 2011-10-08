// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_INPUT_H_
#define _SHARED_PLATFORM_INPUT_H_

#include <map>
#include <string>
#include <cassert>

#include "projectConfig.h"

#ifdef USE_SDL
#	include <SDL.h>
#	ifndef SDL_BUTTON_WHEELUP
#		define SDL_BUTTON_WHEELUP	4
#		define SDL_BUTTON_WHEELDOWN	5
#	endif
#	ifndef SDL_BUTTON_X1
#		define SDL_BUTTON_X1		6
#		define SDL_BUTTON_X2		7
#	endif
#elif defined(WIN32)  || defined(WIN64)
#	define NOMINMAX // see http://support.microsoft.com/kb/143208
#	include <windows.h>
#	ifndef VK_OEM_102
#		define VK_OEM_102		0xE2
#	endif
#endif

#include "types.h"
#include "vec.h"
#include "input_enums.h"

using std::map;
using std::string;
using Shared::Math::Vec2i;

namespace Shared { namespace Platform {

class Key;

class MouseState {
private:
	bool states[MouseButton::COUNT];

private:
	MouseState(const MouseState &);
	MouseState &operator=(const MouseState &);

public:
	MouseState() {
		memset(this, 0, sizeof(MouseState));
	}

	bool get(MouseButton b) const {
		assert(b > 0 && b < MouseButton::COUNT);
		return states[b];
	}

	void set(MouseButton b, bool state) {
		assert(b > 0 && b < MouseButton::COUNT);
		states[b] = state;
	}
};

struct MouseEvent {
	int x, y;

	MouseEvent(int x, int y) : x(x), y(y) {}
};

struct ClickEvent {
	MouseButton button;
	int x, y;

	ClickEvent(MouseButton btn, int x, int y) : button(btn), x(x), y(y) {}
};

struct WheelEvent {
	int x, y, zDelta;

	WheelEvent(int x, int y, int z) : x(x), y(y), zDelta(z) {}
};

class Input {
public:
#ifdef USE_SDL
	static const size_t NATIVE_MOUSE_BUTTON_START = SDL_BUTTON_LEFT;
	static const size_t NATIVE_MOUSE_BUTTON_LAST = SDL_BUTTON_X2;
	static const size_t NATIVE_KEY_CODE_START = SDLK_FIRST;
	static const size_t NATIVE_KEY_CODE_LAST = SDLK_UNDO;
#elif defined(WIN32) || defined(WIN64)
	static const size_t NATIVE_MOUSE_BUTTON_START = VK_LBUTTON;
	static const size_t NATIVE_MOUSE_BUTTON_LAST = VK_XBUTTON2;
	static const size_t NATIVE_KEY_CODE_START = 0;
	static const size_t NATIVE_KEY_CODE_LAST = 255;
#endif

private:
	static const unsigned char native2mb[NATIVE_MOUSE_BUTTON_LAST + 1];
	static const unsigned char mb2native[MouseButton::COUNT];
	static const short native2kc[NATIVE_KEY_CODE_LAST + 1];
	static const NativeKeyCodeCompact kc2native[KeyCode::COUNT];

	Vec2i mousePos;
	MouseState mouseState;
	unsigned int lastMouseEvent;	/** for use in mouse hover calculations */
	int keyModifiers;

public:
	Input() : mousePos(0), mouseState(), lastMouseEvent(0), keyModifiers(0) {
		verifyTranslationTables();
	}

	static KeyCode getKeyCode(NativeKeyCode nativeKey) {
		// This assertion can fail after new devices and their drivers come out.  If it does fail,
		// we want to catch it in a debug build.  If not in debug build, we translate to KeyCode::UNKNOWN.
		assert(nativeKey >= 0 && nativeKey <= NATIVE_KEY_CODE_LAST);
		if(nativeKey >= 0 && nativeKey <= NATIVE_KEY_CODE_LAST) {
			return static_cast<KeyCode>(native2kc[nativeKey]);
		} else {
			return KeyCode::UNKNOWN;
		}
	}

	static NativeKeyCode getNativeKeyCode(KeyCode key) {
		assert(key >= 0 && key < KeyCode::COUNT);
		return static_cast<NativeKeyCode>(kc2native[key]);
	}

	static MouseButton getMouseButton(int nativeButton) {
		// Fail an assertion for debug builds so we can add the new mouse button codes, otherwise
		// send MouseButton::UNKNOWN.
		assert(nativeButton >= 0 && nativeButton <= NATIVE_MOUSE_BUTTON_LAST);
		if(nativeButton >= 0 && nativeButton <= NATIVE_MOUSE_BUTTON_LAST) {
			return static_cast<MouseButton>(native2mb[nativeButton]);
		} else {
			return MouseButton::UNKNOWN;
		}
	}

	static int getNativeMouseButton(int button) {
		assert(button >= 0 && button <= MouseButton::COUNT);
		return mb2native[button];
	}

	const Vec2i &getMousePos() const					{return mousePos;}
	const MouseState &getMouseState() const				{return mouseState;}
	unsigned int getLastMouseEvent() const				{return lastMouseEvent;}
	int getKeyModifiers() const							{return keyModifiers;}

	void setMousePos(const Vec2i &mousePos)				{this->mousePos = mousePos;}
	void setMouseState(MouseButton b, bool state)		{mouseState.set(b, state);}
//	void setMouseState(const MouseState &mouseState)	{this->mouseState = mouseState;}
	void setLastMouseEvent(unsigned int lastMouseEvent)	{this->lastMouseEvent = lastMouseEvent;}
	void updateKeyModifiers(KeyCode key, bool enabled) {
		if(key >= KeyCode::LEFT_SHIFT && key <= KeyCode::RIGHT_META) {
			KeyModifier mask;
			switch(key) {
				case KeyCode::NUM_LOCK:		mask = kmNum; break;
				case KeyCode::CAPS_LOCK:	mask = kmCaps; break;
				case KeyCode::LEFT_SHIFT:	mask = kmLShift; break;
				case KeyCode::RIGHT_SHIFT:	mask = kmRShift; break;
				case KeyCode::LEFT_CTRL:	mask = kmLCtrl; break;
				case KeyCode::RIGHT_CTRL:	mask = kmRCtrl; break;
				case KeyCode::LEFT_ALT:		mask = kmLAlt; break;
				case KeyCode::RIGHT_ALT:	mask = kmRAlt; break;
				case KeyCode::LEFT_META:	mask = kmLMeta; break;
				case KeyCode::RIGHT_META:	mask = kmRMeta; break;
				case KeyCode::RIGHT_SUPER:
				case KeyCode::LEFT_SUPER:	mask = kmSuper; break;
				default:					return;
			}
			if(enabled) {
				keyModifiers = keyModifiers | mask;
			} else {
				keyModifiers = keyModifiers & (~mask);
			}
		}
	}

#ifdef USE_SDL
	bool isShiftDown() const	{return SDL_GetModState() & (kmLShift | kmRShift);}
	bool isCtrlDown() const		{return SDL_GetModState() & (kmLCtrl | kmRCtrl);}
	bool isAltDown() const		{return SDL_GetModState() & (kmLAlt | kmRAlt);}
	bool isMetaDown() const		{return SDL_GetModState() & (kmLMeta | kmRMeta);}
	bool isNumOn() const		{return SDL_GetModState() & kmNum;}
	bool isCapsOn() const		{return SDL_GetModState() & kmCaps;}
	bool isModeOn() const		{return SDL_GetModState() & kmMode;}

	bool isKeyDown(KeyCode key) const {
		assert(key >= 0 && key < KeyCode::COUNT);
		return SDL_GetKeyState(0)[getNativeKeyCode(key)];
	}

#elif defined(WIN32) || defined(WIN64)
	bool isShiftDown() const	{return GetAsyncKeyState(VK_SHIFT);}
	bool isCtrlDown() const		{return GetAsyncKeyState(VK_CONTROL);}
	bool isAltDown() const		{return GetAsyncKeyState(VK_MENU);}
	bool isMetaDown() const		{return GetAsyncKeyState(VK_LWIN) || GetAsyncKeyState(VK_RWIN);}
	bool isNumOn() const		{return GetAsyncKeyState(VK_NUMLOCK);}
	bool isCapsOn() const		{return GetAsyncKeyState(VK_CAPITAL);}
//	bool isModeOn() const		{return GetKeyState(0);}

	bool isKeyDown(KeyCode key) const {
		assert(key >= 0 && key < KeyCode::COUNT);
		return (GetKeyState(getNativeKeyCode(key)) & 0x8000) != 0;
	}

#endif

private:
	static void verifyTranslationTables();
};

class Key {
private:
	KeyCode key;
	char ascii;

	//static const char*names[KeyCode::COUNT];

public:
#ifdef USE_SDL
	Key(SDL_keysym keysym) :
			key(Input::getKeyCode(keysym.sym)),
			ascii(keysym.sym < 0x80 ? keysym.sym : 0) {
	}
#elif defined(WIN32)  || defined(WIN64)
	Key(KeyCode key, char ascii) : key(key), ascii(ascii) {}
#endif

	operator KeyCode() const	{return key;}
	bool operator==(const KeyCode keyCode) const { return key == keyCode; }

	KeyCode getCode() const		{return key;}
	char getAscii() const		{return ascii;}
	bool isModifier() const		{return key >= KeyCode::NUM_LOCK && key <= KeyCode::MODE;}

	static KeyCode findByName(const char *name);
	static const char* getName(KeyCode key) {
		assert(key >= 0 && key < KeyCode::COUNT);
		return KeyCodeNames[key];
	}

};

}}//end namespace

#endif // _SHARED_PLATFORM_INPUT_H_
