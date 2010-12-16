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
	delete m_mapData;
}

void TerrainRenderer::initMapData(const Vec2i &size, MapVertexData *data) {
	m_size = size;
	m_mapData = data;

	// compute texture coords (for FoW tex unit)
	float xStep = 1.f / (m_size.w - 1.f);
	float yStep = 1.f / (m_size.h - 1.f);
	for (int i=0; i < m_size.w; ++i) {
		for (int j=0; j < m_size.h; ++j) {
			m_mapData->get(i, j).texCoord() = Vec2f(i * xStep, j * yStep);
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

const Texture2D* TerrainRendererGlest::addSurfTex(int tl, int tr, int bl, int br) {
	// center textures
	if (tl == tr && tl == bl && tl == br) {
		SurfaceInfo si(m_tileset->getSurfPixmap(tl));
		m_surfaceAtlas->addSurface(&si);
		return si.getTexture();
	}

	// else splat
	SurfaceInfo si(
		m_tileset->getSurfPixmap(tl),
		m_tileset->getSurfPixmap(tr),
		m_tileset->getSurfPixmap(bl),
		m_tileset->getSurfPixmap(br));
	m_surfaceAtlas->addSurface(&si);
	return si.getTexture();
}


void TerrainRendererGlest::init(Map *map, Tileset *tileset) {
	m_map = map;
	Vec2i size(map->getTileW(), map->getTileH());
	TerrainRenderer::initMapData(size, map->getVertexData());

	m_tileset = tileset;
	m_surfaceAtlas = new SurfaceAtlas();

	for (int i = 0; i < m_map->getTileW() - 1; ++i) {
		for (int j = 0; j < m_map->getTileH() - 1; ++j) {
			Vec2f coord;
			const Texture2D *texture;
			Tile *sctl = m_map->getTile(i, j);
			Tile *sctr = m_map->getTile(i + 1, j);
			Tile *scbl = m_map->getTile(i, j + 1);
			Tile *scbr = m_map->getTile(i + 1, j + 1);

			const Texture2D *tex;
			tex = addSurfTex(sctl->getTileType(), sctr->getTileType(), scbl->getTileType(), scbr->getTileType());
			sctl->setTileTexture(tex);
		}
	}
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
			currTex= static_cast<const Texture2DGl*>(tile->getTileTexture())->getHandle();
			if (currTex != lastTex) {
				lastTex = currTex;
				glBindTexture(GL_TEXTURE_2D, lastTex);
			}

			glBegin(GL_TRIANGLE_STRIP);

			// draw quad using immediate mode
			glMultiTexCoord2fv(Renderer::fowTexUnit, vert01.texCoord().ptr());
			glMultiTexCoord2f(Renderer::baseTexUnit, surfCoord.x, surfCoord.y + coordStep);
			glNormal3fv(vert01.norm().ptr());
			glVertex3fv(vert01.vert().ptr());

			glMultiTexCoord2fv(Renderer::fowTexUnit, vert00.texCoord().ptr());
			glMultiTexCoord2f(Renderer::baseTexUnit, surfCoord.x, surfCoord.y);
			glNormal3fv(vert00.norm().ptr());
			glVertex3fv(vert00.vert().ptr());

			glMultiTexCoord2fv(Renderer::fowTexUnit, vert11.texCoord().ptr());
			glMultiTexCoord2f(Renderer::baseTexUnit, surfCoord.x + coordStep, surfCoord.y + coordStep);
			glNormal3fv(vert11.norm().ptr());
			glVertex3fv(vert11.vert().ptr());

			glMultiTexCoord2fv(Renderer::fowTexUnit, vert10.texCoord().ptr());
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


}} // end namespace Glest::Graphics
