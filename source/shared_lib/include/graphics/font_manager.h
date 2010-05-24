// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_FONTMANAGER_H_
#define _SHARED_GRAPHICS_FONTMANAGER_H_

#include "font.h"

#include <vector>
using std::vector;

namespace Shared{ namespace Graphics{

// =====================================================
//	class FontManager
//
///	Creates, Intializes, Finalizes, and Deletes fonts
// =====================================================

class FontManager{
protected:
	typedef vector<Font*> FontContainer;

protected:
	FontContainer fonts;

public:
	virtual ~FontManager();

	Font* newBitMapFont();
	Font* newFreeTypeFont();

	void init();
	void end();
};

}}//end namespace

#endif
