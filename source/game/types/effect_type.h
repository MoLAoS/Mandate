// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_EFFECTTYPE_H
#define _GLEST_GAME_EFFECTTYPE_H

#include "sound.h"
#include "vec.h"
#include "element_type.h"
#include "xml_parser.h"
#include "particle_type.h"
#include "upgrade_type.h"
#include "flags.h"
#include "game_constants.h"


using Shared::Sound::StaticSound;
using Shared::Xml::XmlNode;
using Shared::Math::Vec3f;

namespace Glest { namespace Game {

class FactionType;
class TechTree;

// ==============================================================
// 	enum EffectTypeFlag & class EffectTypeFlags
// ==============================================================

/**
 * Flags for an EffectType object.
 *
 * @see EffectTypeFlag
 */
class EffectTypeFlags : public XmlBasedFlags<EffectTypeFlag, EffectTypeFlag::COUNT>{
public:
	void load(const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft) {
		XmlBasedFlags<EffectTypeFlag, EffectTypeFlag::COUNT>::load(node, dir, tt, ft, "flag", EffectTypeFlagNames);
	}
};

class EffectType;
typedef vector<EffectType*> EffectTypes;

/**
 * Represents the type for a possibly temporary effect which modifies the
 * stats, regeneration/degeneration and possibly other behaviors &
 * attributes of a Unit.
 */
class EffectType: public EnhancementTypeBase, public DisplayableType {
private:
	EffectBias bias;
	EffectStacking stacking;
	unsigned int effectflags;

	int duration;
	float chance;
	bool light;
	Vec3f lightColor;
	ParticleSystemType *particleSystemType;
	StaticSound *sound;
	float soundStartTime;
	bool loopSound;
	EffectTypes recourse;
	EffectTypeFlags flags;
	const AttackType *damageType;
	bool display;

public:
	EffectType();
	virtual ~EffectType() { delete sound; }
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;

	EffectBias getBias() const				{return bias;}
	EffectStacking getStacking() const		{return stacking;}
	const EffectTypeFlags &getFlags() const	{return flags;}
	bool getFlag(EffectTypeFlag flag) const	{return flags.get(flag);}
	const AttackType *getDamageType() const	{return damageType;}

	bool isEffectsAlly() const				{return getFlag(EffectTypeFlag::ALLY);}
	bool isEffectsFoe() const				{return getFlag(EffectTypeFlag::FOE);}
	bool isEffectsNormalUnits() const		{return !getFlag(EffectTypeFlag::NO_NORMAL_UNITS);}
	bool isEffectsBuildings() const			{return getFlag(EffectTypeFlag::BUILDINGS);}
	bool isEffectsPetsOnly() const			{return getFlag(EffectTypeFlag::PETS_ONLY);}
	bool isEffectsNonLiving() const			{return getFlag(EffectTypeFlag::NON_LIVING);}
	bool isScaleSplashStrength() const		{return getFlag(EffectTypeFlag::SCALE_SPLASH_STRENGTH);}
	bool isEndsWhenSourceDies() const		{return getFlag(EffectTypeFlag::ENDS_WITH_SOURCE);}
	bool idRecourseEndsWithRoot() const		{return getFlag(EffectTypeFlag::RECOURSE_ENDS_WITH_ROOT);}
	bool isPermanent() const				{return getFlag(EffectTypeFlag::PERMANENT);}
	bool isAllowNegativeSpeed() const		{return getFlag(EffectTypeFlag::ALLOW_NEGATIVE_SPEED);}
	bool isTickImmediately() const			{return getFlag(EffectTypeFlag::TICK_IMMEDIATELY);}

	int getDuration() const								{return duration;}
	float getChance() const								{return chance;}
	bool isLight() const								{return light;}
	bool isDisplay() const								{return display;}
	Vec3f getLightColor() const							{return lightColor;}
	ParticleSystemType *getParticleSystemType() const	{return particleSystemType;}
	StaticSound *getSound() const						{return sound;}
	float getSoundStartTime() const						{return soundStartTime;}
	bool isLoopSound() const							{return loopSound;}
	const EffectTypes &getRecourse() const				{return recourse;}
	void getDesc(string &str) const;
};

class Emanation : public EffectType {
private:
	int radius;

public:
	virtual ~Emanation() {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	int getRadius() const	{return radius;}
};

typedef vector<Emanation*> Emanations;

}}//end namespace


#endif
