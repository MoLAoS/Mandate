// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//                2010-2011 Nathan Turner
//                2011      James McCulloch
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_SHADER_H_
#define _SHARED_GRAPHICS_SHADER_H_

#include <string>

#include "gl_wrap.h"
#include "xml_parser.h"

namespace Shared { namespace Graphics {

using namespace Shared::Xml;

// =====================================================
//	class ShaderProgram
// =====================================================

class ShaderProgram {
public:
	virtual ~ShaderProgram(){}

	virtual void begin() = 0;
	virtual void end() = 0;

	virtual unsigned int getId() = 0;
	virtual void setUniform(const string &name, GLuint value) = 0;
	virtual void setUniform(const string &name, const Vec3f &value) = 0;
	virtual int getAttribLoc(const string &name) = 0;
};

// =====================================================
//	class FixedPipeline
// =====================================================

class FixedPipeline : public ShaderProgram {
public:
	virtual void begin() override {}
	virtual void end() override {}

	unsigned int getId() override { return 0; }
	void setUniform(const string &name, GLuint value) override {}
	void setUniform(const string &name, const Vec3f &value) override {}
	int getAttribLoc(const string &name) override { return -1; }
};

// =====================================================
//	class GlslProgram
// =====================================================

class GlslProgram : public ShaderProgram {
private:
	GLuint m_v, m_f, m_p;
	string m_name;

	static void show_info_log(
		GLuint object,
		PFNGLGETSHADERIVPROC glGet__iv,
		PFNGLGETSHADERINFOLOGPROC glGet__InfoLog);

	string getCompileError(const string path, bool vert);
	string getLinkError(const string path);

public:
	GlslProgram(const string &name);
	~GlslProgram();

	void compileAndLink(const char *vertSource, const char *fragSource);

	void begin() override;
	void end() override;

	GLuint getId() override { return m_p; }
	void setUniform(const string &name, GLuint value) override;
	void setUniform(const string &name, const Vec3f &value) override;
	int getAttribLoc(const string &name) override;
};

// =====================================================
//	class UnitShaderSet
// =====================================================

class UnitShaderSet {
private:
	string           m_path;
	string	         m_name;
	ShaderProgram	*m_teamColour;
	ShaderProgram	*m_rgbaColour;

public:
	UnitShaderSet(const string &xmlPath);
	~UnitShaderSet();

	const string&  getName()        {return m_name;}

	ShaderProgram* getTeamProgram() {return m_teamColour;}
	ShaderProgram* getRgbaProgram() {return m_rgbaColour;}
};

typedef vector<UnitShaderSet*> UnitShaderSets;

}} // end namespace

#endif
