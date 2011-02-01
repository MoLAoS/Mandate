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

#define USE_VBOS_FOR_STATIC_MESHES 1

bool use_simd_interpolation;

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

Mesh::Mesh() {
	frameCount = 0;
	vertexCount = 0;
	indexCount = 0;

	vertArrays = 0;
	normArrays = 0;

	vertices = NULL;
	normals = NULL;

	texCoords = NULL;
	tangents = NULL;
	indices = NULL;
	interpolationData = NULL;

	m_vertexBuffer = 0;
	m_indexBuffer = 0;

	for (int i = 0; i < meshTextureCount; ++i) {
		textures[i] = NULL;
	}

	twoSided = false;
	customColor = false;
}

Mesh::~Mesh() {
	if (frameCount == 1) {
		return;
	}
	if (use_simd_interpolation) {
		for (int i=0; i < frameCount; ++i) {
			free_aligned_vec3_array(vertArrays[i]);
			free_aligned_vec3_array(normArrays[i]);
		}
		delete [] vertArrays;
		delete [] normArrays;
	} else {
		delete [] vertices;
		delete [] normals;
	}
	delete [] texCoords;
	delete [] tangents;
	delete [] indices;

	delete interpolationData;
}

void Mesh::initMemory() {
	if (!vertexCount) {
		assert(!indexCount);
		frameCount = 0;
		return;
	}
	assert(vertexCount > 0);
	assert(indexCount > 0);

#	if USE_VBOS_FOR_STATIC_MESHES
	if (frameCount == 1) {
		glGenBuffers(1, &m_vertexBuffer);
		glGenBuffers(1, &m_indexBuffer);
	}
	if (frameCount == 1 || !use_simd_interpolation) {
#	else
	if (!use_simd_interpolation) {
#	endif
		vertices = new Vec3f[frameCount * vertexCount];
		normals = new Vec3f[frameCount * vertexCount];
	} else {
		vertArrays = new Vec3f*[frameCount];
		normArrays = new Vec3f*[frameCount];
		for (int i=0; i < frameCount; ++i) {
			vertArrays[i] = allocate_aligned_vec3_array(vertexCount);
		}
		for (int i=0; i < frameCount; ++i) {
			normArrays[i] = allocate_aligned_vec3_array(vertexCount);
		}
	}
	texCoords = new Vec2f[vertexCount];
	indices = new uint32[indexCount];
}

struct ModelVertex {
	Vec3f   m_position;
	Vec3f   m_normal;
	Vec2f   m_texCoord;
};

void Mesh::fillBuffers() {
	if (!m_vertexBuffer) {
		assert(!m_indexBuffer);
		return;
	}
	assert(m_indexBuffer && vertices && normals && !vertArrays && !normArrays);

	ModelVertex *vertData = new ModelVertex[vertexCount];
	for (int i=0; i < vertexCount; ++i) {
		vertData[i].m_position = vertices[i];
		vertData[i].m_normal = normals[i];
		vertData[i].m_texCoord = texCoords[i];
	}
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	int buffSize = sizeof(ModelVertex) * vertexCount;
	glBufferData(GL_ARRAY_BUFFER, buffSize, (void*)vertData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	delete [] vertData;

	///@todo probably wont ever actually need 32 bit uints for this, 16 should be sufficient
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32) * indexCount, (void*)indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	delete [] vertices;
	vertices = 0;
	delete [] normals;
	normals = 0;
	delete [] texCoords;
	texCoords = 0;
	delete [] indices;
	indices = 0;
}

void Mesh::loadAdditionalTextures(const string &diffusePath, TextureManager *textureManager) {
	string checkPath;
	// normal map ?
	if (!textures[mtNormal]) { // if someone hasn't updated the Blender export script...
		checkPath = diffusePath; // insert _normal before . in filename
		checkPath.insert(checkPath.length()-4, "_normal"); 
		if (fileExists(checkPath)) {
			textures[mtNormal] = static_cast<Texture2D*>(textureManager->getTexture(checkPath));
		}
		///@todo will need to change default tex to use flat normals 
		/// JM: negative, no normal map => no bump mapping shader (why 'pay' for something we wont get?).
	}
	// custom texture? 
	if (!textures[mtCustom1]) {		
		checkPath = diffusePath;
		checkPath.insert(checkPath.length()-4, "_custom1"); 
		if (fileExists(checkPath)) {
			textures[mtCustom1] = static_cast<Texture2D*>(textureManager->getTexture(checkPath));
		}
	}
}

// ========================== shadows & interpolation =========================

void Mesh::buildInterpolationData() {
	interpolationData = new InterpolationData(this);
}

void Mesh::updateInterpolationData(float t, bool cycle) const {
	interpolationData->update(t, cycle);
}

void Mesh::updateInterpolationVertices(float t, bool cycle) const {
	interpolationData->updateVertices(t, cycle);
}

// ==================== load ====================

void Mesh::loadV3(const string &dir, FileOps *f, TextureManager *textureManager) {
	// read header
	MeshHeaderV3 meshHeader;
	f->read(&meshHeader, sizeof(MeshHeaderV3), 1);
	if (meshHeader.normalFrameCount!=meshHeader.vertexFrameCount) {
		throw runtime_error("Old model: vertex frame count different from normal frame count");
	}

	// init
	frameCount = meshHeader.vertexFrameCount;
	vertexCount = meshHeader.pointCount;
	indexCount = meshHeader.indexCount;

	initMemory();

	// misc
	twoSided = (meshHeader.properties & mp3TwoSided) != 0;
	customColor = (meshHeader.properties & mp3CustomColor) != 0;

	// texture
	if (!(meshHeader.properties & mp3NoTexture) && textureManager != NULL) {
		string texPath = toLower(reinterpret_cast<char*>(meshHeader.texName));
		texturePaths[mtDiffuse] = toLower(reinterpret_cast<char*>(meshHeader.texName));
		texPath = dir + "/" + texPath;
		texPath = cleanPath(texPath);

		textures[mtDiffuse] = static_cast<Texture2D*>(textureManager->getTexture(texPath));
		loadAdditionalTextures(texPath, textureManager);
	}

	// read data
	if (use_simd_interpolation && vertArrays) {
		assert(sizeof(Vec3f) == 12);
		int frameRead = sizeof(Vec3f) * vertexCount;
		int nFloats = vertexCount * 3;
		int framePad = nFloats % 4 == 0 ? 0 : 4 - (nFloats % 4);

		for (int i=0; i < frameCount; ++i) {
			f->read(vertArrays[i], frameRead, 1);
			float *ptr = vertArrays[i][vertexCount].raw;
			for (int j=0; j < framePad; ++j) {
				*ptr++ = 0.f;
			}
		}
		for (int i=0; i < frameCount; ++i) {
			f->read(normArrays[i], frameRead, 1);
			float *ptr = normArrays[i][vertexCount].raw;
			for (int j=0; j < framePad; ++j) {
				*ptr++ = 0.f;
			}
		}
	} else {
		size_t vfCount = frameCount * vertexCount;
		f->read(vertices, sizeof(Vec3f)*vfCount, 1);
		f->read(normals, sizeof(Vec3f)*vfCount, 1);
	}
	if(textures[mtDiffuse]!=NULL){
		for(int i=0; i<meshHeader.texCoordFrameCount; ++i){
			f->read(texCoords, sizeof(Vec2f)*vertexCount, 1);
		}
	}
	f->read(&diffuseColor, sizeof(Vec3f), 1);
	f->read(&opacity, sizeof(float32), 1);
	f->seek(sizeof(Vec4f)*(meshHeader.colorFrameCount-1), SEEK_CUR);
	f->read(indices, sizeof(uint32)*indexCount, 1);

	if (textures[mtNormal]) {
		computeTangents();
	}
	fillBuffers();
}

// G3D V4
void Mesh::load(const string &dir, FileOps *f, TextureManager *textureManager){
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

	// properties
	customColor = (meshHeader.properties & mpfCustomColor) != 0;
	twoSided = (meshHeader.properties & mpfTwoSided) != 0;

	// material
	diffuseColor= Vec3f(meshHeader.diffuseColor);
	specularColor= Vec3f(meshHeader.specularColor);
	specularPower= meshHeader.specularPower;
	opacity= meshHeader.opacity;

	// maps
	uint32 flag = 1;
	for (int i=0; i<meshTextureCount; ++i) {
		if ((meshHeader.textures & flag) && textureManager != NULL) {
			uint8 cMapPath[mapPathSize];
			f->read(cMapPath, mapPathSize, 1);
			string mapPath = toLower(reinterpret_cast<char*>(cMapPath));
			string mapFullPath = dir + "/" + mapPath;
			assert(mapFullPath != "");

			textures[i] = static_cast<Texture2D*>(textureManager->getTexture(mapFullPath));
		}
		flag *= 2;
	}
	if (textures[mtDiffuse]) {
		loadAdditionalTextures(textures[mtDiffuse]->getPath(), textureManager);
	}

	// read data. (Assume packed vectors)
	if (use_simd_interpolation && vertArrays) {
		assert(sizeof(Vec3f) == 12);
		int frameRead = sizeof(Vec3f) * vertexCount;
		int nFloats = vertexCount * 3;
		int framePad = nFloats % 4 == 0 ? 0 : 4 - (nFloats % 4);

		for (int i=0; i < frameCount; ++i) {
			if (f->read(vertArrays[i], frameRead, 1) != 1) {
				throw runtime_error("error reading mesh, insufficient vertex data.");
				//cout << "read() Failed! getLastError() == " << f->getLastError() << endl;
			}
			float *ptr = vertArrays[i][vertexCount].raw;
			for (int j=0; j < framePad; ++j) {
				*ptr++ = 0.f;
			}
		}
		for (int i=0; i < frameCount; ++i) {
			if (f->read(normArrays[i], frameRead, 1) != 1) {
				throw runtime_error("error reading mesh, insufficient normal vector data.");
				//cout << "read() Failed! getLastError() == " << f->getLastError() << endl;
			}
			float *ptr = normArrays[i][vertexCount].raw;
			for (int j=0; j < framePad; ++j) {
				*ptr++ = 0.f;
			}
		}
	} else {
		size_t vfCount = frameCount * vertexCount;
		if (vfCount) {
			if (f->read(vertices, 12 * vfCount, 1) != 1) {
				throw runtime_error("error reading mesh, insufficient vertex data.");
			}
			//cout << "reading " << (vfCount * 12) << " bytes of normal data.\n";
			if (f->read(normals, 12 * vfCount, 1) != 1) {
				throw runtime_error("error reading mesh, insufficient normal vector data.");
			}
		}
	}

	if (meshHeader.textures && f->read(texCoords, sizeof(Vec2f)*vertexCount, 1) != 1) {
		throw runtime_error("error reading mesh, insufficient texture co-ordinate data.");
	}
	if (indexCount) {
		if (f->read(indices, sizeof(uint32)*indexCount, 1) != 1) {
			throw runtime_error("error reading mesh, insufficient vertex index data.");
		}
	}
	if (textures[mtNormal]) {
		computeTangents();
	}
	fillBuffers();
}

void Mesh::buildCube(int size, int height, Texture2D *tex) {
	frameCount = 1;
	vertexCount = 5 * 4;
	indexCount = 5 * 6;

	vertArrays = new Vec3f*[frameCount];
	normArrays = new Vec3f*[frameCount];
	vertArrays[0] = allocate_aligned_vec3_array(vertexCount);
	normArrays[0] = allocate_aligned_vec3_array(vertexCount);
	texCoords = new Vec2f[vertexCount];
	indices = new uint32[indexCount];

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
	vertArrays[0][0] = Vec3f(-xzDist, yDist, -xzDist);
	vertArrays[0][1] = Vec3f(xzDist,  yDist, -xzDist);
	vertArrays[0][2] = Vec3f(-xzDist, yDist, xzDist);
	vertArrays[0][3] = Vec3f(xzDist,  yDist, xzDist);
	for (; n < 4; ++n) {
		normArrays[0][n] = up;
		texCoords[n] = tCoord[n % 4];
	}
	indices[0] = 2;
	indices[1] = 1;
	indices[2] = 0;
	indices[3] = 1;
	indices[4] = 2;
	indices[5] = 3;

	// 'north' face
	vertArrays[0][4] = Vec3f(-xzDist,	0.f, xzDist);
	vertArrays[0][5] = Vec3f(-xzDist, yDist, xzDist);
	vertArrays[0][6] = Vec3f(xzDist,	0.f, xzDist);
	vertArrays[0][7] = Vec3f(xzDist,  yDist, xzDist);
	for (; n < 8; ++n) {
		normArrays[0][n] = north;
		texCoords[n] = tCoord[n % 4];
	}
	indices[6]  = 6;
	indices[7]  = 5;
	indices[8]  = 4;
	indices[9]  = 5;
	indices[10] = 6;
	indices[11] = 7;

	// 'east' face
	vertArrays[0][8] = Vec3f(xzDist,    0.f, xzDist);
	vertArrays[0][9] = Vec3f(xzDist,  yDist, xzDist);
	vertArrays[0][10] = Vec3f(xzDist,   0.f, -xzDist);
	vertArrays[0][11] = Vec3f(xzDist, yDist, -xzDist);
	for (; n < 12; ++n) {
		normArrays[0][n] = east;
		texCoords[n] = tCoord[n % 4];
	}
	indices[12] = 10;
	indices[13] = 9;
	indices[14] = 8;
	indices[15] = 9;
	indices[16] = 10;
	indices[17] = 11;

	// south
	vertArrays[0][12] = Vec3f(xzDist,	 0.f, -xzDist);
	vertArrays[0][13] = Vec3f(xzDist, yDist, -xzDist);
	vertArrays[0][14] = Vec3f(-xzDist,	 0.f, -xzDist);
	vertArrays[0][15] = Vec3f(-xzDist,  yDist, -xzDist);
	for (; n < 16; ++n) {
		normArrays[0][n] = south;
		texCoords[n] = tCoord[n % 4];
	}
	indices[18] = 14;
	indices[19] = 13;
	indices[20] = 12;
	indices[21] = 13;
	indices[22] = 14;
	indices[23] = 15;

	// west
	vertArrays[0][16] = Vec3f(-xzDist,    0.f, -xzDist);
	vertArrays[0][17] = Vec3f(-xzDist,  yDist, -xzDist);
	vertArrays[0][18] = Vec3f(-xzDist,    0.f, xzDist);
	vertArrays[0][19] = Vec3f(-xzDist,  yDist, xzDist);
	for (; n < 20; ++n) {
		normArrays[0][n] = west;
		texCoords[n] = tCoord[n % 4];
	}
	indices[24] = 18;
	indices[25] = 17;
	indices[26] = 16;
	indices[27] = 17;
	indices[28] = 18;
	indices[29] = 19;

	this->textures[mtDiffuse] = tex;
	this->customColor = false;
	this->specularColor = this->diffuseColor = Vec3f(0.5f, 0.5f, 0.5f);
	this->specularPower = 0.5f;
	this->opacity = 1.f;
	this->twoSided = false;
}

void Mesh::computeTangents() {
	delete [] tangents;
	tangents= new Vec3f[vertexCount];
	for(unsigned int i=0; i<vertexCount; ++i){
		tangents[i]= Vec3f(0.f);
	}

	Vec3f *verts = vertices ? vertices : vertArrays[0];

	for(unsigned int i=0; i<indexCount; i+=3){
		for(int j=0; j<3; ++j){
			uint32 i0= indices[i+j];
			uint32 i1= indices[i+(j+1)%3];
			uint32 i2= indices[i+(j+2)%3];

			Vec3f p0 = verts[i0];
			Vec3f p1 = verts[i1];
			Vec3f p2 = verts[i2];

			float u0= texCoords[i0].x;
			float u1= texCoords[i1].x;
			float u2= texCoords[i2].x;

			float v0= texCoords[i0].y;
			float v1= texCoords[i1].y;
			float v2= texCoords[i2].y;

			tangents[i0]+=
				((p2-p0)*(v1-v0)-(p1-p0)*(v2-v0))/
				((u2-u0)*(v1-v0)-(u1-u0)*(v2-v0));
		}
	}

	for(unsigned int i=0; i<vertexCount; ++i){
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

	cout << "loading G3D from " << path << endl;

	// file header
	FileHeader fileHeader;
	f->read(&fileHeader, sizeof(FileHeader), 1);
	if (strncmp(reinterpret_cast<char*>(fileHeader.id), "G3D", 3) != 0) {
		cout << "Error: Bad magic cookie. Not a valid G3D model.\n";
		throw runtime_error("Bad magic. Not a valid G3D model");
	}
	fileVersion = fileHeader.version;

	if (fileHeader.version == 4) { // version 4
		cout << "\tVersion: 4\n";
		// model header
		ModelHeader modelHeader;
		f->read(&modelHeader, sizeof(ModelHeader), 1);
		meshCount = modelHeader.meshCount;
		if (modelHeader.type != mtMorphMesh) {
			cout << "\tError: invalid mesh type.\n";
			throw runtime_error("Invalid model type");
		}
		cout << "\tMesh count: " << meshCount << endl;

		//load meshes
		meshes = new Mesh[meshCount];
		for(uint32 i=0; i < meshCount; ++i){
			meshes[i].load(dir, f.get(), textureManager);
			meshes[i].buildInterpolationData();

			cout << "\tLoaded mesh " << i << endl;
			cout << "\t\tVertex count: " << meshes[i].getVertexCount() << endl;
			cout << "\t\tFrame count: " << meshes[i].getFrameCount() << endl;
		}
	} else if (fileHeader.version == 3) { // version 3
		cout << "\tVersion: 3\n";
		f->read(&meshCount, sizeof(meshCount), 1);
		cout << "\tMesh count: " << meshCount << endl;
		meshes= new Mesh[meshCount];
		for(uint32 i=0; i < meshCount; ++i){
			meshes[i].loadV3(dir, f.get(), textureManager);
			meshes[i].buildInterpolationData();

			cout << "\tLoaded mesh " << i << endl;
			cout << "\t\tVertex count: " << meshes[i].getVertexCount() << endl;
			cout << "\t\tFrame count: " << meshes[i].getFrameCount() << endl;
		}
	} else {
		cout << "\tError: Invalid version: " << fileHeader.version << "\n";
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
