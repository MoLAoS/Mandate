// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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

class ShaderSet {
private:
	const string	 m_name;
	ShaderProgram	*m_teamColour;
	ShaderProgram	*m_alphaColour;

public:
	ShaderSet(const string &name);
	~ShaderSet();
	bool load();

	const string&	getName() {return m_name;}
	ShaderProgram	*getTeamProgram() {return m_teamColour;}
	ShaderProgram	*getAlphaProgram() {return m_alphaColour;}
};

typedef vector<ShaderSet*> ShaderSets;

// =====================================================
//	class ModelRendererGl
// =====================================================

class ModelRendererGl: public ModelRenderer {
private:
	bool rendering;
	bool duplicateTexCoords;
	bool shaderOverride;
	int secondaryTexCoordUnit;
	GLuint lastTexture;

	ShaderSets	m_shaders;
	int			m_shaderIndex; // index in m_shaders of ShaderSet we a currently using, or -1
	
	ShaderProgram	*m_lastShaderProgram;
	ShaderProgram	*m_fixedFunctionProgram;

	static const int diffuseTextureUnit = GL_TEXTURE0;
	static const int normalTextureUnit = GL_TEXTURE2;
	static const int specularTextureUnt = GL_TEXTURE1;

public:
	ModelRendererGl();
	~ModelRendererGl();

	void loadShaders(const vector<string> &programNames);

	void cycleShaderSet();
	const string& getShaderName();
	bool isUsingShaders() const { return m_shaderIndex != -1; }

	virtual void begin(bool renderNormals, bool renderTextures, bool renderColors, MeshCallback *meshCallback);

	virtual void end();
	
	virtual void render(const Model *model) {
		//assertions
		assert(rendering);
		assertGl();
	
		//render every mesh
		for (uint32 i = 0; i < model->getMeshCount(); ++i) {
			renderMesh(model->getMesh(i));
		}
	
		//assertions
		assertGl();
	}
	
	virtual void renderNormalsOnly(const Model *model) {
		//assertions
		assert(rendering);
		assertGl();
	
		//render every mesh
		for (uint32 i = 0; i < model->getMeshCount(); ++i) {
			renderMeshNormals(model->getMesh(i));
		}
	
		//assertions
		assertGl();
	}

	virtual void renderMeshNormalsOnly(const Mesh *mesh) {
		renderMeshNormals(mesh);
	}
	//virtual void end();
	//virtual void render(const Model *model);
	//virtual void renderNormalsOnly(const Model *model);

	void setDuplicateTexCoords(bool duplicateTexCoords)			{this->duplicateTexCoords= duplicateTexCoords;}
	void setSecondaryTexCoordUnit(int secondaryTexCoordUnit)	{this->secondaryTexCoordUnit= secondaryTexCoordUnit;}

	void renderMeshNormals(const Mesh *mesh);
	void renderMesh(const Mesh *mesh);
};

}}}//end namespace

#endif
