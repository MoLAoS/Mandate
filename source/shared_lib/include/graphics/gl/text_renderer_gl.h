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

#ifndef _SHARED_GRAPHICS_GL_TEXTRENDERERGL_H_
#define _SHARED_GRAPHICS_GL_TEXTRENDERERGL_H_

#include "text_renderer.h"

namespace Shared{ namespace Graphics{ namespace Gl{

class FreeTypeFont;

// =====================================================
//	class TextRendererFT
// =====================================================

class TextRendererFT: public TextRenderer {
private:
	const FreeTypeFont *font;
	bool rendering;

public:
	TextRendererFT();

	virtual void begin(const Font *font);
	virtual void render(const string &text, int x, int y, bool centered);
	virtual void end();
};

}}}//end namespace

#endif
