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

#include "pch.h"
#include "model.h"

#include <cstdio>
#include <cassert>
#include <stdexcept>
#include <memory>  // for auto_ptr

#include "interpolation.h"
#include "conversion.h"
#include "util.h"

#include "leak_dumper.h"

#include "FSFactory.hpp"

using std::exception;
using namespace Shared::Platform;

namespace Shared{ namespace Graphics{

using namespace Util;

LerpMethod meshLerpMethod;
bool use_vbos;
bool use_tangents;

Vec3f* allocate_aligned_vec3_array(unsigned n) {
	int numFloat = n * 3;
	int padFloat = (numFloat % 4 == 0 ? 0 : 4 - (numFloat % 4));
	int bytes = (numFloat + padFloat) * sizeof(float);
	assert(bytes % 16 == 0);
	void *res = _mm_malloc(bytes, 16);
	assert(((size_t)res & 0xF) == 0);
	return static_cast<Vec3f*>(res);
}

void free_aligned_vec3_array(Vec3f *ptr) {
	assert(((size_t)ptr & 0xF) == 0);
	_mm_free(ptr);
}

// =====================================================
//	class Mesh
// =====================================================

// ==================== constructor & destructor ====================

/** init, set all members to 0 */
Mesh::Mesh() {
	memset(this, 0, sizeof(*this));
}

/** delete VBOs and any remaining data in system RAM */
Mesh::~Mesh() {
	delete interpolationData;
	if (frameCount > 1) {
		delete [] m_vertices_anim;
	}
}

#define OUTPUT_MODEL_INFO(x)
//#define OUTPUT_MODEL_INFO(x) cout << x

#define MESH_DEBUG(x)
//#define MESH_DEBUG(x) cout << "\t\t\t" << x << endl

/** Allocate memory to read in mesh data, and generate VBO handles */
void Mesh::initMemory() {
	if (!vertexCount) {
		assert(!indexCount);
		MESH_DEBUG( "Mesh has no vertices!" );
		frameCount = 0;
		return;
	}
	assert(vertexCount > 0);
	assert(indexCount > 0);

	MESH_DEBUG( "Mesh::initMemory() : vertexCount = " << vertexCount << ", indexCount = " <<  indexCount 
				<< ", frameCount = " << frameCount );

	// generate buffer handles
	if (use_vbos) {
		if (frameCount == 1) {
			glGenBuffers(1, &m_vertices_frame0.vbo_handle);
		} else {
			glGenBuffers(1, &m_texCoordData.vbo_handle);
			// seperate tex-coords
		}
		glGenBuffers(1, &m_indices.vbo_handle);
		//MESH_DEBUG( "Using VBOs, handles generated: vertexBuffer = " << m_vertexBuffer << ", indexBuffer = " << m_indexBuffer );
	}
}

/** Fill vertex and index VBOs and delete system RAM copies */
void Mesh::fillBuffers(Vec3f *vertices, Vec3f *normals, Vec3f *tangents, Vec2f *texCoords, uint32 *indices) {

	if (use_tangents && tangents) {

		if (frameCount > 1) {
			// fill tex-coord buffer
			m_texCoordData.init(MeshVertexBlock::UV, vertexCount);
			for (int i=0; i < vertexCount; ++i) {
				m_texCoordData.m_texOnly[i].uv = texCoords[i];
			}
			if (use_vbos) {
				glBindBuffer(GL_ARRAY_BUFFER, m_texCoordData.vbo_handle);
				int buffSize = sizeof(Vertex_U) * vertexCount;
				glBufferData(GL_ARRAY_BUFFER, buffSize, m_texCoordData.m_arrayPtr, GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				m_texCoordData.freeMemory();
			}
			// fill interleaved arrays (Vertex_PNT)
			m_vertices_anim = new MeshVertexBlock[frameCount];
			for (int i=0; i < frameCount; ++i) {
				m_vertices_anim[i].init(MeshVertexBlock::POS_NORM_TAN, vertexCount);
				Vertex_PNT *vertData = m_vertices_anim[i].m_posNormTan;
				for (int j=0; j < vertexCount; ++j) {
					vertData[j].pos = vertices[i * vertexCount + j];
					vertData[j].norm = normals[i * vertexCount + j];
					vertData[j].tan = tangents[i * vertexCount + j];
				}
			}
		} else {
			// fill interleaved array (Vertex_PNTU)
			m_vertices_frame0.init(MeshVertexBlock::POS_NORM_TAN_UV, vertexCount);
			Vertex_PNTU *vertData = m_vertices_frame0.m_posNormTanTex;
			for (int i=0; i < vertexCount; ++i) {
				vertData[i].pos = vertices[i];
				vertData[i].norm = normals[i];
				vertData[i].uv = texCoords[i];
				vertData[i].tan = tangents[i];
			}
			if (use_vbos) {
				glBindBuffer(GL_ARRAY_BUFFER, m_vertices_frame0.vbo_handle);
				int buffSize = sizeof(Vertex_PNTU) * vertexCount;
				glBufferData(GL_ARRAY_BUFFER, buffSize, (void*)vertData, GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				m_vertices_frame0.freeMemory();
			}
		}

	} else {

		if (frameCount > 1) {
			// fill tex-coord buffer
			m_texCoordData.init(MeshVertexBlock::UV, vertexCount);
			for (int i=0; i < vertexCount; ++i) {
				m_texCoordData.m_texOnly[i].uv = texCoords[i];
			}
			if (use_vbos) {
				glBindBuffer(GL_ARRAY_BUFFER, m_texCoordData.vbo_handle);
				int buffSize = sizeof(Vertex_U) * vertexCount;
				glBufferData(GL_ARRAY_BUFFER, buffSize, m_texCoordData.m_arrayPtr, GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				m_texCoordData.freeMemory();
			}

			// fill interleaved arrays (Vertex_PN)
			m_vertices_anim = new MeshVertexBlock[frameCount];
			for (int i=0; i < frameCount; ++i) {
				m_vertices_anim[i].init(MeshVertexBlock::POS_NORM, vertexCount);
				Vertex_PN *vertData = m_vertices_anim[i].m_posNorm;
				for (int j=0; j < vertexCount; ++j) {
					vertData[j].pos = vertices[i * vertexCount + j];
					vertData[j].norm = normals[i * vertexCount + j];
				}
			}
		} else {
			// fill interleaved array (Vertex_PNU)
			m_vertices_frame0.init(MeshVertexBlock::POS_NORM_UV, vertexCount);
			Vertex_PNU *vertData = m_vertices_frame0.m_posNormTex;
			for (int i=0; i < vertexCount; ++i) {
				vertData[i].pos = vertices[i];
				vertData[i].norm = normals[i];
				vertData[i].uv = texCoords[i];
			}
			if (use_vbos) {
				glBindBuffer(GL_ARRAY_BUFFER, m_vertices_frame0.vbo_handle);
				int buffSize = sizeof(Vertex_PNU) * vertexCount;
				glBufferData(GL_ARRAY_BUFFER, buffSize, (void*)vertData, GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				m_vertices_frame0.freeMemory();
			}
		}
	}

	if (vertexCount < 65536) {
		m_indices.init(MeshIndexBlock::UNSIGNED_16, indexCount);
		for (int i=0; i < indexCount; ++i) {
			m_indices.m_16bit_indices[i] = (uint16)indices[i];
		}
		if (use_vbos) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indices.vbo_handle);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16) * indexCount, m_indices.m_indices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	} else {
		m_indices.init(MeshIndexBlock::UNSIGNED_32, m_indices.vbo_handle);
		for (int i=0; i < indexCount; ++i) {
			m_indices.m_32bit_indices[i] = (uint32)indices[i];
		}
		if (use_vbos) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indices.vbo_handle);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32) * indexCount, m_indices.m_indices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	}
	delete [] vertices;
	delete [] normals;
	delete [] tangents;
	delete [] texCoords;
	delete [] indices;
}

/** somewhat hacky way to load textures into the specular, normal and 2 custom slots */
void Mesh::loadAdditionalTextures(const string &diffusePath, TextureManager *textureManager) {
	string checkPath;
	// spec map
	if (!textures[MeshTexture::SPECULAR]) {
		checkPath = diffusePath; // insert _specular before . in filename
		checkPath.insert(checkPath.length() - 4, "_specular");
		if (fileExists(checkPath)) {
			textures[MeshTexture::SPECULAR] = textureManager->getTexture(checkPath);
		}
	}
	// bump map
	if (!textures[MeshTexture::NORMAL]) {
		checkPath = diffusePath; // insert _normal before . in filename
		checkPath.insert(checkPath.length() - 4, "_normal");
		if (fileExists(checkPath)) {
			textures[MeshTexture::NORMAL] = textureManager->getTexture(checkPath);
		}
	}
	//  light map
	if (!textures[MeshTexture::LIGHT]) {
		checkPath = diffusePath;
		checkPath.insert(checkPath.length() - 4, "_light");
		if (fileExists(checkPath)) {
			textures[MeshTexture::NORMAL] = textureManager->getTexture(checkPath);
		}
	}
	// custom textures
	if (!textures[MeshTexture::CUSTOM]) {
		checkPath = diffusePath;
		checkPath.insert(checkPath.length() - 4, "_custom");
		if (fileExists(checkPath)) {
			textures[MeshTexture::CUSTOM] = textureManager->getTexture(checkPath);
		}
	}
}

// ========================== shadows & interpolation =========================

void Mesh::buildInterpolationData() {
	if (frameCount > 1) {
		interpolationData = new InterpolationData(this);
	}
}

void Mesh::updateInterpolationData(float t, bool cycle) const {
	if (frameCount > 1) {
		interpolationData->update(t, cycle);
	}
}

//void Mesh::updateInterpolationVertices(float t, bool cycle) const {
//	interpolationData->updateVertices(t, cycle);
//}

// ==================== load ====================

void Mesh::loadV3(const string &dir, FileOps *f, TextureManager *textureManager) {
	Vec3f *vertices = 0;
	Vec3f *normals = 0;
	Vec2f *texCoords = 0;
	Vec3f *tangents = 0;
	uint32 *indices = 0;

	// read header
	MeshHeaderV3 meshHeader;
	f->read(&meshHeader, sizeof(MeshHeaderV3), 1);
	if (meshHeader.normalFrameCount != meshHeader.vertexFrameCount) {
		throw runtime_error("Old model: vertex frame count different from normal frame count");
	}

	// init
	frameCount = meshHeader.vertexFrameCount;
	vertexCount = meshHeader.pointCount;
	indexCount = meshHeader.indexCount;

	initMemory();
	vertices = new Vec3f[frameCount * vertexCount];
	normals = new Vec3f[frameCount * vertexCount];
	texCoords = new Vec2f[vertexCount];
	indices = new uint32[indexCount];

	MESH_DEBUG( "Reading flags." );

	// misc
	twoSided = (meshHeader.properties & mp3TwoSided) != 0;
	customColor = (meshHeader.properties & mp3CustomColor) != 0;
	noSelect = false;

	// texture
	if (!(meshHeader.properties & mp3NoTexture) && textureManager != NULL) {
		MESH_DEBUG( "texture...1" );
		string texPath = toLower(reinterpret_cast<char*>(meshHeader.texName));
		MESH_DEBUG( "texture...2 texPath = " << texPath );
		texPath = dir + "/" + texPath;
		texPath = cleanPath(texPath);
		MESH_DEBUG( "Loading diffuse texture '" << texPath << "'." );
		textures[MeshTexture::DIFFUSE] = textureManager->getTexture(texPath);
		loadAdditionalTextures(texPath, textureManager);
	} else {
		MESH_DEBUG( "no texture." );
	}

	// read data
	size_t vfCount = frameCount * vertexCount;
	MESH_DEBUG( "Vertex position and normal data: Reading " << (vfCount * 6) << " floats into arrays." );
	f->read(vertices, sizeof(Vec3f)*vfCount, 1);
	f->read(normals, sizeof(Vec3f)*vfCount, 1);
	if (textures[MeshTexture::DIFFUSE] != 0) {
		int n = meshHeader.texCoordFrameCount * vertexCount * 2;
		MESH_DEBUG( "Texture co-ordinate data: Reading " << n << " floats into array(s)." );
		for (int i=0; i < meshHeader.texCoordFrameCount; ++i) {
			f->read(texCoords, sizeof(Vec2f) * vertexCount, 1);
		}
	}
	MESH_DEBUG( "Reading diffuse colour and opacity." );
	f->read(&diffuseColor, sizeof(Vec3f), 1);
	f->read(&opacity, sizeof(float32), 1);

	f->seek(sizeof(Vec4f)*(meshHeader.colorFrameCount-1), SEEK_CUR);
	f->read(indices, sizeof(uint32)*indexCount, 1);

	if (textures[MeshTexture::NORMAL]) {
		computeTangents(vertices, texCoords, indices, tangents);
	}
	fillBuffers(vertices, normals, tangents, texCoords, indices);
}

// G3D V4
void Mesh::load(const string &dir, FileOps *f, TextureManager *textureManager){
	Vec3f *vertices = 0;
	Vec3f *normals = 0;
	Vec2f *texCoords = 0;
	Vec3f *tangents = 0;
	uint32 *indices = 0;

	// read header
	MeshHeader meshHeader;
	if (f->read(&meshHeader, sizeof(MeshHeader), 1) != 1) {
		throw runtime_error("Could not read mesh header");
	}

	// init
	frameCount = meshHeader.frameCount;
	vertexCount = meshHeader.vertexCount;
	indexCount = meshHeader.indexCount;

	initMemory();
	vertices = new Vec3f[frameCount * vertexCount];
	normals = new Vec3f[frameCount * vertexCount];
	texCoords = new Vec2f[vertexCount];
	indices = new uint32[indexCount];

	// properties
	customColor = (meshHeader.properties & mpfCustomColor) != 0;
	twoSided = (meshHeader.properties & mpfTwoSided) != 0;
	noSelect = (meshHeader.properties & mpfNoSelect) != 0;

	// material
	diffuseColor= Vec3f(meshHeader.diffuseColor);
	specularColor= Vec3f(meshHeader.specularColor);
	specularPower= meshHeader.specularPower;
	opacity= meshHeader.opacity;

	// maps
	uint32 flag = 1;
	string diffuseTexPath;
	for (int i=0; i < MeshTexture::COUNT; ++i) {
		if ((meshHeader.textures & flag) && textureManager != NULL) {
			uint8 cMapPath[mapPathSize];
			f->read(cMapPath, mapPathSize, 1);
			string mapPath = toLower(reinterpret_cast<char*>(cMapPath));
			assert(mapPath != "");
			string mapFullPath = dir + "/" + mapPath;
			if (flag == 1) {
				diffuseTexPath = mapFullPath;
			}
			//texturePaths[i] = mapPath;
			textures[i] = static_cast<Texture2D*>(textureManager->getTexture(mapFullPath));
		}
		flag *= 2;
	}
	if (textures[MeshTexture::DIFFUSE]) {
		assert(!diffuseTexPath.empty());
		loadAdditionalTextures(diffuseTexPath, textureManager);
	}

	// read data. (Assume packed vectors)
	size_t vfCount = frameCount * vertexCount;
	if (vfCount) {
		if (f->read(vertices, 12 * vfCount, 1) != 1) {
			delete [] vertices;
			delete [] normals;
			delete [] tangents;
			delete [] texCoords;
			delete [] indices;
			throw runtime_error("error reading mesh, insufficient vertex data.");
		}
		//cout << "reading " << (vfCount * 12) << " bytes of normal data.\n";
		if (f->read(normals, 12 * vfCount, 1) != 1) {
			delete [] vertices;
			delete [] normals;
			delete [] tangents;
			delete [] texCoords;
			delete [] indices;
			throw runtime_error("error reading mesh, insufficient normal vector data.");
		}
	}

	if (meshHeader.textures && f->read(texCoords, sizeof(Vec2f)*vertexCount, 1) != 1) {
		delete [] vertices;
		delete [] normals;
		delete [] tangents;
		delete [] texCoords;
		delete [] indices;
		throw runtime_error("error reading mesh, insufficient texture co-ordinate data.");
	}
	if (indexCount) {
		if (f->read(indices, sizeof(uint32)*indexCount, 1) != 1) {
			delete [] vertices;
			delete [] normals;
			delete [] tangents;
			delete [] texCoords;
			delete [] indices;
			throw runtime_error("error reading mesh, insufficient vertex index data.");
		}
	}
	if (textures[MeshTexture::NORMAL]) {
		computeTangents(vertices, texCoords, indices, tangents);
	}
	fillBuffers(vertices, normals, tangents, texCoords, indices);
}

void Mesh::buildCube(int size, int height, Texture2D *tex) {
	frameCount = 1;
	vertexCount = 5 * 4;
	indexCount = 5 * 6;

	Vec3f *vertArray = new Vec3f[vertexCount];//allocate_aligned_vec3_array(vertexCount);
	Vec3f *normArray = new Vec3f[vertexCount];//allocate_aligned_vec3_array(vertexCount);

	Vec2f *texCoords = new Vec2f[vertexCount];
	uint32 *indices = new uint32[indexCount];

	// normals
	Vec3f	up(0.f, 1.f, 0.f),
			north(0.f, 0.f, 1.f),
			east(1.f, 0.f, 0.f),
			south(0.f, 0.f, -1.f),
			west(-1.f, 0.f, 0.f);
	
	Vec2f	tCoord[4];
	tCoord[0] = Vec2f(0.f, 0.f);
	tCoord[1] = Vec2f(0.f, 1.f);
	tCoord[2] = Vec2f(1.f, 0.f);
	tCoord[3] = Vec2f(1.f, 1.f);

	float xzDist = float(size) / 2.f;
	float yDist = float(height);

	int n = 0;
	int i = 0;

	// top face
	vertArray[0] = Vec3f(-xzDist, yDist, -xzDist);
	vertArray[1] = Vec3f(xzDist,  yDist, -xzDist);
	vertArray[2] = Vec3f(-xzDist, yDist, xzDist);
	vertArray[3] = Vec3f(xzDist,  yDist, xzDist);
	for (; n < 4; ++n) {
		normArray[n] = up;
		texCoords[n] = tCoord[n % 4];
	}
	indices[0] = 2;
	indices[1] = 1;
	indices[2] = 0;
	indices[3] = 1;
	indices[4] = 2;
	indices[5] = 3;

	// 'north' face
	vertArray[4] = Vec3f(-xzDist,	0.f, xzDist);
	vertArray[5] = Vec3f(-xzDist, yDist, xzDist);
	vertArray[6] = Vec3f(xzDist,	0.f, xzDist);
	vertArray[7] = Vec3f(xzDist,  yDist, xzDist);
	for (; n < 8; ++n) {
		normArray[n] = north;
		texCoords[n] = tCoord[n % 4];
	}
	indices[6]  = 6;
	indices[7]  = 5;
	indices[8]  = 4;
	indices[9]  = 5;
	indices[10] = 6;
	indices[11] = 7;

	// 'east' face
	vertArray[8] = Vec3f(xzDist,    0.f, xzDist);
	vertArray[9] = Vec3f(xzDist,  yDist, xzDist);
	vertArray[10] = Vec3f(xzDist,   0.f, -xzDist);
	vertArray[11] = Vec3f(xzDist, yDist, -xzDist);
	for (; n < 12; ++n) {
		normArray[n] = east;
		texCoords[n] = tCoord[n % 4];
	}
	indices[12] = 10;
	indices[13] = 9;
	indices[14] = 8;
	indices[15] = 9;
	indices[16] = 10;
	indices[17] = 11;

	// south
	vertArray[12] = Vec3f(xzDist,	 0.f, -xzDist);
	vertArray[13] = Vec3f(xzDist, yDist, -xzDist);
	vertArray[14] = Vec3f(-xzDist,	 0.f, -xzDist);
	vertArray[15] = Vec3f(-xzDist,  yDist, -xzDist);
	for (; n < 16; ++n) {
		normArray[n] = south;
		texCoords[n] = tCoord[n % 4];
	}
	indices[18] = 14;
	indices[19] = 13;
	indices[20] = 12;
	indices[21] = 13;
	indices[22] = 14;
	indices[23] = 15;

	// west
	vertArray[16] = Vec3f(-xzDist,    0.f, -xzDist);
	vertArray[17] = Vec3f(-xzDist,  yDist, -xzDist);
	vertArray[18] = Vec3f(-xzDist,    0.f, xzDist);
	vertArray[19] = Vec3f(-xzDist,  yDist, xzDist);
	for (; n < 20; ++n) {
		normArray[n] = west;
		texCoords[n] = tCoord[n % 4];
	}
	indices[24] = 18;
	indices[25] = 17;
	indices[26] = 16;
	indices[27] = 17;
	indices[28] = 18;
	indices[29] = 19;

	this->textures[MeshTexture::DIFFUSE] = tex;
	this->customColor = false;
	this->specularColor = this->diffuseColor = Vec3f(0.5f, 0.5f, 0.5f);
	this->specularPower = 0.5f;
	this->opacity = 1.f;
	this->twoSided = false;

	initMemory();
	fillBuffers(vertArray, normArray, 0, texCoords, indices);
}

void Mesh::computeTangents(Vec3f *verts, Vec2f *texCoords, uint32 *indices, Vec3f *&tangents) {
	tangents = new Vec3f[vertexCount * frameCount];
	for (unsigned int i = 0; i < vertexCount * frameCount; ++i) {
		tangents[i] = Vec3f(0.f);
	}
	for (int frame = 0; frame < frameCount; ++frame) {
		unsigned int vertexBase = frame * vertexCount;
		for (unsigned int i = 0; i < indexCount; i += 3) {
			for (int j = 0; j < 3; ++j) {
				uint32 i0 = indices[i + j];
				uint32 i1 = indices[i + (j + 1) % 3];
				uint32 i2 = indices[i + (j + 2) % 3];

				Vec3f p0 = verts[vertexBase + i0];
				Vec3f p1 = verts[vertexBase + i1];
				Vec3f p2 = verts[vertexBase + i2];

				float u0 = texCoords[i0].x;
				float u1 = texCoords[i1].x;
				float u2 = texCoords[i2].x;

				float v0 = texCoords[i0].y;
				float v1 = texCoords[i1].y;
				float v2 = texCoords[i2].y;

				tangents[vertexBase + i0] +=
					((p2 - p0) * (v1 - v0) - (p1 - p0) * (v2 - v0)) /
					((u2 - u0) * (v1 - v0) - (u1 - u0) * (v2 - v0));
			}
		}
	}

	for (unsigned int i=0; i < vertexCount * frameCount; ++i) {
		/*Vec3f binormal= normals[i].cross(tangents[i]);
		tangents[i]+= binormal.cross(normals[i]);*/
		tangents[i].normalize();
	}
}

void Mesh::save(const string &dir, FileOps *f){
	/*MeshHeader meshHeader;
	meshHeader.vertexFrameCount= vertexFrameCount;
	meshHeader.normalFrameCount= normalFrameCount;
	meshHeader.texCoordFrameCount= texCoordFrameCount;
	meshHeader.colorFrameCount= colorFrameCount;
	meshHeader.pointCount= pointCount;
	meshHeader.indexCount= indexCount;
	meshHeader.properties= 0;

	if(twoSided) meshHeader.properties|= mpTwoSided;
	if(customTexture) meshHeader.properties|= mpCustomTexture;

	if(texture==NULL){
		meshHeader.properties|= mpNoTexture;
		meshHeader.texName[0]= '\0';
	}
	else{
		strcpy(reinterpret_cast<char*>(meshHeader.texName), texName.c_str());
		texture->getPixmap()->saveTga(dir+"/"+texName);
	}

	fwrite(&meshHeader, sizeof(MeshHeader), 1, f);
	fwrite(vertices, sizeof(Vec3f)*vertexFrameCount*pointCount, 1, f);
	fwrite(normals, sizeof(Vec3f)*normalFrameCount*pointCount, 1, f);
	fwrite(texCoords, sizeof(Vec2f)*texCoordFrameCount*pointCount, 1, f);
	fwrite(colors, sizeof(Vec4f)*colorFrameCount, 1, f);
	fwrite(indices, sizeof(uint32)*indexCount, 1, f);*/
}

// ===============================================
//	class Model
// ===============================================

// ==================== constructor & destructor ====================

Model::Model(){
	meshCount= 0;
	meshes= NULL;
	textureManager= NULL;
}

Model::~Model(){
	delete [] meshes;
}

// ==================== data ====================


// ==================== get ====================

uint32 Model::getTriangleCount() const{
	uint32 triangleCount= 0;
	for(uint32 i=0; i<meshCount; ++i){
		triangleCount+= meshes[i].getIndexCount()/3;
	}
	return triangleCount;
}

uint32 Model::getVertexCount() const{
	uint32 vertexCount= 0;
	for(uint32 i=0; i<meshCount; ++i){
		vertexCount+= meshes[i].getVertexCount();
	}
	return vertexCount;
}

// ==================== io ====================

void Model::load(const string &path, int size, int height) {
	string extension = path.substr(path.find_last_of('.') + 1);
	try {
		if (extension == "g3d" || extension == "G3D") {
			loadG3d(path);
		} else {
			throw runtime_error("Unknown model format: " + extension);
		}
	} catch (runtime_error &e) {
		meshCount = 1;
		meshes = new Mesh[1];
		meshes[0].buildCube(size, height, Texture2D::defaultTexture);
		meshes[0].buildInterpolationData();
		mediaErrorLog.add(e.what(), path);
	}
}

void Model::save(const string &path){
	string extension = path.substr(path.find_last_of('.') + 1);
	if (extension == "g3d" || extension == "G3D" || extension == "s3d" || extension == "S3D") {
		saveS3d(path);
	} else {
		throw runtime_error("Unknown model format: " + extension);
	}
}

// load a model from a g3d file
void Model::loadG3d(const string &path){
	std::auto_ptr<FileOps> f(FSFactory::getInstance()->getFileOps());
	f->openRead(path.c_str());

	string dir = dirname(path);

	OUTPUT_MODEL_INFO("loading G3D from " << path << endl);

	// file header
	FileHeader fileHeader;
	f->read(&fileHeader, sizeof(FileHeader), 1);
	if (strncmp(reinterpret_cast<char*>(fileHeader.id), "G3D", 3) != 0) {
		OUTPUT_MODEL_INFO("Error: Bad magic cookie. Not a valid G3D model.\n");
		throw runtime_error("Bad magic. Not a valid G3D model");
	}
	fileVersion = fileHeader.version;

	if (fileHeader.version == 4) { // version 4
		OUTPUT_MODEL_INFO("\tVersion: 4\n");
		// model header
		ModelHeader modelHeader;
		f->read(&modelHeader, sizeof(ModelHeader), 1);
		meshCount = modelHeader.meshCount;
		if (modelHeader.type != mtMorphMesh) {
			OUTPUT_MODEL_INFO("\tError: invalid mesh type.\n");
			throw runtime_error("Invalid model type");
		}
		OUTPUT_MODEL_INFO("\tMesh count: " << meshCount << endl);

		//load meshes
		meshes = new Mesh[meshCount];
		for (uint32 i=0; i < meshCount; ++i) {
			OUTPUT_MODEL_INFO("\tLoading mesh " << i << endl);
			meshes[i].load(dir, f.get(), textureManager);
			meshes[i].buildInterpolationData();
			OUTPUT_MODEL_INFO("\t\tVertex count: " << meshes[i].getVertexCount() << endl);
			OUTPUT_MODEL_INFO("\t\tFrame count: " << meshes[i].getFrameCount() << endl);
		}
	} else if (fileHeader.version == 3) { // version 3
		OUTPUT_MODEL_INFO("\tVersion: 3\n");
		f->read(&meshCount, sizeof(meshCount), 1);
		OUTPUT_MODEL_INFO("\tMesh count: " << meshCount << endl);
		meshes= new Mesh[meshCount];
		for (uint32 i=0; i < meshCount; ++i) {
			OUTPUT_MODEL_INFO("\tLoading mesh " << i << endl);
			meshes[i].loadV3(dir, f.get(), textureManager);
			meshes[i].buildInterpolationData();
			OUTPUT_MODEL_INFO("\t\tVertex count: " << meshes[i].getVertexCount() << endl);
			OUTPUT_MODEL_INFO("\t\tFrame count: " << meshes[i].getFrameCount() << endl);
		}
	} else {
		OUTPUT_MODEL_INFO("\tError: Invalid version: " << fileHeader.version << "\n");
		throw runtime_error("Invalid model version: "+ intToStr(fileHeader.version));
	}
	
}

//save a model to a g3d file
void Model::saveS3d(const string &path){

	/*FILE *f= fopen(path.c_str(), "wb");
	if(f==NULL){
		throw runtime_error("Cant open file for writting: "+path);
	}

	ModelHeader modelHeader;
	modelHeader.id[0]= 'G';
	modelHeader.id[1]= '3';
	modelHeader.id[2]= 'D';
	modelHeader.version= 3;
	modelHeader.meshCount= meshCount;

	string dir= dirname(path);

	fwrite(&modelHeader, sizeof(ModelHeader), 1, f);
	for(int i=0; i<meshCount; ++i){
		meshes[i].save(dir, f);
	}

	fclose(f);*/
}

}}//end namespace
