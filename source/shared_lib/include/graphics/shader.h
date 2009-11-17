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

#ifndef _SHARED_GRAPHICS_SHADER_H_
#define _SHARED_GRAPHICS_SHADER_H_

#include "vec.h"
#include "matrix.h"
#include "texture.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class ShaderProgram
// =====================================================

class VertexShader;
class FragmentShader;

class ShaderProgram{
public:
	virtual ~ShaderProgram(){}
	virtual void init() PURE_VIRTUAL;
	virtual void end() PURE_VIRTUAL;

	virtual void attach(VertexShader *vs, FragmentShader *fs) PURE_VIRTUAL;
	virtual bool link(string &messages) PURE_VIRTUAL;
	virtual void activate() PURE_VIRTUAL;
	virtual void deactivate() PURE_VIRTUAL;

	virtual void setUniform(const string &name, int value) PURE_VIRTUAL;
	virtual void setUniform(const string &name, float value) PURE_VIRTUAL;
	virtual void setUniform(const string &name, const Vec2f &value) PURE_VIRTUAL;
	virtual void setUniform(const string &name, const Vec3f &value) PURE_VIRTUAL;
	virtual void setUniform(const string &name, const Vec4f &value) PURE_VIRTUAL;
	virtual void setUniform(const string &name, const Matrix3f &value) PURE_VIRTUAL;
	virtual void setUniform(const string &name, const Matrix4f &value) PURE_VIRTUAL;
};

// =====================================================
//	class Shader
// =====================================================

class Shader{
public:
	virtual ~Shader(){}
	virtual void init() PURE_VIRTUAL;
	virtual void end() PURE_VIRTUAL;

	virtual void load(const string &path) PURE_VIRTUAL;
	virtual bool compile(string &messages) PURE_VIRTUAL;
};

class VertexShader: virtual public Shader{
};

class FragmentShader: virtual public Shader{
};

// =====================================================
//	class ShaderSource
// =====================================================

class ShaderSource{
private:
	string pathInfo;
	string code;

public:
	const string &getPathInfo() const	{return pathInfo;}
	const string &getCode() const		{return code;}

	void load(const string &path);
};

}}//end namespace

#endif
