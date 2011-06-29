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

string getLastErrorMsg() {
	DWORD errCode = GetLastError();
	LPVOID errMsg;
	DWORD msgRes = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER  | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errMsg, 0, NULL
	);
	std::stringstream msg;
	if (msgRes) {
		msg << (LPTSTR)errMsg;
		LocalFree((HLOCAL)errMsg);
	} else {
		msg << "FormatMessage() call failed. :~( [Error code: " << errCode << "]";
	}
	return msg.str();
}

void PlatformContextGl::setupDeviceContext() {
	int iFormat;
	PIXELFORMATDESCRIPTOR pfd;
	BOOL err;

	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize= sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion= 1;
	pfd.dwFlags= PFD_GENERIC_ACCELERATED | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType= PFD_TYPE_RGBA;
	pfd.cColorBits= m_colorBits;
	pfd.cDepthBits= m_depthBits;
	pfd.iLayerType= PFD_MAIN_PLANE;
	pfd.cStencilBits= m_stencilBits;

	iFormat = ChoosePixelFormat(dch, &pfd);
	if (!iFormat) {
		std::stringstream msg;
		msg << "ChoosePixelFormat() failed.\n" << getLastErrorMsg();
		throw runtime_error(msg.str());
	}
	err = SetPixelFormat(dch, iFormat, &pfd);
	if (!err) {
		std::stringstream msg;
		msg << "SetPixelFormat() failed.\n" << getLastErrorMsg();
		throw runtime_error(msg.str());
	}
}

void PlatformContextGl::init(int colorBits, int depthBits, int stencilBits){
	m_colorBits = colorBits;
	m_depthBits = depthBits;
	m_stencilBits = stencilBits;

	//Set8087CW($133F);
	dch = GetDC(GetActiveWindow());
	assert(dch!=NULL);

	setupDeviceContext();

	glch= wglCreateContext(dch);
	if(glch==NULL){
		std::stringstream msg;
		msg << "wglCreateContext() failed.\n" << getLastErrorMsg();
		throw runtime_error(msg.str());
	}
	makeCurrent();

	GLint glewErr = glewInit();
	if (glewErr != GLEW_OK) {
		throw runtime_error(string("Error initialising Glew: ") + (char*)glewGetErrorString(glewErr));
	}
}

}}//end namespace
