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

#ifndef _GLEST_GRAPHICS_TARRAIN_RENDERER_H_
#define _GLEST_GRAPHICS_TARRAIN_RENDERER_H_

// shared_lib
#include "vec.h"
#include "math_util.h"
#include "pixmap.h"
#include "matrix.h"
#include "texture.h"

// game
#include "scene_culler.h"
#include "surface_atlas.h"
#include "map.h"

using namespace Shared::Math;
using namespace Shared::Graphics;

namespace Glest { namespace Sim {
	class Tileset;
}}

namespace Glest { namespace Graphics {

using namespace Sim;

// ===========================================================
// 	class TerrainRenderer
//
///	Abstract base class for terrain renderering sub-systems
// ===========================================================
class TerrainRenderer {
protected:
	MapVertexData *m_mapData;
	Vec2i          m_size;

protected:
	void initMapData(const Vec2i &size, MapVertexData *data);

private:
	TerrainRenderer(const TerrainRenderer&) {} // no copy

public:
	TerrainRenderer() : m_mapData(0), m_size(0) {}

	virtual void init(Map *map, Tileset *tileset) = 0;

	virtual ~TerrainRenderer();

	virtual bool checkCaps() = 0; /**< Check GL caps, can this renderer be used? */
	virtual void render(SceneCuller &culler) = 0; /**< render visible terrain */
};

// ===========================================================
// 	class TerrainRendererGlest
//
///	Original glest immidiate mode surface renderering code
/// using pre-splatted textures
// ===========================================================

class TerrainRendererGlest : public TerrainRenderer {
protected:
	Map				*m_map;
	Tileset			*m_tileset;
	SurfaceAtlas	*m_surfaceAtlas;

	const Texture2D* addSurfTex(int tl, int tr, int bl, int br);
	void splatTextures();

public:
	TerrainRendererGlest();
	~TerrainRendererGlest();

	virtual bool checkCaps() override; /**< Check GL caps, can this renderer be used? */
	virtual void init(Map *map, Tileset *tileset) override;
	virtual void render(SceneCuller &culler) override; /**< render visible terrain */
};

// ===========================================================
// 	class TerrainRenderer2
//
///	experimental
// ===========================================================

class TerrainRenderer2 : public TerrainRendererGlest {
protected:
	GLuint			m_vertexBuffer;
	vector<uint32>	m_indexArray;

	void updateVertexData(Rect2i &area = Rect2i(-1, -1, -1, -1));

public:
	TerrainRenderer2();
	~TerrainRenderer2();

	virtual bool checkCaps() override;
	virtual void init(Map *map, Tileset *tileset) override;
	virtual void render(SceneCuller &culler) override;
};

}} // end namespace Glest::Graphics

#endif // ndef _GLEST_GRAPHICS_TARRAIN_RENDERER_H_
