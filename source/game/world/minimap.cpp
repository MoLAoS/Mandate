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

Minimap::Minimap(bool FoW, Container::Ptr parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, MouseWidget(this)
		, m_draggingCamera(false)
		, m_draggingWidget(false)
		, m_leftClickOrder(false)
		, m_rightClickOrder(false) {
	fowPixmap0= NULL;
	fowPixmap1= NULL;
	fogOfWar = FoW;
	shroudOfDarkness = FoW;
	
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

	Renderer &renderer= Renderer::getInstance();

	//fow pixmaps
	float f= 0.f;
	fowPixmap0= new Pixmap2D(nextPowerOf2(scaledW), nextPowerOf2(scaledH), 1);
	fowPixmap1= new Pixmap2D(nextPowerOf2(scaledW), nextPowerOf2(scaledH), 1);
	fowPixmap0->setPixels(&f);
	fowPixmap1->setPixels(&f);

	if (resumingGame) {
		setExploredState(world);
	}

	//fow tex
	fowTex= renderer.newTexture2D(ResourceScope::GAME);
	fowTex->setMipmap(false);
	fowTex->setPixmapInit(false);
	fowTex->setFormat(Texture::fAlpha);
	fowTex->getPixmap()->init(nextPowerOf2(scaledW), nextPowerOf2(scaledH), 1);
	fowTex->getPixmap()->setPixels(&f);

	//tex
	tex= renderer.newTexture2D(ResourceScope::GAME);
	tex->getPixmap()->init(scaledW, scaledH, 3);
	tex->setMipmap(false);

	computeTexture(world);
}

Minimap::~Minimap(){
	delete fowPixmap0;
	delete fowPixmap1;
}

// ==================== set ====================

void Minimap::resetFowTex() {
	Pixmap2D *tmpPixmap= fowPixmap0;
	fowPixmap0= fowPixmap1;
	fowPixmap1= tmpPixmap;

	for(int i=0; i<fowTex->getPixmap()->getW(); ++i){
		for(int j=0; j<fowTex->getPixmap()->getH(); ++j){
			if(fogOfWar){
				float p0= fowPixmap0->getPixelf(i, j);
				float p1= fowPixmap1->getPixelf(i, j);

				if(p1>exploredAlpha){
					fowPixmap1->setPixel(i, j, exploredAlpha);
				}
				if(p0>p1){
					fowPixmap1->setPixel(i, j, p0);
				}
			}
			else{
				fowPixmap1->setPixel(i, j, 1.f);
			}
		}
	}
}

void Minimap::updateFowTex(float t){
	for(int i=0; i<fowPixmap0->getW(); ++i){
		for(int j=0; j<fowPixmap0->getH(); ++j){
			float p1= fowPixmap1->getPixelf(i, j);
			if(p1!=fowTex->getPixmap()->getPixelf(i, j)){
				float p0= fowPixmap0->getPixelf(i, j);
				fowTex->getPixmap()->setPixel(i, j, p0+(t*(p1-p0)));
			}
		}
	}
}

// ==================== PRIVATE ====================

void Minimap::computeTexture(const World *world){

	Vec3f color;
	const Map *map= world->getMap();

	tex->getPixmap()->setPixels(Vec4f(1.f, 1.f, 1.f, 0.1f).ptr());

	for(int j=0; j<tex->getPixmap()->getH(); ++j){
		for(int i=0; i<tex->getPixmap()->getW(); ++i){
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
			tex->getPixmap()->setPixel(i, j, color);
		}
	}
}

void Minimap::setExploredState(const World *world) {
	const Map &map = *world->getMap();
	for (int y=0; y < map.getTileH(); ++y) {
		for (int x=0; x < map.getTileW(); ++x) {
			if (map.getTile(x,y)->isExplored(world->getThisFactionIndex())) {
				fowPixmap0->setPixel(x, y, exploredAlpha);
				fowPixmap1->setPixel(x, y, exploredAlpha);
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
	const Pixmap2D *pixmap = tex->getPixmap();

	Vec2i pos = getScreenPos() + Vec2i(4, 4);
	Vec2i size = getSize() - Vec2i(8, 16);

	Vec2f zoom = Vec2f(float(size.x)/ pixmap->getW(), float(size.y)/ pixmap->getH());

	assertGl();
	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_TEXTURE_BIT);

	//draw map
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glActiveTexture(Renderer::fowTexUnit);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(fowTex)->getHandle());
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE);

	glActiveTexture(Renderer::baseTexUnit);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(tex)->getHandle());

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

	glDisable(GL_BLEND);

	glActiveTexture(Renderer::fowTexUnit);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(Renderer::baseTexUnit);
	glDisable(GL_TEXTURE_2D);

	// draw units
	glBegin(GL_QUADS);

	for(int i=0; i < g_world.getFactionCount(); ++i) {
		int colourNdx = g_simInterface->getGameSettings().getColourIndex(i);
		glColor3fv(Faction::factionColours[colourNdx].ptr());
		for(int j=0; j < g_world.getFaction(i)->getUnitCount(); ++j) {
			Unit *unit = g_world.getFaction(i)->getUnit(j);
			if (g_world.toRenderUnit(unit) && unit->isAlive()) {
				Vec2i upos = unit->getPos() / GameConstants::cellScale;
				int usize = unit->getType()->getSize();
				glVertex2f(pos.x + upos.x * zoom.x, pos.y + size.y - (upos.y * zoom.y));
				glVertex2f(pos.x + (upos.x+1) * zoom.x + usize, pos.y + size.y - (upos.y * zoom.y));
				glVertex2f(pos.x + (upos.x+1) * zoom.x + usize, pos.y + size.y - ((upos.y + usize) * zoom.y));
				glVertex2f(pos.x + upos.x * zoom.x, pos.y + size.y - ((upos.y + usize) * zoom.y));
			}
		}
	}
	glEnd();

	//draw camera
	float wRatio = float(size.x) / g_map.getW();
	float hRatio = float(size.y) / g_map.getH();

	int x = static_cast<int>(gameCamera->getPos().x * wRatio);
	int y = static_cast<int>(gameCamera->getPos().z * hRatio);

	float ang= degToRad(gameCamera->getHAng());

	glEnable(GL_BLEND);

	glBegin(GL_TRIANGLES);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glVertex2i(pos.x + x, pos.y + size.y - y);

	glColor4f(1.f, 1.f, 1.f, 0.0f);
	glVertex2i(
		pos.x + x + static_cast<int>(20*sinf(ang-pi/5)),
		pos.y + size.y - (y-static_cast<int>(20*cosf(ang-pi/5))));

	glColor4f(1.f, 1.f, 1.f, 0.0f);
	glVertex2i(
		pos.x + x + static_cast<int>(20*sinf(ang+pi/5)),
		pos.y + size.y - (y-static_cast<int>(20*cosf(ang+pi/5))));

	glEnd();
	glPopAttrib();

	assertGl();
}

bool Minimap::mouseDown(MouseButton btn, Vec2i pos) {
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (pos.y > myPos.y + mySize.y - getBorderTop()) {
		if (btn == MouseButton::LEFT) {
			m_draggingWidget = true;
			moveOffset = myPos - pos;
			return true;
		}
	}

	// on map?
	if (pos.x >= myPos.x + getBorderLeft() && pos.y >= myPos.y + getBorderBottom()
	&& pos.x < myPos.x + mySize.x - getBorderRight() && pos.y < myPos.y + mySize.y - getBorderTop()) {
		int x = pos.x - myPos.x - getBorderLeft();
		int y = 128 - (pos.y - myPos.y - getBorderBottom());

		float xratio = float(x) / 128.f;
		float yratio = float(y) / 128.f;
		Vec2f cellPos(xratio * g_map.getW(), yratio * g_map.getH());

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
			int y = 128 - (pos.y - myPos.y - getBorderBottom());

			float xratio = float(x) / float(textureSize.x);
			float yratio = float(y) / float(textureSize.y);
			Vec2f cellPos(xratio * g_map.getW(), yratio * g_map.getH());
			g_gameState.getGameCamera()->setPos(cellPos);
		}
	} else if (m_draggingWidget) {
		setPos(pos + moveOffset);
	}
	return true;
}

}}//end namespace
