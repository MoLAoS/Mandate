// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//                     2010 Nathan Turner      
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_SHADER_H_
#define _SHARED_GRAPHICS_SHADER_H_

#include "gl_wrap.h"

/*#include "vec.h"
#include "matrix.h"
#include "texture.h"
#include "leak_dumper.h"
*/
namespace Shared{ namespace Graphics{


// =====================================================
//	class ShaderProgram
// =====================================================

/*class VertexShader;
class FragmentShader;

class ShaderProgram{
public:
	virtual ~ShaderProgram(){}
	virtual void init()= 0;
	virtual void end()= 0;

	virtual void attach(VertexShader *vs, FragmentShader *fs)= 0;
	virtual bool link(string &messages)= 0;
	virtual void activate()= 0;
	virtual void deactivate()= 0;

	virtual void setUniform(const string &name, int value)= 0;
	virtual void setUniform(const string &name, float value)= 0;
	virtual void setUniform(const string &name, const Vec2f &value)= 0;
	virtual void setUniform(const string &name, const Vec3f &value)= 0;
	virtual void setUniform(const string &name, const Vec4f &value)= 0;
	virtual void setUniform(const string &name, const Matrix3f &value)= 0;
	virtual void setUniform(const string &name, const Matrix4f &value)= 0;
};

// =====================================================
//	class Shader
// =====================================================

class Shader{
public:
	virtual ~Shader(){}
	virtual void init()= 0;
	virtual void end()= 0;

	virtual void load(const string &path)= 0;
	virtual bool compile(string &messages)= 0;
};

class VertexShader: virtual public Shader{
};

class FragmentShader: virtual public Shader{
};

// =====================================================
//	class ShaderSource
// =====================================================
*/
#include <string>

class ShaderSource {
private:
	std::string pathInfo;
	char *code;

public:
	ShaderSource(){ code = NULL; }
	~ShaderSource(){ if(code) delete[] code; }
	
	const std::string &getPathInfo() const	{return pathInfo;}
	const char *getCode() const		{return code;}

	void load(const std::string &path);
};

class ShaderProgram {
public:
	virtual ~ShaderProgram(){}

	virtual void load(const std::string &vertex, const std::string &fragment) = 0;
	virtual void begin() = 0;
	virtual void end() = 0;

	virtual unsigned int getId() = 0;
	virtual void setUniform(const std::string &name, GLuint value) = 0;
	virtual void setUniform(const std::string &name, const Vec3f &value) = 0;
};

class FixedFunctionProgram : public ShaderProgram {
public:
	virtual void load(const std::string &vertex, const std::string &fragment) {}
	virtual void begin() {}
	virtual void end() {}

	unsigned int getId() { return 0; }
	virtual void setUniform(const std::string &name, GLuint value) {}
	virtual void setUniform(const std::string &name, const Vec3f &value) {}
};

class DefaultShaderProgram : public ShaderProgram {
private:
	GLuint m_v,m_f,m_p;
	ShaderSource m_vertexShader, m_fragmentShader;

	static void show_info_log(
		GLuint object,
		PFNGLGETSHADERIVPROC glGet__iv,
		PFNGLGETSHADERINFOLOGPROC glGet__InfoLog);

	string getError(const string path, bool vert);

public:
	DefaultShaderProgram();
	~DefaultShaderProgram();
	void load(const std::string &vertex, const std::string &fragment);
	void begin();
	void end();

	unsigned int getId() { return m_p; }
	virtual void setUniform(const std::string &name, GLuint value);
	virtual void setUniform(const std::string &name, const Vec3f &value);
};

}}//end namespace

#endif
