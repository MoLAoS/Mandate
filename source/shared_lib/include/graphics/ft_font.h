// ==============================================================
//	This file is part of Glest Advanced Engine
//
//	Based on code by Sven Olsen, 2003
//  http://nehe.gamedev.net/data/lessons/lesson.asp?lesson=43
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef FREETYPE_FONT_INCLUDED
#define FREETYPE_FONT_INCLUDED

#include "gl_wrap.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>

#include <string>
#include <vector>

namespace Shared { namespace Graphics {
	
namespace Freetype {

using std::vector;
using std::string;

// This holds all of the information related to any freetype font that we want to create.
struct font_data {
	float h;			///< Holds the height of the font.
	GLuint * textures;	///< Holds the texture id's 
	GLuint list_base;	///< Holds the first display list id
	bool initialised;

	font_data() : textures(0), initialised(false) {}

	// The init function will create a font of of the height h from the file fname.
	void init(const char * fname, unsigned int h, FontMetrics &metrics);

	// Free all the resources assosiated with the font.
	void clean();
};

// The flagship function of the library - this thing will print out text at window coordinates
// x,y, using the font ft_font. The current modelview matrix will also be applied to the text.
void print(const font_data &ft_font, float x, float y, const unsigned char *txt) ;

} // namespace Freetype

namespace Gl {

class FreeTypeFont : public Font {
public:
	Freetype::font_data fontData;

	virtual void init() override;
	virtual void reInit() override;
	virtual void end() override;
};

}}}

#endif