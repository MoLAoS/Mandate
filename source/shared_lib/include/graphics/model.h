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

#ifndef _SHARED_GRAPHICS_MODEL_H_
#define _SHARED_GRAPHICS_MODEL_H_

#include <string>
#include <map>

#include "gl_wrap.h"
#include "types.h"
#include "pixmap.h"
#include "texture_manager.h"
#include "texture.h"
#include "model_header.h"
#include "FileOps.hpp"

using namespace Shared::Math;

namespace Shared { namespace Graphics {

class Model;
class Mesh;
class ShadowVolumeData;
class InterpolationData;
class TextureManager;

WRAPPED_ENUM( LerpMethod, x87, SIMD, GLSL );

extern LerpMethod meshLerpMethod;

extern bool use_vbos;

Vec3f* allocate_aligned_vec3_array(unsigned n);
void free_aligned_vec3_array(Vec3f *ptr);

// =====================================================
// class Mesh
//
// Part of a 3D model
// =====================================================

class Mesh {
private:
	// mesh data
	Texture2D *textures[meshTextureCount];
	string texturePaths[meshTextureCount];

	// vertex data counts
	uint32 frameCount;
	uint32 vertexCount;
	uint32 indexCount;

	// vertex data
	Vec3f **vertArrays; // if using SIMD interpolation
	Vec3f **normArrays;

	Vec3f *vertices;	// if using x87 interpolation
	Vec3f *normals;

	GLuint *m_vertBuffers;  // if using GLSL interpolation

	GLuint	m_vertexBuffer; // vertex buffer handle if static mesh (single frame)
	GLuint  m_indexBuffer;  // index buffer handle if static mesh (single frame)

	Vec2f *texCoords;
	Vec3f *tangents;
	uint32 *indices;

	// material data
	Vec3f diffuseColor;
	Vec3f specularColor;
	float specularPower;
	float opacity;

	// properties
	bool twoSided;
	bool customColor;

	InterpolationData *interpolationData;

private:
	void initMemory();
	void fillBuffers();
	void loadAdditionalTextures(const string &dtPath, TextureManager *textureManager);
	void computeTangents();

public:
	// init & end
	Mesh();
	~Mesh();

	// maps
	const Texture2D *getTexture(int i) const	{return textures[i];}

	// counts
	uint32 getFrameCount() const			{return frameCount;}
	uint32 getVertexCount() const			{return vertexCount;}

	uint32 getIndexCount() const			{return indexCount;}
	uint32 getTriangleCount() const;

	// data
	// simd interpolated mesh
	const Vec3f *getVertArray(int n) const	{return vertArrays[n]; }
	const Vec3f *getNormArray(int n) const	{return normArrays[n]; }

	// x87 interpolated mesh
	const Vec3f *getVertices() const 		{return vertices;}
	const Vec3f *getNormals() const 		{return normals;}

	// simd or x87 versions
	const Vec2f *getTexCoords() const		{return texCoords;}
	const Vec3f *getTangents() const		{return tangents;}
	const uint32 *getIndices() const 		{return indices;}
	
	// VBO handles, for static meshes only atm
	GLuint getVertexBuffer() const       { return m_vertexBuffer; }
	GLuint getIndexBuffer() const        { return m_indexBuffer; }

	GLuint getVertBuffer(unsigned i) const {
		assert(m_vertBuffers && i < frameCount);
		return m_vertBuffers[i];
	}

	// material
	const Vec3f &getDiffuseColor() const	{return diffuseColor;}
	const Vec3f &getSpecularColor() const	{return specularColor;}
	float getSpecularPower() const			{return specularPower;}
	float getOpacity() const				{return opacity;}

	// properties
	bool getTwoSided() const				{return twoSided;}
	bool getCustomTexture() const			{return customColor;}

	// external data
	const InterpolationData *getInterpolationData() const {return interpolationData;}

	// interpolation
	void buildInterpolationData();
	void updateInterpolationData(float t, bool cycle) const;
	void updateInterpolationVertices(float t, bool cycle) const;

	// load
	void loadV3(const string &dir, FileOps *f, TextureManager *textureManager);
	void load(const string &dir, FileOps *f, TextureManager *textureManager);
	void save(const string &dir, FileOps *f);

	void buildCube(int size, int height, Texture2D *tex);
};

// =====================================================
// class Model
//
// 3D Model, than can be loaded from a g3d file
// =====================================================

class Model {
private:
	TextureManager *textureManager;

private:
	uint8 fileVersion;
	uint32 meshCount;
	Mesh *meshes;

public:
	// constructor & destructor
	Model();
	virtual ~Model();
	virtual void init() = 0;
	virtual void end() = 0;

	// data
	void buildShadowVolumeData() const;
	void updateInterpolationData(float t, bool cycle) const {
		for (int i = 0; i < meshCount; ++i) {
			meshes[i].updateInterpolationData(t, cycle);
		}
	}
	void updateInterpolationVertices(float t, bool cycle) const {
		for (int i = 0; i < meshCount; ++i) {
			meshes[i].updateInterpolationVertices(t, cycle);
		}
	}

	// get
	uint8 getFileVersion() const  {return fileVersion;}
	uint32 getMeshCount() const   {return meshCount;}
	const Mesh *getMesh(int i) const {return &meshes[i];}

	uint32 getTriangleCount() const;
	uint32 getVertexCount() const;

	// io
	void load(const string &path, int size, int height);
	void save(const string &path);
	void loadG3d(const string &path);
	void saveS3d(const string &path);

	void setTextureManager(TextureManager *textureManager) {this->textureManager = textureManager;}

private:
	void buildInterpolationData() const {
		for (int i = 0; i < meshCount; ++i) {
			meshes[i].buildInterpolationData();
		}
	}
};

}}//end namespace

#endif
