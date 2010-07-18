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
#include "font_gl.h"
#include "ft_font.h"

#include "leak_dumper.h"


namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class TextRendererBM
// =====================================================

TextRendererBM::TextRendererBM(){
	rendering= false;
}

void TextRendererBM::begin(const Font *font){
	assert(!rendering);
	rendering= true;

	this->font= static_cast<const BitMapFont*>(font);
}

void TextRendererBM::render(const string &text, int x, int y, bool centered){
	assert(rendering);

	assertGl();

	int line=0;
	int size= font->getSize();
    const unsigned char *utext= reinterpret_cast<const unsigned char*>(text.c_str());

	Vec2f rasterPos;
	const FontMetrics *metrics= font->getMetrics();
	if(centered){
		Vec2f textDiminsions = metrics->getTextDiminsions(text);
		rasterPos.x = x - textDiminsions.x / 2.f;
		rasterPos.y = y + textDiminsions.y / 2.f;
	}
	else{
		rasterPos= Vec2f(static_cast<float>(x), static_cast<float>(y));
	}
	glRasterPos2f(rasterPos.x, rasterPos.y);

	for (int i = 0; utext[i]; ++i) {
		switch(utext[i]) {
			case '\t':
				rasterPos.x += size;
				glRasterPos2f(rasterPos.x, rasterPos.y);
				break;
			case '\n':
				line++;
				rasterPos= Vec2f(static_cast<float>(x), y-(metrics->getHeight()*2.f)*line);
				glRasterPos2f(rasterPos.x, rasterPos.y);
				break;
			default:
				glCallList(font->getHandle() + utext[i]);
		}
	}

	assertGl();
}

void TextRendererBM::end(){
	assert(rendering);
	rendering= false;
}

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
	Freetype::print(font->fontData, float(x), y + font->getMetrics()->getMaxDescent(), utext);
	assertGl();
}

void TextRendererFT::end(){
	assert(rendering);
	rendering= false;
	assertGl();
}

}}}//end namespace
