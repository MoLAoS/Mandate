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

#include "pch.h"
#include "model.h"

#include <cstdio>
#include <cassert>
#include <stdexcept>

#include "interpolation.h"
#include "conversion.h"
#include "util.h"

#include "leak_dumper.h"

#include "FSFactory.hpp"

using std::exception;
using namespace Shared::Platform;

namespace Shared{ namespace Graphics{

using namespace Util;

bool use_simd_interpolation;

Vec3f* allocate_aligned_vec3_array(unsigned n) {
	int numFloat = n * 3;
	int padFloat = (numFloat % 4 == 0 ? 0 : 4 - (numFloat % 4));
	int bytes = (numFloat + padFloat) * 4;
	assert(bytes % 16 == 0);
#ifdef _MSC_VER
	void *res = _aligned_malloc(bytes, 16);
#else
	void *res = _mm_malloc(bytes, 16);
#endif
	return static_cast<Vec3f*>(res);
}

void free_aligned_vec3_array(Vec3f *ptr) {
#ifdef _MSC_VER
	_aligned_free(ptr);
#else
	_mm_free(ptr);
#endif
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

	for (int i = 0; i < meshTextureCount; ++i) {
		textures[i] = NULL;
	}

	twoSided = false;
	customColor = false;
}

Mesh::~Mesh() {
	end();
}

void Mesh::init() {
	if (use_simd_interpolation) {
		vertArrays = new Vec3f*[frameCount];
		normArrays = new Vec3f*[frameCount];
		for (int i=0; i < frameCount; ++i) {
			vertArrays[i] = allocate_aligned_vec3_array(vertexCount);
		}
		for (int i=0; i < frameCount; ++i) {
			normArrays[i] = allocate_aligned_vec3_array(vertexCount);
		}
	} else {
		vertices = new Vec3f[frameCount * vertexCount];
		normals = new Vec3f[frameCount * vertexCount];
	}
	texCoords = new Vec2f[vertexCount];
	indices = new uint32[indexCount];
}

void Mesh::end() {
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

// ========================== shadows & interpolation =========================

void Mesh::buildInterpolationData(){
	interpolationData = new InterpolationData(this);
}

void Mesh::updateInterpolationData(float t, bool cycle) const{
	interpolationData->update(t, cycle);
}

void Mesh::updateInterpolationVertices(float t, bool cycle) const{
	interpolationData->updateVertices(t, cycle);
}

// ==================== load ====================

void Mesh::loadV2(const string &dir, FileOps *f, TextureManager *textureManager) {
#if 0
	//read header
	MeshHeaderV2 meshHeader;
	f->read(&meshHeader, sizeof(MeshHeaderV2), 1);


	if(meshHeader.normalFrameCount!=meshHeader.vertexFrameCount){
		throw runtime_error("Old model: vertex frame count different from normal frame count");
	}

	if(meshHeader.texCoordFrameCount!=1){
		throw runtime_error("Old model: texture coord frame count is not 1");
	}

	//init
	frameCount= meshHeader.vertexFrameCount;
	vertexCount= meshHeader.pointCount;
	indexCount= meshHeader.indexCount;

	init();

	//misc
	twoSided= false;
	customColor= false;

	//texture
	if(meshHeader.hasTexture && textureManager!=NULL){
		texturePaths[mtDiffuse]= toLower(reinterpret_cast<char*>(meshHeader.texName));
		string texPath= dir+"/"+texturePaths[mtDiffuse];

		textures[mtDiffuse]= static_cast<Texture2D*>(textureManager->getTexture(texPath));
		if(textures[mtDiffuse]==NULL){
			textures[mtDiffuse]= textureManager->newTexture2D();
			textures[mtDiffuse]->load(texPath);
		}
	}

	//read data
	size_t vfCount = frameCount * vertexCount;
	f->read(vertices, sizeof(Vec3f)*vfCount, 1);
	f->read(normals, sizeof(Vec3f)*vfCount, 1);

	if(textures[mtDiffuse]!=NULL){
		f->read(texCoords, sizeof(Vec2f)*vertexCount, 1);
	}
	f->read(&diffuseColor, sizeof(Vec3f), 1);
	f->read(&opacity, sizeof(float32), 1);
	f->seek(sizeof(Vec4f)*(meshHeader.colorFrameCount-1), SEEK_CUR);
	f->read(indices, sizeof(uint32)*indexCount, 1);
#endif
}

void Mesh::loadV3(const string &dir, FileOps *f, TextureManager *textureManager){
	//read header
	MeshHeaderV3 meshHeader;
	f->read(&meshHeader, sizeof(MeshHeaderV3), 1);


	if(meshHeader.normalFrameCount!=meshHeader.vertexFrameCount){
		throw runtime_error("Old model: vertex frame count different from normal frame count");
	}

	//init
	frameCount= meshHeader.vertexFrameCount;
	vertexCount= meshHeader.pointCount;
	indexCount= meshHeader.indexCount;

	init();

	//misc
	twoSided= (meshHeader.properties & mp3TwoSided) != 0;
	customColor= (meshHeader.properties & mp3CustomColor) != 0;

	//texture
	if(!(meshHeader.properties & mp3NoTexture) && textureManager!=NULL){
		texturePaths[mtDiffuse]= toLower(reinterpret_cast<char*>(meshHeader.texName));
		string texPath= dir+"/"+texturePaths[mtDiffuse];

		textures[mtDiffuse]= static_cast<Texture2D*>(textureManager->getTexture(texPath));
		if(textures[mtDiffuse]==NULL){
			textures[mtDiffuse]= textureManager->newTexture2D();
			textures[mtDiffuse]->load(texPath);
		}
	}

	//read data
	if (use_simd_interpolation) {
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
}

void Mesh::load(const string &dir, FileOps *f, TextureManager *textureManager){
	//read header
	MeshHeader meshHeader;
	if (f->read(&meshHeader, sizeof(MeshHeader), 1) != 1) {
		throw runtime_error("Could not read mesh header");
	}

	//init
	frameCount= meshHeader.frameCount;
	vertexCount= meshHeader.vertexCount;
	indexCount= meshHeader.indexCount;

	init();

	//properties
	customColor= (meshHeader.properties & mpfCustomColor) != 0;
	twoSided= (meshHeader.properties & mpfTwoSided) != 0;

	//material
	diffuseColor= Vec3f(meshHeader.diffuseColor);
	specularColor= Vec3f(meshHeader.specularColor);
	specularPower= meshHeader.specularPower;
	opacity= meshHeader.opacity;

	//maps
	uint32 flag= 1;
	for(int i=0; i<meshTextureCount; ++i){
		if((meshHeader.textures & flag) && textureManager!=NULL){
			uint8 cMapPath[mapPathSize];
			f->read(cMapPath, mapPathSize, 1);
			string mapPath= toLower(reinterpret_cast<char*>(cMapPath));

			string mapFullPath= dir + "/" + mapPath;

			textures[i]= static_cast<Texture2D*>(textureManager->getTexture(mapFullPath));
			if(textures[i]==NULL){
				textures[i]= textureManager->newTexture2D();
				if(meshTextureChannelCount[i]!=-1){
					textures[i]->getPixmap()->init(meshTextureChannelCount[i]);
				}
				textures[i]->load(mapFullPath);
			}
		}
		flag*= 2;
	}

	// Assume packed vectors.
	//read data
	if (use_simd_interpolation) {
		assert(sizeof(Vec3f) == 12);
		int frameRead = sizeof(Vec3f) * vertexCount;
		int nFloats = vertexCount * 3;
		int framePad = nFloats % 4 == 0 ? 0 : 4 - (nFloats % 4);

		for (int i=0; i < frameCount; ++i) {
			if (f->read(vertArrays[i], frameRead, 1) != 1) {
				//cout << "read() Failed! getLastError() == " << f->getLastError() << endl;
			}
			float *ptr = vertArrays[i][vertexCount].raw;
			for (int j=0; j < framePad; ++j) {
				*ptr++ = 0.f;
			}
		}
		for (int i=0; i < frameCount; ++i) {
			if (f->read(normArrays[i], frameRead, 1) != 1) {
				//cout << "read() Failed! getLastError() == " << f->getLastError() << endl;
			}
			float *ptr = normArrays[i][vertexCount].raw;
			for (int j=0; j < framePad; ++j) {
				*ptr++ = 0.f;
			}
		}
	} else {
		size_t vfCount = frameCount * vertexCount;
		cout << "reading " << (vfCount * 12) << " bytes of vertex data.\n";
		if (f->read(vertices, 12 * vfCount, 1) != 1) {
			throw runtime_error("error reading mesh, insufficient vertex data.");
		}
		cout << "reading " << (vfCount * 12) << " bytes of normal data.\n";
		if (f->read(normals, 12 * vfCount, 1) != 1) {
			throw runtime_error("error reading mesh, insufficient normal vector data.");
		}
	}

	if (meshHeader.textures && f->read(texCoords, sizeof(Vec2f)*vertexCount, 1) != 1) {
		throw runtime_error("error reading mesh, insufficient texture co-ordinate data.");
	}
	if (f->read(indices, sizeof(uint32)*indexCount, 1) != 1) {
		throw runtime_error("error reading mesh, insufficient vertex index data.");
	}

	//tangents
	if (textures[mtNormal]) {
		computeTangents();
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

void Mesh::computeTangents(){
#if 0
	delete [] tangents;
	tangents= new Vec3f[vertexCount];
	for(int i=0; i<vertexCount; ++i){
		tangents[i]= Vec3f(0.f);
	}

	for(int i=0; i<indexCount; i+=3){
		for(int j=0; j<3; ++j){
			uint32 i0= indices[i+j];
			uint32 i1= indices[i+(j+1)%3];
			uint32 i2= indices[i+(j+2)%3];

			Vec3f p0= vertices[i0];
			Vec3f p1= vertices[i1];
			Vec3f p2= vertices[i2];

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

	for(int i=0; i<vertexCount; ++i){
		/*Vec3f binormal= normals[i].cross(tangents[i]);
		tangents[i]+= binormal.cross(normals[i]);*/
		tangents[i].normalize();
	}
#endif
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

void Model::load(const string &path){
	string extension= path.substr(path.find_last_of('.')+1);
	if(extension=="g3d" || extension=="G3D"){
		loadG3d(path);
	}
	else{
		throw runtime_error("Unknown model format: " + extension);
	}
}

void Model::save(const string &path){
	string extension= path.substr(path.find_last_of('.')+1);
	if(extension=="g3d" ||extension=="G3D" || extension=="s3d" || extension=="S3D"){
		saveS3d(path);
	}
	else{
		throw runtime_error("Unknown model format: " + extension);
	}
}

/*void Model::loadG3dOld(const string &path){
   try{
		FILE *f=fopen(path.c_str(),"rb");
		if (f==NULL){
			throw runtime_error("Error opening 3d model file");
		}

		string dir= dirname(path);

		//read header
		ModelHeaderOld modelHeader;
		fread(&modelHeader, sizeof(ModelHeader), 1, f);
		meshCount= modelHeader.meshCount;

		if(modelHeader.id[0]!='G' || modelHeader.id[1]!='3' || modelHeader.id[2]!='D'){
			throw runtime_error("Model: "+path+": is not a valid G3D model");
		}

		switch(modelHeader.version){
		case 3:{
			meshes= new Mesh[meshCount];
			for(uint32 i=0; i<meshCount; ++i){
				meshes[i].load(dir, f, textureManager);
				meshes[i].buildInterpolationData();
			}
			break;
		}
		default:
			throw runtime_error("Unknown model version");
		}

		fclose(f);
    }
	catch(exception &e){
		throw runtime_error("Exception caught loading 3d file: " + path +"\n"+ e.what());
	}
}*/

//load a model from a g3d file
void Model::loadG3d(const string &path){
    try{
		FileOps *f = FSFactory::getInstance()->getFileOps();
		f->openRead(path.c_str());

		string dir= dirname(path);

		//file header
		FileHeader fileHeader;
		f->read(&fileHeader, sizeof(FileHeader), 1);
		if(strncmp(reinterpret_cast<char*>(fileHeader.id), "G3D", 3)!=0){
			throw runtime_error("Not a valid S3D model");
		}
		fileVersion= fileHeader.version;

		//version 4
		if (fileHeader.version == 4) {

			//model header
			ModelHeader modelHeader;
			f->read(&modelHeader, sizeof(ModelHeader), 1);
			meshCount = modelHeader.meshCount;
			if (modelHeader.type != mtMorphMesh) {
				throw runtime_error("Invalid model type");
			}

			//load meshes
			meshes = new Mesh[meshCount];
			for(uint32 i=0; i < meshCount; ++i){
				meshes[i].load(dir, f, textureManager);
				meshes[i].buildInterpolationData();
			}
		}
		//version 3
		else if (fileHeader.version == 3) {

			f->read(&meshCount, sizeof(meshCount), 1);
			meshes= new Mesh[meshCount];
			for(uint32 i=0; i < meshCount; ++i){
				meshes[i].loadV3(dir, f, textureManager);
				meshes[i].buildInterpolationData();
			}
		}
		//version 2
		else if (fileHeader.version == 2) {

			f->read(&meshCount, sizeof(meshCount), 1);
			meshes= new Mesh[meshCount];
			for(uint32 i=0; i < meshCount; ++i){
				meshes[i].loadV2(dir, f, textureManager);
				meshes[i].buildInterpolationData();
			}
		}
		else{
			throw runtime_error("Invalid model version: "+ intToStr(fileHeader.version));
		}

		delete f;
    }
	catch(exception &e){
		throw runtime_error("Exception caught loading 3d file: " + path +"\n"+ e.what());
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
