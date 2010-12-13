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

TerrainRendererGlest::TerrainRendererGlest()
		: m_map(0), m_tileset(0), m_surfaceAtlas(0) {
}

void TerrainRendererGlest::end() {
	delete m_surfaceAtlas;
	m_surfaceAtlas = 0;
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
			sctl->setTileTexCoord(Vec2f(0.f));
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

		///@todo ? need this ?? ?
		//static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true); ?? surface rendering?
		renderer.enableProjectiveTexturing();
	}

	glActiveTexture(Renderer::baseTexUnit);

	SceneCuller::iterator it = culler.tile_begin();
	for ( ; it != culler.tile_end(); ++it) {
		const Vec2i pos = *it;
		if (mapBounds.isInside(pos)) {

			Tile *tc00= m_map->getTile(pos.x, pos.y);
			Tile *tc10= m_map->getTile(pos.x+1, pos.y);
			Tile *tc01= m_map->getTile(pos.x, pos.y+1);
			Tile *tc11= m_map->getTile(pos.x+1, pos.y+1);

			renderer.incTriangleCount(2);
			renderer.incPointCount(4);

			// set texture
			currTex= static_cast<const Texture2DGl*>(tc00->getTileTexture())->getHandle();
			if (currTex != lastTex) {
				lastTex = currTex;
				glBindTexture(GL_TEXTURE_2D, lastTex);
			}

			Vec2f surfCoord = tc00->getSurfTexCoord();

			glBegin(GL_TRIANGLE_STRIP);

			// draw quad using immediate mode
			glMultiTexCoord2fv(Renderer::fowTexUnit, tc01->getFowTexCoord().ptr());
			glMultiTexCoord2f(Renderer::baseTexUnit, surfCoord.x, surfCoord.y+coordStep);
			glNormal3fv(tc01->getNormal().ptr());
			glVertex3fv(tc01->getVertex().ptr());

			glMultiTexCoord2fv(Renderer::fowTexUnit, tc00->getFowTexCoord().ptr());
			glMultiTexCoord2f(Renderer::baseTexUnit, surfCoord.x, surfCoord.y);
			glNormal3fv(tc00->getNormal().ptr());
			glVertex3fv(tc00->getVertex().ptr());

			glMultiTexCoord2fv(Renderer::fowTexUnit, tc11->getFowTexCoord().ptr());
			glMultiTexCoord2f(Renderer::baseTexUnit, surfCoord.x+coordStep, surfCoord.y+coordStep);
			glNormal3fv(tc11->getNormal().ptr());
			glVertex3fv(tc11->getVertex().ptr());

			glMultiTexCoord2fv(Renderer::fowTexUnit, tc10->getFowTexCoord().ptr());
			glMultiTexCoord2f(Renderer::baseTexUnit, surfCoord.x+coordStep, surfCoord.y);
			glNormal3fv(tc10->getNormal().ptr());
			glVertex3fv(tc10->getVertex().ptr());

			glEnd();
		}
	}

	//Restore
	//static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(false);
	glPopAttrib();

	//assert
	glGetError();	//remove when first mtex problem solved
	assertGl();

	IF_DEBUG_EDITION(
		} // end else, if not renderering textures instead of terrain
		Debug::getDebugRenderer().renderEffects(culler);
	)
}


}} // end namespace Glest::Graphics
