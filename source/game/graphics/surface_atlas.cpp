// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//                2010      James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "surface_atlas.h"

#include <stdexcept>
#include <algorithm>

#include "renderer.h"
#include "util.h"
#include "math_util.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Glest { namespace Graphics {

// =====================================================
//	class SurfaceInfo
// =====================================================

SurfaceInfo::SurfaceInfo(const Pixmap2D *lu, const Pixmap2D *ru, const Pixmap2D *ld, const Pixmap2D *rd)
		: center(NULL)
		, leftUp(lu)
		, rightUp(ru) 
		, leftDown(ld)
		, rightDown(rd)
		, coord(Vec2f(0.f))
		, texture(0) {
}

SurfaceInfo::SurfaceInfo(const Pixmap2D *center)
		: center(center)
		, leftUp(NULL)
		, rightUp(NULL) 
		, leftDown(NULL)
		, rightDown(NULL)
		, coord(Vec2f(0.f))
		, texture(0) {
}

bool SurfaceInfo::operator==(const SurfaceInfo &si) const {
	return this->center == si.getCenter()
		&& this->leftDown == si.getLeftDown()
		&& this->leftUp == si.getLeftUp()
		&& this->rightDown == si.getRightDown()
		&& this->rightUp == si.getRightUp();
}

// ===============================
// 	class SurfaceAtlas
// ===============================

SurfaceAtlas::SurfaceAtlas() : m_coordStep(1.f) {
	surfaceSize = -1;
}

void SurfaceAtlas::addSurface(SurfaceInfo *si) {

	// check dimensions
	if (si->getCenter() != NULL) {
		checkDimensions(si->getCenter());
	} else {
		checkDimensions(si->getLeftUp());
		checkDimensions(si->getLeftDown());
		checkDimensions(si->getRightUp());
		checkDimensions(si->getRightDown());
	}

	// add info
	SurfaceInfos::iterator it = find(surfaceInfos.begin(), surfaceInfos.end(), *si);
	if (it == surfaceInfos.end()) {
		// add new texture
		Texture2D *t = Renderer::getInstance().newTexture2D(ResourceScope::GAME);
		t->setWrapMode(Texture::wmClampToEdge);
		t->getPixmap()->init(surfaceSize, surfaceSize, 3);

		si->setCoord(Vec2f(0.f, 0.f));
		si->setTexture(t);
		surfaceInfos.push_back(*si);

		// copy texture to pixmap
		if (si->getCenter() != NULL) {
			t->getPixmap()->copy(si->getCenter());
			//t->getPixmap()->splat(si->getCenter(), si->getCenter(), si->getCenter(), si->getCenter());
		} else {
			t->getPixmap()->splat(si->getLeftUp(), si->getRightUp(), si->getLeftDown(), si->getRightDown());
		}
	} else {
		si->setCoord(it->getCoord());
		si->setTexture(it->getTexture());
	}
}

void SurfaceAtlas::checkDimensions(const Pixmap2D *p) {
	if (surfaceSize == -1) {
		surfaceSize = p->getW();
		if (!isPowerOfTwo(surfaceSize)) {
			throw runtime_error("Bad surface texture dimensions (not power of two)");
		}
	}
	if (p->getW() != surfaceSize || p->getH() != surfaceSize) {
		throw runtime_error("Bad surface texture dimensions (all tileset textures are not same size)");
	}
}

void SurfaceAtlas2::buildTexture() {
	int numTex = surfaceInfos.size();
	int sideLength = int(sqrtf(float(numTex))) + 1;
	///@todo fix, no need to be square, this is wasting lots of tex mem
	m_width = m_height = nextPowerOf2(sideLength * surfaceSize);
	sideLength = m_width / surfaceSize;
	RUNTIME_CHECK(m_width % surfaceSize == 0);
	Texture2D *tex = g_renderer.newTexture2D(ResourceScope::GAME);
	tex->setWrapMode(Texture::wmClampToEdge);
	m_texture = tex;
	
	Pixmap2D *pixmap = tex->getPixmap();
	pixmap->init(m_width, m_height, 3);

	//Vec3f debugColour(1.f, 0.f, 0.f);
	//for (int y=0; y < m_height; ++y) {
	//	for (int x=0; x < m_width; ++x) {
	//		pixmap->setPixel(x, y, debugColour.ptr());
	//	}
	//}

	int x = 0, y = 0;
	int tx = 0, ty = 0;
	float stepSize = 1.f / float(sideLength);
	float pixelSize = 1.f / float(m_width);
	m_coordStep = stepSize - 2.f * pixelSize;

	foreach (SurfaceInfos, it, surfaceInfos) {
		pixmap->subCopy(x, y, it->getTexture()->getPixmap());
		float s = tx * stepSize + pixelSize;
		float t = ty * stepSize + pixelSize;
		it->setCoord(Vec2f(s, t));
		x += surfaceSize;
		++tx;
		if (x + surfaceSize > m_width) {
			x = 0;
			y += surfaceSize;
			++ty;
			tx = 0;
		}
	}
	//pixmap->savePng("terrain_tex.png");
}

}} // end namespace
