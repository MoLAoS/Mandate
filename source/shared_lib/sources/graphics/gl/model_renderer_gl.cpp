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
#include "model_renderer_gl.h"

#include "opengl.h"
#include "gl_wrap.h"
#include "texture_gl.h"
#include "interpolation.h"

#include "leak_dumper.h"


using namespace Shared::Platform;

namespace Shared { namespace Graphics { namespace Gl {

ShaderSet::ShaderSet(const string &programName)
		: m_name(programName)
		, m_teamColour(0)
		, m_alphaColour(0) {
}

ShaderSet::~ShaderSet() {
	delete m_teamColour;
	delete m_alphaColour;
}

bool ShaderSet::load() {
	try {
		string vs = "gae/shaders/" + m_name + ".vert";
		string fs = "gae/shaders/" + m_name + "_team.frag";
		m_teamColour = new DefaultShaderProgram();
		m_teamColour->load(vs, fs);

		fs = "gae/shaders/" + m_name + "_alpha.frag";
		m_alphaColour = new DefaultShaderProgram();
		m_alphaColour->load(vs, fs);

		m_teamColour->begin();
		m_teamColour->setUniform("baseTexture", 0);
		m_teamColour->setUniform("teamTexture", 1);
		m_teamColour->setUniform("normalMap", 2);
		m_teamColour->end();
		m_alphaColour->begin();
		m_alphaColour->setUniform("baseTexture", 0);
		m_alphaColour->setUniform("normalMap", 2);
		m_alphaColour->end();
	} catch (runtime_error &e) {
		delete m_teamColour;
		delete m_alphaColour;
		m_teamColour = m_alphaColour = 0;
		return false;
	}
	return true;
}

// =====================================================
//	class ModelRendererGl
// =====================================================

// ===================== PUBLIC ========================

ModelRendererGl::ModelRendererGl()
		: ModelRenderer()
		, rendering(false)
		, duplicateTexCoords(false)
		, shaderOverride(false)
		, secondaryTexCoordUnit(1)
		, lastTexture()
		, m_shaderIndex(-1)
		, m_lastShaderProgram(0) {
	m_fixedFunctionProgram = new FixedFunctionProgram();
}

ModelRendererGl::~ModelRendererGl() {
	foreach_const (ShaderSets, it, m_shaders) {
		delete *it;
	}
	delete m_fixedFunctionProgram;
}

void ModelRendererGl::loadShaders(const vector<string> &programNames) {
	if (isGlVersionSupported(2, 0, 0)) {
		foreach_const (vector<string>, it, programNames) {
			ShaderSet *shaderSet = new ShaderSet(*it);
			if (shaderSet->load()) {
				m_shaders.push_back(shaderSet);
			} else {
				delete shaderSet;
			}
		}
	}
	if (!m_shaders.empty()) {
		m_shaderIndex = 0; // use first in list
	}
}

void ModelRendererGl::cycleShaderSet() {
	++m_shaderIndex;
	if (m_shaderIndex >= m_shaders.size()) {
		m_shaderIndex = -1;
	}
}

const string fixedPipeString = "Fixed function pipeline.";

const string& ModelRendererGl::getShaderName() {
	if (m_shaderIndex == -1) {
		return fixedPipeString;
	}
	return m_shaders[m_shaderIndex]->getName();
}

void ModelRendererGl::begin(bool renderNormals, bool renderTextures, bool renderColors, MeshCallback *meshCallback) {
	assert(!rendering);
	assertGl();

	this->renderTextures = renderTextures;
	this->renderNormals = renderNormals;
	this->renderColors = renderColors;
	this->meshCallback = meshCallback;

	rendering = true;
	m_lastShaderProgram = 0;
	lastTexture = 0;
	glBindTexture(GL_TEXTURE_2D, 0);

	//push attribs
	glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	//init opengl
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glFrontFace(GL_CCW);
//	glEnable(GL_NORMALIZE); // we don't scale or shear, don't need this

	glEnableClientState(GL_VERTEX_ARRAY);

	if (renderNormals) {
		glEnableClientState(GL_NORMAL_ARRAY);
	}

	if (renderTextures) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}	

	//assertions
	assertGl();
}


void ModelRendererGl::end() {
	// assertions
	assert(rendering);
	assertGl();

	// set render state
	rendering = false;

	// restore stuff
	if (m_lastShaderProgram) {
		m_lastShaderProgram->end();
	}
	glPopAttrib();
	glPopClientAttrib();

	// assertions
	assertGl();
}

void ModelRendererGl::render(const Model *model, Vec3f *anim, ShaderProgram *customProgram) {
	//assertions
	assert(rendering);
	assertGl();

	//render every mesh
	for (uint32 i = 0; i < model->getMeshCount(); ++i) {
		renderMesh(model->getMesh(i), anim, customProgram);
	}

	//assertions
	assertGl();
}

void ModelRendererGl::renderNormalsOnly(const Model *model) {
	//assertions
	assert(rendering);
	assertGl();

	//render every mesh
	for (uint32 i = 0; i < model->getMeshCount(); ++i) {
		renderMeshNormals(model->getMesh(i));
	}

	//assertions
	assertGl();
}

void ModelRendererGl::renderMeshNormalsOnly(const Mesh *mesh) {
	renderMeshNormals(mesh);
}
// ===================== PRIVATE =======================

float remap(float in, const float oldMin, const float oldMax, const float newMin, const float newMax) {
	float t = (in - oldMin) / (oldMax - oldMin);
	return newMin + t * (newMax - newMin);
}

void ModelRendererGl::renderMesh(const Mesh *mesh, Vec3f *anim, ShaderProgram *customProgram) {

	//assertions
	assertGl();

	//set cull face
	if (mesh->getTwoSided()){
		glDisable(GL_CULL_FACE);
	} else {
		glEnable(GL_CULL_FACE);
	}

	if (renderColors) {
		Vec4f color(mesh->getDiffuseColor(), mesh->getOpacity());
		glColor4fv(color.ptr());
	}

	const Texture2DGl *texture = 0, *textureNormal = 0, *textureCustom = 0;

	// diffuse texture
	glActiveTexture(diffuseTextureUnit);
	texture = static_cast<const Texture2DGl*>(mesh->getTexture(mtDiffuse));
	if (texture != NULL && renderTextures) {
		if (lastTexture != texture->getHandle()) {
			assert(glIsTexture(texture->getHandle()));
			glBindTexture(GL_TEXTURE_2D, texture->getHandle());
			lastTexture = texture->getHandle();
		}
	} else {
		glBindTexture(GL_TEXTURE_2D, 0);
		lastTexture = 0;
	}
	
	// bump map
	textureNormal = static_cast<const Texture2DGl*>(mesh->getTexture(mtNormal));
	glActiveTexture(normalTextureUnit);
	if (textureNormal != NULL && renderTextures) {
		assert(glIsTexture(textureNormal->getHandle()));
		glBindTexture(GL_TEXTURE_2D, textureNormal->getHandle());
	} else {
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// custom map
	if (customProgram && renderTextures) {
		textureCustom = static_cast<const Texture2DGl*>(mesh->getTexture(mtCustom1));
		glActiveTexture(customTextureUnit);
		if (textureCustom) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, textureCustom->getHandle());
		} else {
			glDisable(GL_TEXTURE_2D);
		}
	}
	glActiveTexture(diffuseTextureUnit);

	//misc vars
	uint32 vertexCount = mesh->getVertexCount();
	uint32 indexCount = mesh->getIndexCount();

	//assertions
	assertGl();

	if (!vertexCount) {
		return;
	}

	ShaderProgram *shaderProgram;
	if (m_shaderIndex == -1 || !meshCallback) {
		// (!meshCallback) == hacky way to not use shaders for tileset objects and in menu... for now
		shaderProgram = m_fixedFunctionProgram;
		if (meshCallback) {
			meshCallback->execute(mesh);
		}
	} else {
		if (customProgram) {
			shaderProgram = customProgram;
		} else  if (mesh->getCustomTexture()) {
			shaderProgram = m_shaders[m_shaderIndex]->getTeamProgram();
		} else {
			shaderProgram = m_shaders[m_shaderIndex]->getAlphaProgram();
		}
	}
	if (shaderProgram != m_lastShaderProgram) {
		if (m_lastShaderProgram) {
			m_lastShaderProgram->end();
		}
		shaderProgram->begin();
		m_lastShaderProgram = shaderProgram;
	}
	///@todo would be better to do this once only per faction, set from the game somewhere/somehow
	shaderProgram->setUniform("teamColour", getTeamColour());
	if (customProgram && anim) {
		shaderProgram->setUniform("baseTexture", 0);
		shaderProgram->setUniform("customTex", 3);
		float anim_r = remap(anim->r, 0.f, 1.f, 0.15f, 1.f);
		float anim_g = remap(anim->g, 0.f, 1.f, 0.15f, 1.f);
		float anim_b = remap(anim->b, 0.f, 1.f, 0.15f, 1.f);
		shaderProgram->setUniform("time", Vec3f(anim_r, anim_g, anim_b));///@todo
	}

	//vertices
	if (mesh->getVertexBuffer()) {
#		define OFFSET(x) ((void*)(x * sizeof(float)))
		const int stride = 32;
		glBindBuffer(GL_ARRAY_BUFFER, mesh->getVertexBuffer());
		glVertexPointer(3, GL_FLOAT, stride, OFFSET(0));
		//normals
		if (renderNormals) {
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, stride, OFFSET(3));
		} else {
			glDisableClientState(GL_NORMAL_ARRAY);
		}
		//tex coords
		if (renderTextures && mesh->getTexture(mtDiffuse) != NULL) {
			if (duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + secondaryTexCoordUnit);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, stride, OFFSET(6));
			}

			glActiveTexture(GL_TEXTURE0);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, stride, OFFSET(6));
		} else {
			if (duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + secondaryTexCoordUnit);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			glActiveTexture(GL_TEXTURE0);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->getIndexBuffer());

		//draw model
		glDrawRangeElements(GL_TRIANGLES, 0, vertexCount-1, indexCount, GL_UNSIGNED_INT, OFFSET(0));
#		undef OFFSET
	} else {
		glVertexPointer(3, GL_FLOAT, 0, mesh->getInterpolationData()->getVertices());
		//normals
		if (renderNormals) {
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, mesh->getInterpolationData()->getNormals());
		} else {
			glDisableClientState(GL_NORMAL_ARRAY);
		}
		//tex coords
		if (renderTextures && mesh->getTexture(mtDiffuse) != NULL) {
			if (duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + secondaryTexCoordUnit);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 0, mesh->getTexCoords());
			}

			glActiveTexture(GL_TEXTURE0);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, mesh->getTexCoords());
		} else {
			if (duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + secondaryTexCoordUnit);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			glActiveTexture(GL_TEXTURE0);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		//draw model
		glDrawRangeElements(GL_TRIANGLES, 0, vertexCount-1, indexCount, GL_UNSIGNED_INT, mesh->getIndices());
	}

	//assertions
	assertGl();
}

void ModelRendererGl::renderMeshNormals(const Mesh *mesh) {
	glBegin(GL_LINES);
	for (int i = 0; i < mesh->getIndexCount(); ++i) {
		Vec3f vertex = mesh->getInterpolationData()->getVertices()[mesh->getIndices()[i]];
		Vec3f normal = vertex + mesh->getInterpolationData()->getNormals()[mesh->getIndices()[i]];

		glVertex3fv(vertex.ptr());
		glVertex3fv(normal.ptr());
	}
	glEnd();
}

}}}//end namespace
