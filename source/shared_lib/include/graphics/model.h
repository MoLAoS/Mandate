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
extern bool use_tangents;

Vec3f* allocate_aligned_vec3_array(unsigned n);
void free_aligned_vec3_array(Vec3f *ptr);

struct Vertex_U {
	Vec2f uv;
};

struct Vertex_PN {
	Vec3f pos;
	Vec3f norm;
};

struct Vertex_PNU {
	Vec3f pos;
	Vec3f norm;
	Vec2f uv;
};

struct Vertex_PNT {
	Vec3f pos;
	Vec3f norm;
	Vec3f tan;
};

struct Vertex_PNTU {
	Vec3f pos;
	Vec3f norm;
	Vec3f tan;
	Vec2f uv;
};

static const int uvOffsets[6] = { -1, 0, -1, 6, -1, 9 };

struct MeshVertexBlock {
	enum Type { NONE, UV, POS_NORM, POS_NORM_UV, POS_NORM_TAN, POS_NORM_TAN_UV };
	Type type;
	int  count;
	GLuint vbo_handle;
	union {
		void        *m_arrayPtr;
		Vertex_U    *m_texOnly;
		Vertex_PN   *m_posNorm;
		Vertex_PNU  *m_posNormTex;
		Vertex_PNT  *m_posNormTan;
		Vertex_PNTU *m_posNormTanTex;
	};

	MeshVertexBlock() : type(NONE), count(0), m_texOnly(0), vbo_handle(0) {}

	~MeshVertexBlock() {
		freeMemory();
		if (vbo_handle != 0) {
			glDeleteBuffers(1, &vbo_handle);
		}
	}

	int getStride() const {
		switch (type) {
			case UV:
				return sizeof(Vertex_U);
			case POS_NORM:
				return sizeof(Vertex_PN);
			case POS_NORM_UV:
				return sizeof(Vertex_PNU);
			case POS_NORM_TAN:
				return sizeof(Vertex_PNT);
			case POS_NORM_TAN_UV:
				return sizeof(Vertex_PNTU);
			default:
				throw runtime_error("Invalid vertex type.");
		}		
	}

	int getUvOffset() const { return uvOffsets[type]; }

	void init(Type type, unsigned count) {
		this->type = type;
		this->count = count;
		Vec3f *p;
		switch (type) {
			case UV:
				m_texOnly = new Vertex_U[count]; break;
			case POS_NORM:
				p = allocate_aligned_vec3_array(count * 2);
				m_posNorm = reinterpret_cast<Vertex_PN*>(p); break;
			case POS_NORM_UV:
				m_posNormTex = new Vertex_PNU[count]; break;
			case POS_NORM_TAN:
				p = allocate_aligned_vec3_array(count * 3);
				m_posNormTan = reinterpret_cast<Vertex_PNT*>(p); break;
			case POS_NORM_TAN_UV:
				m_posNormTanTex = new Vertex_PNTU[count]; break;
			case NONE:
				/*NOP*/ break;
		}
	}

	void freeMemory() {
		switch (type) {
			case UV:
				delete [] m_texOnly; break;
			case POS_NORM_UV:
				delete [] m_posNormTex; break;
			case POS_NORM_TAN_UV:
				delete [] m_posNormTanTex; break;
			case POS_NORM:
				free_aligned_vec3_array((Vec3f*)m_arrayPtr); break;
			case POS_NORM_TAN:
				free_aligned_vec3_array((Vec3f*)m_arrayPtr); break;
			case NONE:
				/*NOP*/ break;
		}
		m_arrayPtr = 0;
	}
};

struct MeshIndexBlock {
	enum IndexType { UNSIGNED_16, UNSIGNED_32 };
	IndexType type;
	unsigned count;
	GLuint vbo_handle;
	union {
		void    *m_indices;
		uint16  *m_16bit_indices;
		uint32  *m_32bit_indices;
	};

	MeshIndexBlock() : type(UNSIGNED_16), count(0), m_indices(0), vbo_handle(0) { }

	~MeshIndexBlock() {
		freeMemory();
		if (vbo_handle != 0) {
			glDeleteBuffers(1, &vbo_handle);
		}
	}

	void init(IndexType type, unsigned count) {
		this->type = type;
		this->count = count;
		switch (type) {
			case UNSIGNED_16:
				m_16bit_indices = new uint16[count]; break;
			case UNSIGNED_32:
				m_32bit_indices = new uint32[count]; break;
		}
	}

	void freeMemory() {
		switch (type) {
			case UNSIGNED_16:
				delete[] m_16bit_indices; break;
			case UNSIGNED_32:
				delete[] m_32bit_indices; break;
		}
		m_indices = 0;
	}
};

// =====================================================
// class Mesh
//
// Part of a 3D model
// =====================================================

class Mesh {
private:
	// mesh data
	Texture2D *textures[MeshTexture::COUNT];

	// vertex data counts
	uint32 frameCount;
	uint32 vertexCount;
	uint32 indexCount;

	// vertex data
	MeshVertexBlock m_vertices_frame0;
	MeshVertexBlock *m_vertices_anim;
	MeshVertexBlock m_texCoordData;
	MeshIndexBlock  m_indices;

	// material data
	Vec3f diffuseColor;
	Vec3f specularColor;
	float specularPower;
	float opacity;

	// properties
	bool twoSided;
	bool customColor;
	bool noSelect;

	InterpolationData *interpolationData;

private:
	void initMemory();
	void fillBuffers(Vec3f *pos, Vec3f *norm, Vec3f *tan, Vec2f *uv, uint32 *indices);
	void loadAdditionalTextures(const string &dtPath, TextureManager *textureManager);
	void computeTangents(Vec3f *verts, Vec2f *uvs, uint32 *indices, Vec3f *&tangents);

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
	MeshVertexBlock& getStaticVertData() { return m_vertices_frame0; }
	MeshVertexBlock& getAnimVertBlock(int i) { return m_vertices_anim[i]; }
	MeshVertexBlock& getTecCoordBlock() { return m_texCoordData; }
	MeshIndexBlock&  getIndices() { return m_indices; }

	const MeshVertexBlock& getStaticVertData() const { return m_vertices_frame0; }
	const MeshVertexBlock& getAnimVertBlock(int i) const { return m_vertices_anim[i]; }
	const MeshVertexBlock& getTecCoordBlock() const { return m_texCoordData; }
	const MeshIndexBlock&  getIndices() const { return m_indices; }

	// material
	const Vec3f &getDiffuseColor() const	{return diffuseColor;}
	const Vec3f &getSpecularColor() const	{return specularColor;}
	float getSpecularPower() const			{return specularPower;}
	float getOpacity() const				{return opacity;}

	// properties
	bool isTwoSided() const                 {return twoSided;}
	bool usesTeamTexture() const            {return customColor;}
	bool isNoSelect() const                 {return noSelect;}

	// external data
	const InterpolationData *getInterpolationData() const {return interpolationData;}

	// interpolation
	void buildInterpolationData();
	void updateInterpolationData(float t, bool cycle) const;
	//void updateInterpolationVertices(float t, bool cycle) const;

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
