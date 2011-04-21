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

#ifndef _GLEST_GAME_TIMEFLOW_H_
#define _GLEST_GAME_TIMEFLOW_H_

#include "tileset.h"
#include "sound.h"

using namespace Shared::Sound;

namespace Glest { namespace Sim {

// =====================================================
// 	class TimeFlow
//
/// Raises time related events (day/night cycle)
// =====================================================

class TimeFlow {
public:
	static const float dawn;
	static const float midday;
	static const float dusk;

private:
	bool firstTime;
	Tileset *tileset;
	float time;
	float lastTime;
	float timeInc;

public:
	void init(Tileset *tileset);

	float getTime() const				{return time;}
	bool isDay() const					{return time > dawn && time < dusk;}
	bool isNight() const				{return !isDay();}
	bool isTotalNight() const			{return time < dawn - 1.f || time > dusk + 1.f;}
	
	string describeTime() const;

	void update();
	void save(XmlNode *node) const;
	void load(const XmlNode *node);

	// for debugging only!
	void setTime(float t) {time = t;}
//private:
//	bool isAproxTime(float time) const;
};

}} //end namespace

#endif
