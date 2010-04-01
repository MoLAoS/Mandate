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

#ifndef _SHARED_GRAPHICS_GL_PARTICLERENDERERGL_H_
#define _SHARED_GRAPHICS_GL_PARTICLERENDERERGL_H_

#include "particle_renderer.h"
#include "opengl.h"

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class ParticleRendererGl
// =====================================================

class ParticleRendererGl: public ParticleRenderer {
public:
	static const int bufferSize = 1024;
	static const GLenum glBlendFactors[Particle::BLEND_FUNC_COUNT];
	static const GLenum glBlendEquations[Particle::BLEND_EQUATION_COUNT];

private:
	bool rendering;
	Vec3f vertexBuffer[bufferSize];
	Vec2f texCoordBuffer[bufferSize];
	Vec4f colorBuffer[bufferSize];

public:
	//particles
	ParticleRendererGl();
	virtual void renderManager(ParticleManager *pm, ModelRenderer *mr);
	virtual void renderSystem(ParticleSystem *ps);
	virtual void renderSystemLine(ParticleSystem *ps);
	virtual void renderSingleModel(ParticleSystem *ps, ModelRenderer *mr);

	/** Translate a Particle::BlendFactor into an OpenGL GLenum value for glBlendFunc() */
	static GLenum translate(Particle::BlendFactor v) {
		assert(v >= 0 && v < Particle::BLEND_FUNC_COUNT);
		return glBlendFactors[v];
	}

	/** Translate a Particle::BlendEquation into an OpenGL GLenum value for glBlendEquation() */
	static GLenum translate(Particle::BlendEquation v) {
		assert(v >= 0 && v < Particle::BLEND_EQUATION_COUNT);
		return glBlendEquations[v];
	}

protected:
	void renderBufferQuads(int quadCount);
	void renderBufferLines(int lineCount);

	static void setBlendFunc(Particle::BlendFactor sfactor, Particle::BlendFactor dfactor) {
		glBlendFunc(translate(sfactor), translate(dfactor));
	}

	static void setBlendEquation(Particle::BlendEquation equation) {
		glBlendEquation(translate(equation));
	}
};

}}}//end namespace

#endif
