// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Jaagup Repän <jrepan@gmail.com>,
//				     2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_EARTHQUAKE_H_
#define _GLEST_GAME_EARTHQUAKE_H_
#ifdef EARTHQUAKE_CODE
#include <cassert>
#include <map>

#include "vec.h"
#include "math_util.h"
#include "command_type.h"
#include "logger.h"
#include "object.h"
#include "game_constants.h"
#include "selection.h"
#include "exceptions.h"
#include "pos_iterator.h"

namespace Glest { namespace ProtoTypes {

using Shared::Math::Vec4f;
using Shared::Math::Quad2i;
using Shared::Math::Rect2i;
using Shared::Math::Vec4f;
using Shared::Math::Vec2f;
using Shared::Math::Vec2i;
using Shared::Graphics::Texture2D;
using Glest::Util::PosCircularIteratorFactory;

class Tileset;
class Unit;
class Resource;
class TechTree;
class World;
class UnitContainer;

class EarthquakeType {
private:
	float magnitude;			/**< max variance in height */
	float frequency;			/**< oscilations per second */
	float speed;				/**< speed siesmic waves travel per second (in surface cells) */
	float duration;				/**< duration in seconds */
	float radius;				/**< radius in surface cells */
	float initialRadius;		/**< radius of area of initial activity (in surface cells) */
	float shiftsPerSecond;		/**< number of sudden shifts per second */
	float shiftsPerSecondVar;	/**< variance of shiftsPerSecond (+-) */
	Vec3f shiftLengthMult;		/**< multiplier of magnitude to determine max length of each shift */
	float maxDps;				/**< maxiumum damage per second caused */
	const AttackType *attackType;/**< the damage type to use for damage reports */
	bool affectsAllies;			/**< rather or not this earthquake affects allies */
	StaticSound *sound;


public:
	EarthquakeType(float maxDps, const AttackType *attackType);
	~EarthquakeType() { delete sound; }
	void load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	void spawn(Map &map, Unit *cause, const Vec2i &epicenter, float strength) const;

	float getMagnitude() const				{return magnitude;}
	float getFrequency() const				{return frequency;}
	float getSpeed() const					{return speed;}
	float getDuration() const				{return duration;}
	float getRadius() const					{return radius;}
	float getInitialRadius() const			{return initialRadius;}
	float getshiftsPerSecond() const		{return shiftsPerSecond;}
	float getshiftsPerSecondVar() const		{return shiftsPerSecondVar;}
	const Vec3f &getshiftLengthMult() const	{return shiftLengthMult;}
	float getMaxDps() const					{return maxDps;}
	const AttackType *getAttackType() const	{return attackType;}
	bool isAffectsAllies() const			{return affectsAllies;}
	StaticSound *getSound() const			{return sound;}
};

class Earthquake {
public:
	class UnitImpact {
	public:
		int count;		/**< total number of cells unit occupies that are affected */
		float intensity;/**< total intensity (average = intensity / count) */

		UnitImpact() : count(0), intensity(0.f) {}
		void impact(float intensity)			{++count; this->intensity += intensity;}
	};
	typedef std::map<Unit *, UnitImpact> DamageReport;

private:
	Map &map;
	UnitId cause;	/**< unit that caused this earthquake */
	const EarthquakeType *type; /**< the type */
	Vec2i epicenter;		/**< epicenter in surface coordinates */
	float magnitude;		/**< max variance in height */
	float frequency;		/**< oscilations per second */
	float speed;			/**< unit cells traveled per second */
	float duration;			/**< duration in seconds */
	float radius;			/**< radius in surface cells */
	float initialRadius;	/**< radius of area of initial activity (in surface cells) */
	float progress;			/**< 0 when started, 1 when complete at epicenter (continues longer for waves to even out) */
	Rect2i bounds;			/**< surface cell area effected */
	Rect2i uBounds;			/**< unit cell area effected */
	float nextShiftTime;	/**< when the shift should occurred */
	Vec3f currentShift;		/**< the current xyz offset caused by seismic shift (before magnitude multiplier) */
	Vec2i size;
	size_t cellCount;
	Random random;

public:
	Earthquake(Map &map, Unit *cause, const Vec2i &epicenter, const EarthquakeType* type, float strength);

	Unit *getCause() const					{return cause;}
	const EarthquakeType *getType() const	{return type;};
	const Vec2i getEpicenter() const		{return epicenter;}
	void update(float slice);
	void getDamageReport(DamageReport &results, float slice);
	bool isDone()							{return progress >= 1.f + radius / speed / duration;}
	void resetSurface();

private:
	float temporalMagnitude(float temporalProgress);
	float temporalPressure(float time, float &tm);
	void calculateEffectiveValues(float &effectiveTime, float &effectiveMagnitude, const Vec2i &surfacePos, float time);
};


}} //end namespace

#endif
#endif
