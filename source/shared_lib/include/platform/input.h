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
#ifndef SDL_BUTTON_WHEELUP
#	define SDL_BUTTON_WHEELUP	4
#	define SDL_BUTTON_WHEELDOWN	5
#endif
#ifndef SDL_BUTTON_X1
#	define SDL_BUTTON_X1		6
#	define SDL_BUTTON_X2		7
#endif

#elif defined(WIN32)  || defined(WIN64)
#	include <windows.h>
#	ifndef VK_OEM_102
#		define VK_OEM_102		0xE2
#	endif
#endif

#include "types.h"
#include "vec.h"

using std::map;
using std::string;
using Shared::Math::Vec2i;

namespace Shared { namespace Platform {

class Key;

enum MouseButton {
	mbUnknown,
	mbLeft,
	mbCenter,
	mbRight,
	mbWheelUp,
	mbWheelDown,
	mbButtonX1,
	mbButtonX2,

	mbCount
};

enum MouseEvent {
	meDown,
	meDoubleClick,
	meUp,
	meHWheel,
	meWheel,
	meMove
};

class MouseState {
private:
	bool states[mbCount];

private:
	MouseState(const MouseState &);
	MouseState &operator=(const MouseState &);

public:
	MouseState() {
		memset(this, 0, sizeof(MouseState));
	}

	bool get(MouseButton b) const {
		assert(b > 0 && b < mbCount);
		return states[b];
	}

	void set(MouseButton b, bool state) {
		assert(b > 0 && b < mbCount);
		states[b] = state;
	}
};

enum KeyCode {
	keyNone,
	keyUnknown,
	keyLButton,		// left mouse
	keyRButton,		// right mouse
	keyMButton,		// middle mouse
	keyXButton1,	// x1 mouse button (perhaps horizontal wheel)
	keyXButton2,	// x2 mouse button (perhaps horizontal wheel)
	keyBackspace,
	keyTab,
	keyClear,
	keyReturn,
	keyPause,
	keyIMEKana,
	keyIMEHangul,
	keyIMEJunja,
	keyIMEFinal,
	keyIMEHanja,
	keyIMEKanji,
	keyEscape,
	keyIMEConvert,
	keyIMENonConvert,
	keyIMEAccept,
	keyIMEModeChange,
	keyIMEProcess,
	keySpace,
	keyExclaim,
	keyQuoteDbl,
	keyHash,
	keyDollar,
	keyPercent,
	keyAmpersand,
	keyQuote,
	keyLeftParen,
	keyRightParen,
	keyAsterisk,
	keyPlus,
	keyComma,
	keyMinus,
	keyPeriod,
	keySlash,
	key0,
	key1,
	key2,
	key3,
	key4,
	key5,
	key6,
	key7,
	key8,
	key9,
	keyColon,
	keySemicolon,
	keyLess,
	keyEquals,
	keyGreater,
	keyQuestion,
	keyAt,
	keyA,
	keyB,
	keyC,
	keyD,
	keyE,
	keyF,
	keyG,
	keyH,
	keyI,
	keyJ,
	keyK,
	keyL,
	keyM,
	keyN,
	keyO,
	keyP,
	keyQ,
	keyR,
	keyS,
	keyT,
	keyU,
	keyV,
	keyW,
	keyX,
	keyY,
	keyZ,
	keyLeftBracket,
	keyBackslash,
	keyRightBracket,
	keyCaret,
	keyUnderscore,
	keyBackquote,
	keyDelete,
	keyWorld0,
	keyWorld1,
	keyWorld2,
	keyWorld3,
	keyWorld4,
	keyWorld5,
	keyWorld6,
	keyWorld7,
	keyWorld8,
	keyWorld9,
	keyWorld10,
	keyWorld11,
	keyWorld12,
	keyWorld13,
	keyWorld14,
	keyWorld15,
	keyWorld16,
	keyWorld17,
	keyWorld18,
	keyWorld19,
	keyWorld20,
	keyWorld21,
	keyWorld22,
	keyWorld23,
	keyWorld24,
	keyWorld25,
	keyWorld26,
	keyWorld27,
	keyWorld28,
	keyWorld29,
	keyWorld30,
	keyWorld31,
	keyWorld32,
	keyWorld33,
	keyWorld34,
	keyWorld35,
	keyWorld36,
	keyWorld37,
	keyWorld38,
	keyWorld39,
	keyWorld40,
	keyWorld41,
	keyWorld42,
	keyWorld43,
	keyWorld44,
	keyWorld45,
	keyWorld46,
	keyWorld47,
	keyWorld48,
	keyWorld49,
	keyWorld50,
	keyWorld51,
	keyWorld52,
	keyWorld53,
	keyWorld54,
	keyWorld55,
	keyWorld56,
	keyWorld57,
	keyWorld58,
	keyWorld59,
	keyWorld60,
	keyWorld61,
	keyWorld62,
	keyWorld63,
	keyWorld64,
	keyWorld65,
	keyWorld66,
	keyWorld67,
	keyWorld68,
	keyWorld69,
	keyWorld70,
	keyWorld71,
	keyWorld72,
	keyWorld73,
	keyWorld74,
	keyWorld75,
	keyWorld76,
	keyWorld77,
	keyWorld78,
	keyWorld79,
	keyWorld80,
	keyWorld81,
	keyWorld82,
	keyWorld83,
	keyWorld84,
	keyWorld85,
	keyWorld86,
	keyWorld87,
	keyWorld88,
	keyWorld89,
	keyWorld90,
	keyWorld91,
	keyWorld92,
	keyWorld93,
	keyWorld94,
	keyWorld95,
	keyKP0,
	keyKP1,
	keyKP2,
	keyKP3,
	keyKP4,
	keyKP5,
	keyKP6,
	keyKP7,
	keyKP8,
	keyKP9,
	keyKPPeriod,
	keyKPDivide,
	keyKPMultiply,
	keyKPMinus,
	keyKPPlus,
	keyKPEnter,
	keyKPEquals,
	keyUp,
	keyDown,
	keyRight,
	keyLeft,
	keyInsert,
	keyHome,
	keyEnd,
	keyPageUp,
	keyPageDown,
	keyF1,
	keyF2,
	keyF3,
	keyF4,
	keyF5,
	keyF6,
	keyF7,
	keyF8,
	keyF9,
	keyF10,
	keyF11,
	keyF12,
	keyF13,
	keyF14,
	keyF15,
	keyF16,
	keyF17,
	keyF18,
	keyF19,
	keyF20,
	keyF21,
	keyF22,
	keyF23,
	keyF24,
	keyNumLock,
	keyCapsLock,
	keyScrollLock,
	keyLShift,
	keyRShift,
	keyLCtrl,
 	keyRCtrl,
	keyLAlt,
	keyRAlt,
	keyLMeta,
	keyRMeta,
	keyRSuper,
	keyLSuper,
	keyMode,
	keyCompose,
	keyHelp,
	keyPrint,
	keySysreq,
	keyBreak,
	keyMenu,
	keyPower,
	keyEuro,
	keyUndo,
	keyBrowserBack,
	keyBrowserForward,
	keyBrowserRefresh,
	keyBrowserStop,
	keyBrowserSearch,
	keyBrowserFavorites,
	keyBrowserHome,
	keyVolumeMute,
	keyVolumeDown,
	keyVolumeUp,
	keyMediaNext,
	keyMediaPrev,
	keyMediaStop,
	keyMediaPlayPause,
	keyLaunchMail,
	keyLaunchMediaSelect,
	keyLaunchApp1,
	keyLaunchApp2,
	keyPlay,
	keyZoom,

	keyCount
};


enum KeyModifier {
	kmNone		= 0x0000,
	kmLShift	= 0x0001,
	kmRShift	= 0x0002,/*
	kmAnyShift	= 0x0004,
	kmAnyCtrl	= 0x0008,
	kmAnyAlt	= 0x0010,
	kmAnyMeta	= 0x0020,*/
	kmLCtrl		= 0x0040,
	kmRCtrl		= 0x0080,
	kmLAlt		= 0x0100,
	kmRAlt		= 0x0200,
	kmLMeta		= 0x0400,
	kmRMeta		= 0x0800,
	kmNum		= 0x1000,
	kmCaps		= 0x2000,
	kmMode		= 0x4000,
	kmSuper		= 0x8000,
};
/*
class MouseEvent {
public:
	enum EventType 
	Vec2i pos;
	MouseButton button;

};
*/
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
	static const unsigned char mb2native[mbCount];
	static const short native2kc[NATIVE_KEY_CODE_LAST + 1];
	static const NativeKeyCodeCompact kc2native[keyCount];

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
		// we want to catch it in a debug build.  If not in debug build, we translate to keyUnknown.
		assert(nativeKey >= 0 && nativeKey <= NATIVE_KEY_CODE_LAST);
		if(nativeKey >= 0 && nativeKey <= NATIVE_KEY_CODE_LAST) {
			return static_cast<KeyCode>(native2kc[nativeKey]);
		} else {
			return keyUnknown;
		}
	}

	static NativeKeyCode getNativeKeyCode(KeyCode key) {
		assert(key >= 0 && key < keyCount);
		return static_cast<NativeKeyCode>(kc2native[key]);
	}

	static MouseButton getMouseButton(int nativeButton) {
		// Fail an assertion for debug builds so we can add the new mouse button codes, otherwise
		// send mbUnknown.
		assert(nativeButton >= 0 && nativeButton <= NATIVE_MOUSE_BUTTON_LAST);
		if(nativeButton >= 0 && nativeButton <= NATIVE_MOUSE_BUTTON_LAST) {
			return static_cast<MouseButton>(native2mb[nativeButton]);
		} else {
			return mbUnknown;
		}
	}

	static int getNativeMouseButton(int button) {
		assert(button >= 0 && button <= mbCount);
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
		if(key >= keyLShift && key <= keyRMeta) {
			KeyModifier mask;
			switch(key) {
				case keyNumLock:	mask = kmNum; break;
				case keyCapsLock:	mask = kmCaps; break;
				case keyLShift:		mask = kmLShift; break;
				case keyRShift:		mask = kmRShift; break;
				case keyLCtrl:		mask = kmLCtrl; break;
				case keyRCtrl:		mask = kmRCtrl; break;
				case keyLAlt:		mask = kmLAlt; break;
				case keyRAlt:		mask = kmRAlt; break;
				case keyLMeta:		mask = kmLMeta; break;
				case keyRMeta:		mask = kmRMeta; break;
				case keyRSuper:
				case keyLSuper:		mask = kmSuper; break;
				default:			return;
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
		assert(key >= 0 && key < keyCount);
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
		assert(key >= 0 && key < keyCount);
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
	
	static const char*names[keyCount];

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

	KeyCode getCode() const		{return key;}
	char getAscii() const		{return ascii;}
	bool isModifier() const		{return key >= keyNumLock && key <= keyMode;}
	
	static KeyCode findByName(const char *name);
	static const char* getName(KeyCode key) {
		assert(key >= 0 && key < keyCount);
		return names[key];
	}

};

}}//end namespace

#endif // _SHARED_PLATFORM_INPUT_H_
