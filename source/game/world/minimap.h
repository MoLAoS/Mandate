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

#ifndef _GLEST_GAME_MINIMAP_H_
#define _GLEST_GAME_MINIMAP_H_

#include <cassert>

#include "pixmap.h"
#include "texture.h"

namespace Shared { namespace Platform {
	class NetworkDataBuffer;
}}

namespace Glest{ namespace Game{

using Shared::Graphics::Vec4f;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Pixmap2D;
using Shared::Graphics::Texture2D;
using Shared::Platform::NetworkDataBuffer;

class World;
class Map;

enum ExplorationState{
    esNotExplored,
    esExplored,
    esVisible
};

// =====================================================
// 	class Minimap
//
/// State of the in-game minimap
// =====================================================

class Minimap{
private:
	Pixmap2D *fowPixmap0;
	Pixmap2D *fowPixmap1;
	Texture2D *tex;
	Texture2D *fowTex;    //Fog Of War Texture2D
	bool fogOfWar;

private:
	static const float exploredAlpha;

public:
    void init(int x, int y, const World *world);
	Minimap();
	~Minimap();

	const Texture2D *getFowTexture() const	{return fowTex;}
	const Texture2D *getTexture() const		{return tex;}

	void resetFowTex();
	void updateFowTex(float t);
	// heavy use function
	void incFowTextureAlphaSurface(const Vec2i &sPos, float alpha) {
		assert(sPos.x < fowPixmap1->getW() && sPos.y < fowPixmap1->getH());
	
		if(fowPixmap1->getPixelf(sPos.x, sPos.y) < alpha) {
			fowPixmap1->setPixel(sPos.x, sPos.y, alpha);
		}
	}

	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;
	void synthesize(const Map *map, int team);

private:
	void computeTexture(const World *world);
	static void writeRepition(NetworkDataBuffer &buf, uint8 lastResult, uint32 count, int x, int y);
};

}}//end namespace

#endif
