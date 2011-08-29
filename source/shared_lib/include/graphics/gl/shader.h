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
#include "util.h"

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
	virtual bool addUniform(const string &name) = 0;
	virtual bool setUniform(const string &name, GLuint value) = 0;
	virtual bool setUniform(const string &name, GLint value) = 0;
	virtual bool setUniform(const string &name, GLfloat value) = 0;
	virtual bool setUniform(const string &name, const Vec3f &value) = 0;
	virtual bool setUniform(int handle, GLuint value) = 0;
	virtual bool setUniform(int handle, GLint value) = 0;
	virtual bool setUniform(int handle, GLfloat value) = 0;
	virtual bool setUniform(int handle, const Vec3f &value) = 0;
	virtual int getUniformLoc(const string &name) = 0;
	virtual int getAttribLoc(const string &name) = 0;
};

// =====================================================
//	class FixedPipeline
// =====================================================

class FixedPipeline : public ShaderProgram {
public:
	virtual void begin() override {}
	virtual void end() override {}

	virtual unsigned int getId() override { return 0; }
	virtual bool addUniform(const string &name) override {return false;}
	virtual bool setUniform(const string &name, GLuint value) override {return false;}
	virtual bool setUniform(const string &name, GLint value) override {return false;}
	virtual bool setUniform(const string &name, GLfloat value) override {return false;}
	virtual bool setUniform(const string &name, const Vec3f &value) override {return false;}
	virtual bool setUniform(int handle, GLuint value) override {return false;}
	virtual bool setUniform(int handle, GLint value) override {return false;}
	virtual bool setUniform(int handle, GLfloat value) override {return false;}
	virtual bool setUniform(int handle, const Vec3f &value) override {return false;}
	virtual int  getUniformLoc(const string &name) override { return -1; }
	virtual int  getAttribLoc(const string &name) override { return -1; }
};


WRAPPED_ENUM( ShaderType,
	VERTEX,
	FRAGMENT
);

// Shader
class GlslShader {
protected:
	GLuint      m_handle;
	string      m_path;
	string      m_name;
	ShaderType  m_type;
	string      m_log;	

public:
	GlslShader();

	int getHandle() const { return m_handle; }
	const string& getPath() const { return m_path; }
	const string& getLog() const { return m_log; }

	bool load(const string &path, ShaderType st);
};

// =====================================================
//	class GlslProgram
// =====================================================

typedef std::vector<GlslShader*> ShaderList;
typedef std::map<std::string, int> UniformHandles;

class GlslProgram : public ShaderProgram {
private:
	UniformHandles m_uniformHandles;
	ShaderList m_shaders;
	GLuint     m_handle;
	string     m_dir;
	string     m_name;
	string     m_log;

public:
	GlslProgram(const string &name);
	~GlslProgram();

	int getHandle() const { return m_handle; }
	const string& getName() const { return m_name; }
	const string& getLog() const { return m_log; }

	bool load(const string &path, bool vertex);
	void add(GlslShader *shader);
	bool link();

	virtual void begin() override;
	virtual void end() override;

	virtual unsigned int getId() override { return m_handle; }
	virtual bool addUniform(const string &name) override;
	virtual bool setUniform(const string &name, GLuint value) override;
	virtual bool setUniform(const string &name, GLint value) override;
	virtual bool setUniform(const string &name, GLfloat value) override;
	virtual bool setUniform(const string &name, const Vec3f &value) override;
	virtual bool setUniform(int handle, GLuint value) override;
	virtual bool setUniform(int handle, GLint value) override;
	virtual bool setUniform(int handle, GLfloat value) override;
	virtual bool setUniform(int handle, const Vec3f &value) override;
	virtual int  getUniformLoc(const string &name) override;
	virtual int  getAttribLoc(const string &name) override;
};

typedef std::vector<GlslProgram*> GlslPrograms;

}} // end namespace

#endif
