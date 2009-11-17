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
	virtual void execute(const Mesh *mesh) PURE_VIRTUAL;
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

public:
	ModelRenderer() : meshCallback(NULL) {}

	virtual ~ModelRenderer(){};

	virtual void begin(bool renderNormals, bool renderTextures, bool renderColors, MeshCallback *meshCallback= NULL) PURE_VIRTUAL;
	virtual void end() PURE_VIRTUAL;
	virtual void render(const Model *model) PURE_VIRTUAL;
	virtual void renderNormalsOnly(const Model *model) PURE_VIRTUAL;
};

}}//end namespace

#endif
