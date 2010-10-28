// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "font.h"
#include "util.h"
#include "leak_dumper.h"

namespace Shared { namespace Graphics {

// =====================================================
//	class FontMetrics
// =====================================================

FontMetrics::FontMetrics() : freeType(false) {
	widths = new float[Font::charCount];
	height = 0;

	for(int i = 0; i < Font::charCount; ++i) {
		widths[i] = 0;
	}
}

FontMetrics::~FontMetrics() {
	delete [] widths;
}

void FontMetrics::wrapText(string &inout_text, unsigned in_maxWidth) const {
	const unsigned spacesPerTab = 4;
	std::stringstream result;
	float currentLineWidth = 0.f;
	float maxLineWidth = float(in_maxWidth);
	string::size_type offset = 0;
	string::size_type count = 0;
	string::size_type lastEndWord = 0;

	foreach (string, it, inout_text) {
		++count;
		const char &c = *it;
		if (c == ' ') {
			currentLineWidth += widths[c];
			lastEndWord = offset + count;
		} else if (c == '\t') {
			currentLineWidth += widths[c] * spacesPerTab;
			lastEndWord = offset + count;
		} else if (c == '\n') {
			currentLineWidth = 0;
			result << inout_text.substr(offset, count);
			offset += count;
			count = 0;
			lastEndWord = offset;
		} else {
			if (currentLineWidth + widths[c] > maxLineWidth) {
				result << inout_text.substr(offset, lastEndWord - offset) << endl;
				count -= (lastEndWord - offset);
				offset = lastEndWord;
				currentLineWidth = 0.f;
				for (unsigned i=offset; i < offset + count; ++i) {
					currentLineWidth += widths[inout_text[i]];
				}
			}			
			currentLineWidth += widths[c];
		}
	}
	result << inout_text.substr(offset, count);
	inout_text = result.str();
}

Vec2f FontMetrics::getTextDiminsions(const string &str) const {
	Vec2f dim(0.f, 0.f);
	float width = 0.f;

	if (str.empty()) {
		return dim;
	}
	for(int i = 0; i < str.size(); ++i) {
		if(str[i] == '\n') {
			if(dim.x < width) {
				dim.x = width;
			}

			width = 0.f;
			dim.y += getHeight();
		} else {
			width += widths[str[i]];
		}
	}
	if(dim.x < width) {
		dim.x = width;
	}
	if (str[str.size()-1] != '\n') {
		dim.y += getHeight();
	}
	return dim;
}

float FontMetrics::getHeight() const {
	return freeType ? maxAscent + 2/*maxDescent*/ : height;
}

// ===============================================
//	class Font
// ===============================================

const int Font::charCount= 256;

Font::Font() {
	inited = false;
	type = "Times New Roman";
	width = 400;
	size = 10;
}

}}//end namespace
