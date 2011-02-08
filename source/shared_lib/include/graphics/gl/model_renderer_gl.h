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
	bool rendering;
	bool duplicateTexCoords;
	bool shaderOverride;
	int secondaryTexCoordUnit;
	GLuint lastTexture;

	UnitShaderSets   m_shaders;
	int	             m_shaderIndex; // index in m_shaders of UnitShaderSet we a currently using, or -1
	
	ShaderProgram	*m_lastShaderProgram;
	ShaderProgram	*m_fixedFunctionProgram;

	static const int diffuseTextureUnit = GL_TEXTURE0;
	static const int normalTextureUnit = GL_TEXTURE2;
	static const int specularTextureUnt = GL_TEXTURE3;
	static const int customTextureUnit = GL_TEXTURE4;

public:
	ModelRendererGl();
	~ModelRendererGl();

	void loadShaders(const vector<string> &programNames);
	UnitShaderSet* loadShaderSet(const string &xmlPath);
	void deleteShaders(UnitShaderSet *ss);

	void cycleShaderSet();
	const string& getShaderName();
	bool isUsingShaders() const { return m_shaderIndex != -1; }

	void begin(bool renderNormals, bool renderTextures, bool renderColors, MeshCallback *meshCallback) override;
	void end() override;
	
	void render(const Model *model, Vec3f *anim = 0, UnitShaderSet *customShaders = 0) override;
	void renderNormalsOnly(const Model *model) override;
	void renderMeshNormalsOnly(const Mesh *mesh) override;

	void setDuplicateTexCoords(bool duplicateTexCoords)			{this->duplicateTexCoords= duplicateTexCoords;}
	void setSecondaryTexCoordUnit(int secondaryTexCoordUnit)	{this->secondaryTexCoordUnit= secondaryTexCoordUnit;}

	void renderMeshNormals(const Mesh *mesh);
	void renderMesh(const Mesh *mesh, Vec3f *anim = 0, UnitShaderSet *customShaders = 0) override;
};

}}}//end namespace

#endif
