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
#include <sys/resource.h>

#include <SDL.h>

#include "util.h"
#include "conversion.h"
#include "sdl_private.h"
#include "window.h"
#include "noimpl.h"
#include "FSFactory.hpp"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

namespace Shared{ namespace Platform{

namespace Private {
	bool shouldBeFullscreen = false;
	int ScreenWidth;
	int ScreenHeight;
}

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

PlatformExceptionHandler *PlatformExceptionHandler::singleton = NULL;

//PlatformExceptionHandler::~PlatformExceptionHandler(){}

void PlatformExceptionHandler::install() {
	assert(this);
	assert(singleton == this);
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = NULL;
	//action.sa_mask
	action.sa_flags = SA_SIGINFO | SA_NODEFER;
	action.sa_sigaction = PlatformExceptionHandler::handler;
#ifndef DEBUG	
#endif
	sigaction(SIGILL, &action, NULL);
	sigaction(SIGSEGV, &action, NULL);
	sigaction(SIGABRT, &action, NULL);
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

		case SIGABRT:
			signame = "SIGABRT";
			sigcode = "";
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

	--recursionCount; //???, we exit program below

	// change working directory to get core dump there
	chdir(g_fileFactory.getConfigDir().c_str());
	// enable core dump for this process
	struct rlimit rl;
	rl.rlim_cur = RLIM_INFINITY;
	rl.rlim_max = RLIM_INFINITY;
	if(setrlimit(RLIMIT_CORE, &rl)==-1){
		  fprintf(stderr, "setrlimit failed: %s\n", strerror(errno));
		  exit(1);
	}
	// uninstall signal handler
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = SIG_DFL;
	sigaction(signo, &action, NULL);
	// resend signal so we get a core dump
	raise(signo);

	//exit(1);
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

string videoModeToString(const VideoMode in_mode) {
	return toStr(in_mode.w) + "x" + toStr(in_mode.h) + " " + toStr(in_mode.bpp) + "bpp @ " 
		+ toStr(in_mode.freq) + "Hz.";
}

bool changeVideoMode(const VideoMode in_mode) {
	//FIXME: huh? what's that? SDL_SetVideoMode(in_mode.w, in_mode.h, in_mode.bpp, /* FLAGS*/); better
	Private::shouldBeFullscreen = true;
	return true;
}

void restoreVideoMode() {
}

void getPossibleScreenModes(vector<VideoMode> &out_modes) {
	//FIXME: use real flags, without SDL_FULLSCREEN most likely all possible
	SDL_Rect **modes = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_OPENGL);
	if (modes == (SDL_Rect**)0) { // Check if there are any modes available
		// No modes available!
	}else if (modes == (SDL_Rect**)-1) { // Check if our resolution is restricted
		// All resolutions available.
	}else{
		// valid modes
		for (int i=0; modes[i]; ++i){
			VideoMode mode(modes[i]->w, modes[i]->h, 0, 0);
			out_modes.push_back(mode);
		}
	}
}

void getScreenMode(int &width, int &height){
	const SDL_VideoInfo *vidinfo = SDL_GetVideoInfo();
	width = vidinfo->current_w;
	height = vidinfo->current_h;
}

}}//end namespace
