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
using Shared::Platform::NetworkDataBuffer;

namespace Glest{ namespace Game{

// =====================================================
// 	class Minimap
// =====================================================

const float Minimap::exploredAlpha= 0.5f;

Minimap::Minimap(){
	fowPixmap0= NULL;
	fowPixmap1= NULL;
	fogOfWar= Config::getInstance().getGsFogOfWarEnabled();
	shroudOfDarkness = Config::getInstance().getGsShroudOfDarknessEnabled();
}

void Minimap::init(int w, int h, const World *world){
	int scaledW= w/Map::cellScale;
	int scaledH= h/Map::cellScale;

	Renderer &renderer= Renderer::getInstance();

	//fow pixmaps
	float f= 0.f;
	fowPixmap0= new Pixmap2D(next2Power(scaledW), next2Power(scaledH), 1);
	fowPixmap1= new Pixmap2D(next2Power(scaledW), next2Power(scaledH), 1);
	fowPixmap0->setPixels(&f);
	fowPixmap1->setPixels(&f);

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

/**
 * See Minimap::write(NetworkDataBuffer &buf) const for description.
 */
void Minimap::read(NetworkDataBuffer &buf) {
	assert(fowPixmap0->getW() == fowPixmap1->getW());
	assert(fowPixmap0->getH() == fowPixmap1->getH());

	int w = fowPixmap0->getW();
	int h = fowPixmap0->getH();
	uint8 *data0 = fowPixmap0->getPixels();
	uint8 *data1 = fowPixmap1->getPixels();

	uint32 count = 0;
	uint8 lastValue = 0;
	uint8 b0;

	for(int x = 0; x < w; ++x) {
		for(int y = 0; y < h; ++y) {
			if(count) {
				data0[w * y + x] = lastValue;
				data1[w * y + x] = lastValue;
				--count;
				continue;
			}
			buf.read(b0);

			if(b0 & 0x80) {								// bit 7: repitition or single value
				lastValue = (b0 & 0x40) ? 0x7f : 0;		// bit 6: explored or unexplored
				if(!(b0 & 0x20)) {						// bit 5: count in b0 (low) or next byte(s)
					count = b0 & 0x1f;
				} else if(!(b0 & 0x10)) {				// bit 3 & 4: 0x = count is 8 bits
					uint8 count8;
					buf.read(count8);
					count = count8;
				} else if(!(b0 & 0x8)) {				// bit 3 & 4: 10 = count is 16 bits
					uint16 count16;
					buf.read(count16);
					count = count16;
				} else {								// bit 3 & 4: 11 = count is 32 bits
					buf.read(count);
				}
				assert(count);
				//fprintf(stderr, "b0 = 0x%02x, count = %d (%d, %d)\n", b0, count, x, y);
				data0[w * y + x] = lastValue;			// set p0 and p1
				data1[w * y + x] = lastValue;
				--count;
			} else {									// b0 contains p0 and the next byte is p1
				data0[w * y + x] = b0;
				buf.read(data1[w * y + x]);
				//fprintf(stderr, "p0 = 0x%02x, p1 = 0x%02x (%d, %d)\n", data0[w * y + x], data1[w * y + x], x, y);

				// at least one of these must be non-zero
				assert(data0[w * y + x] || data1[w * y + x]);
			}
		}
	}
}

inline void Minimap::writeRepition(NetworkDataBuffer &buf, uint8 lastResult, uint32 count, int x, int y) {
	// write lastResult with it's count
	// bit 7 of 1st byte will always be high here to indicate a pattern
	// bit 6 indicates fully explored (high) or fully unexplored (low)
	uint8 b0 = 0x80 | (lastResult == 1 ? 0x40 : 0);

	if(count < 0x20) {
		b0 = b0 | (count & 0x1f);		// bit 5 low	(count stored in b0)
		buf.write(b0);
	} else {
		b0 = b0 | 0x20;					// bit 5 high	(count not stored in b0)
		if(count <= UCHAR_MAX) {
			uint8 count8 = count;
			buf.write(b0);				// bits 4 & 3 = 0x	(count is 8 bit)
			buf.write(count8);
		} else if (count <= USHRT_MAX) {
			uint16 count16 = count;
			b0 = b0 | 0x10;				// bits 4 & 3 = 10	(count is 16 bit)
			buf.write(b0);
			buf.write(count16);
		} else {
			b0 = b0 | 0x18;				// bits 4 & 3 = 11	(count is 32 bit)
			buf.write(b0);
			buf.write(count);
		}
	}
	//fprintf(stderr, "b0 = 0x%02x, count = %d (%d, %d) - 1\n", b0, count, x, y);
}

/**
 * byte 0 when bit 7 is high:
 *   bit 7 high indicates that this is a repitition (1 or more) of completely explored or unexplored
 *   bit 6: indicates rather explored (high) or unexplored (low)
 *   bit 5: indicates rather the count is in the next byte (high) or in bits 0-4 of this byte (low)
 *   bit 4: if 5 is high, indicates that count is 2 bytes (unless 3 is high)
 *   bit 3: if 4 is high, indicates that count is 4 bytes.
 *   bits 0-2: unused if 5 is high
 * byte 0 when bit 7 is low:
 *   bit 7 low indicates that this cell is between fully explored and unexplored
 *   bits 0-6: 0-6 indicate the p0 value and the next byte will contain p1.
 */
void Minimap::write(NetworkDataBuffer &buf) const {
	assert(fowPixmap0->getW() == fowPixmap1->getW());
	assert(fowPixmap0->getH() == fowPixmap1->getH());

	int w = fowPixmap0->getW();
	int h = fowPixmap0->getH();
	uint8 *data0 = fowPixmap0->getPixels();
	uint8 *data1 = fowPixmap1->getPixels();

	uint32 count = 0;
	// 0 = no pattern
	// 1 = last was explored and and possibly visible
	// 2 = last was completely unexplored
	uint8 lastResult = 0;
	int x, y;

	for(x = 0; x < w; ++x) {
		for(y = 0; y < h; ++y) {
			uint8 p0 = data0[w * y + x];
			uint8 p1 = data1[w * y + x];
			uint8 thisResult;

			if(p0 >= 0x7f || p1 >= 0x7f) {	// fully explored
				thisResult = 1;
			} else if(!p0 && !p1) {			// fully unexplored
				thisResult = 2;
			} else {						// in between
				thisResult = 0;
			}

			if(thisResult && thisResult == lastResult) {
				++count;
				continue;
			}

			if(count) {
				writeRepition(buf, lastResult, count, x, y);
				count = 0;
			}

			if(!thisResult) {
				// write current value, without a pattern
				// bit 7 should always be low here
				assert(!(p0 & 0x80));
				buf.write(p0);
				buf.write(p1);
				//fprintf(stderr, "p0 = 0x%02x, p1 = 0x%02x (%d, %d)\n", p0, p1, x, y);
			} else {
				// start a new pattern (of possibly only 1 repition)
				lastResult = thisResult;
				count = 1;
			}
		}
	}
	if(count) {
		writeRepition(buf, lastResult, count, x, y);
	}
}

/**
 * Sythesize alphas based upon explored areas from map object.  In order to proximate the proper
 * alphas from when areas are explored in normal game play, the exporation states of the surrounding
 * 120 cells are examined.  If less than 100 of those cells are explored, the alpha for the center
 * cell is completely black, otherwise, an alpha between 0.25 and 0.5 is used, 0.25 if only 110 of
 * the neighbors are explored and 0.5 if all 120 of them are explored.
 *
 * Alpha states from the cell being visible will be calculated later, so visilbity is ignored here.
 */
void Minimap::synthesize(const Map *map, int team) {
	int h = map->getTileH();
	int w = map->getTileW();

	// smooth edges of fow alpha
	for(int x = 0; x < w; ++x) {
		for(int y = 0; y < h; ++y) {
			if(!map->getTile(x, y)->isExplored(team)) {
				continue;
			}
			int exploredNeighbors = 0;
			for(int xoff = -5; xoff <= 5; ++xoff) {
				for(int yoff = -5; yoff <= 5; ++yoff) {
					Vec2i pos(x + xoff, y + yoff);

					// we'll count out of bounds as explored so edges aren't messed up
					if(!map->isInsideTile(pos) || (map->getTile(pos)->isExplored(team)
							&& (xoff || yoff))) {
						++exploredNeighbors;
					}
				}
			}
			// 120 possible neighbors, if less than 100, we don't make it visible.
			if(exploredNeighbors >= 100) {
				// alpha between 0.25 and 0.5
				float ratio = (float)(exploredNeighbors - 100) / 20.f;
				float alpha = 0.2f + 0.3f * ratio;
				incFowTextureAlphaSurface(Vec2i(x, y), alpha);
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

}}//end namespace
