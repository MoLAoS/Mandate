// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_METRICS_H_
#define _GLEST_GAME_METRICS_H_

#include "config.h"

namespace Glest { namespace Global {

// =====================================================
//	class Metrics
// =====================================================

class Metrics{
private:
	int virtualW;
	int virtualH;
	int screenW;
	int screenH;
	int minimapX;
	int minimapY;
	int minimapW;
	int minimapH;
	int displayX;
	int displayY;
	int displayH;
	int displayW;

	Metrics();

public:

	static const Metrics &getInstance(){
		static const Metrics singleton;
		return singleton;
	}

	int getVirtualW() const	{return virtualW;}
	int getVirtualH() const	{return virtualH;}
	int getScreenW() const	{return screenW;}
	int getScreenH() const	{return screenH;}
	Vec2i getScreenDims() const { return Vec2i(screenW, screenH); }
	int getMinimapX() const	{return minimapX;}
	int getMinimapY() const	{return minimapY;}
	int getMinimapW() const	{return minimapW;}
	int getMinimapH() const	{return minimapH;}
	int getDisplayX() const	{return displayX;}
	int getDisplayY() const	{return displayY;}
	int getDisplayH() const	{return displayH;}
	int getDisplayW() const	{return displayW;}

	float getAspectRatio() const	{return static_cast<float>(screenW) / screenH;}
	int toVirtualX(int w) const		{return w * virtualW / screenW;}
	int toVirtualY(int h) const		{return h * virtualH / screenH;}

	bool isInDisplay(int x, int y) const{
		return
			x > displayX &&
			y > displayY &&
			x < displayX+displayW &&
			y < displayY+displayH;
	}

	bool isInMinimap(int x, int y) const{
		return
			x > minimapX &&
			y > minimapY &&
			x < minimapX+minimapW &&
			y < minimapY+minimapH;
	}
};

}}//end namespace

#endif
