// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_MAIN_H_
#define _SHARED_PLATFORM_MAIN_H_

#define NOMINMAX
#include <windows.h>

#if _GAE_DEBUG_EDITION_ || !defined(NDEBUG)

#	define MAIN_FUNCTION(X)						\
		int main(int argc, char **argv) {		\
			return X(argc, argv);				\
		}
#else

#	define MAIN_FUNCTION(X)						\
		int main(int argc, char *argv[]) {		\
			HWND hWnd = GetConsoleWindow();		\
			ShowWindow(hWnd, SW_HIDE);			\
			return X(argc, argv);				\
		}
#endif

#endif
