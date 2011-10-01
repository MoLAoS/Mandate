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
		, m_teamTintShader(0)
		, m_shaderIndex(-1)
		, m_lastShaderProgram(0) {
	m_fixedFunctionProgram = new FixedPipeline();
	m_perVertexLighting = new GlslShader();
	if (!m_perVertexLighting->load("gae/shaders/per_vert_lighting.vs", ShaderType::VERTEX)) {
		cout << "Error loading gae/shaders/per_vert_lighting.vs\n";
		while (mediaErrorLog.hasError()) {
			MediaErrorLog::ErrorRecord rec = mediaErrorLog.popError();
			cout << "\t" << rec.msg << endl;
		}
	}
}

ModelRendererGl::~ModelRendererGl() {
	foreach_const (GlslPrograms, it, m_shaders) {
		delete *it;
	}
	delete m_fixedFunctionProgram;
	delete m_perVertexLighting;
}

GlslProgram* ModelRendererGl::loadShader(const string &dir, const string &name) {
	if (isGlVersionSupported(2, 0, 0)) {
		string vertPath = dir + "/" + name + ".vs";
		string fragPath = dir + "/" + name + ".fs";
		GlslProgram *shaderProgram = new GlslProgram(name);
		shaderProgram->add(m_perVertexLighting);
		if (!shaderProgram->load(vertPath, true)) {
			mediaErrorLog.add(shaderProgram->getLog(), vertPath);
			delete shaderProgram;
			return 0;
		}
		if (!shaderProgram->load(fragPath, false)) {
			mediaErrorLog.add(shaderProgram->getLog(), fragPath);
			delete shaderProgram;
			return 0;
		}
		if (!shaderProgram->link()) {
			mediaErrorLog.add(shaderProgram->getLog(), dir + name + ".*");
			delete shaderProgram;
			return 0;
		}
		return shaderProgram;
	}
	return 0;
}

void ModelRendererGl::loadShaders(const vector<string> &setNames) {
	if (isGlVersionSupported(2, 0, 0)) {
		foreach_const (vector<string>, it, setNames) {
			GlslProgram *program = loadShader("gae/shaders/", *it);
			if (program) {
				m_shaders.push_back(program);
				initUniformHandles(program);
			}
		}
	}
	if (!m_shaders.empty()) {
		m_shaderIndex = 0; // use first in list
	}
}

void ModelRendererGl::setShader(const string &name) {
	for (int i=0; i < m_shaders.size(); ++i) {
		if (m_shaders[i]->getName() == name) {
			m_shaderIndex = i;
			return;
		}
	}
	GlslProgram *program = loadShader("gae/shaders/", name);
	if (program) {
		m_shaders.push_back(program);
		m_shaderIndex = m_shaders.size() - 1;
		initUniformHandles(program);
	} else {
		m_shaderIndex = -1;
	}
}

void ModelRendererGl::initUniformHandles(ShaderProgram *shader) {
	shader->addUniform("gae_HasNormalMap");
	shader->addUniform("gae_EntityId");
	shader->addUniform("gae_FrameNumber");
	shader->addUniform("gae_LightCount");
	shader->addUniform("gae_AlphaThreshold");
	shader->addUniform("gae_UsesTeamColour");
	shader->addUniform("gae_TeamColour");
	shader->addUniform("gae_IsUsingFog");
}

ShaderProgram* ModelRendererGl::getTeamTintShader() {
	if (!m_teamTintShader) {
		m_teamTintShader = loadShader("gae/shaders/misc_model/", "team-tint");
	}
	return m_teamTintShader;
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

	// push attribs
	glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	// init opengl
	if (m_renderMode < RenderMode::OBJECTS) {
		glDisable(GL_BLEND);
	} else {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	glFrontFace(GL_CCW);

	glEnableClientState(GL_VERTEX_ARRAY);

	if (m_renderMode >= RenderMode::OBJECTS && m_renderMode < RenderMode::SHADOWS) {
		glEnableClientState(GL_NORMAL_ARRAY);
	}

	if (m_renderMode >= RenderMode::OBJECTS) {
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

void ModelRendererGl::render(const Model *model, float fade, int frame, int id, ShaderProgram *customShaders) {
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

void ModelRendererGl::renderOutlined(const Model *model, int lineWidth, const Vec3f &colour, float fade, int frame, int id, ShaderProgram *customShaders) {
	//assertions
	assert(m_rendering);
	assertGl();

	glEnable(GL_STENCIL_TEST);
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	// render every mesh
	for (uint32 i = 0; i < model->getMeshCount(); ++i) {
		renderMesh(model->getMesh(i), fade, frame, id, customShaders);
	}

	glColor3fv(colour.ptr());
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(float(lineWidth));

	// disable texture0 and lighting
	glActiveTexture(diffuseTextureUnit);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glEnable (GL_LINE_SMOOTH);

	glStencilFunc(GL_NOTEQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	// render outline
	for (uint32 i = 0; i < model->getMeshCount(); ++i) {
		renderMeshOutline(model->getMesh(i));
	}

	glDisable(GL_STENCIL_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);

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

void ModelRendererGl::renderMesh(const Mesh *mesh, float fade, int frame, int id, ShaderProgram *customShaders) {
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
	if (m_shaderIndex == -1 || mode == RenderMode::SELECTION || mode == RenderMode::SHADOWS) {
		shaderProgram = m_fixedFunctionProgram;
		if (m_meshCallback) {
			m_meshCallback->execute(mesh);
		}
	} else {
		if (customShaders) {
			shaderProgram = customShaders;
		} else {
			shaderProgram = m_shaders[m_shaderIndex];
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
	int teamColourFlag = (mesh->usesTeamTexture() && mode == RenderMode::UNITS) ? 1 : 0;
	shaderProgram->setUniform("gae_UsesTeamColour", teamColourFlag);
	shaderProgram->setUniform("gae_AlphaThreshold", m_alphaThreshold);
	shaderProgram->setUniform("gae_LightCount", m_currentLightCount);
	if (customShaders) {
		shaderProgram->setUniform("gae_FrameNumber", frame);
		shaderProgram->setUniform("gae_EntityId", id);
	}

	// vertices
	const MeshVertexBlock *mainBlock = 0;
	const MeshVertexBlock &staticBlock = mesh->getStaticVertData();
	if (staticBlock.count != 0) {
		mainBlock = &staticBlock;
	} else {
		mainBlock = &mesh->getInterpolationData()->getVertexBlock();
	}
	const int stride = mainBlock->getStride();
	int tanAttribLoc = -1;
	if (mainBlock->vbo_handle) {
#		define VBO_OFFSET(x) ((void*)(x * sizeof(float)))
		glBindBuffer(GL_ARRAY_BUFFER, mainBlock->vbo_handle);
		glVertexPointer(3, GL_FLOAT, stride, VBO_OFFSET(0));
		if (sendNormals) {
			glNormalPointer(GL_FLOAT, stride, VBO_OFFSET(3));
		}
		if (sendNormals && textureNormal
		&& (mainBlock->type == MeshVertexBlock::POS_NORM_TAN 
		|| mainBlock->type == MeshVertexBlock::POS_NORM_TAN_UV)) {
			shaderProgram->setUniform("gae_HasNormalMap", 1u);
			tanAttribLoc = shaderProgram->getAttribLoc("gae_Tangent");
			if (tanAttribLoc != -1) {
				glEnableVertexAttribArray(tanAttribLoc);
				glVertexAttribPointer(tanAttribLoc, 3, GL_FLOAT, GL_TRUE, stride, VBO_OFFSET(6));
			}
		} else {
			shaderProgram->setUniform("gae_HasNormalMap", 0u);
		}
		if (renderTextures && mesh->getTexture(MeshTexture::DIFFUSE)) {
			int uvOffset = mainBlock->getUvOffset();
			if (uvOffset != -1) {
				if (m_duplicateTexCoords) {
					glActiveTexture(GL_TEXTURE0 + m_secondaryTexCoordUnit);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(2, GL_FLOAT, stride, VBO_OFFSET(uvOffset));
				}
				glActiveTexture(GL_TEXTURE0);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, stride, VBO_OFFSET(uvOffset));
			} else {
				const MeshVertexBlock &uvBlock = mesh->getTecCoordBlock();
				assert(uvBlock.count != 0);
				glBindBuffer(GL_ARRAY_BUFFER, uvBlock.vbo_handle);
				if (m_duplicateTexCoords) {
					glActiveTexture(GL_TEXTURE0 + m_secondaryTexCoordUnit);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(2, GL_FLOAT, 0, VBO_OFFSET(0));
				}
				glActiveTexture(GL_TEXTURE0);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 0, VBO_OFFSET(0));
			} 
		} else {
			if (m_duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + m_secondaryTexCoordUnit);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			glActiveTexture(GL_TEXTURE0);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		assertGl();
		// draw model
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->getIndices().vbo_handle);
		int indexType = mesh->getIndices().type == MeshIndexBlock::UNSIGNED_16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
		glDrawRangeElements(GL_TRIANGLES, 0, vertexCount-1, indexCount, indexType, VBO_OFFSET(0));

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		if (tanAttribLoc != -1) {
			glDisableVertexAttribArray(tanAttribLoc);
		}

		assertGl();

	} else { // Non static mesh or VBOs disabled
		glVertexPointer(3, GL_FLOAT, stride, mainBlock->m_arrayPtr);
		if (sendNormals) {
			glNormalPointer(GL_FLOAT, stride, &mainBlock->m_posNorm[0].norm);
		}
		if (sendNormals && textureNormal
		&& (mainBlock->type == MeshVertexBlock::POS_NORM_TAN 
		|| mainBlock->type == MeshVertexBlock::POS_NORM_TAN_UV)) {
			shaderProgram->setUniform("gae_HasNormalMap", 1u);
			tanAttribLoc = shaderProgram->getAttribLoc("gae_Tangent");
			if (tanAttribLoc != -1) {
				glEnableVertexAttribArray(tanAttribLoc);
				glVertexAttribPointer(tanAttribLoc, 3, GL_FLOAT, GL_TRUE, stride, &mainBlock->m_posNormTan[0].tan);
			}
		} else {
			shaderProgram->setUniform("gae_HasNormalMap", 0u);
		}
		if (renderTextures && mesh->getTexture(MeshTexture::DIFFUSE)) {
			int uvOffset = mainBlock->getUvOffset();
			if (uvOffset != -1) {
				if (m_duplicateTexCoords) {
					glActiveTexture(GL_TEXTURE0 + m_secondaryTexCoordUnit);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(2, GL_FLOAT, stride, &((float*)mainBlock->m_arrayPtr)[uvOffset]);
				}
				glActiveTexture(GL_TEXTURE0);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, stride, &((float*)mainBlock->m_arrayPtr)[uvOffset]);
			} else {
				const MeshVertexBlock &uvBlock = mesh->getTecCoordBlock();
				assert(uvBlock.count != 0);
				if (uvBlock.vbo_handle) {
					glBindBuffer(GL_ARRAY_BUFFER, uvBlock.vbo_handle);
				}
				if (m_duplicateTexCoords) {
					glActiveTexture(GL_TEXTURE0 + m_secondaryTexCoordUnit);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					if (uvBlock.vbo_handle) {
						glTexCoordPointer(2, GL_FLOAT, 0, VBO_OFFSET(0));
					} else {
						glTexCoordPointer(2, GL_FLOAT, 0, uvBlock.m_arrayPtr);
					}
				}
				glActiveTexture(GL_TEXTURE0);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				if (uvBlock.vbo_handle) {
					glTexCoordPointer(2, GL_FLOAT, 0, VBO_OFFSET(0));
				} else {
					glTexCoordPointer(2, GL_FLOAT, 0, uvBlock.m_arrayPtr);
				}
			} 
		} else {
			if (m_duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + m_secondaryTexCoordUnit);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			glActiveTexture(GL_TEXTURE0);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		int indexType = mesh->getIndices().type == MeshIndexBlock::UNSIGNED_16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

		if (use_vbos) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->getIndices().vbo_handle);
			glDrawRangeElements(GL_TRIANGLES, 0, vertexCount-1, indexCount, indexType, VBO_OFFSET(0));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		} else {
			glDrawRangeElements(GL_TRIANGLES, 0, vertexCount-1, indexCount, indexType, mesh->getIndices().m_indices);
		}
		if (tanAttribLoc != -1) {
			glDisableVertexAttribArray(tanAttribLoc);
		}
	}
	assertGl();
}

void ModelRendererGl::renderMeshOutline(const Mesh *mesh) {
	// assertions
	assert(m_renderMode == RenderMode::UNITS);
	assertGl();

	const uint32 vertexCount = mesh->getVertexCount();
	const uint32 indexCount = mesh->getIndexCount();
	if (!mesh->getVertexCount()) {
		return;
	}
	if (m_lastShaderProgram) {
		m_lastShaderProgram->end();
	}

	// set cull face
	if (mesh->isTwoSided()) {
		glDisable(GL_CULL_FACE);
	} else {
		glEnable(GL_CULL_FACE);
	}

	// push outline back a bit
	glPolygonOffset(0.f, -1.f);

	// disable tex-coord arrays
	if (m_duplicateTexCoords) {
		glActiveTexture(GL_TEXTURE0 + m_secondaryTexCoordUnit);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	glActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	//assertions
	assertGl();

	// vertices
	const MeshVertexBlock *vertexBlock = 0;
	if (mesh->getStaticVertData().count != 0) {
		vertexBlock = &mesh->getStaticVertData();
	} else {
		vertexBlock = &mesh->getInterpolationData()->getVertexBlock();
	}
	
	const int stride = vertexBlock->getStride();
	int indexType = mesh->getIndices().type == MeshIndexBlock::UNSIGNED_16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
	if (vertexBlock->vbo_handle) { // all VBO
		glBindBuffer(GL_ARRAY_BUFFER, vertexBlock->vbo_handle);
		glVertexPointer(3, GL_FLOAT, stride, 0);
 		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->getIndices().vbo_handle);
		glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, indexType, VBO_OFFSET(0));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		assertGl();

	} else { // Non static mesh or VBOs disabled
		glVertexPointer(3, GL_FLOAT, stride, vertexBlock->m_arrayPtr);
		if (use_vbos) { // non-static mesh, indices are in VBO
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->getIndices().vbo_handle);
			glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, indexType, VBO_OFFSET(0));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			assertGl();
		} else { // all old-school
			glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, indexType, mesh->getIndices().m_indices);
			assertGl();
		}
	}

	glPolygonOffset(0.f, 0.f);
	assertGl();
	if (m_lastShaderProgram) {
		m_lastShaderProgram->begin();
	}
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
	//glBegin(GL_LINES);
	//for (int i = 0; i < mesh->getIndexCount(); ++i) {
	//	Vec3f vertex = mesh->getInterpolationData()->getVertices()[mesh->getIndices()[i]];
	//	Vec3f normal = vertex + mesh->getInterpolationData()->getNormals()[mesh->getIndices()[i]];

	//	glVertex3fv(vertex.ptr());
	//	glVertex3fv(normal.ptr());
	//}
	//glEnd();
}

}}}//end namespace
