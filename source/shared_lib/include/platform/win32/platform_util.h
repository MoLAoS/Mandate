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

#include <string>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <io.h>
#include <errno.h>

#include "types.h"
#include "conversion.h"

namespace Shared { namespace Platform {

using std::string;
using std::vector;
using std::exception;
using std::runtime_error;
using namespace Util::Conversion;

void fail(const char *msg, HRESULT hr);

#define CHECK_HR(hr, errorMsg) \
	if (FAILED(hr)) {     \
		fail(errorMsg, hr);    \
	}


// =====================================================
//	class PlatformExceptionHandler
// =====================================================

class PlatformExceptionHandler {
private:
	static PlatformExceptionHandler *singleton;
	static LONG WINAPI handler(LPEXCEPTION_POINTERS pointers);

public:
	PlatformExceptionHandler()			{assert(!singleton); singleton = this;}
	virtual ~PlatformExceptionHandler()	{assert(singleton == this); singleton = NULL;}
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

string videoModeToString(const VideoMode in_mode);
bool changeVideoMode(const VideoMode in_mode);
void restoreVideoMode();
void getPossibleScreenModes(vector<VideoMode> &out_modes);
void getScreenMode(int &width, int &height);

void message(string message);
bool ask(string message);
void exceptionMessage(const exception &excp);

inline int getScreenW()			{return GetSystemMetrics(SM_CXSCREEN);}
inline int getScreenH()			{return GetSystemMetrics(SM_CYSCREEN);}

inline void sleep(int millis)	{Sleep(millis);}
inline void showCursor(bool b)	{ShowCursor(b);}

string getCommandLine();

}}//end namespace

#endif
