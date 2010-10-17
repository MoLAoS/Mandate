// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "minimap.h"

#include <cassert>

#include "world.h"
#include "vec.h"
#include "renderer.h"
#include "config.h"
#include "object.h"
#include "types.h"
#include "socket.h"
#include "map.h"
#include "program.h"
#include "game.h"
#include "sim_interface.h"
#include "texture_gl.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using Shared::Platform::uint8;

namespace Glest { namespace Sim {
using Main::Program;

// =====================================================
// 	class Minimap
// =====================================================

const float Minimap::exploredAlpha = 0.5f;
const Vec2i Minimap::textureSize = Vec2i(128, 128);

Minimap::Minimap(bool FoW, Container* parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, MouseWidget(this)
		, m_fowPixmap0(0)
		, m_fowPixmap1(0)
		, m_terrainTex(0)
		, m_fowTex(0)
		, m_unitsTex(0)
		, m_unitsPMap(0)
		, m_w(0)
		, m_h(0)
		, m_ratio(1)
		, m_currZoom(1)
		, m_maxZoom(1)
		, m_minZoom(1)
		, m_fogOfWar(FoW)
		, m_shroudOfDarkness(FoW)
		, m_draggingCamera(false)
		, m_draggingWidget(false)
		, m_leftClickOrder(false)
		, m_rightClickOrder(false)
		, m_moveOffset(0) {
	
	m_borderStyle.setSizes(4);
	m_borderStyle.m_sizes[Border::TOP] = 12;
	m_borderStyle.m_type = BorderType::CUSTOM_CORNERS;
	m_borderStyle.m_colourIndices[Corner::TOP_LEFT]		= g_widgetConfig.getColourIndex(Colour(255, 255, 255, 127));
	m_borderStyle.m_colourIndices[Corner::TOP_RIGHT]	= g_widgetConfig.getColourIndex(Colour(191, 191, 191, 127));
	m_borderStyle.m_colourIndices[Corner::BOTTOM_RIGHT] = g_widgetConfig.getColourIndex(Colour(63, 63, 63, 127));
	m_borderStyle.m_colourIndices[Corner::BOTTOM_LEFT]	= g_widgetConfig.getColourIndex(Colour(191, 191, 191, 127));
}

void Minimap::init(int w, int h, const World *world, bool resumingGame){
	int scaledW = w / GameConstants::cellScale;
	int scaledH = h / GameConstants::cellScale;
	m_w = w;
	m_h = h;

	m_ratio = fixed(w) / h;
	m_minZoom = m_currZoom = fixed(128) / std::max(w, h);
	m_maxZoom = std::max(w, h) / fixed(128);

	// fow pixmaps
	float f= 0.f;
	m_fowPixmap0 = new Pixmap2D(nextPowerOf2(scaledW), nextPowerOf2(scaledH), 1);
	m_fowPixmap1 = new Pixmap2D(nextPowerOf2(scaledW), nextPowerOf2(scaledH), 1);
	m_fowPixmap0->setPixels(&f);
	m_fowPixmap1->setPixels(&f);

	if (resumingGame) {
		setExploredState(world);
	}

	// fow texture
	m_fowTex = g_renderer.newTexture2D(ResourceScope::GAME);
	m_fowTex->setMipmap(false);
	m_fowTex->setPixmapInit(false);
	m_fowTex->setFormat(Texture::fAlpha);
	m_fowTex->getPixmap()->init(nextPowerOf2(scaledW), nextPowerOf2(scaledH), 1);
	m_fowTex->getPixmap()->setPixels(&f);

	// terrain texture
	m_terrainTex = g_renderer.newTexture2D(ResourceScope::GAME);
	m_terrainTex->getPixmap()->init(scaledW, scaledH, 3);
	m_terrainTex->setMipmap(false);

	m_unitsTex = g_renderer.newTexture2D(ResourceScope::GAME);
	m_unitsTex->setMipmap(false);
	int tw = m_ratio < 1 ? (m_ratio * 128).intp() : 128;
	int th = m_ratio > 1 ? (128 / m_ratio).intp() : 128;
	m_unitsTex->getPixmap()->init(tw, th, 4);
	m_unitsTex->setFormat(Texture::fRgba);
	Colour blank((uint8)0u);
	m_unitsTex->getPixmap()->setPixels(blank.ptr());

	m_unitsPMap = new TypeMap<int8>(Rectangle(0, 0, w, h), -1);
	m_unitsPMap->clearMap(-1);
	
	computeTexture(world);
}

Minimap::~Minimap(){
	delete m_fowPixmap0;
	delete m_fowPixmap1;
	delete m_unitsPMap;
}

// ==================== set ====================

void Minimap::resetFowTex() {
	Pixmap2D *tmpPixmap= m_fowPixmap0;
	m_fowPixmap0= m_fowPixmap1;
	m_fowPixmap1= tmpPixmap;

	for(int i=0; i<m_fowTex->getPixmap()->getW(); ++i){
		for(int j=0; j<m_fowTex->getPixmap()->getH(); ++j){
			if(m_fogOfWar){
				float p0= m_fowPixmap0->getPixelf(i, j);
				float p1= m_fowPixmap1->getPixelf(i, j);

				if(p1>exploredAlpha){
					m_fowPixmap1->setPixel(i, j, exploredAlpha);
				}
				if(p0>p1){
					m_fowPixmap1->setPixel(i, j, p0);
				}
			}
			else{
				m_fowPixmap1->setPixel(i, j, 1.f);
			}
		}
	}
}

void Minimap::updateFowTex(float t){
	for(int i=0; i<m_fowPixmap0->getW(); ++i){
		for(int j=0; j<m_fowPixmap0->getH(); ++j){
			float p1= m_fowPixmap1->getPixelf(i, j);
			if(p1!=m_fowTex->getPixmap()->getPixelf(i, j)){
				float p0= m_fowPixmap0->getPixelf(i, j);
				m_fowTex->getPixmap()->setPixel(i, j, p0+(t*(p1-p0)));
			}
		}
	}
}

// ==================== PRIVATE ====================

void buildVisLists(ConstUnitVector &srfList, ConstUnitVector &airList) {
	UnitSet srfSet; // surface units seen already
	UnitSet airSet; // air units seen already
	RectIterator iter(Vec2i(0), Vec2i(g_map.getW() - 1, g_map.getH() - 1));
	while (iter.more()) {
		Vec2i pos = iter.next();
		Tile *tile = g_map.getTile(Map::toTileCoords(pos));
		if (tile->isVisible(g_world.getThisFaction()->getTeam())) {
			Cell *cell = g_map.getCell(pos);
			Unit *u = cell->getUnit(Zone::AIR);
			if (u && u->getTeam() != -1 && !u->isCarried()) {
				if (airSet.find(u) == airSet.end()) {
					airSet.insert(u);
					airList.push_back(u);
				}
			} else {
				u = cell->getUnit(Zone::LAND);
				if (u && u->getTeam() != -1 && !u->isCarried()) {
					if (srfSet.find(u) == srfSet.end()) {
						srfSet.insert(u);
						srfList.push_back(u);
					}
				}
			}
		}
	}
}

void processList(ConstUnitVector &units, TypeMap<int8>* overlayMap) {
	GameSettings &gs = g_simInterface->getGameSettings();
	foreach_const (ConstUnitVector, it, units) {
		const Vec2i pos = (*it)->getPos();
		const UnitType *ut = (*it)->getType();
		const PatchMap<1> &pMap = ut->getMinimapFootprint();
		RectIterator iter(Vec2i(0), Vec2i(ut->getSize() - 1));
		while (iter.more()) {
			Vec2i iPos = iter.next();
			if (pMap.getInfluence(iPos)) {
				const Vec2i cellPos = pos + iPos;
				overlayMap->setInfluence(cellPos, gs.getColourIndex((*it)->getFactionIndex()));
			}
		}
	}
}

void buildUnitOverlay(TypeMap<int8>* overlayMap) {
	ConstUnitVector srfList; // visible surface units
	ConstUnitVector airList; // visible air units
	buildVisLists(srfList, airList);

	overlayMap->clearMap(-1);
	
	processList(srfList, overlayMap);
	processList(airList, overlayMap);
}

void Minimap::updateUnitTex() {
	if (!g_world.getThisFaction()) {
		return;
	}

	buildUnitOverlay(m_unitsPMap);
	Pixmap2D *pm = m_unitsTex->getPixmap();

	Colour blank((uint8)0u);
	pm->setPixels(blank.ptr());

	Vec2i sPos, pos;
	int ndx;
	if (m_currZoom == 1) {
		RectIterator iter(Vec2i(0), Vec2i(m_w - 1, m_h - 1));
		while (iter.more()) {
			pos = iter.next();
			ndx = m_unitsPMap->getInfluence(pos);
			if (ndx >= 0) {
				pm->setPixel(pos, factionColours[ndx]);
			} else {
				PerimeterIterator iter(pos - Vec2i(1), pos + Vec2i(1));
				while (iter.more()) {
					ndx = m_unitsPMap->getInfluence(iter.next());
					if (ndx >= 0) {
						pm->setPixel(pos, factionColoursOutline[ndx]);
					}
				}
			}
		}
	} else if (m_currZoom > 1) {
		assert(m_currZoom.frac() == 0);
		int ppc = m_currZoom.intp(); // pixels per cell
		RectIterator iter(Vec2i(0), Vec2i(m_w - 1, m_h - 1));
		while (iter.more()) {
			sPos = iter.next();
			ndx = m_unitsPMap->getInfluence(sPos);
			if (ndx >= 0) {
				RectIterator iter2(sPos * 2, sPos * 2 + Vec2i(ppc - 1));
				while (iter2.more()) {
					pm->setPixel(iter2.next(), factionColours[ndx]);
				}
			} else {
				// scan surrounds
				PerimeterIterator iter3(sPos * 2 - Vec2i(1), sPos * 2 + Vec2i(ppc));
				while (iter3.more()) {
					ndx = m_unitsPMap->getInfluence(iter3.next());
					if (ndx >= 0) {
						RectIterator iter2(sPos * 2, sPos * 2 + Vec2i(ppc - 1));
						while (iter2.more()) {
							pm->setPixel(iter2.next(), factionColoursOutline[ndx]);
						}
						break;
					}
				}
			}
		}
	} else {
		assert((1 / m_currZoom).frac() == 0);
		int cpp = (1 / m_currZoom).intp(); // cells per pixel
		RectIterator iter(Vec2i(0), Vec2i(m_w / cpp - 1, m_h / cpp - 1));
		while (iter.more()) {
			sPos = iter.next();
			RectIterator iter2(sPos * cpp, sPos * cpp + Vec2i(cpp - 1));
			bool outlineCheck = true;
			while (iter2.more()) {
				pos = iter2.next();
				ndx = m_unitsPMap->getInfluence(pos);
				if (ndx >= 0) {
					outlineCheck = false;;
					pm->setPixel(sPos, factionColours[ndx]);
					break;
				}
			}
			if (outlineCheck) {
				for (int i=0; i < cpp; ++i) {
					PerimeterIterator iter3(sPos * cpp - Vec2i(1 + i), sPos * cpp + Vec2i(cpp + i));
					while (iter3.more()) {
						pos = iter3.next();
						ndx = m_unitsPMap->getInfluence(pos);
						if (ndx >= 0) {
							pm->setPixel(sPos, factionColoursOutline[ndx]);
							break;
						}
					}
				}
			}
		}
	}

	glActiveTexture(Renderer::baseTexUnit);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(m_unitsTex)->getHandle());

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_unitsTex->getPixmap()->getW(), m_unitsTex->getPixmap()->getH(),
		GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, m_unitsTex->getPixmap()->getPixels());
	//glTexImage2D(GL_TEXTURE_2D, 0, 4, m_unitsTex->getPixmap()->getW(), m_unitsTex->getPixmap()->getH(),
	//	0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, m_unitsTex->getPixmap()->getPixels());
}

void Minimap::computeTexture(const World *world){

	Vec3f color;
	const Map *map= world->getMap();

	m_terrainTex->getPixmap()->setPixels(Vec4f(1.f, 1.f, 1.f, 0.1f).ptr());

	for(int j=0; j<m_terrainTex->getPixmap()->getH(); ++j){
		for(int i=0; i<m_terrainTex->getPixmap()->getW(); ++i){
			Tile *sc= map->getTile(i, j);

			if(sc->getObject()==NULL || sc->getObject()->getType()==NULL){
				const Pixmap2D *p= world->getTileset()->getSurfPixmap(sc->getTileType(), 0);
				color= p->getPixel3f(p->getW()/2, p->getH()/2);
				color= color * static_cast<float>(sc->getVertex().y/6.f);

				if(sc->getVertex().y<= world->getMap()->getWaterLevel()){
					color+= Vec3f(0.5f, 0.5f, 1.0f);
				}

				if(color.x>1.f) color.x=1.f;
				if(color.y>1.f) color.y=1.f;
				if(color.z>1.f) color.z=1.f;
			}
			else{
				color= sc->getObject()->getType()->getColor();
			}
			m_terrainTex->getPixmap()->setPixel(i, j, color);
		}
	}
}

void Minimap::setExploredState(const World *world) {
	const Map &map = *world->getMap();
	for (int y=0; y < map.getTileH(); ++y) {
		for (int x=0; x < map.getTileW(); ++x) {
			if (map.getTile(x,y)->isExplored(world->getThisFactionIndex())) {
				m_fowPixmap0->setPixel(x, y, exploredAlpha);
				m_fowPixmap1->setPixel(x, y, exploredAlpha);
			}
		}
	}
}

void Minimap::render() {
	if (g_config.getUiPhotoMode()) {
		return;
	}

	Widget::renderBgAndBorders(false);
	const GameCamera *gameCamera = g_gameState.getGameCamera();
	const Pixmap2D *pixmap = m_terrainTex->getPixmap();

	Vec2i pos = getScreenPos() + Vec2i(4, 4);
	Vec2i size = getSize() - Vec2i(8, 16);

	Vec2f zoom = Vec2f(float(size.x)/ pixmap->getW(), float(size.y)/ pixmap->getH());

	assertGl();
//	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);

	glEnable(GL_BLEND);
	glActiveTexture(Renderer::baseTexUnit);
	glEnable(GL_TEXTURE_2D);

	glPushAttrib(GL_TEXTURE_BIT);

	//draw map
	// FIXME ? this assumes glTexSubImage() was called in Renderer::renderSurface()
	// may not be the case in Debug Edition.
	glActiveTexture(Renderer::fowTexUnit);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(m_fowTex)->getHandle());
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE);

	glActiveTexture(Renderer::baseTexUnit);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(m_terrainTex)->getHandle());

	glColor4f(0.5f, 0.5f, 0.5f, 0.1f);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.0f, 1.0f);
		glMultiTexCoord2f(Renderer::fowTexUnit, 0.0f, 1.0f);
		glVertex2i(pos.x, pos.y);
		glTexCoord2f(0.0f, 0.0f);
		glMultiTexCoord2f(Renderer::fowTexUnit, 0.0f, 0.0f);
		glVertex2i(pos.x, pos.y + size.y);
		glTexCoord2f(1.0f, 1.0f);
		glMultiTexCoord2f(Renderer::fowTexUnit, 1.0f, 1.0f);
		glVertex2i(pos.x + size.x, pos.y);
		glTexCoord2f(1.0f, 0.0f);
		glMultiTexCoord2f(Renderer::fowTexUnit, 1.0f, 0.0f);
		glVertex2i(pos.x + size.x, pos.y + size.y);
	glEnd();

	glPopAttrib(); // TEXTURE_BIT

	glActiveTexture(Renderer::baseTexUnit);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(m_unitsTex)->getHandle());
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.0f, 1.0f);
		glVertex2i(pos.x, pos.y);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2i(pos.x, pos.y + size.y);
		glTexCoord2f(1.0f, 1.0f);
		glVertex2i(pos.x + size.x, pos.y);
		glTexCoord2f(1.0f, 0.0f);
		glVertex2i(pos.x + size.x, pos.y + size.y);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	//draw camera
	float ang = degToRad(gameCamera->getHAng());
	float amp = 15.f;
	const Vec2i camPos = Vec2i(int(gameCamera->getPos().x), int(gameCamera->getPos().z));
	Vec2i cPos((camPos.x * m_currZoom).intp() + pos.x, -(camPos.y * m_currZoom).intp() + pos.y + size.y);
	Vec2i cPos1(cPos.x + int(amp * sinf(ang - pi / 5)), cPos.y + int(amp * cosf(ang - pi / 5)));
	Vec2i cPos2(cPos.x + int(amp * sinf(ang + pi / 5)), cPos.y + int(amp * cosf(ang + pi / 5)));

	glBegin(GL_TRIANGLES);
		glColor4f(1.f, 1.f, 1.f, 0.7f);
		glVertex2iv(cPos.ptr());
		glColor4f(1.f, 1.f, 1.f, 0.0f);
		glVertex2iv(cPos1.ptr());
		glVertex2iv(cPos2.ptr());
	glEnd();

//	glPopAttrib();

	assertGl();
}

bool Minimap::mouseDown(MouseButton btn, Vec2i pos) {
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (pos.y > myPos.y + mySize.y - getBorderTop()) {
		if (btn == MouseButton::LEFT) {
			m_draggingWidget = true;
			m_moveOffset = myPos - pos;
			return true;
		}
	}

	// on map?
	if (pos.x >= myPos.x + getBorderLeft() && pos.y >= myPos.y + getBorderBottom()
	&& pos.x < myPos.x + mySize.x - getBorderRight() && pos.y < myPos.y + mySize.y - getBorderTop()) {
		int x = pos.x - myPos.x - getBorderLeft();
		int y = (m_ratio > 1 ? 128 / m_ratio.intp() : 128) - (pos.y - myPos.y - getBorderBottom());

		float xratio = float(x) / (m_ratio < 1 ? m_ratio.toFloat() * 128.f : 128.f);
		float yratio = float(y) / (m_ratio > 1 ? 128.f / m_ratio.toFloat() : 128.f);
		Vec2f cellPos(xratio * m_w, yratio * m_h);

		if (btn == MouseButton::LEFT) {
			if (m_leftClickOrder) {
				LeftClickOrder(Vec2i(cellPos));
			} else {
				g_gameState.getGameCamera()->setPos(cellPos);
				m_draggingCamera = true;
			}
		} else if (btn == MouseButton::RIGHT) {
			if (m_rightClickOrder) {
				RightClickOrder(Vec2i(cellPos));
			}
		}
	}
	return true;
}

bool Minimap::mouseUp(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		m_draggingCamera = m_draggingWidget = false;
	}
	return true;
}

bool Minimap::mouseMove(Vec2i pos) {
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (m_draggingCamera) {
		// on map?
		if (pos.x >= myPos.x + getBorderLeft() && pos.y >= myPos.y + getBorderBottom()
		&& pos.x < myPos.x + mySize.x - getBorderRight() && pos.y < myPos.y + mySize.y - getBorderTop()) {
			int x = pos.x - myPos.x - getBorderLeft();
			int y = (m_ratio > 1 ? 128 / m_ratio.intp() : 128) - (pos.y - myPos.y - getBorderBottom());

			float xratio = float(x) / (m_ratio < 1 ? m_ratio.toFloat() * 128.f : 128.f);
			float yratio = float(y) / (m_ratio > 1 ? 128.f / m_ratio.toFloat() : 128.f);
			Vec2f cellPos(xratio * m_w, yratio * m_h);
			g_gameState.getGameCamera()->setPos(cellPos);
		}
	} else if (m_draggingWidget) {
		setPos(pos + m_moveOffset);
	}
	return true;
}

}}//end namespace
