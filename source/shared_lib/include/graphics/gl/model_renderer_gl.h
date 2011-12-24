// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//	              2009-2011 James McCulloch
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_GL_MODELRENDERERGL_H_
#define _SHARED_GRAPHICS_GL_MODELRENDERERGL_H_

#include "model_renderer.h"
#include "model.h"
#include "opengl.h"
#include "shader.h"

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class ModelRendererGl
// =====================================================

class ModelRendererGl: public ModelRenderer {
private:
	bool    m_rendering;
	bool    m_duplicateTexCoords;
	//bool    m_shaderOverride;
	int     m_secondaryTexCoordUnit;
	GLuint  m_lastTexture;
	float   m_alphaThreshold;
	int     m_currentLightCount;

	RenderMode    m_renderMode;
	bool          m_useFog;
	MeshCallback *m_meshCallback;

	GlslPrograms   m_shaders;
	GlslProgram   *m_teamTintShader;
	
	GlslShader    *m_perVertexLighting;

	int	           m_shaderIndex; // index in m_shaders of UnitShaderSet we are currently using, or -1
	
	ShaderProgram	*m_lastShaderProgram;
	ShaderProgram	*m_fixedFunctionProgram;

	static const int diffuseTextureUnit = GL_TEXTURE0;
	static const int normalTextureUnit  = GL_TEXTURE3;
	static const int specularTextureUnt = GL_TEXTURE4;
	static const int TBD_TextureUnit    = GL_TEXTURE5;
	static const int customTextureUnit  = GL_TEXTURE6;

	GlslProgram* loadShader(const string &dir, const string &programName);

public:
	ModelRendererGl();
	~ModelRendererGl();

	//void loadShader(const string &programName);
	void loadShaders(const vector<string> &programNames);

	void setShader(const string &programName, bool gl3 = false);

	void deleteModelShader();

	void initUniformHandles(ShaderProgram *shader);

	ShaderProgram* getTeamTintShader();

	void cycleShaderSet();
	const string& getShaderName();
	bool isUsingShaders() const { return m_shaderIndex != -1; }

	virtual void setAlphaThreshold(float a) override;
	virtual void setLightCount(int n) override;
	virtual void setFogColour(const Vec3f &colour);
	virtual void setMainLight(const Vec3f &direction, const Vec3f &diffuse, const Vec3f &ambient);
	virtual void setPointLight(int i, const Vec3f &position, const Vec3f &colour, const Vec3f &spec, const Vec3f &attenuation);

	void begin(RenderMode mode, bool fog, MeshCallback *meshCallback = 0) override;
	void end() override;
	
	void render(const Model *model, float fade = 1.f, int frame = 0, int id = 0, ShaderProgram *customShaders = 0) override;
	void renderOutlined(const Model *model, int lineWidth, const Vec3f &colour, float fade = 1.f, int frame = 0, int id = 0, ShaderProgram *customShaders = 0) override;
	void renderNormalsOnly(const Model *model) override;
	void renderMeshNormalsOnly(const Mesh *mesh) override;

	void setDuplicateTexCoords(bool v)      { m_duplicateTexCoords = v;    }
	void setSecondaryTexCoordUnit(int v)    { m_secondaryTexCoordUnit = v; }

	void renderMeshNormals(const Mesh *mesh);
	void renderMesh(const Mesh *mesh, float fade = 1.f, int frame = 0, int id = 0, ShaderProgram *customShaders = 0) override;
	void renderMeshOutline(const Mesh *mesh);
};

}}}//end namespace

#endif
