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

#ifndef _SHARED_GRAPHICS_TEXTRENDERER_H_
#define _SHARED_GRAPHICS_TEXTRENDERER_H_

#include <string>

#include "vec.h"
#include "font.h"

using std::string;

namespace Shared{ namespace Graphics{

// =====================================================
//	class TextRenderer
// =====================================================

class TextRenderer {
public:	
	virtual ~TextRenderer(){};

	virtual void begin(const Font *font)= 0;
	virtual void render(const string &text, int x, int y, bool centered= false)= 0;
	virtual void end()= 0;
};

}}//end namespace

#endif
