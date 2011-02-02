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
#include "shader.h"
#include "util.h"
#include "FSFactory.hpp"

#include <stdexcept>
#include <fstream>

//#include "leak_dumper.h"

using namespace std;
using namespace Shared::PhysFS;

namespace Shared{ namespace Graphics{

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

void GlslProgram::show_info_log(
    GLuint object,
    PFNGLGETSHADERIVPROC glGet__iv,
    PFNGLGETSHADERINFOLOGPROC glGet__InfoLog)
{
	GLint log_length;
	char *log;

	glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
	log = new char[log_length];
	glGet__InfoLog(object, log_length, NULL, log);
	cerr << log << endl;
	lastShaderError = log;
	delete[] log;
}

string GlslProgram::getCompileError(const string path, bool vert) {
	show_info_log(vert ? m_v : m_f, glGetShaderiv, glGetShaderInfoLog);
	string res = string("Compile error in ") + (vert ? "vertex" : "fragment");
	res +=  " shader: " + path + "\n" + lastShaderError;
	return res;
}

string GlslProgram::getLinkError(const string name) {
	show_info_log(m_p, glGetProgramiv, glGetProgramInfoLog);
	string res = string("Link error with shader: ") + name + "\n" + lastShaderError;
	return res;
}

void GlslProgram::compileAndLink(const char *vs, const char *fs) {
	m_v = glCreateShader(GL_VERTEX_SHADER);
	m_f = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(m_v, 1, &vs,NULL);
	glShaderSource(m_f, 1, &fs,NULL);

	GLint ok;
	glCompileShader(m_v);
	glGetShaderiv(m_v, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		string msg = getCompileError(m_name, true);
		glDeleteShader(m_v);
		throw runtime_error(msg);
	}

	glCompileShader(m_f);
	glGetShaderiv(m_f, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		string msg = getCompileError(m_name, false);
		glDeleteShader(m_f);
		throw runtime_error(msg);
	}

	glAttachShader(m_p, m_f);
	glAttachShader(m_p, m_v);

	glLinkProgram(m_p);
	glGetProgramiv(m_p, GL_LINK_STATUS, &ok);
	if (!ok) {
		string msg = getLinkError(m_name);
		glDeleteProgram(m_p);
		throw runtime_error(msg);
	}
}

void GlslProgram::begin() {
	glUseProgram(m_p);
}

void GlslProgram::end() {
	glUseProgram(0);
}

void GlslProgram::setUniform(const string &name, GLuint value) {
	int texture_location = glGetUniformLocation(m_p, name.c_str());
	glUniform1i(texture_location, value);
}

void GlslProgram::setUniform(const string &name, const Vec3f &value) {
	int uniform_loc = glGetUniformLocation(m_p, name.c_str());
	glUniform3fv(uniform_loc, 1, value.ptr());
}

int GlslProgram::getAttribLoc(const string &name) {
	return glGetAttribLocation(m_p, name.c_str());
}

// =====================================================
//	class UnitShaderSet
// =====================================================

void initUnitShader(ShaderProgram *program) {
	program->begin();
	program->setUniform("baseTexture", 0);
	//program->setUniform("shadowMap", 1);
	program->setUniform("normalMap", 2);
	program->setUniform("specMap", 3);
	program->setUniform("customTex", 4);
	program->setUniform("customTex2", 5);
	program->end();
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
}

UnitShaderSet::~UnitShaderSet() {
	delete m_teamColour;
	delete m_rgbaColour;
}

}}//end namespace
