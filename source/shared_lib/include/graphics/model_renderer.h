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


#ifndef _SHARED_GRAPHICS_MODELRENDERER_H_
#define _SHARED_GRAPHICS_MODELRENDERER_H_

#include "model.h"
#include "shader.h"

namespace Shared{ namespace Graphics{

class Texture;

// =====================================================
//	class MeshCallback
//
/// This gets called before rendering mesh
// =====================================================

class MeshCallback{
public:
	virtual ~MeshCallback(){};
	virtual void execute(const Mesh *mesh)= 0;
};

// =====================================================
//	class ModelRenderer
// =====================================================

class ModelRenderer {
protected:
	bool renderNormals;
	bool renderTextures;
	bool renderColors;
	MeshCallback *meshCallback;
	Vec3f	teamColour;

public:
	ModelRenderer() : teamColour(0.f), meshCallback(NULL) {}

	virtual ~ModelRenderer(){};

	void setTeamColour(const Vec3f &colour) { teamColour = colour; }
	const Vec3f& getTeamColour() const {return teamColour; }

	virtual void begin(bool renderNormals, bool renderTextures, bool renderColors, MeshCallback *meshCallback= NULL) = 0;
	virtual void end() = 0;
	virtual void render(const Model *model, Vec3f *anim = 0, ShaderProgram *customProgram = 0) = 0;
	virtual void renderNormalsOnly(const Model *model) = 0;
	virtual void renderMesh(const Mesh *mesh, Vec3f *anim = 0, ShaderProgram *customProgram = 0) = 0;
	virtual void renderMeshNormalsOnly(const Mesh *mesh) = 0;
};

}}//end namespace

#endif
