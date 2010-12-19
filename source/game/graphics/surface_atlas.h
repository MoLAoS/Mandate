// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
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
	const Texture2D *texture;

public:
	SurfaceInfo(const Pixmap2D *center);
	SurfaceInfo(const Pixmap2D *lu, const Pixmap2D *ru, const Pixmap2D *ld, const Pixmap2D *rd);
	bool operator==(const SurfaceInfo &si) const;

	const Pixmap2D *getCenter() const		{return center;}
	const Pixmap2D *getLeftUp() const		{return leftUp;}
	const Pixmap2D *getRightUp() const		{return rightUp;}
	const Pixmap2D *getLeftDown() const		{return leftDown;}
	const Pixmap2D *getRightDown() const	{return rightDown;}
	const Vec2f &getCoord() const			{return coord;}
	const Texture2D *getTexture() const		{return texture;}

	void setCoord(const Vec2f &coord)			{this->coord= coord;}
	void setTexture(const Texture2D *texture)	{this->texture= texture;}
};	

// =====================================================
// 	class SurfaceAtlas
//
/// Holds all surface textures for a given Tileset
// =====================================================

class SurfaceAtlas{
protected:
	typedef vector<SurfaceInfo> SurfaceInfos;

protected:
	SurfaceInfos surfaceInfos;
	int surfaceSize;
	float m_coordStep;

public:
	SurfaceAtlas();

	void addSurface(SurfaceInfo *si);
	float getCoordStep() const { return m_coordStep; }

private:
	void checkDimensions(const Pixmap2D *p);
};

class SurfaceAtlas2 : public SurfaceAtlas {
private:
	const Texture2D *m_texture;
	int		m_width, m_height;

public:
	SurfaceAtlas2() : m_texture(0), m_width(0), m_height(0) { }

	void buildTexture();
	Vec2f getTexCoords(const Texture2D *tex) {
		foreach (SurfaceInfos, it, surfaceInfos) {
			if (it->getTexture() == tex) {
				return it->getCoord();
			}
		}
		throw runtime_error("Texture not in atlas!");
	}

	const Texture2D* getMasterTexture() const { return m_texture; }
};

}}//end namespace

#endif
