//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under
//the terms of the GNU General Public License as published by the Free Software
//Foundation; either version 2 of the License, or (at your option) any later
//version.

#include "pch.h"
#include "platform_util.h"

#include <iostream>
#include <sstream>
#include <cassert>

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <execinfo.h>

#include <SDL.h>

#include "util.h"
#include "conversion.h"
#include "sdl_private.h"
#include "window.h"
#include "noimpl.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

namespace Shared{ namespace Platform{

namespace Private {

bool shouldBeFullscreen = false;
int ScreenWidth;
int ScreenHeight;

}

// =====================================
//          PerformanceTimer
// =====================================

PerformanceTimer::PerformanceTimer(float fps, int maxTimes, int maxBacklog) :
		lastTicks(SDL_GetTicks()),
		updateTicks(static_cast<int>(1000.f / fps)),
		times(0),
		maxTimes(maxTimes),
		maxBacklog(maxBacklog) {
	assert(maxTimes == -1 || maxTimes > 0);
	assert(maxBacklog >= -1);
}



// =====================================
//         Chrono
// =====================================

Chrono::Chrono() {
	freq = 1000;
	stopped= true;
	accumCount= 0;
}


// =====================================================
//	class PlatformExceptionHandler
// =====================================================

PlatformExceptionHandler *PlatformExceptionHandler::singleton = NULL;

PlatformExceptionHandler::~PlatformExceptionHandler(){}

void PlatformExceptionHandler::install() {
	singleton = this;
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = NULL;
	//action.sa_mask
	action.sa_flags = SA_SIGINFO | SA_NODEFER;
	action.sa_sigaction = PlatformExceptionHandler::handler;
	sigaction(SIGILL, &action, NULL);
	sigaction(SIGSEGV, &action, NULL);
//	sigaction(SIGABRT, &action, NULL);
	sigaction(SIGFPE, &action, NULL);
	sigaction(SIGBUS, &action, NULL);
}

void PlatformExceptionHandler::handler(int signo, siginfo_t *info, void *context) {
	// how many times we've been here
	static int recursionCount = 0;
	static bool logged = false;
	++recursionCount;

	if(recursionCount > 3) {
		// if we've crashed more than three times, it means we can't even fprintf so just fuck it.
		exit(1);
	}

	if(recursionCount > 2) {
		// if we've crashed more than twice, it means that our error handler crashed in both pretty
		// and non-pretty modes
		fprintf(stderr, "Multiple segfaults, cannot correctly display error.  If you're lucky, the "
				"log is at least populated.\n");
		exit(1);
	}

	if (!logged && recursionCount == 1) {
		const char* signame;
		const char* sigcode;
		char description[128];

		void *array[256];
		size_t size = 256;
		char **strings;

		switch (signo) {

		case SIGILL:
			signame = "SIGILL";

			switch (info->si_code) {

			case ILL_ILLOPC:
				sigcode = "illegal opcode";
				break;

			case ILL_ILLOPN:
				sigcode = "illegal operand";
				break;

			case ILL_ILLADR:
				sigcode = "illegal addressing mode";
				break;

			case ILL_ILLTRP:
				sigcode = "illegal trap";
				break;

			case ILL_PRVOPC:
				sigcode = "privileged opcode";
				break;

			case ILL_PRVREG:
				sigcode = "privileged register";
				break;

			case ILL_COPROC:
				sigcode = "coprocessor error";
				break;

			case ILL_BADSTK:
				sigcode = "internal stack error";
				break;

			default:
				sigcode = "unrecognized si_code";
				break;
			}

			break;

		case SIGFPE:
			signame = "SIGFPE";

			switch (info->si_code) {

			case FPE_INTDIV:
				sigcode = "integer divide by zero";
				break;

			case FPE_INTOVF:
				sigcode = "integer overflow";
				break;

			case FPE_FLTDIV:
				sigcode = "floating point divide by zero";
				break;

			case FPE_FLTOVF:
				sigcode = "floating point overflow";
				break;

			case FPE_FLTUND:
				sigcode = "floating point underflow";
				break;

			case FPE_FLTRES:
				sigcode = "floating point inexact result";
				break;

			case FPE_FLTINV:
				sigcode = "floating point invalid operation";
				break;

			case FPE_FLTSUB:
				sigcode = "subscript out of range";
				break;

			default:
				sigcode = "unrecognized si_code";
				break;
			}

			break;

		case SIGSEGV:
			signame = "SIGSEGV";

			switch (info->si_code) {

			case SEGV_MAPERR:
				sigcode = "address not mapped to object";
				break;

			case SEGV_ACCERR:
				sigcode = "invalid permissions for mapped object";
				break;

			default:
				sigcode = "unrecognized si_code";
				break;
			}

			break;

		case SIGBUS:
			signame = "SIGBUS";

			switch (info->si_code) {

			case BUS_ADRALN:
				sigcode = "invalid address alignment";
				break;

			case BUS_ADRERR:
				sigcode = "non-existent physical address";
				break;

			case BUS_OBJERR:
				sigcode = "object specific hardware error";
				break;

			default:
				sigcode = "unrecognized si_code";
				break;
			}

			break;

		default:
			signame = "unexpected sinal";
			sigcode = "(wtf?)";
		}
		snprintf(description, sizeof(description), "%s: %s", signame, sigcode);

		size = backtrace(array, size);
		strings = backtrace_symbols(array, size);
		if(!strings) {
			fprintf(stderr, "%s\n", description);
			backtrace_symbols_fd(array, size, STDERR_FILENO);
		}
		singleton->log(description, info->si_addr, (const char**)strings, size, NULL);
		logged = true;
		free(strings);
	}

	// If this is our 1st entry, try a pretty, opengl-based error message.  Otherwise, resort to
	// primative format.
	singleton->notifyUser(recursionCount == 1);

	--recursionCount;

	exit(1);
}


// =====================================
//         Misc
// =====================================

char *initDirIterator(const string &path, DirIterator &di) {
	string mypath = path;
	di.i = 0;

	/* Stupid win32 is searching for all files without extension when *. is
	 * specified as wildcard
	 */
	if(mypath.compare(mypath.size() - 2, 2, "*.") == 0) {
		mypath = mypath.substr(0, mypath.size() - 2);
		mypath += "*";
	}

	if(glob(mypath.c_str(), 0, 0, &di.globbuf) < 0) {
		throw runtime_error("Error searching for files '" + path + "': " + strerror(errno));
	}
	return di.globbuf.gl_pathc ? di.globbuf.gl_pathv[di.i] : NULL;
}

void mkdir(const string &path, bool ignoreDirExists) {
	if(::mkdir(path.c_str(), S_IRWXU | S_IROTH | S_IXOTH)) {
		int e = errno;
		if(!(ignoreDirExists && e == EEXIST)) {
			char buf[0x400];
			*buf = 0;
			strerror_r(e, buf, sizeof(buf));
			throw runtime_error("Failed to create directory " + path + ": " + buf);
		}
	}
}

size_t getFileSize(const string &path) {
	struct stat s;
	if(stat(path.c_str(), &s)) {
		char buf[0x400];
		*buf = 0;
		strerror_r(errno, buf, sizeof(buf));
		throw runtime_error("Failed to stat file " + path + ": " + buf);
	}
	return s.st_size;
}

bool changeVideoMode(int resW, int resH, int colorBits, int ) {
	Private::shouldBeFullscreen = true;
	return true;
}

void restoreVideoMode() {
}

void message(string message) {
	std::cerr << "******************************************************\n";
	std::cerr << "    " << message << "\n";
	std::cerr << "******************************************************\n";
}

bool ask(string message) {
	std::cerr << "Confirmation: " << message << "\n";
	int res;
	std::cin >> res;
	return res != 0;
}

bool isKeyDown(int virtualKey) {
	char key = static_cast<char> (virtualKey);
	const Uint8* keystate = SDL_GetKeyState(0);

	// kinda hack and wrong...
	if(key >= 0) {
		return keystate[key];
	}
	switch(key) {
		case vkAdd:
			return keystate[SDLK_PLUS] | keystate[SDLK_KP_PLUS];
		case vkSubtract:
			return keystate[SDLK_MINUS] | keystate[SDLK_KP_MINUS];
		case vkAlt:
			return keystate[SDLK_LALT] | keystate[SDLK_RALT];
		case vkControl:
			return keystate[SDLK_LCTRL] | keystate[SDLK_RCTRL];
		case vkShift:
			return keystate[SDLK_LSHIFT] | keystate[SDLK_RSHIFT];
		case vkEscape:
			return keystate[SDLK_ESCAPE];
		case vkUp:
			return keystate[SDLK_UP];
		case vkLeft:
			return keystate[SDLK_LEFT];
		case vkRight:
			return keystate[SDLK_RIGHT];
		case vkDown:
			return keystate[SDLK_DOWN];
		case vkReturn:
			return keystate[SDLK_RETURN] | keystate[SDLK_KP_ENTER];
		case vkBack:
			return keystate[SDLK_BACKSPACE];
		default:
			std::cerr << "isKeyDown called with unknown key.\n";
			break;
	}
	return false;
}

}}//end namespace
