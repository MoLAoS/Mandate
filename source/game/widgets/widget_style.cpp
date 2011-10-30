// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "widget_style.h"
#include "leak_dumper.h"

namespace Glest { namespace Widgets {


BorderStyle::BorderStyle() {
	setNone();
}

BorderStyle::BorderStyle(const BorderStyle &style) {
	memcpy(this, &style, sizeof(BorderStyle));
}

void BorderStyle::setSizes(int left, int top, int right, int bottom) {
	m_sizes[Border::LEFT] = left;
	m_sizes[Border::TOP] = top;
	m_sizes[Border::RIGHT] = right;
	m_sizes[Border::BOTTOM] = bottom;
}

void BorderStyle::setNone() {
	memset(this, 0, sizeof(*this));
}

void BorderStyle::setRaise(int lightColourIndex, int darkColourIndex) {
	m_type = BorderType::RAISE;
	m_colourIndices[0] = lightColourIndex;
	m_colourIndices[1] = darkColourIndex;
}

void BorderStyle::setEmbed(int lightColourIndex, int darkColourIndex) {
	m_type = BorderType::EMBED;
	m_colourIndices[0] = lightColourIndex;
	m_colourIndices[1] = darkColourIndex;
}

void BorderStyle::setSolid(int colourIndex) {
	m_type = BorderType::SOLID;
	m_colourIndices[0] = colourIndex;
}

void BorderStyle::setImage(int imageNdx, int borderSize, int cornerSize) {
	m_type = BorderType::TEXTURE;
	m_imageNdx = imageNdx;
	setSizes(borderSize);
	m_cornerSize = cornerSize;
}

PaddingStyle::PaddingStyle() {
	memset(this, 0, sizeof(*this));
}

void PaddingStyle::setUniform(int pad) {
}

void PaddingStyle::setVertical(int pad) {
}

void PaddingStyle::setHorizontal(int pad) {
}

void PaddingStyle::setValues(int top, int right, int bottom, int left) {
}


BackgroundStyle::BackgroundStyle() : m_type(BackgroundType::NONE) {
	memset(&m_colourIndices, 0, sizeof(int) * Corner::COUNT);
	m_imageIndex = 0;
	m_insideBorders = true;
}

BackgroundStyle::BackgroundStyle(const BackgroundStyle &style) {
	memcpy(this, &style, sizeof(BackgroundStyle));
}

void BackgroundStyle::setNone() {
	m_type = BackgroundType::NONE;
}

void BackgroundStyle::setColour(int colourIndex) {
	m_type = BackgroundType::COLOUR;
	m_colourIndices[0] = colourIndex;
}

void BackgroundStyle::setCustom(int colourIndex1, int colourIndex2, int colourIndex3, int colourIndex4) {
}

void BackgroundStyle::setTexture(int imageIndex) {
	m_type = BackgroundType::TEXTURE;
	m_imageIndex = imageIndex;
}

TextStyle::TextStyle() {
	memset(this, 0, sizeof(*this));
	m_smallFontIndex = m_largeFontIndex = -1;
}

void TextStyle::setNormal(int fontIndex, int colourIndex) {
	m_fontIndex = fontIndex;
	m_colourIndex = colourIndex;
}

void TextStyle::setShadow(int fontIndex, int colourIndex, int shadowColourIndex) {
	m_fontIndex = fontIndex;
	m_colourIndex = colourIndex;
	m_shadowColourIndex = shadowColourIndex;
	m_shadow = true;
}


}}
