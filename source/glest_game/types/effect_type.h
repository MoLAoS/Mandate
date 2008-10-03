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


using Shared::Sound::StaticSound;
using Shared::Xml::XmlNode;
using Shared::Graphics::Vec3f;

namespace Glest { namespace Game {

class FactionType;
class TechTree;

// ==============================================================
// 	enum EffectTypeFlag & class EffectTypeFlags
// ==============================================================

enum EffectTypeFlag {
	etfAlly,					// effects allies
	etfFoe,						// effects foes
	etfNoNormalUnits,			// doesn't effects normal units
	etfBuildings,				// effects buildings
	eftPetsOnly,				// only effects pets of the originator
	etfNonLiving,				//
	etfScaleSplashStrength,		// decrease strength when applied from splash
	etfEndsWhenSourceDies,		// ends when the unit causing the effect dies
	etfRecourseEndsWithRoot,	// ends when root effect ends (recourse effects only)
	etfPermanent,				// the effect has an infinite duration
	etfAllowNegativeSpeed,		//
	etfTickImmediately,			//

	//ai hints
	etfAIDamaged,				// use on damaged units (benificials only)
	etfAIRanged,				// use on ranged attack units
	etfAIMelee,					// use on melee units
	etfAIWorker,				// use on worker units
	etfAIBuilding,				// use on buildings
	etfAIHeavy,					// perfer to use on heavy units
	etfAIScout,					// useful for scouting units
	etfAICombat,				// don't use outside of combat (benificials only)
	etfAISparingly,				// use sparingly
	etfAILiberally,				// use liberally

	etfCount
};

/**
 * Flags for an EffectType object.
 *
 * @see EffectTypeFlag
 */
class EffectTypeFlags : public XmlBasedFlags<EffectTypeFlag, etfCount>{
private:
	static const char *names[etfCount];

public:
	void load(const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft) {
		XmlBasedFlags<EffectTypeFlag, etfCount>::load(node, dir, tt, ft, "flag", names);
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


public:
	/** Rather the effect is detrimental, neutral or benificial */
	enum EffectBias {
		ebDetrimental,
		ebNeutral,
		ebBenificial,

		ebCount
	};

	/**
	 * How the attempt to apply multiple instances of this effect should be
	 * handled
	 */
	enum EffectStacking {
		esStack,
		esExtend,
		esOverwrite,
		esReject,

  		esCount
	};

	/** Valid flag names/descriptions for values in EffectBias enum. */
	static const char *effectBiasNames[ebCount];

	/** Valid flag names/descriptions for values in EffectStacking enum. */
	static const char *effectStackingNames[esCount];


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
	virtual ~EffectType() {}
	virtual void load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);

	EffectBias getBias() const				{return bias;}
	EffectStacking getStacking() const		{return stacking;}
	const EffectTypeFlags &getFlags() const	{return flags;}
	bool getFlag(EffectTypeFlag flag) const	{return flags.get(flag);}
	const AttackType *getDamageType() const	{return damageType;}

	bool isEffectsAlly() const				{return getFlag(etfAlly);}
	bool isEffectsFoe() const				{return getFlag(etfFoe);}
	bool isEffectsNormalUnits() const		{return !getFlag(etfNoNormalUnits);}
	bool isEffectsBuildings() const			{return getFlag(etfBuildings);}
	bool isEffectsPetsOnly() const			{return getFlag(eftPetsOnly);}
	bool isEffectsNonLiving() const			{return getFlag(etfNonLiving);}
	bool isScaleSplashStrength() const		{return getFlag(etfScaleSplashStrength);}
	bool isEndsWhenSourceDies() const		{return getFlag(etfEndsWhenSourceDies);}
	bool idRecourseEndsWithRoot() const		{return getFlag(etfRecourseEndsWithRoot);}
	bool isPermanent() const				{return getFlag(etfPermanent);}
	bool isAllowNegativeSpeed() const		{return getFlag(etfAllowNegativeSpeed);}
	bool isTickImmediately() const			{return getFlag(etfTickImmediately);}

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
	string &getDesc(string &str) const;
};

class Emanation : public EffectType {
private:
	int radius;

public:
	virtual ~Emanation() {}
	virtual void load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	int getRadius() const	{return radius;}
};

typedef vector<Emanation*> Emanations;

}}//end namespace


#endif
