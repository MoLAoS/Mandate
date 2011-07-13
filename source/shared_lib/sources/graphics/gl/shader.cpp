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
	} catch (exception &e) {
		mediaErrorLog.add(e.what(), path);
		delete [] code;
		code = 0;
	}
	delete f;
	return code;
}

// =====================================================
//	class GlslProgram
// =====================================================

GlslProgram::GlslProgram(const string &name) 
		: m_name(name) {
	m_p = glCreateProgram();
}

GlslProgram::~GlslProgram() {
	glDetachShader(m_p, m_v);
	glDetachShader(m_p, m_f);

	glDeleteShader(m_v);
	glDeleteShader(m_f);
	glDeleteProgram(m_p);
}

static string lastShaderError;

inline void trimTrailingNewlines(string &str) {
	while (!str.empty() && *(str.end() - 1) == '\n') {
		str.erase(str.end() - 1);
	}
}

bool GlslProgram::compileShader(GLuint handle, const char *src, string &out_log) {
	glShaderSource(handle, 1, &src, NULL);
	GLint ok;
	glCompileShader(handle);
	glGetShaderiv(handle, GL_COMPILE_STATUS, &ok);

	GLint log_length = 0;
	glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length) {
		GLchar *buffer = new GLchar[log_length];
		glGetShaderInfoLog(handle, log_length, 0, buffer);
		out_log = buffer;
		trimTrailingNewlines(out_log);
		delete [] buffer;
	}
	return ok == GL_TRUE;
}

bool GlslProgram::compileVertexShader(const char *src) {
	string log, res;
	res = "Vertex shader ";
	bool retVal = compileShader(m_v, src, log);
	if (retVal) {
		res += "compiled OK.\n";
	} else {
		res += "compile failed.\n";
		lastShaderError = log;
	}
	if (!log.empty()) {
		res += "Compile log:\n" + log + "\n";
	} else {
		res += "\n";
	}
	cout << res;
	return retVal;
}

bool GlslProgram::compileFragmentShader(const char *src) {
	string log, res;
	bool retVal = compileShader(m_f, src, log);
	res = "Fragment shader ";
	if (retVal) {
		res += "compiled OK.\n";
	} else {
		res += "compile failed.\n";
		lastShaderError = log;
	}
	if (!log.empty()) {
		res += "Compile log:\n" + log + "\n";
	} else {
		res += "\n";
	}
	cout << res;
	return retVal;
}

bool GlslProgram::linkShaderProgram() {
	glAttachShader(m_p, m_f);
	glAttachShader(m_p, m_v);
	glLinkProgram(m_p);
	GLint ok;
	glGetProgramiv(m_p, GL_LINK_STATUS, &ok);
	string res = "Program ";
	res += (ok == GL_TRUE ? "linked Ok.\n" : "link failed.\n");

	GLint log_length;
	glGetProgramiv(m_p, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length) {
		GLchar *buffer = new GLchar[log_length];
		glGetProgramInfoLog(m_p, log_length, 0, buffer);
		string log = buffer;
		delete [] buffer;
		trimTrailingNewlines(log);
		res += string("Link log:\n") + log + "\n";
		if (!ok) {
			lastShaderError = buffer;
		}
	} else {
		res += "\n";
	}
	cout << res;
	return ok == GL_TRUE;
}

void GlslProgram::compileAndLink(const char *vs, const char *fs) {
	string header = "GLSL Program: " + m_name;
	cout << header << endl;
	for (int i=0; i < header.size(); ++i) {
		cout << "-";
	}
	cout << endl;
	cout.flush();

	m_v = glCreateShader(GL_VERTEX_SHADER);
	m_f = glCreateShader(GL_FRAGMENT_SHADER);

	if (!compileVertexShader(vs) || !compileFragmentShader(fs)) {
		throw runtime_error(lastShaderError);
	}
	if (!linkShaderProgram()) {
		throw runtime_error(lastShaderError);
	}
}

void GlslProgram::begin() {
	glUseProgram(m_p);
	assertGl();
}

void GlslProgram::end() {
	glUseProgram(0);
	assertGl();
}

bool GlslProgram::setUniform(const string &name, GLuint value) {
	return setUniform(name, GLint(value));
}

bool GlslProgram::setUniform(const string &name, GLint value) {
	int handle = glGetUniformLocation(m_p, name.c_str());
	if (handle != -1) {
		glUniform1i(handle, value);
		return true;
	}
	return false;
}

bool GlslProgram::setUniform(const string &name, GLfloat value) {
	int handle = glGetUniformLocation(m_p, name.c_str());
	if (handle != -1) {
		glUniform1f(handle, value);
		return true;
	}
	return false;
}

bool GlslProgram::setUniform(const string &name, const Vec3f &value) {
	int handle = glGetUniformLocation(m_p, name.c_str());
	if (handle != -1) {
		glUniform3fv(handle, 1, value.ptr());
		return true;
	}
	return false;
}

int GlslProgram::getAttribLoc(const string &name) {
	return glGetAttribLocation(m_p, name.c_str());
}

// =====================================================
//	class UnitShaderSet
// =====================================================

void initUnitShader(ShaderProgram *program) {
	assertGl();
	program->begin();
	program->setUniform( "gae_DiffuseTex", 0u );
	program->setUniform( "gae_NormalMap",  3u );
	program->setUniform( "gae_SpecMap",    4u );
	program->setUniform( "gae_LightMap",   5u );
	program->setUniform( "gae_CustomTex",  6u );
	program->end();
	assertGl();
}

UnitShaderSet::UnitShaderSet(const string &xmlPath)
		: m_path(xmlPath)
		, m_name()
		, m_teamColour(0)
		, m_rgbaColour(0) {
	const XmlNode *node = XmlIo::getInstance().load(xmlPath);
	m_name = node->getAttribute("name")->getRestrictedValue();
	string progHeader = "#version " + intToStr(node->getChild("glsl-version")->getIntValue())
		+ "\n" + node->getChild("variables")->getText() + "\n\n";

	string vertexShader = node->getChild("vertex-shader")->getText();
	string teamFragShader = node->getChild("team-fragment-shader")->getText();
	string rgbaFragShader = node->getChild("rgba-fragment-shader")->getText();

	delete node;

	string vss = progHeader + vertexShader;
	string team_fss = progHeader + teamFragShader;
	string rgba_fss = progHeader + rgbaFragShader;

	try {
		GlslProgram *p = new GlslProgram(m_name + "-team");
		p->compileAndLink(vss.c_str(), team_fss.c_str());
		m_teamColour = p;
		p = new GlslProgram(m_name + "-rgba");
		p->compileAndLink(vss.c_str(), rgba_fss.c_str());
		m_rgbaColour = p;
		initUnitShader(m_teamColour);
		initUnitShader(m_rgbaColour);
	} catch (runtime_error &e) {
		delete m_teamColour;
		delete m_rgbaColour;
		m_teamColour = m_rgbaColour = 0;
		throw e;
	}
	assertGl();
}

UnitShaderSet::~UnitShaderSet() {
	delete m_teamColour;
	delete m_rgbaColour;
}

}}//end namespace
