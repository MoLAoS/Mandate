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

#include "pch.h"
#include "gl_wrap.h"

#include <cassert>
#include <windows.h>

#include "opengl.h"

#include "leak_dumper.h"


using namespace Shared::Graphics::Gl;

namespace Shared { namespace Platform {

// =====================================================
//	class PlatformContextGl
// =====================================================

void PlatformContextGl::init(int colorBits, int depthBits, int stencilBits){

	int iFormat;
	PIXELFORMATDESCRIPTOR pfd;
	BOOL err;

	//Set8087CW($133F);
	dch = GetDC(GetActiveWindow());
	assert(dch!=NULL);

	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize= sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion= 1;
	pfd.dwFlags= PFD_GENERIC_ACCELERATED | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType= PFD_TYPE_RGBA;
	pfd.cColorBits= colorBits;
	pfd.cDepthBits= depthBits;
	pfd.iLayerType= PFD_MAIN_PLANE;
	pfd.cStencilBits= stencilBits;

	iFormat= ChoosePixelFormat(dch, &pfd);
	assert(iFormat!=0);

	err= SetPixelFormat(dch, iFormat, &pfd);
	assert(err);

	glch= wglCreateContext(dch);
	if(glch==NULL){
		throw runtime_error("Error initing OpenGL device context");
	}
	makeCurrent();

	GLint glewErr = glewInit();
	if (glewErr != GLEW_OK) {
		throw runtime_error(string("Error initialising Glew: ") + (char*)glewGetErrorString(glewErr));
	}
}

}}//end namespace
