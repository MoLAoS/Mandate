// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_PLATFORMUTIL_H_
#define _SHARED_PLATFORM_PLATFORMUTIL_H_

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <glob.h>
#include <signal.h>

#include "types.h"
#include "timer.h"
#include "conversion.h"

using std::string;
using std::vector;
using std::exception;

using Shared::Platform::int64;

namespace Shared { namespace Platform {

using namespace Util::Conversion;

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

class PlatformExceptionHandler {
private:
	static PlatformExceptionHandler *singleton;
	static void handler(int signo, siginfo_t *info, void *context);

public:
	PlatformExceptionHandler()			{assert(!singleton); singleton = this;}
	virtual ~PlatformExceptionHandler()	{assert(singleton == this); singleton = NULL;}
	void install();
	virtual void log(const char *description, void *address, const char **backtrace, size_t count, const exception *e) = 0;
	virtual void notifyUser(bool pretty) = 0;
};

// =====================================================
//	Misc
// =====================================================

typedef struct _DirIterator {
	int i;
	glob_t globbuf;
} DirIterator;

char *initDirIterator(const string &path, DirIterator &di);

inline char *getNextFile(DirIterator &di) {
	return ++di.i < di.globbuf.gl_pathc ? di.globbuf.gl_pathv[di.i] : NULL;
}

inline void freeDirIterator(DirIterator &di) {
	globfree(&di.globbuf);
}

void mkdir(const string &path, bool ignoreDirExists = false);
size_t getFileSize(const string &path);

string videoModeToString(const VideoMode in_mode);
bool changeVideoMode(const VideoMode in_mode);
void restoreVideoMode();
void getPossibleScreenModes(vector<VideoMode> &out_modes);
void getScreenMode(int &width, int &height);

void message(string message);
bool ask(string message);

inline void exceptionMessage(const exception &excp) {
	std::cerr << "Exception: " << excp.what() << std::endl;
}

inline int getScreenW() {
	return SDL_GetVideoSurface()->w;
}

inline int getScreenH() {
	return SDL_GetVideoSurface()->h;
}

inline void sleep(int millis) {
	SDL_Delay(millis);
}

inline void showCursor(bool b) {
	SDL_ShowCursor(b ? SDL_ENABLE : SDL_DISABLE);
}

//bool isKeyDown(int virtualKey);
string getCommandLine();

/*
malloc16
double *p;
double *np;
p = (double *)malloc(sizeof(double) * number_of_doubles + 15);
np = (double *)((((ptrdiff_t)(p)) + 15L) & (-16L));
*/

}}//end namespace

#endif
