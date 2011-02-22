// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2011 Nathan Turner <hailstone3, sf.net>
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "imageset.h"
#include "pixmap.h"
#include "renderer.h"

#include "leak_dumper.h"

using Shared::Graphics::Pixmap2D;

namespace Glest{ namespace Graphics{

// =====================================================
//  class Imageset
// =====================================================

/** Add images with uniform dimensions
  * @param source the source texture
  * @param width the width of a single image
  * @param height the height of a single image
  */
void Imageset::addImages(const Texture2D *source, int width, int height) {
	assert(source != NULL && width > 0 && height > 0);
	///@todo check the dimensions to make sure there will be enough image for all the images
	/// current behaviour cuts some off if not exact? (untested) - hailstone 30Dec2010
	/// maybe check dimensions are power of 2 and then check that maches source dimensions

	const Pixmap2D *sourcePixmap = source->getPixmap();

	// extract a single image from the source pixmap and add the texture to the list.
	// (0,0) is at bottom left but is setup so ordering goes from top left to the right
	// and wraps around to bottom right. Think of it as each row is appended to the previous.
	for (int j = sourcePixmap->getH() - height; j >= 0; j = j - height) {
		for (int i = 0; i < sourcePixmap->getW(); i = i + width) {
			Texture2D *tex = g_renderer.newTexture2D(Graphics::ResourceScope::GLOBAL);
			tex->getPixmap()->copy(i, j, width, height, sourcePixmap); //may throw, should this be handled here? - hailstone 30Dec2010
			tex->setMipmap(false);
			tex->init(Texture::fBilinear);
			m_textures.push_back(tex);
		}
	}

	assert(m_textures.size());

	m_size.x = m_textures[m_active]->getPixmap()->getW();
	m_size.y = m_textures[m_active]->getPixmap()->getH();
}

void Imageset::render() {
	g_renderer.renderTextureQuad(m_pos.x, m_pos.y, m_size.x, m_size.y, m_textures[m_active], m_fade);
}

int Imageset::getNumImages() {
	return m_textures.size();
}

void Imageset::setPos(int x, int y) {
	m_pos.x = x;
	m_pos.y = y;
}

void Imageset::setPos(Vec2i pos) {
	m_pos = pos;
}

void Imageset::setSize(int w, int h) {
	m_size.x = w;
	m_size.y = h;
}

void Imageset::setSize(Vec2i size) {
	m_size = size;
}

/// @param ndx must be valid to change default from 0
void Imageset::setDefaultImage(int ndx) {
	if (hasImage(ndx)) {
		m_defaultImage = ndx;
	}
}

/// @param ndx must be valid to change
void Imageset::setActive(int ndx) {
	if (hasImage(ndx)) {
		m_active = ndx;
	}
}

// =====================================================
//	class Animset
// =====================================================
/*
void Animset::update() {
	///@todo timing needs to be fixed - hailstone 2Jan2011
	if (isEnabled()) {
		m_timeElapsed++;
	}

	if (m_timeElapsed > 60) {
		m_timeElapsed = 0;
		++m_currentFrame;
		if (m_currentFrame > m_end) {
			if (m_loop) {
				reset();
			} else {
				stop();
			}
		}
		m_imageset->setActive(m_currentFrame);
	}
}
*/

}}//end namespace