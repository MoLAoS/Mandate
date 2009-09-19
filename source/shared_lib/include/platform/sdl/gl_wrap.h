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

#ifndef _SHARED_PLATFORM_GLWRAP_H_
#define _SHARED_PLATFORM_GLWRAP_H_

#include <SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>

#include <string>
#include <cassert>

#include "font.h"
#include "types.h"
#include "noimpl.h"

using std::string;
using Shared::Graphics::FontMetrics;

namespace Shared { namespace Platform {

// =====================================================
// class PlatformContextGl
// =====================================================

class PlatformContextGl {
public:
	virtual ~PlatformContextGl() {}

	void init(int colorBits, int depthBits, int stencilBits);
	void end()			{}

	void makeCurrent()	{}
	void swapBuffers()	{SDL_GL_SwapBuffers();}

	DeviceContextHandle getHandle() const { return 0; }
};

// =====================================================
// Global Fcs
// =====================================================

void createGlFontBitmaps(uint32 &base, const string &type, int size, int width, int charCount, FontMetrics &metrics);
void createGlFontOutlines(uint32 &base, const string &type, int width, float depth, int charCount, FontMetrics &metrics);
inline const char *getPlatformExtensions(const PlatformContextGl *pcgl) {return "";}

inline void *getGlProcAddress(const char *procName) {
	void* proc = SDL_GL_GetProcAddress(procName);
	assert(proc);
	return proc;
}

inline void createGlFontOutlines(uint32 &base, const string &type, int width, float depth,
		int charCount, FontMetrics &metrics) {
	NOIMPL;
}

}}//end namespace

#endif
