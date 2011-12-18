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
#include "opengl.h"

#include <stdexcept>

#include "graphics_interface.h"
#include "context_gl.h"
#include "gl_wrap.h"

#include "leak_dumper.h"

using namespace Shared::Platform;

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class Globals
// =====================================================

bool isGlExtensionSupported(const char *extensionName) {
    const char *s = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
	GLint len = strlen(extensionName);

	while ((s = strstr(s, extensionName)) != NULL) {
		s += len;
		if (*s == ' ' || *s == '\0') {
			return true;
		}
	}
	return false;
}

void getGlVersion(int &major, int &minor, int &release) {
	const char *strVersion = getGlVersion();
	char *tokString = new char[strlen(strVersion) + 1];
	strcpy(tokString, strVersion);
	major = atoi(strtok(tokString, "."));
	minor = atoi(strtok(0, "."));
	release = atoi(strtok(0, "."));
	delete [] tokString;
}

bool isGlVersionSupported(int major, int minor, int release) {
	int majorVersion, minorVersion, releaseVersion;
	getGlVersion(majorVersion, minorVersion, releaseVersion);

	// major
	if (majorVersion < major) {
		return false;
	} else if (majorVersion > major) {
		return true;
	}

	// minor
	if (minorVersion < minor) {
		return false;
	} else if (minorVersion > minor) {
		return true;
	}

	// release
	if (releaseVersion < release) {
		return false;
	}

	return true;
}

const char *getGlVersion(){
	return reinterpret_cast<const char *>(glGetString(GL_VERSION));
}

const char *getGlRenderer(){
	return reinterpret_cast<const char *>(glGetString(GL_RENDERER));
}

const char *getGlVendor(){
	return reinterpret_cast<const char *>(glGetString(GL_VENDOR));
}

const char *getGlExtensions(){
	return reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
}

const char *getGlPlatformExtensions(){
	Context *c= GraphicsInterface::getInstance().getCurrentContext();
	return getPlatformExtensions(static_cast<ContextGl*>(c)->getPlatformContextGl());
}

int getGlMaxLights(){
	int i;
	glGetIntegerv(GL_MAX_LIGHTS, &i);
	return i;
}

int getGlMaxTextureSize(){
	int i;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &i);
	return i;
}

int getGlMaxTextureUnits(){
	int i;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &i);
	return i;
}

int getGlMaxShaderTexUnits() {
	int i;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB, &i);
	return i;
}

int getGlModelviewMatrixStackDepth(){
	int i;
	glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &i);
	return i;
}

int getGlProjectionMatrixStackDepth(){
	int i;
	glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &i);
	return i;
}

void checkGlExtension(const char *extensionName){
	if(!isGlExtensionSupported(extensionName)){
		throw runtime_error("OpenGL extension not supported: " + string(extensionName));
	}
}

}}}// end namespace
