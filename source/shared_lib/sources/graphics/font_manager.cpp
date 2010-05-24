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
#include "font_manager.h"

#include "graphics_interface.h"
#include "graphics_factory.h"

#include "leak_dumper.h"


namespace Shared{ namespace Graphics{

// =====================================================
//	class FontManager
// =====================================================

FontManager::~FontManager(){
	end();
}

Font* FontManager::newBitMapFont() {
	Font *font= GraphicsInterface::getInstance().getFactory()->newBitMapFont();
	fonts.push_back(font);
	return font;
}

Font* FontManager::newFreeTypeFont() {
	Font *font= GraphicsInterface::getInstance().getFactory()->newFreeTypeFont();
	fonts.push_back(font);
	return font;
}

void FontManager::init(){
	for(size_t i=0; i<fonts.size(); ++i) {
		fonts[i]->init();
	}
}

void FontManager::end(){
	for(size_t i=0; i<fonts.size(); ++i) {
		fonts[i]->end();
		delete fonts[i];
	}
	fonts.clear();
}


}}//end namespace
