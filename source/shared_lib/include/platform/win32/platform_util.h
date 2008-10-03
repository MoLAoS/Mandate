// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_PLATFORMUTIL_H_
#define _SHARED_PLATFORM_PLATFORMUTIL_H_

#include <windows.h>

#include <string>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <io.h>
#include <errno.h>

#include "types.h"

using std::string;
using std::vector;
using std::exception;
using std::runtime_error;

using Shared::Platform::int64;

namespace Shared{ namespace Platform{

// =====================================================
//	class PerformanceTimer
// =====================================================

class PerformanceTimer{
private:
	int64 lastTicks;
	int64 updateTicks;
	int64 freq;

	int times;			// number of consecutive times
	int maxTimes;		// maximum number consecutive times
	int maxBacklog;

public:
	PerformanceTimer(float fps, int maxTimes= -1, int maxBacklog = -1);

	bool isTime() {
		int64 curTicks;
		QueryPerformanceCounter((LARGE_INTEGER*) &curTicks);
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

	uint32 timeToWait() {
		int64 curTicks;
		QueryPerformanceCounter((LARGE_INTEGER*) &curTicks);
		int64 elapsed = curTicks - lastTicks;
		return (uint32)(elapsed >= updateTicks ? 0 : (updateTicks - elapsed) * 1000 / freq);
	}

	void reset() 			{QueryPerformanceCounter((LARGE_INTEGER*) &lastTicks);}
	void setFps(float fps)	{updateTicks= static_cast<int>(1000./fps);}
	void setMaxTimes(int maxTimes)		{this->maxTimes = maxTimes;}
	void setMaxBacklog(int maxBacklog)	{this->maxBacklog = maxBacklog;}
};

// =====================================================
//	class Chrono
// =====================================================

class Chrono{
private:
	int64 startCount;
	int64 accumCount;
	int64 freq;
	bool stopped;

public:
	Chrono();
	void start();
	void stop();
	int64 getMicros() const;
	int64 getMillis() const;
	int64 getSeconds() const;

private:
	int64 queryCounter(int multiplier) const;
};

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

LONG WINAPI UnhandledExceptionFilter2(struct _EXCEPTION_POINTERS *ExceptionInfo);

class PlatformExceptionHandler {
private:
	static PlatformExceptionHandler *singleton;
	static LONG WINAPI handler(LPEXCEPTION_POINTERS pointers);

public:
	virtual ~PlatformExceptionHandler() {}
	void install();
	virtual void log(const char *description, void *address, const char **backtrace, size_t count, const exception *e) = 0;
	virtual void notifyUser(bool pretty) = 0;
	static string codeToStr(DWORD code);
};

// =====================================================
//	Misc
// =====================================================

typedef struct _DirIterator {
	struct __finddata64_t fi;
	intptr_t handle;
} DirIterator;


inline char *initDirIterator(const string &path, DirIterator &di) {
	if((di.handle = _findfirst64(path.c_str(), &di.fi)) == -1) {
		if(errno == ENOENT) {
			return NULL;
		}
		throw runtime_error("Error searching for files '" + path + "': " + strerror(errno));
	}
	return di.fi.name;
}

inline char *getNextFile(DirIterator &di) {
	assert(di.handle != -1);
	return !_findnext64(di.handle, &di.fi) ? di.fi.name : NULL;
}

inline void freeDirIterator(DirIterator &di) {
	assert(di.handle != -1);
	_findclose(di.handle);
}

void mkdir(const string &path, bool ignoreDirExists = false);
size_t getFileSize(const string &path);

bool changeVideoMode(int resH, int resW, int colorBits, int refreshFrequency);
void restoreVideoMode();

void message(string message);
bool ask(string message);
void exceptionMessage(const exception &excp);

int getScreenW();
int getScreenH();

void sleep(int millis);

void showCursor(bool b);
bool isKeyDown(int virtualKey);
string getCommandLine();

}}//end namespace

#endif
