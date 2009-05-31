// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PCH_H_
#define _SHARED_PCH_H_
#ifdef USE_PCH

#if defined(WIN32) || defined(WIN64)

	// sanity checks
	#if defined (USE_POSIX_SOCKETS)
		#error USE_POSIX_SOCKETS is not compatible with WIN32 or WIN64
	#endif

	#if defined (USE_SDL)
		#error USE_SDL is not compatible with WIN32 or WIN64
	#endif

	#include <windows.h>
	#include <io.h>

#else
	#include <unistd.h>
#endif

// some local headers of importance
#include "types.h"

// POSIX base
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>


// lib c++ & stl
#include <cstdlib>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <fstream>
#include <algorithm>
#include <vector>
#include <list>
#include <map>
#include <deque>
#include <string>
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <ctime>

#include <sys/types.h>

#ifdef USE_POSIX_SOCKETS

	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <fcntl.h>
//	#include <sys/filio.h>
	#include <sys/ioctl.h>

#endif

#ifdef USE_SDL
	#include <SDL.h>
	#include <SDL_opengl.h>
	#include <SDL_thread.h>
	#include <SDL_mutex.h>
	#include <glob.h>
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <sys/stat.h>
#endif

// zlib
#include <zlib.h>

// opengl
#include <GL/gl.h>
#include <GL/glu.h>
#if !(defined(WIN32) || defined(WIN64))
	#include <GL/glx.h>
#endif


// vorbis
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

// xerces-c
#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/Wrapper4InputSource.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XercesDefs.hpp>

#if defined(WIN32) || defined(WIN64)
	#include <glprocs.h>
	#include <winsock.h>
	#include <dsound.h>
#endif

#endif // USE_PCH
#endif // _SHARED_PCH_H_

