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

#include "terrain_renderer.h"
#include "renderer.h"
#include "texture_gl.h"
#include "game.h"
#include "opengl.h"
#include "faction.h"
#include "sim_interface.h"
#include "debug_stats.h"
#include "map.h"
#include "tileset.h"

#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

namespace Glest { namespace Graphics {

// ===========================================================
// 	class TerrainRenderer
// ===========================================================

TerrainRenderer::~TerrainRenderer() {
}

void TerrainRenderer::initMapData(const Vec2i &size, MapVertexData *data) {
	m_size = size;
	m_mapData = data;

	// compute texture coords (for FoW tex unit)
	float xStep = 1.f / (m_size.w - 1.f);
	float yStep = 1.f / (m_size.h - 1.f);
	for (int i=0; i < m_size.w; ++i) {
		for (int j=0; j < m_size.h; ++j) {
			m_mapData->get(i, j).fowTexCoord() = Vec2f(i * xStep, j * yStep);
		}
	}
}

// ===========================================================
// 	class TerrainRendererGlest
// ===========================================================

TerrainRendererGlest::TerrainRendererGlest()
		: m_map(0), m_tileset(0), m_surfaceAtlas(0) {
}

TerrainRendererGlest::~TerrainRendererGlest() {
	delete m_surfaceAtlas;
}

bool TerrainRendererGlest::checkCaps() {
	return isGlVersionSupported(1, 3, 0);
}

const Pixmap2D* TerrainRendererGlest::addSurfTex(int tl, int tr, int bl, int br) {
	// center textures
	if (tl == tr && tl == bl && tl == br) {
		SurfaceInfo si(m_tileset->getSurfPixmap(tl));
		m_surfaceAtlas->addSurface(&si);
		return si.getPixmap();
	}

	// else splat
	SurfaceInfo si(
		m_tileset->getSurfPixmap(tl),
		m_tileset->getSurfPixmap(tr),
		m_tileset->getSurfPixmap(bl),
		m_tileset->getSurfPixmap(br));
	m_surfaceAtlas->addSurface(&si);
	return si.getPixmap();
}

void TerrainRendererGlest::splatTextures() {
	map<const Pixmap2D*, Texture2D*> texMap;
	Texture2D::Filter textureFilter = Renderer::strToTextureFilter(g_config.getRenderFilter());
	int maxAnisotropy = g_config.getRenderFilterMaxAnisotropy();
	for (int i = 0; i < m_map->getTileW() - 1; ++i) {
		for (int j = 0; j < m_map->getTileH() - 1; ++j) {
			Vec2f coord;
			Tile *sctl = m_map->getTile(i, j);
			Tile *sctr = m_map->getTile(i + 1, j);
			Tile *scbl = m_map->getTile(i, j + 1);
			Tile *scbr = m_map->getTile(i + 1, j + 1);

			const Pixmap2D *pixmap = addSurfTex(sctl->getTileType(), sctr->getTileType(), scbl->getTileType(), scbr->getTileType());
			Texture2D *tex = texMap[pixmap];
			if (!tex) {
				tex = g_renderer.newTexture2D(ResourceScope::GAME);				
				tex->setWrapMode(Texture::wmClampToEdge);
				tex->setPixmap(pixmap);
				tex->init(textureFilter, maxAnisotropy);
				texMap[pixmap] = tex;
			}
			sctl->setTexId(static_cast<Texture2DGl*>(tex)->getHandle());
		}
	}
}

void TerrainRendererGlest::init(Map *map, Tileset *tileset) {
	m_map = map;
	Vec2i size(map->getTileW(), map->getTileH());
	TerrainRenderer::initMapData(size, map->getVertexData());

	m_tileset = tileset;
	m_surfaceAtlas = new SurfaceAtlas(size);

	splatTextures();
	m_surfaceAtlas->disposePixmaps();
}

void TerrainRendererGlest::render(SceneCuller &culler) {
	SECTION_TIMER(RENDER_SURFACE);
	IF_DEBUG_EDITION(
		if (Debug::getDebugRenderer().willRenderSurface()) {
			Debug::getDebugRenderer().renderSurface(culler);
		} else {
	)

	Renderer &renderer = g_renderer;
	int lastTex=-1;
	int currTex;
	const Rect2i mapBounds(0, 0, m_map->getTileW() - 1, m_map->getTileH() - 1);

	Vec2f surfCoord(0.f);
	float coordStep = m_surfaceAtlas->getCoordStep();

	assertGl();

	const Texture2D *fowTex = g_userInterface.getMinimap()->getFowTexture();

	glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT);

	glEnable(GL_BLEND);
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_ALPHA_TEST);

	// fog of war tex unit
	glActiveTexture(Renderer::fowTexUnit);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(fowTex)->getHandle());
	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0,
		fowTex->getPixmap()->getW(), fowTex->getPixmap()->getH(),
		GL_ALPHA, GL_UNSIGNED_BYTE, fowTex->getPixmap()->getPixels());

	// shadow texture
	ShadowMode shadows = renderer.getShadowMode();
	if (shadows == ShadowMode::PROJECTED || shadows == ShadowMode::MAPPED) {
		glActiveTexture(Renderer::shadowTexUnit);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, renderer.getShadowMapHandle());
		renderer.enableProjectiveTexturing();
	}

	glActiveTexture(Renderer::baseTexUnit);

	SceneCuller::iterator it = culler.tile_begin();
	for ( ; it != culler.tile_end(); ++it) {
		const Vec2i pos = *it;
		if (mapBounds.isInside(pos)) {
			Tile *tile = m_map->getTile(pos);
			TileVertex &vert00 = m_mapData->get(pos.x + 0, pos.y + 0);
			TileVertex &vert10 = m_mapData->get(pos.x + 1, pos.y + 0);
			TileVertex &vert01 = m_mapData->get(pos.x + 0, pos.y + 1);
			TileVertex &vert11 = m_mapData->get(pos.x + 1, pos.y + 1);

			renderer.incTriangleCount(2);
			renderer.incPointCount(4);

			// set texture
			currTex = tile->getTexId();
			if (currTex != lastTex) {
				lastTex = currTex;
				glBindTexture(GL_TEXTURE_2D, lastTex);
			}

			glBegin(GL_TRIANGLE_STRIP);

			// draw quad using immediate mode
			glMultiTexCoord2fv(Renderer::fowTexUnit, vert01.fowTexCoord().ptr());
			glMultiTexCoord2f(Renderer::baseTexUnit, surfCoord.x, surfCoord.y + coordStep);
			glNormal3fv(vert01.norm().ptr());
			glVertex3fv(vert01.vert().ptr());

			glMultiTexCoord2fv(Renderer::fowTexUnit, vert00.fowTexCoord().ptr());
			glMultiTexCoord2f(Renderer::baseTexUnit, surfCoord.x, surfCoord.y);
			glNormal3fv(vert00.norm().ptr());
			glVertex3fv(vert00.vert().ptr());

			glMultiTexCoord2fv(Renderer::fowTexUnit, vert11.fowTexCoord().ptr());
			glMultiTexCoord2f(Renderer::baseTexUnit, surfCoord.x + coordStep, surfCoord.y + coordStep);
			glNormal3fv(vert11.norm().ptr());
			glVertex3fv(vert11.vert().ptr());

			glMultiTexCoord2fv(Renderer::fowTexUnit, vert10.fowTexCoord().ptr());
			glMultiTexCoord2f(Renderer::baseTexUnit, surfCoord.x + coordStep, surfCoord.y);
			glNormal3fv(vert10.norm().ptr());
			glVertex3fv(vert10.vert().ptr());

			glEnd();
		}
	}

	//Restore
	glPopAttrib();

	//assert
	assertGl();

	IF_DEBUG_EDITION(
		} // end else, if not renderering debug textures instead of regular terrain
		Debug::getDebugRenderer().renderEffects(culler);
	)
}

// ===========================================================
// 	class TerrainRenderer2
// ===========================================================

TerrainRenderer2::TerrainRenderer2()
		: m_vertexBuffer(0) {
}

TerrainRenderer2::~TerrainRenderer2() {
	if (m_vertexBuffer) {
		glDeleteBuffers(1, &m_vertexBuffer);
	}
	delete [] m_indexArrays;
}

bool TerrainRenderer2::checkCaps() {
	return isGlVersionSupported(1, 5, 0);
}

void TerrainRenderer2::updateVertexData(Rect2i area) {
	if (area.p[0] == Vec2i(-1)) {
		area = Rect2i(Vec2i(0), Vec2i(m_size.w - 2, m_size.h - 2));
	}
	const Vec2i right(1, 0);
	const Vec2i down(0, 1);
	const Vec2i diag(1, 1);
	const float step = m_surfaceAtlas->getCoordStep();

	///@todo fix (does not do partial updates at all right)
	TileVertex *myData = new TileVertex[(m_size.w - 1) * (m_size.h - 1) * 4];
	RectIterator iter(area.p[0], area.p[1]);
	while (iter.more()) {
		Vec2i pos = iter.next();
		const int ndx = 4 * pos.y * (m_size.w - 1) + 4 * pos.x;
		TileVertex &tl = m_mapData->get(pos);
		TileVertex &tr = m_mapData->get(pos + right);
		TileVertex &br = m_mapData->get(pos + diag);
		TileVertex &bl = m_mapData->get(pos + down);

		Vec2f ttCoord = static_cast<SurfaceAtlas2*>(m_surfaceAtlas)->getSurfaceInfo(pos)->getCoord();
		if (ttCoord.u < 0.f || ttCoord.u > 1.f || ttCoord.v < 0.f || ttCoord.v > 1.f) {
			DEBUG_HOOK();
		}

		myData[ndx + 0] = tl;
		myData[ndx + 0].tileTexCoord() = ttCoord + Vec2f(0.f, 0.f);
		myData[ndx + 1] = tr;
		myData[ndx + 1].tileTexCoord() = ttCoord + Vec2f(step, 0.f);
		myData[ndx + 2] = br;
		myData[ndx + 2].tileTexCoord() = ttCoord + Vec2f(step, step);
		myData[ndx + 3] = bl;
		myData[ndx + 3].tileTexCoord() = ttCoord + Vec2f(0.f, step);
	}

	size_t size = sizeof(TileVertex) * (m_size.w - 1) * (m_size.h - 1) * 4;
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	void *vbo = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(vbo, myData, size);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	delete [] myData;
}

void TerrainRenderer2::init(Map *map, Tileset *tileset) {
	m_map = map;
	Vec2i size(map->getTileW(), map->getTileH());

	g_logger.logProgramEvent("init tr2");

	TerrainRenderer::initMapData(size, map->getVertexData());

	m_tileset = tileset;
	SurfaceAtlas2 *atlas = new SurfaceAtlas2(size);
	m_surfaceAtlas = atlas;
	splatTextures();
	try {
		atlas->buildTexture();
	} catch (...) {
		delete m_surfaceAtlas;
		throw;
	}

	for (int y=0; y < size.h - 1; ++y) {
		for (int x=0; x < size.w - 1; ++x) {
			int id = atlas->getSurfaceInfo(Vec2i(x, y))->getTexId();
			map->getTile(x, y)->setTexId(id);
		}
	}
	m_indexArrays = new vector<uint32>[atlas->getTextureCount()];

	// delete pixmap data...
	m_surfaceAtlas->deletePixmaps();

	glGenBuffers(1, &m_vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);

	int buffSize = sizeof(TileVertex) * (m_size.w - 1) * (m_size.h - 1) * 4;
	glBufferData(GL_ARRAY_BUFFER, buffSize, 0, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	updateVertexData();
}

void TerrainRenderer2::splatTextures() {
	SurfaceAtlas2 *atlas = static_cast<SurfaceAtlas2*>(m_surfaceAtlas);
	for (int i = 0; i < m_map->getTileW() - 1; ++i) {
		for (int j = 0; j < m_map->getTileH() - 1; ++j) {
			Vec2f coord;
			int tl = m_map->getTile(i, j)->getTileType();
			int tr = m_map->getTile(i + 1, j)->getTileType();
			int bl = m_map->getTile(i, j + 1)->getTileType();
			int br = m_map->getTile(i + 1, j + 1)->getTileType();

			// center textures
			if (tl == tr && tl == bl && tl == br) {
				SurfaceInfo si(m_tileset->getSurfPixmap(tl));
				atlas->addSurface(Vec2i(i, j), &si);
			} else {
				// else splat
				SurfaceInfo si(
					m_tileset->getSurfPixmap(tl),
					m_tileset->getSurfPixmap(tr),
					m_tileset->getSurfPixmap(bl),
					m_tileset->getSurfPixmap(br));
				atlas->addSurface(Vec2i(i, j), &si);
			}
//			sctl->setTexId(0);
		}
	}
}

void TerrainRenderer2::render(SceneCuller &culler) {
	SECTION_TIMER(RENDER_SURFACE);

	Renderer &renderer = g_renderer;
	const Rect2i mapBounds(0, 0, m_map->getTileW() - 1, m_map->getTileH() - 1);

	assertGl();

	// build index arrays (one per texture)
	int texCount = static_cast<SurfaceAtlas2*>(m_surfaceAtlas)->getTextureCount();
	int *tileCounts = new int[texCount];
	for (int i=0; i < texCount; ++i) {
		m_indexArrays[i].clear();
		tileCounts[i] = 0;
	}
	SceneCuller::iterator it = culler.tile_begin();
	for ( ; it != culler.tile_end(); ++it) {
		const Vec2i pos = *it;
		if (!mapBounds.isInside(pos)) {
			continue;
		}
		const int ndx = 4 * pos.y * (m_size.w - 1) + 4 * pos.x;
		const int tex = m_map->getTile(pos)->getTexId();
		ASSERT_RANGE(tex, texCount);
		m_indexArrays[tex].push_back(ndx + 0);
		m_indexArrays[tex].push_back(ndx + 1);
		m_indexArrays[tex].push_back(ndx + 2);
		m_indexArrays[tex].push_back(ndx + 3);
		++tileCounts[tex];
	}

	// set up gl state
	assertGl();
	glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT);
	glEnable(GL_BLEND);
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_ALPHA_TEST);

	const int stride = sizeof(TileVertex);

#	define VBO_OFFSET(x) ((void*)(x * sizeof(float)))

	// bind vbo, enable arrays & set vert and normal offsets
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, VBO_OFFSET(0));
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, stride, VBO_OFFSET(3));

	// fog of war texture
	glActiveTexture(Renderer::fowTexUnit);
	glClientActiveTexture(Renderer::fowTexUnit);
	const Texture2D *fowTex = g_userInterface.getMinimap()->getFowTexture();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(fowTex)->getHandle());
	
	// update gl texture...
	///@todo don't do this here, update after 'unitTex' is updated (every fifth call to Minimap::update())
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fowTex->getPixmap()->getW(), fowTex->getPixmap()->getH(),
		GL_ALPHA, GL_UNSIGNED_BYTE, fowTex->getPixmap()->getPixels());
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, stride, VBO_OFFSET(8));

	// shadow texture
	ShadowMode shadows = renderer.getShadowMode();
	if (shadows == ShadowMode::PROJECTED || shadows == ShadowMode::MAPPED) {
		glActiveTexture(Renderer::shadowTexUnit);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, renderer.getShadowMapHandle());
		renderer.enableProjectiveTexturing();
	}
	assertGl();

	// activate base texture
	glActiveTexture(Renderer::baseTexUnit);
	glClientActiveTexture(Renderer::baseTexUnit);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, stride, VBO_OFFSET(6));
	assertGl();

	// for each texture
	for (int i=0; i < texCount; ++i) {
		if (m_indexArrays[i].empty()) {
			continue;
		}
		// set texture
		const Texture2D *baseTex = static_cast<SurfaceAtlas2*>(m_surfaceAtlas)->getTexture(i);
		GLuint texHandle = static_cast<const Texture2DGl*>(baseTex)->getHandle();
		glBindTexture(GL_TEXTURE_2D, texHandle);

		// assert
		assertGl();
		assert(tileCounts[i] * 4 == m_indexArrays[i].size());

		// zap
		glDrawElements(GL_QUADS, tileCounts[i] * 4, GL_UNSIGNED_INT, &m_indexArrays[i][0]);
	}
	delete [] tileCounts;

	// disable arrays/buffers & restore state
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glClientActiveTexture(Renderer::fowTexUnit);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(Renderer::baseTexUnit);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glPopAttrib();
	assertGl();
}

}} // end namespace Glest::Graphics
