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

namespace Shared { namespace Graphics {

class Texture;

// =====================================================
//	class MeshCallback
//
/// This gets called before rendering mesh
// =====================================================

class MeshCallback {
public:
	virtual ~MeshCallback() {}
	virtual void execute(const Mesh *mesh) = 0;
};

WRAPPED_ENUM( RenderMode, 
	WIREFRAME,
	SELECTION,
	OBJECTS,
	UNITS,
	SHADOWS
);

// =====================================================
//	class ModelRenderer
// =====================================================

class ModelRenderer {
protected:
	Vec3f teamColour;

public:
	ModelRenderer() {}

	virtual ~ModelRenderer() {}

	void setTeamColour(const Vec3f &colour) { teamColour = colour; }
	const Vec3f& getTeamColour() const {return teamColour; }

	virtual void setAlphaThreshold(float a) = 0;
	virtual void setLightCount(int n) = 0;
	virtual void setFogColour(const Vec3f &colour) = 0;
	virtual void setMainLight(const Vec3f &direction, const Vec3f &diffuse, const Vec3f &ambient) = 0;
	virtual void setPointLight(int i, const Vec3f &position, const Vec3f &colour, const Vec3f &spec, const Vec3f &attenuation) = 0;

	virtual void begin(RenderMode mode, bool fog, MeshCallback *meshCallback = 0) = 0;
	virtual void end() = 0;
	virtual void render(const Model *model, float fade = 1.f, int frame = 0, int id = 0, ShaderProgram *shaderProgram = 0) = 0;
	virtual void renderOutlined(const Model *model, int lineWidth, const Vec3f &colour, float fade = 1.f, int frame = 0, int id = 0, ShaderProgram *shaderProgram = 0) = 0;
	virtual void renderNormalsOnly(const Model *model) = 0;
	virtual void renderMesh(const Mesh *mesh, float fade = 1.f, int frame = 0, int id = 0, ShaderProgram *shaderProgram = 0) = 0;
	virtual void renderMeshNormalsOnly(const Mesh *mesh) = 0;
};

}} // end namespace

#endif
