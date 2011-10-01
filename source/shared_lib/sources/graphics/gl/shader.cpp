// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//                2010-2011 Nathan Turner
//                2011      James McCulloch
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "opengl.h"
#include "shader.h"
#include "util.h"
#include "FSFactory.hpp"

#include <stdexcept>
#include <fstream>

//#include "leak_dumper.h"

using namespace std;
using namespace Shared::PhysFS;

namespace Shared{ namespace Graphics{

using namespace Gl;

// =====================================================
//	function loadSourceFromFile
// =====================================================

char* loadSourceFromFile(const string &path) {
	char *code = 0;
	// load the whole file into memory
	FileOps *f = FSFactory::getInstance()->getFileOps();
	try {
		f->openRead(path.c_str());
		int length = f->fileSize();
		code = new char[length + 1];
		f->read(code, length, 1);
		code[length] = '\0'; // null terminate
	} catch (runtime_error &e) {
		mediaErrorLog.add(e.what(), path);
		delete [] code;
		code = 0;
	}
	delete f;
	return code;
}

inline void trimTrailingNewlines(string &str) {
	while (!str.empty() && *(str.end() - 1) == '\n') {
		str.erase(str.end() - 1);
	}
}

void initUnitShader(ShaderProgram *program) {
	assertGl();
	program->begin();
	program->setUniform( program->getUniformLoc("gae_DiffuseTex"), 0u );
	program->setUniform( program->getUniformLoc("gae_NormalMap"),  3u );
	program->setUniform( program->getUniformLoc("gae_SpecMap"),    4u );
	program->setUniform( program->getUniformLoc("gae_LightMap"),   5u );
	program->setUniform( program->getUniformLoc("gae_CustomTex"),  6u );
	program->end();
	assertGl();
}



// GlslShader

GlslShader::GlslShader() {
	m_handle = 0;
} 

bool GlslShader::load(const string &path, ShaderType st) {
	m_path = cleanPath(path);
	m_name = cutLastExt(basename(m_path));
	m_type = st;
	
	const char *src = loadSourceFromFile(m_path);
	if (src) {
		m_handle = glCreateShader(st == ShaderType::VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
		glShaderSource(m_handle, 1, &src, NULL);

		glCompileShader(m_handle);

		GLint ok;
		glGetShaderiv(m_handle, GL_COMPILE_STATUS, &ok);

		GLint log_length = 0;
		glGetShaderiv(m_handle, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length) {
			GLchar *buffer = new GLchar[log_length];
			glGetShaderInfoLog(m_handle, log_length, 0, buffer);
			m_log = string(buffer);
			trimTrailingNewlines(m_log);
			delete [] buffer;
		}
		delete [] src;
		return ok == GL_TRUE;
	}
	return false;
}

// =====================================================
//	class GlslProgram
// =====================================================

GlslProgram::GlslProgram(const string &name) 
		: m_name(name) {
	m_handle = glCreateProgram();
}

///@todo manage individual shaders somewhere...

GlslProgram::~GlslProgram() {
	foreach (ShaderList, it, m_shaders) {
		glDetachShader(m_handle, (*it)->getHandle());
		glDeleteShader((*it)->getHandle());
	}
	glDeleteProgram(m_handle);
}

static string lastShaderError;

bool GlslProgram::load(const string &path, bool vertex) {
	GlslShader *shader = new GlslShader();
	if (shader->load(path, vertex ? ShaderType::VERTEX : ShaderType::FRAGMENT)) {
		m_shaders.push_back(shader);
		return true;
	} else {
		m_log = shader->getLog();
		delete shader;
		return false;
	}
}

void GlslProgram::add(GlslShader *shader) {
	m_shaders.push_back(shader);
}

bool GlslProgram::link() {
	foreach (ShaderList, it, m_shaders) {
		glAttachShader(m_handle, (*it)->getHandle());
	}
	glLinkProgram(m_handle);
	GLint ok;
	glGetProgramiv(m_handle, GL_LINK_STATUS, &ok);

	GLint log_length;
	glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length) {
		GLchar *buffer = new GLchar[log_length];
		glGetProgramInfoLog(m_handle, log_length, 0, buffer);
		m_log = buffer;
		delete [] buffer;
		trimTrailingNewlines(m_log);
	}
	if (ok == GL_TRUE) {
		initUnitShader(this);
		return true;
	}
	return false;
}

void GlslProgram::begin() {
	glUseProgram(m_handle);
	assertGl();
}

void GlslProgram::end() {
	glUseProgram(0);
	assertGl();
}

bool GlslProgram::addUniform(const string &name) {
	int handle = getUniformLoc(name);
	m_uniformHandles.insert(std::pair<string, int>(name, handle));
	
	return handle != -1;
}

bool GlslProgram::setUniform(const string &name, GLuint value) {
	return setUniform(m_uniformHandles[name], value);
}

bool GlslProgram::setUniform(const string &name, GLint value) {
	return setUniform(m_uniformHandles[name], value); 
}

bool GlslProgram::setUniform(const string &name, GLfloat value) {
	return setUniform(m_uniformHandles[name], value);
}

bool GlslProgram::setUniform(const string &name, const Vec3f &value) {
	return setUniform(m_uniformHandles[name], value);
}

///

bool GlslProgram::setUniform(int handle, GLuint value) {
	return setUniform(handle, GLint(value));
}

bool GlslProgram::setUniform(int handle, GLint value) {
	if (handle != -1) {
		glUniform1i(handle, value);
		return true;
	}
	return false;
}

bool GlslProgram::setUniform(int handle, GLfloat value) {
	if (handle != -1) {
		glUniform1f(handle, value);
		return true;
	}
	return false;
}

bool GlslProgram::setUniform(int handle, const Vec3f &value) {
	if (handle != -1) {
		glUniform3fv(handle, 1, value.ptr());
		return true;
	}
	return false;
}

int GlslProgram::getUniformLoc(const string &name) {
	return glGetUniformLocation(m_handle, name.c_str());
}

int GlslProgram::getAttribLoc(const string &name) {
	return glGetAttribLocation(m_handle, name.c_str());
}

}}//end namespace
