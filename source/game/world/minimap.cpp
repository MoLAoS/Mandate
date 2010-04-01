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

#include "leak_dumper.h"

using namespace Shared::Graphics;
using Shared::Platform::uint8;

namespace Glest{ namespace Game{

// =====================================================
// 	class Minimap
// =====================================================

const float Minimap::exploredAlpha= 0.5f;

Minimap::Minimap(bool FoW){
	fowPixmap0= NULL;
	fowPixmap1= NULL;
	fogOfWar = FoW;
	shroudOfDarkness = FoW;
}

void Minimap::init(int w, int h, const World *world, bool resumingGame){
	int scaledW = w / GameConstants::cellScale;
	int scaledH = h / GameConstants::cellScale;

	Renderer &renderer= Renderer::getInstance();

	//fow pixmaps
	float f= 0.f;
	fowPixmap0= new Pixmap2D(next2Power(scaledW), next2Power(scaledH), 1);
	fowPixmap1= new Pixmap2D(next2Power(scaledW), next2Power(scaledH), 1);
	fowPixmap0->setPixels(&f);
	fowPixmap1->setPixels(&f);

	if (resumingGame) {
		setExploredState(world);
	}

	//fow tex
	fowTex= renderer.newTexture2D(rsGame);
	fowTex->setMipmap(false);
	fowTex->setPixmapInit(false);
	fowTex->setFormat(Texture::fAlpha);
	fowTex->getPixmap()->init(next2Power(scaledW), next2Power(scaledH), 1);
	fowTex->getPixmap()->setPixels(&f);

	//tex
	tex= renderer.newTexture2D(rsGame);
	tex->getPixmap()->init(scaledW, scaledH, 3);
	tex->setMipmap(false);

	computeTexture(world);
}

Minimap::~Minimap(){
	Logger::getInstance().add("Minimap", !Program::getInstance()->isTerminating());
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

}}//end namespace
