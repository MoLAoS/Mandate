// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2005 Matthias Braun <matze@braunis.de>
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_TIMER_H_
#define _SHARED_PLATFORM_TIMER_H_

#include "projectConfig.h"

// Prefer posix, then SDL then native windows call
#ifdef HAVE_SYS_TIME_H
	// use gettimeofday with microsecond precision, although the actual resolution may be anywhere
	// from one microsecond to 100 milliseconds.
#	define	_CHRONO_USE_POSIX
#	include <sys/time.h>
#else
#ifdef USE_SDL
	// use SDL_GetTicks() with millisecond precision.
#	define	_CHRONO_USE_SDL
#	include <SDL.h>
#else
#if defined(WIN32) || defined(WIN64)
	// use QueryPerformanceCounter with variable precision
#	define	_CHRONO_USE_WIN
//#	include <winbase.h>
#	include <windows.h>
#else
#	error No usable timer
#endif
#endif
#endif

#include <cassert>

#include "types.h"

using Shared::Platform::int64;

namespace Shared { namespace Platform {

// =====================================================
//	class Chrono
// =====================================================

class Chrono {
private:
	int64 startTime;
	int64 stopTime;
	int64 accumTime;
#ifdef _CHRONO_USE_POSIX
	static struct timezone tz;
#endif
	static int64 freq;
	static bool initialized;

public:
	Chrono() : startTime(0), stopTime(0), accumTime(0) {
		assert(initialized);/*
		if(!initialized) {
			initialized = init();
		}*/
	}

	void start() {
	    getCurTicks(startTime);
	    stopTime = 0;
	}

	void stop() {
	    getCurTicks(stopTime);
		accumTime += stopTime - startTime;
	}

	const int64 &getStartTime() const	{return startTime;}
	const int64 &getStopTime() const	{return stopTime;}
	const int64 &getAccumTime() const	{return accumTime;}
	int64 getMicros() const				{return queryCounter(1000000);}
	int64 getMillis() const				{return queryCounter(1000);}
	int64 getSeconds() const			{return queryCounter(1);}
	static const int64 &getResolution()	{return freq;}

#ifdef _CHRONO_USE_POSIX

	static int64 getCurMicros()			{return getCurTicks();}
	static int64 getCurMillis()			{return getCurTicks() / 1000;}
	static int64 getCurSeconds()		{return getCurTicks() / 1000000;}
	static void getCurTicks(int64 &dest){dest = getCurTicks();}
	static int64 getCurTicks() {
		struct timeval now;
		gettimeofday(&now, &tz);
		return 1000000LL * now.tv_sec + now.tv_usec;
	}

#endif
#ifdef _CHRONO_USE_SDL

	static int64 getCurMicros()			{return SDL_GetTicks() * 1000;}
	static int64 getCurMillis()			{return SDL_GetTicks();}
	static int64 getCurSeconds()		{return SDL_GetTicks() / 1000;}
	static void getCurTicks(int64 &dest){dest = getCurTicks();}
	static int64 getCurTicks()			{return SDL_GetTicks();}

#endif
#ifdef _CHRONO_USE_WIN

	static int64 getCurMicros()			{return getCurTicks() * 1000000 / freq;}
	static int64 getCurMillis()			{return getCurTicks() * 1000 / freq;}
	static int64 getCurSeconds()		{return getCurTicks() / freq;}
	static void getCurTicks(int64 &dest){QueryPerformanceCounter((LARGE_INTEGER*) &dest);}
	static int64 getCurTicks() {
		int64 now;
		QueryPerformanceCounter((LARGE_INTEGER*) &now);
		return now;
	}

#endif

private:
	static bool init();
	int64 queryCounter(int multiplier) const {
		return multiplier * (accumTime + (stopTime ? 0 : getCurTicks() - startTime)) / freq;
	}
};



// =====================================================
//	class PerformanceTimer
// =====================================================

class PerformanceTimer {
private:
	int64 lastTicks;
	int64 updateTicks;

	int times;			// number of consecutive times
	int maxTimes;		// maximum number consecutive times
	int maxBacklog;		// maxiumum backlog to allow after maxTimes is reached

public:
	PerformanceTimer(float fps, int maxTimes = -1, int maxBacklog = -1);

	/** Returns the amount of time to wait in milliseconds before the timer is due. */
	uint32 timeToWait() {
		int64 elapsed = Chrono::getCurTicks() - lastTicks;
		return elapsed >= updateTicks ? 0 : (updateTicks - elapsed) * 1000 / Chrono::getResolution();
	}

	bool isTime();
	void reset() 						{Chrono::getCurTicks(lastTicks);}
	void setFps(float fps)				{updateTicks = (int64)((float)Chrono::getResolution() / fps);}
	void setMaxTimes(int maxTimes)		{this->maxTimes = maxTimes;}
	void setMaxBacklog(int maxBacklog)	{this->maxBacklog = maxBacklog;}
};

}}//end namespace

#endif // _SHARED_PLATFORM_TIMER_H_
