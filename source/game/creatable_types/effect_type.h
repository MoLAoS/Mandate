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

#ifndef _GLEST_PROTOTYPES_EFFECT_DEFINED_
#define _GLEST_PROTOTYPES_EFFECT_DEFINED_

#include "sound.h"
#include "vec.h"
#include "element_type.h"
#include "xml_parser.h"
#include "particle_type.h"
#include "upgrade_type.h"
#include "flags.h"
#include "game_constants.h"
#include "abilities.h"

using Shared::Sound::StaticSound;
using Shared::Xml::XmlNode;
using Shared::Math::Vec3f;

namespace Glest { namespace ProtoTypes {

class TechTree;
class FactionType;

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
	void load(const XmlNode *node, const string &dir) {
		XmlBasedFlags<EffectTypeFlag, EffectTypeFlag::COUNT>::load(node, dir, "flag", EffectTypeFlagNames);
	}
};

class EffectType;
typedef vector<EffectType*> EffectTypes;

/**
 * Represents the type for a possibly temporary effect which modifies the
 * stats, regeneration/degeneration and possibly other behaviors &
 * attributes of a Unit.
 */
class EffectType: public DisplayableType {
private:
    Statistics statistics;
	EffectBias bias;
	EffectStacking stacking;
	int duration;
	fixed chance;
	bool light;
	Vec3f lightColour;
	StaticSound *sound;
	float soundStartTime;
	bool loopSound;
	EffectTypes recourse;
	EffectTypeFlags flags;
	string affectTag;
	bool display;
	UnitParticleSystemTypes particleSystems;
public:
	static EffectClass typeClass() { return EffectClass::EFFECT; }
public:
	EffectType();
	virtual ~EffectType() { delete sound; }
	virtual bool load(const XmlNode *n, const string &dir);
	virtual void doChecksum(Checksum &checksum) const;

    const Statistics *getStatistics() const {return &statistics;}

	EffectBias getBias() const				 {return bias;}
	EffectStacking getStacking() const		 {return stacking;}
	const EffectTypeFlags &getFlags() const	 {return flags;}
	bool getFlag(EffectTypeFlag flag) const	 {return flags.get(flag);}
	const string& getAffectTag() const		 {return affectTag;}

	bool isEffectsAlly() const				{return getFlag(EffectTypeFlag::ALLY);}
	bool isEffectsFoe() const				{return getFlag(EffectTypeFlag::FOE);}
	//bool isEffectsNormalUnits() const		{return !getFlag(EffectTypeFlag::NO_NORMAL_UNITS);}
	//bool isEffectsBuildings() const			{return getFlag(EffectTypeFlag::BUILDINGS);}
	bool isEffectsPetsOnly() const			{return getFlag(EffectTypeFlag::PETS_ONLY);}
	//bool isEffectsNonLiving() const			{return getFlag(EffectTypeFlag::NON_LIVING);}
	bool isScaleSplashStrength() const		{return getFlag(EffectTypeFlag::SCALE_SPLASH_STRENGTH);}
	bool isEndsWhenSourceDies() const		{return getFlag(EffectTypeFlag::ENDS_WITH_SOURCE);}
	bool idRecourseEndsWithRoot() const		{return getFlag(EffectTypeFlag::RECOURSE_ENDS_WITH_ROOT);}
	bool isPermanent() const				{return getFlag(EffectTypeFlag::PERMANENT);}
	bool isTickImmediately() const			{return getFlag(EffectTypeFlag::TICK_IMMEDIATELY);}
	bool isCauseCloak() const				{return getFlag(EffectTypeFlag::CAUSES_CLOAK);}

	int getDuration() const								{return duration;}
	fixed getChance() const								{return chance;}
	bool isLight() const								{return light;}
	bool isDisplay() const								{return display;}
	Vec3f getLightColour() const						{return lightColour;}
	UnitParticleSystemTypes& getParticleTypes()			{return particleSystems;}
	const UnitParticleSystemTypes& getParticleTypes() const	{return particleSystems;}
	StaticSound *getSound() const						{return sound;}
	float getSoundStartTime() const						{return soundStartTime;}
	bool isLoopSound() const							{return loopSound;}
	const EffectTypes &getRecourse() const				{return recourse;}
	void getDesc(string &str);
	void save(XmlNode *node) const;
};

class EmanationType : public EffectType {
private:
	int radius;

public:
	static EffectClass typeClass() { return EffectClass::EMANATION; }

public:
	virtual ~EmanationType() {}
	virtual bool load(const XmlNode *n, const string &dir);
	virtual void doChecksum(Checksum &checksum) const;
	int getRadius() const	{return radius;}
};

typedef vector<EmanationType*> Emanations;

}}//end namespace


#endif
