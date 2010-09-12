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
	static const GLenum glBlendFactors[BlendFactor::COUNT];
	static const GLenum glBlendEquations[BlendMode::COUNT];

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

	/** Translate a BlendFactor into an OpenGL GLenum value for glBlendFunc() */
	static GLenum toGLenum(BlendFactor blendFactor) {
		assert(blendFactor > BlendFactor::INVALID && blendFactor < BlendFactor::COUNT);
		return glBlendFactors[blendFactor];
	}

	/** Translate a BlendMode into an OpenGL GLenum value for glBlendEquation() */
	static GLenum toGLenum(BlendMode blendMode) {
		assert(blendMode > BlendMode::INVALID && blendMode < BlendMode::COUNT);
		return glBlendEquations[blendMode];
	}

protected:
	void renderBufferQuads(int quadCount);
	void renderBufferLines(int lineCount);

	static void setBlendFunc(BlendFactor sfactor, BlendFactor dfactor) {
		glBlendFunc(toGLenum(sfactor), toGLenum(dfactor));
	}

	static void setBlendEquation(BlendMode equation) {
		glBlendEquation(toGLenum(equation));
	}
};

}}}//end namespace

#endif
