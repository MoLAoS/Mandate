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
	//cout << "Timer maxTimes : " << maxTimes << ", maxBackLog == " << maxBacklog << endl;
}

uint32 PerformanceTimer::timeToWait() {
	const int64 &now = Chrono::getCurTicks();
	if (now > nextRollOver) {
		nextRollOver += Chrono::getResolution();
		lastTicks += padTicks; // delay next tick by padTicks
	}
	int64 elapsed = now - lastTicks;
	return elapsed >= updateTicks ? 0 : (updateTicks - elapsed) * 1000 / now;
}

bool PerformanceTimer::isTime() {
	const int64 &now = Chrono::getCurTicks();
	int64 elapsed = now - lastTicks;
	int64 cyclesDue = elapsed / updateTicks;

	if(cyclesDue && (times < maxTimes || maxTimes == -1)) {
		--cyclesDue;
		if (maxBacklog >= 0 && cyclesDue > maxBacklog) {
			//if (updateTicks == 14914) { // if updateTimer on Silnarm's computer ;)
			//	cout << "dropping ticks ... " << (cyclesDue - maxBacklog) << endl;
			//}
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
	updateTicks = Chrono::getResolution() / fps;
	padTicks = Chrono::getResolution() - updateTicks * fps;
	cout << "Timer FPS set : " << fps << ", updateTicks == " << updateTicks << endl;
}

void PerformanceTimer::setMaxTimes(int v) {
	maxTimes = v;
	//cout << "Timer maxTimes set : " << maxTimes << endl;
}

void PerformanceTimer::setMaxBacklog(int v) {
	maxBacklog = v;
	//cout << "Timer maxBacklog set : " << maxBacklog << endl;
}


}}//end namespace
