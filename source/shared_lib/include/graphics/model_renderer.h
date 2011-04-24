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

WRAPPED_ENUM( RenderMode, WIREFRAME, OBJECTS, UNITS, SHADOWS, SELECTION );
//
//struct RenderUnitsParams {
//	bool          useFog;
//	MeshCallback *meshCallback;
//}
//
//struct RenderObjectsParams {
//	bool          useFog;
//}
//
//struct RenderParams {
//	bool          textures;
//	bool          normals;
//	bool          colours;
//	bool          useFog;
//	bool          shadows;	 // rendering shadows, disables shaders
//	bool          selection; // rendering for selection, disable shaders and don't render 'no-select' meshes 
//	MeshCallback *meshCallback;
//
//	RenderParams() : meshCallback(0) {}
//
//	RenderParams(bool fog, MeshCallback *meshCallback) 
//		: textures(true), normals(true), colours(true), useFog(fog), 
//
//	RenderParams(bool textures, bool normals, bool colours, bool fog, bool fast = false) 
//		: textures(textures)
//		, normals(normals)
//		, colours(colours)
//		, useFog(fog)
//		, fast(fast)
//		, meshCallback(0) {
//	}
//
//	RenderParams(RenderMode mode) {
//	}
//
//	void setMeshCallback(MeshCallback *callback) {
//		this->meshCallback = callback;
//	}
//};

// =====================================================
//	class ModelRenderer
// =====================================================

class ModelRenderer {
protected:
	Vec3f teamColour;

public:
	ModelRenderer() {}

	virtual ~ModelRenderer(){};

	void setTeamColour(const Vec3f &colour) { teamColour = colour; }
	const Vec3f& getTeamColour() const {return teamColour; }

	virtual void setAlphaThreshold(float a) = 0;
	virtual void setLightCount(int n) = 0;

	virtual void begin(RenderMode mode, bool fog, MeshCallback *meshCallback = 0) = 0;
	virtual void end() = 0;
	virtual void render(const Model *model, int frame = 0, int id = 0, UnitShaderSet *shaderSet = 0) = 0;
	virtual void renderNormalsOnly(const Model *model) = 0;
	virtual void renderMesh(const Mesh *mesh, int frame = 0, int id = 0, UnitShaderSet *shaderSet = 0) = 0;
	virtual void renderMeshNormalsOnly(const Mesh *mesh) = 0;
};

}}//end namespace

#endif
