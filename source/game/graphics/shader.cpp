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
#include "shader.h"

#include <stdexcept>
#include <fstream>

//#include "leak_dumper.h"

using namespace std;

namespace Shared{ namespace Graphics{

// =====================================================
//	class ShaderSource
// =====================================================

void ShaderSource::load(const string &path){
	pathInfo+= path + " ";

	//open file
	ifstream ifs(path.c_str());
	if(ifs.fail()){
		throw runtime_error("Can't open shader file: " + path);
	}

	//read source
	while(true){
		fstream::int_type c= ifs.get();
		if(ifs.eof() || ifs.fail() || ifs.bad()){
			break;
		}
		code+= c;
	}
}

DefaultShaderProgram::DefaultShaderProgram() {
	m_p = glCreateProgram();
}

DefaultShaderProgram::~DefaultShaderProgram() {
	glDetachShader(m_p, m_v);
	glDetachShader(m_p, m_f);

	glDeleteShader(m_v);
	glDeleteShader(m_f);
	glDeleteProgram(m_p);
}

void DefaultShaderProgram::load(const string &vertex, const string &fragment) {
	m_v = glCreateShader(GL_VERTEX_SHADER);
	m_f = glCreateShader(GL_FRAGMENT_SHADER);

	m_vertexShader.load(vertex);
	m_fragmentShader.load(fragment);

	const char *vs = NULL,*fs = NULL;
	vs = m_vertexShader.getCode().c_str();
	fs = m_fragmentShader.getCode().c_str();
	
	glShaderSource(m_v, 1, &vs,NULL);
	glShaderSource(m_f, 1, &fs,NULL);

	glCompileShader(m_v);
	glCompileShader(m_f);

	glAttachShader(m_p,m_f);
	glAttachShader(m_p,m_v);

	glLinkProgram(m_p);
}

void DefaultShaderProgram::begin() {
	glUseProgram(m_p);
}

void DefaultShaderProgram::end() {
	glUseProgram(0);
}

void DefaultShaderProgram::setUniform(const std::string &name, GLuint value) {
	int texture_location = glGetUniformLocation(m_p, name.c_str());
	glUniform1i(texture_location, value);
}

}}//end namespace
