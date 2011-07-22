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

#ifndef _SHARED_GRAPHICS_GL_TEXTUREGL_H_
#define _SHARED_GRAPHICS_GL_TEXTUREGL_H_

#include "texture.h"
#include "opengl.h"

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class TextureGl
// =====================================================

class TextureGl {
protected:
	GLuint handle;

	static bool compressTextures;

public:
	GLuint getHandle() const	{return handle;}

	static void setCompressTextures(bool enable) { compressTextures = enable; }
	
};

// =====================================================
//	class Texture1DGl
// =====================================================

class Texture1DGl: public Texture1D, public TextureGl{
public:
	virtual void init(Filter filter, int maxAnisotropy= 1) override;
	virtual void deletePixmap() override;
	virtual void end() override;
};

// =====================================================
//	class Texture2DGl
// =====================================================

class Texture2DGl: public Texture2D, public TextureGl{
public:
	virtual void init(Filter filter, int maxAnisotropy= 1) override;
	virtual void deletePixmap() override;
	virtual void end() override;
	GLuint getHandle() const {
		// ensure call init before use
		assert(inited);
		return handle;
	}
};

// =====================================================
//	class Texture3DGl
// =====================================================

class Texture3DGl: public Texture3D, public TextureGl{
public:
	virtual void init(Filter filter, int maxAnisotropy= 1) override;
	virtual void deletePixmap() override;
	virtual void end() override;
};

// =====================================================
//	class TextureCubeGl
// =====================================================

class TextureCubeGl: public TextureCube, public TextureGl{
public:
	virtual void init(Filter filter, int maxAnisotropy= 1) override;
	virtual void deletePixmap() override;
	virtual void end() override;
};

}}}//end namespace

#endif
