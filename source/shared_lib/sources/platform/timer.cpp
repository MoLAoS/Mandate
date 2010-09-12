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
#include "timer.h"

#include <stdexcept>

//#include "util.h"
//#include "conversion.h"
//#include "leak_dumper.h"
#include <iostream>

//using namespace Shared::Util;

namespace Shared { namespace Platform {

using std::cout;
using std::endl;

// =====================================================
//	class Chrono
// =====================================================

bool Chrono::initialized = Chrono::init();
int64 Chrono::freq;

#ifdef _CHRONO_USE_POSIX

struct timezone Chrono::tz;

bool Chrono::init() {
	tz.tz_minuteswest = 0;
	tz.tz_dsttime = 0;
	freq = 1000000;
	return true;
}

#endif
#ifdef _CHRONO_USE_SDL

#error SDL timers are shit.
bool Chrono::init() {
	freq = 1000;
	return true;
}

#endif
#ifdef _CHRONO_USE_WIN

bool Chrono::init() {
	if(!QueryPerformanceFrequency((LARGE_INTEGER*) &freq)) {
		throw std::runtime_error("Performance counters not supported");
	}
	return true;
}

#endif


// =====================================================
//	class PerformanceTimer
// =====================================================

PerformanceTimer::PerformanceTimer(int fps, int maxTimes, int maxBacklog)
		: times(0)
		, maxTimes(maxTimes)
		, maxBacklog(maxBacklog) {
	assert(maxTimes == -1 || maxTimes > 0);
	assert(maxBacklog >= -1);
	reset();
	setFps(fps);
}

bool PerformanceTimer::checkTime(int64 &now, int64 &elapsed) {
	Chrono::getCurTicks(now);
	if (now > nextRollOver) {
		nextRollOver += Chrono::getResolution();
		lastTicks += padTicks; // delay next tick by padTicks
	}
	elapsed = now - lastTicks;
	return (elapsed >= updateTicks);
}

uint32 PerformanceTimer::timeToWait() {
	int64 now, elapsed;
	if (checkTime(now, elapsed)) {
		return 0;
	}
	// (updateTicks - elapsed) is 'ticks' to wait, convert to millis and return
	return (updateTicks - elapsed) * 1000 / Chrono::getResolution();
}

bool PerformanceTimer::isTime() {
	assert(updateTicks > 0);
	int64 now, elapsed;
	if (checkTime(now, elapsed) && times <= maxTimes) {
		int64 cyclesDue = elapsed / updateTicks;
		if (maxBacklog >= 0 && (cyclesDue - 1) > maxBacklog) {
			lastTicks = now - updateTicks * maxBacklog; // drop ticks
		} else {
			lastTicks += updateTicks;
		}
		++times;
		return true;
	}
	times = 0;
	return false;
}

void PerformanceTimer::reset() {
	Chrono::getCurTicks(lastTicks);
	nextRollOver = lastTicks + Chrono::getResolution();
	times = 0;
}

void PerformanceTimer::setFps(int fps) {
	assert(fps > 0);
	updateTicks = Chrono::getResolution() / fps;
	padTicks = Chrono::getResolution() - updateTicks * fps;
}

void PerformanceTimer::setMaxTimes(int v) {
	maxTimes = v;
}

void PerformanceTimer::setMaxBacklog(int v) {
	maxBacklog = v;
}


}}//end namespace
