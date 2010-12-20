//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under
//the terms of the GNU General Public License as published by the Free Software
//Foundation; either version 2 of the License, or (at your option) any later
//version.

#include "pch.h"
#include "gl_wrap.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>

#include "projectConfig.h"

#include <SDL.h>

#ifdef X11_AVAILABLE
#	ifdef __APPLE__
#		include <OpenGL/OpenGL.h>
#	else
#		include <GL/glx.h>	
#	endif
#endif

#include "opengl.h"
#include "sdl_private.h"
#include "noimpl.h"

#include "leak_dumper.h"

using namespace Shared::Graphics::Gl;

namespace Shared{ namespace Platform{

// ======================================
//	class PlatformContextGl
// ======================================

void PlatformContextGl::init(int colorBits, int depthBits, int stencilBits) {

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilBits);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBits);
	int flags = SDL_OPENGL;
	if(Private::shouldBeFullscreen)
		flags |= SDL_FULLSCREEN;

	int resW = Private::ScreenWidth;
	int resH = Private::ScreenHeight;
	SDL_Surface* screen = SDL_SetVideoMode(resW, resH, colorBits, flags);
	if(screen == 0) {
		std::ostringstream msg;
		msg << "Couldn't set video mode "
			<< resW << "x" << resH << " (" << colorBits
			<< "bpp " << stencilBits << " stencil "
			<< depthBits << " depth-buffer). SDL Error is: " << SDL_GetError();
		throw std::runtime_error(msg.str());
	}

	GLint glewErr = glewInit();
	if (glewErr != GLEW_OK) {
		throw runtime_error(string("Error initialising Glew: ") + (char*)glewGetErrorString(glewErr));
	}
}

}}//end namespace
