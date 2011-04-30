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
#include "model_renderer_gl.h"

#include "opengl.h"
#include "gl_wrap.h"
#include "texture_gl.h"
#include "interpolation.h"
#include "opengl.h"

#include "leak_dumper.h"

using namespace Shared::Platform;

namespace Shared { namespace Graphics { namespace Gl {

// =====================================================
//	class ModelRendererGl
// =====================================================

// ===================== PUBLIC ========================

ModelRendererGl::ModelRendererGl()
		: ModelRenderer()
		, m_rendering(false)
		, m_duplicateTexCoords(false)
		//, m_shaderOverride(false)
		, m_secondaryTexCoordUnit(1)
		, m_lastTexture()
		, m_alphaThreshold(0.f)
		, m_currentLightCount(1)
		, m_shaderIndex(-1)
		, m_lastShaderProgram(0) {
	m_fixedFunctionProgram = new FixedPipeline();
}

ModelRendererGl::~ModelRendererGl() {
	foreach_const (UnitShaderSets, it, m_shaders) {
		delete *it;
	}
	delete m_fixedFunctionProgram;
}

void ModelRendererGl::loadShaders(const vector<string> &setNames) {
	if (isGlVersionSupported(2, 0, 0)) {
		foreach_const (vector<string>, it, setNames) {
			string path = "gae/shaders/" + *it + ".xml";
			UnitShaderSet *shaderSet = 0;
			try {
				shaderSet = new UnitShaderSet(path);
				m_shaders.push_back(shaderSet);
			} catch (runtime_error &e) {
				mediaErrorLog.add(e.what(), path);
				delete shaderSet;
			}
		}
	}
	if (!m_shaders.empty()) {
		m_shaderIndex = 0; // use first in list
	}
}

void ModelRendererGl::loadShader(const string &name) {
	if (!m_shaders.empty()) { // delete last (check if changed??)
		foreach (UnitShaderSets, it, m_shaders) {
			delete *it;
		}
		m_shaders.clear();
	}
	if (name.empty()) {
		m_shaderIndex = -1;
		return;
	}
	if (isGlVersionSupported(2, 0, 0)) {
		string path = "gae/shaders/" + name + ".xml";
		UnitShaderSet *shaderSet = 0;
		try {
			shaderSet = new UnitShaderSet(path);
			m_shaders.push_back(shaderSet);
		} catch (runtime_error &e) {
			mediaErrorLog.add(e.what(), path);
			delete shaderSet;
		}
	}
	if (!m_shaders.empty()) {
		m_shaderIndex = 0; // use if loaded ok
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

void ModelRendererGl::begin(RenderMode mode, bool fog, MeshCallback *meshCallback) {
	assert(!m_rendering);
	assertGl();

	m_renderMode = mode;
	m_useFog = fog;
	m_meshCallback = meshCallback;

	m_rendering = true;
	m_lastShaderProgram = 0;
	m_lastTexture = 0;
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

	if (mode == RenderMode::UNITS || mode == RenderMode::OBJECTS) {
		glEnableClientState(GL_NORMAL_ARRAY);
	}

	if (mode == RenderMode::UNITS || mode == RenderMode::OBJECTS || mode == RenderMode::SHADOWS) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}	

	//assertions
	assertGl();
}


void ModelRendererGl::end() {
	// assertions
	assert(m_rendering);
	assertGl();

	// set render state
	m_rendering = false;

	// restore stuff
	if (m_lastShaderProgram) {
		m_lastShaderProgram->end();
	}
	glPopAttrib();
	glPopClientAttrib();

	// assertions
	assertGl();
}

void ModelRendererGl::render(const Model *model, float fade, int frame, int id, UnitShaderSet *customShaders) {
	//assertions
	assert(m_rendering);
	assertGl();

	//render every mesh
	for (uint32 i = 0; i < model->getMeshCount(); ++i) {
		renderMesh(model->getMesh(i), fade, frame, id, customShaders);
	}

	//assertions
	assertGl();
}

void ModelRendererGl::renderNormalsOnly(const Model *model) {
	//assertions
	assert(m_rendering);
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

void ModelRendererGl::renderMesh(const Mesh *mesh, float fade, int frame, int id, UnitShaderSet *customShaders) {
	//assertions
	assertGl();

	const RenderMode &mode = m_renderMode;
	if (mode == RenderMode::SELECTION && mesh->isNoSelect()) {
		return;
	}

	bool renderTextures = (mode == RenderMode::UNITS || mode == RenderMode::OBJECTS || mode == RenderMode::SHADOWS);
	bool sendNormals = (mode == RenderMode::UNITS || mode == RenderMode::OBJECTS);

	// set cull face
	if (mesh->isTwoSided()) {
		glDisable(GL_CULL_FACE);
	} else {
		glEnable(GL_CULL_FACE);
	}

	// mesh colour (units only, tileset objects use colour set by engine based on FoW tex)
	if (m_renderMode == RenderMode::UNITS) {
		Vec4f color(mesh->getDiffuseColor(), std::min(mesh->getOpacity(), fade));
		glColor4fv(color.ptr());
	}

	const Texture2DGl *texture = 0, *textureNormal = 0, *textureCustom = 0;

	// diffuse texture
	glActiveTexture(diffuseTextureUnit);
	texture = static_cast<const Texture2DGl*>(mesh->getTexture(MeshTexture::DIFFUSE));
	if (texture != NULL && renderTextures) {
		if (m_lastTexture != texture->getHandle()) {
			assert(glIsTexture(texture->getHandle()));
			glBindTexture(GL_TEXTURE_2D, texture->getHandle());
			m_lastTexture = texture->getHandle();
		}
	} else {
		glBindTexture(GL_TEXTURE_2D, 0);
		m_lastTexture = 0;
	}
	
	// bump map
	textureNormal = static_cast<const Texture2DGl*>(mesh->getTexture(MeshTexture::NORMAL));
	glActiveTexture(normalTextureUnit);
	if (textureNormal != NULL && sendNormals) {
		assert(glIsTexture(textureNormal->getHandle()));
		glBindTexture(GL_TEXTURE_2D, textureNormal->getHandle());
	} else {
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	// custom map
	if (customShaders && renderTextures) {
		textureCustom = static_cast<const Texture2DGl*>(mesh->getTexture(MeshTexture::CUSTOM));
		glActiveTexture(custom1TextureUnit);
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
	if (m_shaderIndex == -1 || mode == RenderMode::SELECTION || mode == RenderMode::SHADOWS) {
		shaderProgram = m_fixedFunctionProgram;
		if (m_meshCallback) {
			m_meshCallback->execute(mesh);
		}
	} else {
		if (customShaders) {
			if (mesh->usesTeamTexture()) {
				shaderProgram = customShaders->getTeamProgram();
			} else {
				shaderProgram = customShaders->getRgbaProgram();
			}
		} else if (mesh->usesTeamTexture()) {
			shaderProgram = m_shaders[m_shaderIndex]->getTeamProgram();
		} else {
			shaderProgram = m_shaders[m_shaderIndex]->getRgbaProgram();
		}
	}
	if (shaderProgram != m_lastShaderProgram) {
		if (m_lastShaderProgram) {
			m_lastShaderProgram->end();
		}
		shaderProgram->begin();
		shaderProgram->setUniform("gae_IsUsingFog", GLuint(m_useFog));
		m_lastShaderProgram = shaderProgram;
	}
	///@todo would be better to do this once only per faction, set from the game somewhere/somehow
	shaderProgram->setUniform("gae_TeamColour", getTeamColour());
	shaderProgram->setUniform("gae_AlphaThreshold", m_alphaThreshold);
	shaderProgram->setUniform("gae_LightCount", m_currentLightCount);
	if (customShaders) {
		shaderProgram->setUniform("gae_FrameNumber", frame);
		shaderProgram->setUniform("gae_EntityId", id);
	}

	// vertices
	if (mesh->getVertexBuffer()) {
#		define OFFSET(x) ((void*)(x * sizeof(float)))
		const int stride = 32;
		glBindBuffer(GL_ARRAY_BUFFER, mesh->getVertexBuffer());
		glVertexPointer(3, GL_FLOAT, stride, OFFSET(0));
		// normals
		if (sendNormals) {
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, stride, OFFSET(3));
		} else {
			glDisableClientState(GL_NORMAL_ARRAY);
		}
		// tex coords
		if (renderTextures && mesh->getTexture(MeshTexture::DIFFUSE) != NULL) {
			if (m_duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + m_secondaryTexCoordUnit);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, stride, OFFSET(6));
			}

			glActiveTexture(GL_TEXTURE0);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, stride, OFFSET(6));
		} else {
			if (m_duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + m_secondaryTexCoordUnit);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			glActiveTexture(GL_TEXTURE0);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->getIndexBuffer());
		if (sendNormals && textureNormal) {
			shaderProgram->setUniform("gae_HasNormalMap", 1u);
			int loc = shaderProgram->getAttribLoc("gae_Tangent");
			if (loc != -1) {
				assert(mesh->getTangents());
				glEnableVertexAttribArray(loc);
				glVertexAttribPointer(loc, 3, GL_FLOAT, GL_TRUE, 0, mesh->getTangents());
			}
		} else {
			shaderProgram->setUniform("gae_HasNormalMap", 0u);
		}

		// draw model
		glDrawRangeElements(GL_TRIANGLES, 0, vertexCount-1, indexCount, GL_UNSIGNED_INT, OFFSET(0));
#		undef OFFSET
	} else {
		glVertexPointer(3, GL_FLOAT, 0, mesh->getInterpolationData()->getVertices());
		// normals
		if (sendNormals) {
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, mesh->getInterpolationData()->getNormals());
		} else {
			glDisableClientState(GL_NORMAL_ARRAY);
		}
		// tex coords
		if (renderTextures && mesh->getTexture(MeshTexture::DIFFUSE) != NULL) {
			if (m_duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + m_secondaryTexCoordUnit);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 0, mesh->getTexCoords());
			}

			glActiveTexture(GL_TEXTURE0);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, mesh->getTexCoords());
		} else {
			if (m_duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + m_secondaryTexCoordUnit);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			glActiveTexture(GL_TEXTURE0);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		if (sendNormals && textureNormal) {
			shaderProgram->setUniform("gae_HasNormalMap", 1u);
			int loc = shaderProgram->getAttribLoc("gae_Tangent");
			if (loc != -1) {
				glEnableVertexAttribArray(loc);
				glVertexAttribPointer(loc, 3, GL_FLOAT, GL_TRUE, 0, mesh->getTangents());
			}
		} else {
			shaderProgram->setUniform("gae_HasNormalMap", 0u);
		}
		// draw model
		glDrawRangeElements(GL_TRIANGLES, 0, vertexCount-1, indexCount, GL_UNSIGNED_INT, mesh->getIndices());
	}
	assertGl();
}

void ModelRendererGl::setAlphaThreshold(float a) {
	if (a < 0.01f) {
		m_alphaThreshold = 0.f;
		glDisable(GL_ALPHA_TEST);
	} else {
		m_alphaThreshold = a;
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, m_alphaThreshold);
	}
}

void ModelRendererGl::setLightCount(int n) {
	m_currentLightCount = n;
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
