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

#ifndef _GLEST_GAME_METRICS_H_
#define _GLEST_GAME_METRICS_H_

#include "config.h"

namespace Glest { namespace Global {

// =====================================================
//	class Metrics
// =====================================================

class Metrics{
private:
	int screenW;
	int screenH;
	Metrics();

public:
	static Metrics &getInstance(){
		static Metrics singleton;
		return singleton;
	}

	void setScreenW(int w) { screenW = w; }
	void setScreenH(int h) { screenH = h; }

	int getScreenW() const	{return screenW;}
	int getScreenH() const	{return screenH;}
	Vec2i getScreenDims() const { return Vec2i(screenW, screenH); }
	float getAspectRatio() const	{return static_cast<float>(screenW) / screenH;}
};

}}//end namespace

#endif
