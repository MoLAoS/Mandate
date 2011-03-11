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

#ifndef _GLEST_GRAPHICS_IMAGESET_INCLUDED_
#define _GLEST_GRAPHICS_IMAGESET_INCLUDED_

#include "texture.h"
#include "math.h"
#include <vector>

using Shared::Graphics::Texture2D;
using Shared::Math::Vec2i;
using std::vector;

namespace Glest{ namespace Graphics{

// =====================================================
//	class Imageset
//
/// Images are extracted from a single source image
// =====================================================

class Imageset {
private:
	typedef vector<const Texture2D*> Textures;
	Textures m_textures;
	int m_active;
	int m_defaultImage;
	Vec2i m_pos, m_size;
	float m_fade;

public:
	Imageset() 
			: m_active(0)
			, m_defaultImage(0)
			, m_fade(0.f) {
	}

	/// Constructor for uniform image sizes, see addImages
	Imageset(const Texture2D *source, int width, int height)
			: m_active(0)
			, m_defaultImage(0)
			, m_fade(0.f) {
		addImages(source, width, height);
	}

	// non-uniform squares
	/*Imageset(Texture2D, squarespec) {
		use the squarespec to extract the images
	}*/
	void addImages(const Texture2D *source, int width, int height);
	virtual void render();

	int getNumImages();
	bool hasImage(int ndx) const { return ndx < m_textures.size() && ndx >= 0; }

	void setDefaultImage(int ndx = 0);
	void setActive(int ndx = 0);
	void setPos(int x, int y);
	void setPos(Vec2i pos);
	void setSize(int w, int h);
	void setSize(Vec2i size);
	void setFade(float v) { m_fade = v; }
};

// =====================================================
//	class Animset
/// An animated Imageset
// =====================================================
/*
Should probably inherit from Imageset since it's a specialisation that
adds the ability to update the active image according to frame speed.
I also wanted it to be able to split up a single image into groups, so
you could animate from 1-4, 5-8 independently.
class Animset {
private:
	Imageset *m_imageset;
	int m_currentFrame;
	int m_fps;
	float m_timeElapsed;
	int m_start, m_end;
	bool m_loop;
public:
	Animset(Imageset *imageset, int fps) 
			: m_imageset(imageset)
			, m_currentFrame(0)
			, m_fps(fps)
			, m_timeElapsed(0.0)
			, m_start(0)
			, m_loop(true) {
		m_end = m_imageset->getNumImages()-1;
	}

	/// rendering is handled by Imageset
	virtual void render() {}
	virtual void update();
	virtual string desc() { return string("[Animset: ") + "]"; }

	void setRange(int start, int end) { m_start = start; m_end = end; }
	const Texture2D *getCurrent() { return m_imageset->getImage(m_currentFrame); }
	void setFps(int v) { m_fps = v; }
	void play() { setEnabled(true); }
	void stop() { setEnabled(false); }
	void reset() { m_currentFrame = m_start; }
	void loop(bool v) { m_loop = v; }
};
*/

}}//end namespace

#endif