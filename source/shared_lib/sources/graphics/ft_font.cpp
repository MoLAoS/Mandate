// ==============================================================
//	This file is part of Glest Advanced Engine
//
//	Based on code by Sven Olsen, 2003
//  http://nehe.gamedev.net/data/lessons/lesson.asp?lesson=43
//
//  Wrapped up for use with Glest by James McCulloch, 2010
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "ft_font.h"
#include "math_util.h"
#include "FSFactory.hpp"

#include <stdexcept>

#include "leak_dumper.h"

namespace Shared { namespace Graphics {

using namespace PhysFS;
using Math::nextPowerOf2;

namespace Freetype {

// macro to convert 26.6 fixed point format to float
#define _26_6_TO_FLOAT(x) (float(x >> 6) + float(x & 0x3F) / 64.f)

/// Create a display list coresponding to the give character.
void make_dlist(FT_Face face, unsigned char ch, GLuint list_base, GLuint *tex_base) {

	// 1. Get Glyph and render to texture

	// Load the Glyph for our character.
	if (FT_Load_Glyph(face, FT_Get_Char_Index( face, ch ), FT_LOAD_DEFAULT)) {
		throw std::runtime_error("FT_Load_Glyph failed");
	}
	// Move the face's glyph into a Glyph object
	FT_Glyph glyph;
	if (FT_Get_Glyph(face->glyph, &glyph)) {
		throw std::runtime_error("FT_Get_Glyph failed");
	}
	// Convert the glyph to a bitmap.
	FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);
	FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
	FT_Bitmap &bitmap = bitmap_glyph->bitmap;

	// Create bitmap, padded to power of two dimensions
	int width = nextPowerOf2(bitmap.width);
	int height = nextPowerOf2(bitmap.rows);
	GLubyte* expanded_data = new GLubyte[2 * width * height];

	// Fill in the data for the expanded bitmap.
	for (int j = 0; j < height; ++j) {
		for (int i = 0; i < width; ++i) {
			expanded_data[2 * (i + j * width)] = expanded_data[2 * (i + j * width) + 1] =
				(i >= bitmap.width || j >= bitmap.rows) ? 0 : bitmap.buffer[i + bitmap.width * j];
		}
	}

	// 2. build gl display list

	glBindTexture(GL_TEXTURE_2D, tex_base[ch]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
		  0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, expanded_data);

	// With the texture created, we don't need to expanded data anymore
	delete [] expanded_data;

	// Calc max tex co-ords, accounting for texture padding
	float max_u = bitmap.width / float(width),
		  max_v = bitmap.rows / float(height);

	// Create the display list
	glNewList(list_base + ch, GL_COMPILE);
		glBindTexture(GL_TEXTURE_2D, tex_base[ch]);
		glPushMatrix();
		glTranslatef(float(bitmap_glyph->left), -float(bitmap_glyph->top), 0.f);

		// Draw the glyph
		glBegin(GL_QUADS);
			glTexCoord2f(0.f, max_v);
			glVertex2f(0.f, float(bitmap.rows));
			glTexCoord2f(0.f, 0.f);
			glVertex2f(0.f, 0.f);
			glTexCoord2f(max_u, 0.f);
			glVertex2f(float(bitmap.width), 0.f);
			glTexCoord2f(max_u, max_v);
			glVertex2f(float(bitmap.width), float(bitmap.rows));
		glEnd();
		glPopMatrix();
		glTranslatef(_26_6_TO_FLOAT(face->glyph->advance.x), 0.f, 0.f);
	glEndList();
	
	FT_Done_Glyph(glyph);
}

void font_data::init(const char * fname, unsigned int h, FontMetrics &metrics) {
	textures = new GLuint[256];

	// Create and initilize a freetype font library.
	FT_Library library;
	if (FT_Init_FreeType(&library)) {
		throw std::runtime_error("FT_Init_FreeType failed");
	}
	FT_Face face;
	//if (FT_New_Face(library, FSFactory::getRealPath(fname).c_str(), 0, &face)) {
	if (FSFactory::openFace(library, fname, 0, &face)) {
		throw std::runtime_error("FT_New_Face failed (there is probably a problem with your font file)");
	}
	//FT_Set_Pixel_Sizes(face, 0, h);
	FT_Set_Char_Size(face, h << 6, h << 6, 96, 96);
	list_base = glGenLists(256);
	glGenTextures(256, textures);

	metrics.setFreeType(true);
	metrics.setMaxAscent(0.f);
	metrics.setMaxDescent(0.f);

	// Create display lists for the fonts glyphs
	for (unsigned i = 0; i <= 255U; ++i) {
		// make list
		make_dlist(face, (unsigned char)i, list_base, textures);

		// set metrics
		float ascent = _26_6_TO_FLOAT(face->glyph->metrics.horiBearingY);
		float height = _26_6_TO_FLOAT(face->glyph->metrics.height);
		float descent = height - ascent;
		float advance = _26_6_TO_FLOAT(face->glyph->advance.x);
		metrics.setWidth(i, advance);
		if (metrics.getMaxAscent() < ascent) {
			metrics.setMaxAscent(ascent);
		}
		if (metrics.getMaxDescent() < descent) {
			metrics.setMaxDescent(descent);
		}
	}
	this->h = metrics.getHeight();
	FSFactory::doneFace(face);
	FT_Done_FreeType(library);
	initialised = true;
}

void font_data::clean() {
	if (initialised) {
		glDeleteLists(list_base, 256);
		glDeleteTextures(256, textures);
	}
	delete [] textures;
}

typedef const unsigned char * uchar_ptr;

void render(const font_data &ft_font, Vec2i pos, const vector<string> &lines) {
	GLuint font = ft_font.list_base;
	float h = ft_font.h;// / .63f;

	glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT  | GL_ENABLE_BIT | GL_TRANSFORM_BIT);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glListBase(font);

	float modelview_matrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, modelview_matrix);

	// This is where the text display actually happens.
	float x = float(pos.x);
	float y = float(pos.y);
	for (int i = 0; i < lines.size(); ++i) {
		 glPushMatrix();
		 glLoadIdentity();

		 glTranslatef(x, y + h * i, 0);
		 glMultMatrixf(modelview_matrix);
		 glCallLists(lines[i].length(), GL_UNSIGNED_BYTE, lines[i].c_str());
		 glPopMatrix();
	 }
	 glPopAttrib();
}

///Much like Nehe's glPrint function, but modified to work with freetype fonts.
void print(const font_data &ft_font, float x, float y, const unsigned char *text)  {
	// Here is some code to split the text that we have been given into a set of lines.
	uchar_ptr start_line = text;
	vector<string> lines;
	uchar_ptr c = text;
	for ( ; *c; ++c) {
		if (*c == '\n') {
			string line;
			for (uchar_ptr n = start_line; n < c; ++n) {
				line.append(1, *n);
			}
			lines.push_back(line);
			start_line = c + 1;
		}
	}
	if (*start_line) {
		string line;
		for (uchar_ptr n = start_line; n < c; ++n) {
			line.append(1, *n);
		}
		lines.push_back(line);
	}
	render(ft_font, Vec2i(int(x), int(y)), lines);
}

} // end namespace Freetype

namespace Gl {

void FreeTypeFont::init() {
	fontData.init(getType().c_str(), getSize(), metrics);
}

void FreeTypeFont::reInit() {
	end();
	init();
}

void FreeTypeFont::end() {
	fontData.clean();
}

}}}
