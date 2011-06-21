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

using Shared::Graphics::Gl::getGlMaxTextureSize;

// =====================================================
//	class SurfaceInfo
// =====================================================

SurfaceInfo::SurfaceInfo(const Pixmap2D *lu, const Pixmap2D *ru, const Pixmap2D *ld, const Pixmap2D *rd)
		: center(0)
		, leftUp(lu)
		, rightUp(ru) 
		, leftDown(ld)
		, rightDown(rd)
		, coord(Vec2f(0.f))
		, texId(0)
		, pixmap(0) {
}

SurfaceInfo::SurfaceInfo(const Pixmap2D *center)
		: center(center)
		, leftUp(0)
		, rightUp(0) 
		, leftDown(0)
		, rightDown(0)
		, coord(Vec2f(0.f))
		, texId(0)
		, pixmap(0) {
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

SurfaceAtlas::SurfaceAtlas(Vec2i size) 
		: m_coordStep(1.f)
		, size(size) {
	surfaceSize = -1;
}

SurfaceAtlas::~SurfaceAtlas() {
}

int SurfaceAtlas::addSurface(SurfaceInfo *si) {
	// check dimensions
	if (si->getCenter()) {
		checkDimensions(si->getCenter());
	} else {
		checkDimensions(si->getLeftUp());
		checkDimensions(si->getLeftDown());
		checkDimensions(si->getRightUp());
		checkDimensions(si->getRightDown());
	}

	// add info
	int ndx = -1;
	for (int i=0; i < surfaceInfos.size(); ++i) {
		if (surfaceInfos[i] == *si) {
			ndx = i;
			break;
		}
	}
	if (ndx == -1) {
		// add new texture
		Pixmap2D *pixmap = new Pixmap2D();
		pixmap->init(surfaceSize, surfaceSize, 3);

		si->setCoord(Vec2f(0.f, 0.f));
		si->setPixmap(pixmap);
		surfaceInfos.push_back(*si);

		// copy texture to pixmap
		if (si->getCenter() != NULL) {
			pixmap->copy(si->getCenter());
		} else {
			pixmap->splat(si->getLeftUp(), si->getRightUp(), si->getLeftDown(), si->getRightDown());
		}
		return surfaceInfos.size() - 1;
	} else {
		si->setCoord(surfaceInfos[ndx].getCoord());
		si->setPixmap(surfaceInfos[ndx].getPixmap());
		return ndx;
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

void SurfaceAtlas::deletePixmaps() {
	foreach (SurfaceInfos, it, surfaceInfos) {
		delete it->getPixmap();
		it->setPixmap(0);
	}
}

void SurfaceAtlas::disposePixmaps() {
	foreach (SurfaceInfos, it, surfaceInfos) {
		const_cast<Pixmap2D*>(it->getPixmap())->dispose();
	}
}

SurfaceAtlas2::SurfaceAtlas2(Vec2i size)
		: SurfaceAtlas(size)
		, m_infosByPos(0) {
	m_infosByPos = new uint32[size.w * size.h];
}

SurfaceAtlas2::~SurfaceAtlas2() {
	delete [] m_infosByPos;
}

int SurfaceAtlas2::addSurface(Vec2i pos, SurfaceInfo *si) {
	int res = SurfaceAtlas::addSurface(si);
	m_infosByPos[pos.y * size.w + pos.x] = res;
	return res;
}

void SurfaceAtlas2::buildTexture() {
	int numTex = surfaceInfos.size();
	int sideLength = int(sqrtf(float(numTex))) + 1;
	///@todo fix, no need to be square, this is wasting lots of tex mem
	int texSize = nextPowerOf2(sideLength * surfaceSize);

	int maxTex = getGlMaxTextureSize();
	if (texSize > maxTex) {
		sideLength = maxTex / surfaceSize;
		assert(maxTex % surfaceSize == 0);
		int texesPerPage = sideLength * sideLength;
		assert(numTex > texesPerPage);
		int pagesRequired = numTex / texesPerPage + 1;
		assert(pagesRequired > 1);

		// texture 0, max dims
		Texture2D *tex = g_renderer.newTexture2D(ResourceScope::GAME);
		tex->setWrapMode(Texture::wmClampToEdge);
		tex->getPixmap()->init(maxTex, maxTex, 3);
		m_textures.push_back(tex);

		int nTex = numTex - texesPerPage;
		for (int i=1; i < pagesRequired; ++i) {
			tex = g_renderer.newTexture2D(ResourceScope::GAME);
			tex->setWrapMode(Texture::wmClampToEdge);
			tex->getPixmap()->init(maxTex, maxTex, 3);
			m_textures.push_back(tex);
			nTex -= texesPerPage;
		}
		texSize = maxTex;
	} else {
		assert(texSize <= maxTex);
		Texture2D *tex = g_renderer.newTexture2D(ResourceScope::GAME);
		tex->setWrapMode(Texture::wmClampToEdge);
		tex->getPixmap()->init(texSize, texSize, 3);
		m_textures.push_back(tex);
	}

	sideLength = texSize / surfaceSize;
	float stepSize = 1.f / float(sideLength);
	float pixelSize = 1.f / float(texSize);
	m_coordStep = stepSize - 2.f * pixelSize;

	int texIndex = 0;
	int siIndex = 0;
	do {
		Pixmap2D *pixmap = const_cast<Pixmap2D*>(m_textures[texIndex]->getPixmap());
		int x = 0, y = 0;   // pixel position (to copy to)
		int tx = 0, ty = 0; // 'slot' in texture, to calc tex coords

		for ( ; siIndex < surfaceInfos.size(); ++siIndex) {
			pixmap->subCopy(x, y, surfaceInfos[siIndex].getPixmap());

			float s = tx * stepSize + pixelSize;
			float t = ty * stepSize + pixelSize;
			surfaceInfos[siIndex].setCoord(Vec2f(s, t));
			surfaceInfos[siIndex].setTexId(texIndex);
			x += surfaceSize;
			++tx;
			if (x + surfaceSize > texSize) {
				x = 0;
				y += surfaceSize;
				++ty;
				tx = 0;
				if (y == texSize) {
					break;
				}
			}
		}
		//pixmap->savePng("terrain_tex" + intToStr(texIndex) + ".png");
		++texIndex;
	} while (siIndex < surfaceInfos.size());
}

}} // end namespace
