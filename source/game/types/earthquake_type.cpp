// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C)	2008 Daniel Santos <daniel.santos@pobox.com>,
//					2008 Jaagup Repän <jrepan@gmail.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#ifdef EARTHQUAKE_CODE
#include "earthquake_type.h"

#include <cassert>

#include "world.h"
#include "tileset.h"
#include "unit.h"
#include "resource.h"
#include "logger.h"
#include "tech_tree.h"
#include "config.h"
#include "socket.h"
#include "selection.h"

#include "leak_dumper.h"

using namespace Shared::Math;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class EarthquakeType
// =====================================================

EarthquakeType::EarthquakeType(float maxDps, const AttackType *attackType) :
		maxDps(maxDps), attackType(attackType), affectsAllies(true) {
}

/** Load the EarthquakeType from xml */
void EarthquakeType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	magnitude = n->getChildFloatValue("magnitude");
	frequency = n->getChildFloatValue("frequency");
	speed = n->getChildFloatValue("speed");
	duration = n->getChildFloatValue("duration");
	radius = n->getChildFloatValue("radius");
	initialRadius = n->getChildFloatValue("initial-radius");

	const XmlNode *soundFileNode= n->getChild("sound-file", 0, false);
	if(soundFileNode) {
		string path= soundFileNode->getAttribute("path")->getRestrictedValue();
		sound= new StaticSound();
		sound->load(dir + "/" + path);
	} else {
		sound = NULL;
	}

	shiftsPerSecond = n->getChildFloatValue("shifts-per-second");
	shiftsPerSecondVar = n->getChildFloatValue("shifts-per-second-var");
	shiftLengthMult = n->getChildVec3fValue("shift-length-multiplier");
	affectsAllies = n->getOptionalBoolValue("affects-allies", true);
}

void EarthquakeType::spawn(Map &map, Unit *cause, const Vec2i &epicenter, float strength) const {
	map.add(new Earthquake(map, cause, epicenter, this, strength));
}

// =====================================================
// 	class Earthquake
// =====================================================

Earthquake::Earthquake(Map &map, Unit *cause, const Vec2i &uepicenter, const EarthquakeType* type, float strength) :
		map(map),
		cause(cause),
		type(type),
		epicenter(Map::toTileCoords(uepicenter)),
		magnitude(type->getMagnitude() * strength),
		frequency(type->getFrequency()),
		speed(type->getSpeed()),
		duration(type->getDuration()),
		radius(type->getRadius() / GameConstants::cellScale),
		initialRadius(type->getInitialRadius() / GameConstants::cellScale),
		progress(0.f),
		nextShiftTime(0.f),
		currentShift(0.f),
		random(uepicenter.x) {
	bounds.p[0].x = max((int)(epicenter.x - radius), 0);
	bounds.p[0].y = max((int)(epicenter.y - radius), 0);
	bounds.p[1].x = min((int)(epicenter.x + radius), map.getTileW() - 1);
	bounds.p[1].y = min((int)(epicenter.y + radius), map.getTileH() - 1);
	uBounds = bounds * GameConstants::cellScale;
	size.x = bounds.p[1].x - bounds.p[0].x + 1;
	size.y = bounds.p[1].y - bounds.p[0].y + 1;
	cellCount = size.x * size.y;
}

/**
 * Calculates the magnitude of the earthquake (expressed as a float from zero to one from min to
 * max) at a given point in time (also expressed as zero to one, from begining to end). A sine
 * pattern is used that quickly rises at the start and quickly falls at the end.  Full strength
 * is reached at T=0.25 and the taper occurs at 3/4 the speed of rise.
 */
inline float Earthquake::temporalMagnitude(float temporalProgress) {
	// early peek, gradual decline
	float effectiveProgress = temporalProgress < 0.25f
			? temporalProgress * 2.f
			: 0.5f + (temporalProgress - 0.25f) * 2.f / 3.f;

	// non-linear rise and fall of intensity
	return sinf(effectiveProgress * pi);
}

/** Calculates the positive or negative pressure at a given point in time. */
inline float Earthquake::temporalPressure(float time, float &tm) {
	if(time < -initialRadius / speed || time > duration) {
		return 0.f;
	}

	tm = temporalMagnitude(time / duration) * magnitude;
	return sinf(time * frequency * twopi) * tm;
}

/** Updates the earthquake advancing it slice seconds forward in time. */
void Earthquake::update(float slice) {
	progress += slice / duration;
	float time = progress * duration;
	float sps = type->getshiftsPerSecond();
	float spsvar = type->getshiftsPerSecondVar();

	if(nextShiftTime <= time) {
		do {
			// skip any shifts we missed
			nextShiftTime += 1.f / (sps + random.randRange(-spsvar, spsvar));
		} while(nextShiftTime <= time);

		const Vec3f &slm = type->getshiftLengthMult();
		currentShift.x = random.randRange(-slm.x, slm.x);
		currentShift.y = random.randRange(-slm.y, slm.y),
		currentShift.z = random.randRange(-slm.z, slm.z);
	}

	for(int x = bounds.p[0].x; x <= bounds.p[1].x; ++x) {
		for(int y = bounds.p[0].y; y <= bounds.p[1].y; ++y) {
			Vec2i pos(x, y);
			float dist = epicenter.dist(pos);
			if(dist > radius) {
				continue;
			}
			float timeLag = dist / speed;
			float decayMultiplier = dist == 0 ? 1.f : sinf(dist / radius * pi);
			float tm = 0.f;
			float pressure = temporalPressure(time - timeLag, tm) * decayMultiplier;

			Tile *sc = map.getTile(pos);
			sc->alterVertex(Vec3f(0.f, pressure, 0.f) + (currentShift * tm));
			sc->updateObjectVertex();

		}
	}
	map.computeNormals(uBounds);
	map.computeInterpolatedHeights(uBounds);
}

/**
 * Calculate effective values for damage report given a specific surface cell and point in time.
 * @param effectiveTime the effective time based upon the distance surfacePos is from the epicenter,
 * the speed of the earthquake and the supplied actual time.
 * @param effectiveMagnitude the effective magnitude based upon the calculated effective time, lag
 * (the earthquake disturbance may not have reached this cell yet) and decay.
 */
void Earthquake::calculateEffectiveValues(float &effectiveTime, float &effectiveMagnitude, const Vec2i &surfacePos, float time) {
	float dist = epicenter.dist(surfacePos);
	float timeLag = dist / speed;
	effectiveTime = time - timeLag;
	if(effectiveTime < 0.f || effectiveTime > duration || dist > radius) {
		effectiveMagnitude = 0.f;
		return;
	}
	float decayMultiplier = dist == 0 ? 1.f : sinf(dist / radius * pi);
	effectiveMagnitude = temporalMagnitude(time / duration) * magnitude * decayMultiplier;
}

void Earthquake::getDamageReport(DamageReport &results, float slice) {
	float time = progress * duration - slice;
	DamageReport::iterator i;

	for(int x = uBounds.p[0].x; x <= uBounds.p[1].x; ++x) {
		for(int y = uBounds.p[0].y; y <= uBounds.p[1].y; ++y) {
			Vec2i pos(x, y);
			Vec2i surfacePos = map.toTileCoords(pos);
			Unit *unit = map.getCell(pos)->getUnit(Zone::LAND);
			if(!unit) {
				continue;
			}

			float effectiveTime;
			float effectiveMagnitude;
			calculateEffectiveValues(effectiveTime, effectiveMagnitude, surfacePos, time);
			float damage = effectiveMagnitude / magnitude * slice;
			if(damage < 0.f) {
				damage = 0.f;
			}

			i = results.find(unit);
			if(i == results.end()) {
				results[unit].impact(damage);
			} else {
				i->second.impact(damage);
			}
		}
	}
}

void Earthquake::resetSurface() {
	for(int x = bounds.p[0].x; x <= bounds.p[1].x; ++x) {
		for(int y = bounds.p[0].y; y <= bounds.p[1].y; ++y) {
			map.getTile(x, y)->resetVertex();
		}
	}
}


}}//end namespace
#endif