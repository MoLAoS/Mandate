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

#ifndef _GLEST_GAME_SURFACEATLAS_H_
#define _GLEST_GAME_SURFACEATLAS_H_

#include <vector>
#include <set>

#include "texture.h"
#include "vec.h"

using std::vector;
using std::set;
using Shared::Graphics::Pixmap2D;
using Shared::Graphics::Texture2D;
using Shared::Math::Vec2i;
using Shared::Math::Vec2f;

namespace Glest { namespace Graphics {

// =====================================================
//	class SurfaceInfo
// =====================================================

class SurfaceInfo {
private:
	const Pixmap2D *center;
	const Pixmap2D *leftUp;
	const Pixmap2D *rightUp;
	const Pixmap2D *leftDown;
	const Pixmap2D *rightDown;
	Vec2f coord;
	int   texId;

	const Pixmap2D *pixmap;

public:
	SurfaceInfo() { memset(this, 0, sizeof(*this)); }
	SurfaceInfo(const Pixmap2D *center);
	SurfaceInfo(const Pixmap2D *lu, const Pixmap2D *ru, const Pixmap2D *ld, const Pixmap2D *rd);
	bool operator==(const SurfaceInfo &si) const;

	const Pixmap2D *getCenter() const		{return center;}
	const Pixmap2D *getLeftUp() const		{return leftUp;}
	const Pixmap2D *getRightUp() const		{return rightUp;}
	const Pixmap2D *getLeftDown() const		{return leftDown;}
	const Pixmap2D *getRightDown() const	{return rightDown;}
	const Vec2f &getCoord() const			{return coord;}
	int getTexId() const                    {return texId;}
	const Pixmap2D *getPixmap() const		{return pixmap;}

	void setCoord(const Vec2f &coord)		{this->coord= coord;}
	void setTexId(int id)                   {this->texId = id;}
	void setPixmap(const Pixmap2D *pixmap)	{this->pixmap= pixmap;}
};	

// =====================================================
// 	class SurfaceAtlas
//
/// Holds all surface textures for a given Tileset
// =====================================================

class SurfaceAtlas {
protected:
	typedef vector<SurfaceInfo> SurfaceInfos;

protected:
	SurfaceInfos surfaceInfos;
	Vec2i size;
	int surfaceSize;
	float m_coordStep;

public:
	SurfaceAtlas(Vec2i size);
	virtual ~SurfaceAtlas();

	int addSurface(SurfaceInfo *si);
	float getCoordStep() const { return m_coordStep; }
	void deletePixmaps();
	void disposePixmaps();

private:
	void checkDimensions(const Pixmap2D *p);
};

// =====================================================
// 	class SurfaceAtlas2, hacky extension of SurfaceAtlas
//
/// Composites surface textures into a single texture
/// and adjusts texture co-ordinates accordingly.
// =====================================================

///@todo: possibly best to not to derive from SurfaceAtlas, or refactor more
/// so the SurfaceAtlas doesn't create gl textures for each splatted tile.

class SurfaceAtlas2 : public SurfaceAtlas {
private:
	//const Texture2D *m_texture;
	vector<const Texture2D*> m_textures;
	uint32 *m_infosByPos;

public:
	SurfaceAtlas2(Vec2i size);
	~SurfaceAtlas2();

	void buildTexture();
	int addSurface(Vec2i pos, SurfaceInfo *si);
	
	Vec2f getTexCoords(const Pixmap2D *pm) {
		foreach (SurfaceInfos, it, surfaceInfos) {
			if (it->getPixmap() == pm) {
				return it->getCoord();
			}
		}
		throw runtime_error("Texture not in atlas!");
	}

	SurfaceInfo* getSurfaceInfo(const Vec2i &pos) { return &surfaceInfos[m_infosByPos[pos.y * size.w + pos.x]]; }

	int getTextureCount() const { return m_textures.size(); }
	const Texture2D* getTexture(int i) const { return m_textures[i]; }
};

}}//end namespace

#endif
