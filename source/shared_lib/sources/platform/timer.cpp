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

//using namespace Shared::Util;

namespace Shared { namespace Platform {


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

bool Chrono::init() {
	freq = 1000;
	return true;
}

#endif
#ifdef _CHRONO_USE_WIN

bool Chrono::init() {
	if(!QueryPerformanceFrequency((LARGE_INTEGER*) &freq)) {
		throw runtime_error("Performance counters not supported");
	}
	return true;
}

#endif


// =====================================================
//	class PerformanceTimer
// =====================================================

PerformanceTimer::PerformanceTimer(float fps, int maxTimes, int maxBacklog) :
//		lastTicks(Chrono::getCurTicks()),
//		updateTicks((int64)((float)Chrono::getResolution() / fps)),
		times(0),
		maxTimes(maxTimes),
		maxBacklog(maxBacklog) {
	assert(maxTimes == -1 || maxTimes > 0);
	assert(maxBacklog >= -1);
	reset();
	setFps(fps);
}

bool PerformanceTimer::isTime() {
	int64 curTicks;
	Chrono::getCurTicks(curTicks);
	int64 elapsed = curTicks - lastTicks;
	int64 cyclesDue = elapsed / updateTicks;

	if(cyclesDue && (times < maxTimes || maxTimes == -1)) {
		--cyclesDue;
		if(maxBacklog >= 0 && cyclesDue > maxBacklog) {
			lastTicks = curTicks - updateTicks * maxBacklog;
		} else {
			lastTicks += updateTicks;
		}
		++times;

		return true;
	}

	times = 0;
	return false;
}

}}//end namespace
