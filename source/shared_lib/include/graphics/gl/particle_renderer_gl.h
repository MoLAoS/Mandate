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
	static const GLenum glBlendModes[Particle::BLEND_MODE_COUNT];

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
	virtual void renderSingleModel(AttackParticleSystem *ps, ModelRenderer *mr);

	/** Translate a Particle::BlendMode into an OpenGL value for glBlendFunc() */
	static GLenum glBlendMode(Particle::BlendMode blendMode) {
		assert(blendMode >= 0 && blendMode < Particle::BLEND_MODE_COUNT);
		return glBlendModes[blendMode];
	}

protected:
	void renderBufferQuads(int quadCount);
	void renderBufferLines(int lineCount);

	static void setBlendMode(Particle::BlendMode srcBlendMode, Particle::BlendMode destBlendMode) {
		glBlendFunc(glBlendMode(srcBlendMode), glBlendMode(destBlendMode));
	}
};

}}}//end namespace

#endif
