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
#include "text_renderer_gl.h"

#include "opengl.h"
#include "ft_font.h"

#include "leak_dumper.h"

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class TextRendererFT
// =====================================================

TextRendererFT::TextRendererFT(){
	rendering = false;
}

void TextRendererFT::begin(const Font *font){
	assertGl();
	assert(!rendering);
	rendering = true;
	this->font = static_cast<const FreeTypeFont*>(font);
	assertGl();
}

void TextRendererFT::render(const string &text, int x, int y, bool centered) {
	assertGl();
	assert(rendering);
	const unsigned char *utext= reinterpret_cast<const unsigned char*>(text.c_str());
	Freetype::print(font->fontData, float(x), y, utext);
	assertGl();
}

void TextRendererFT::end(){
	assert(rendering);
	rendering= false;
	assertGl();
}

}}}//end namespace
