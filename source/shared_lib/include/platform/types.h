// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_TYPES_H_
#define _SHARED_PLATFORM_TYPES_H_

#include "projectConfig.h"

#ifdef USE_SDL
#	include <SDL.h>
#	include <SDL_thread.h>
#endif
#if defined(WIN32)  || defined(WIN64)
#	define NOMINMAX
#	include <windows.h>
#endif

namespace Shared { namespace Platform {

#if defined(WIN32)  || defined(WIN64)
	typedef HWND WindowHandle;
	typedef HDC DeviceContextHandle;
	typedef HGLRC GlContextHandle;
	typedef CRITICAL_SECTION MutexType;
	typedef HANDLE ThreadType;
	typedef DWORD NativeKeyCode;
	typedef unsigned char NativeKeyCodeCompact;

	typedef float float32;
	typedef double float64;
	typedef char int8;
	typedef unsigned char uint8;
	typedef short int int16;
	typedef unsigned short int uint16;
	typedef int int32;
	typedef unsigned int uint32;
	typedef long long int64;
	typedef unsigned long long uint64;

#	define strcasecmp stricmp
#	define strtok_r(a,b,c) strtok(a,b)
#	define log2(x) (log(float(x))/log(2.f))
#endif

#ifdef USE_SDL
	// These don't have a real meaning in the SDL port
	typedef void* WindowHandle;
	typedef void* DeviceContextHandle;
	typedef void* GlContextHandle;
	typedef SDL_mutex* MutexType;
	typedef SDL_Thread* ThreadType;
	typedef SDLKey NativeKeyCode;
	typedef unsigned short NativeKeyCodeCompact;

	typedef float float32;
	typedef double float64;
	// don't use Sint8 here because that is defined as signed char
	// and some parts of the code do std::string str = (int8*) var;
	typedef char int8;
	typedef Uint8 uint8;
	typedef Sint16 int16;
	typedef Uint16 uint16;
	typedef Sint32 int32;
	typedef Uint32 uint32;
	typedef Sint64 int64;
	typedef Uint64 uint64;

#	define override
#endif

#ifdef USE_POSIX_SOCKETS
	typedef int SOCKET;
#endif

struct VideoMode {
	int w, h;
	int bpp;
	int freq;

	VideoMode() : w(0), h(0), bpp(0), freq(0) {}
	VideoMode(int w, int h, int bpp, int freq) : w(w), h(h), bpp(bpp), freq(freq) {}

	bool operator==(const VideoMode &m) const {
		return (w == m.w && h == m.h && bpp == m.bpp && freq == m.freq);
	}
	bool operator!=(const VideoMode &m) const {
		return !(*this == m);
	}
};

}}//end namespace

#endif
