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

#ifndef _GLEST_GAME_SKILLTYPE_H_
#define _GLEST_GAME_SKILLTYPE_H_

#include "sound.h"
#include "xml_parser.h"
#include "vec.h"
#include "model.h"
#include "lang.h"
#include "flags.h"
#include "factory.h"

using Shared::Sound::StaticSound;
using Shared::Xml::XmlNode;
using Shared::Math::Vec3f;
using Shared::Graphics::Model;
using Shared::Util::MultiFactory;

#include "damage_multiplier.h"
#include "effect_type.h"
#include "sound_container.h"
#include "particle_type.h"
#include "prototypes_enums.h"
#include "simulation_enums.h"
#include "entities_enums.h"

namespace Glest {
using namespace Sound;
using Global::Lang;
using Sim::CycleInfo;
using Entities::Unit;

namespace ProtoTypes {

// =====================================================
// 	class SkillType
//
///	A basic action that an unit can perform
// =====================================================

class SkillType : public NameIdPair{
	friend class SkillTypeFactory;

public:
	typedef vector<Model *> Animations;

	WRAPPED_ENUM( AnimationsStyle,
		SINGLE,
		SEQUENTIAL,
		RANDOM
	)

protected:
	//SkillClass skillClass;
	int epCost;
	int speed;
	int animSpeed;
	Animations animations;
	AnimationsStyle animationsStyle;
	SoundContainer sounds;
	float soundStartTime;
	const char* typeName;
	int minRange;
	int maxRange;

	float startTime;

	bool projectile;
	ProjectileType* projectileParticleSystemType;
	SoundContainer projSounds;

	bool splash;
	bool splashDamageAll;
	int splashRadius;
	SplashType* splashParticleSystemType;

	EffectTypes effectTypes;

public:
	//varios
	SkillType(SkillClass skillClass, const char* typeName);
	virtual ~SkillType();
	virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const = 0;
	void descEffects(string &str, const Unit *unit) const;
	//void descEffectsRemoved(string &str, const Unit *unit) const;
	void descSpeed(string &str, const Unit *unit, const char* speedType) const;
	void descRange(string &str, const Unit *unit, const char* rangeDesc) const;

	void descEpCost(string &str, const Unit *unit) const {
		if(epCost){
			str+= Lang::getInstance().get("EpCost") + ": " + intToStr(epCost) + "\n";
		}
	}

	CycleInfo calculateCycleTime() const;

	//get
	virtual SkillClass getClass() const	= 0;
	const string &getName() const		{return name;}
	int getEpCost() const				{return epCost;}
	int getSpeed() const				{return speed;}
	int getAnimSpeed() const			{return animSpeed;}
	const Model *getAnimation() const	{return animations.front();}
	StaticSound *getSound() const		{return sounds.getRandSound();}
	float getSoundStartTime() const		{return soundStartTime;}
	int getMaxRange() const				{return maxRange;}
	int getMinRange() const				{return minRange;}
	float getStartTime() const			{return startTime;}

	//other
	virtual string toString() const		{return Lang::getInstance().get(typeName);}
	static string skillClassToStr(SkillClass skillClass);
	static string fieldToStr(Zone field);

	//get proj
	bool getProjectile() const									{return projectile;}
	ProjectileType * getProjParticleType() const	{return projectileParticleSystemType;}
	StaticSound *getProjSound() const							{return projSounds.getRandSound();}

	//get splash
	bool getSplash() const										{return splash;}
	bool getSplashDamageAll() const								{return splashDamageAll;}
	int getSplashRadius() const									{return splashRadius;}
	SplashType * getSplashParticleType() const	{return splashParticleSystemType;}

	// get effects
	const EffectTypes &getEffectTypes() const	{return effectTypes;}
	bool hasEffects() const			{return effectTypes.size() > 0;}
};

// ===============================
// 	class StopSkillType
// ===============================

class StopSkillType: public SkillType {
public:
	StopSkillType() : SkillType(SkillClass::STOP, "Stop"){}
	virtual void getDesc(string &str, const Unit *unit) const {
		Lang &lang= Lang::getInstance();
		str+= lang.get("ReactionSpeed")+": "+ intToStr(speed)+"\n";
		descEpCost(str, unit);
	}
	virtual SkillClass getClass() const { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::STOP; }
};

// ===============================
// 	class MoveSkillType
// ===============================

class MoveSkillType: public SkillType {
private:
	float maxInclination;
	float maxDeclination;

public:
	MoveSkillType() : SkillType(SkillClass::MOVE, "Move"){}
	virtual void getDesc(string &str, const Unit *unit) const {
		descSpeed(str, unit, "WalkSpeed");
		descEpCost(str, unit);
	}
	//virtual void doChecksum(Checksum &checksum) const;
	virtual SkillClass getClass() const { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::MOVE; }
};
/*
class RangedType {
protected:
	int minRange;
	int maxRange;

public:
	RangedType();
	int getMaxRange() const					{return maxRange;}
	int getMinRange() const					{return minRange;}
	virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit, const char* rangeDesc) const;
};
*/
// ===============================
// 	class TargetBasedSkillType
//
/// Base class for both AttackSkillType and CastSpellSkillType
// ===============================

class TargetBasedSkillType: public SkillType {
protected:
	Zones zones;

public:
	TargetBasedSkillType(SkillClass skillClass, const char* typeName);
	virtual ~TargetBasedSkillType();
	virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const	{getDesc(str, unit, "Range");}
	virtual void getDesc(string &str, const Unit *unit, const char* rangeDesc) const;

	Zones getZones () const				{return zones;}
	bool getZone ( const Zone zone ) const		{return zones.get(zone);}
};

// ===============================
// 	class AttackSkillType
// ===============================

class AttackSkillType: public TargetBasedSkillType {
private:
	int attackStrength;
	int attackVar;
	fixed attackPctStolen;
	fixed attackPctVar;
	const AttackType *attackType;
//	EarthquakeType *earthquakeType;

public:
	AttackSkillType() : TargetBasedSkillType(SkillClass::ATTACK, "Attack"), attackType(NULL)/*, earthquakeType(NULL)*/ {}
	virtual ~AttackSkillType();

	virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void doChecksum(Checksum &checksum) const;

	//get
	int getAttackStrength() const				{return attackStrength;}
	int getAttackVar() const					{return attackVar;}
	//fixed getAttackPctStolen() const			{return attackPctStolen;}
	//fixed getAttackPctVar() const				{return attackPctVar;}
	const AttackType *getAttackType() const		{return attackType;}
//	const EarthquakeType *getEarthquakeType() const	{return earthquakeType;}

	virtual SkillClass getClass() const { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::ATTACK; }
};

// ===============================
// 	class BuildSkillType
// ===============================

class BuildSkillType: public SkillType{
public:
	BuildSkillType() : SkillType(SkillClass::BUILD, "Build") {}
	void getDesc(string &str, const Unit *unit) const {
		descSpeed(str, unit, "BuildSpeed");
		descEpCost(str, unit);
	}

	virtual SkillClass getClass() const { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::BUILD; }
};

// ===============================
// 	class HarvestSkillType
// ===============================

class HarvestSkillType: public SkillType{
public:
	HarvestSkillType() : SkillType(SkillClass::HARVEST, "Harvest") {}
	virtual void getDesc(string &str, const Unit *unit) const {}

	virtual SkillClass getClass() const { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::HARVEST; }
};

// ===============================
// 	class RepairSkillType
// ===============================

class RepairSkillType: public SkillType {
private:
	int amount;
	fixed multiplier;
	bool petOnly;
	bool selfOnly;
	bool selfAllowed;
	SplashType *splashParticleSystemType;

public:
	RepairSkillType();
	virtual ~RepairSkillType(){}// { delete splashParticleSystemType; }

	virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;

	int getAmount() const		{return amount;}
	fixed getMultiplier() const	{return multiplier;}
	SplashType *getSplashParticleSystemType() const	{return splashParticleSystemType;}
	bool isPetOnly() const		{return petOnly;}
	bool isSelfOnly() const		{return selfOnly;}
	bool isSelfAllowed() const	{return selfAllowed;}

	virtual SkillClass getClass() const { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::REPAIR; }
};

// ===============================
// 	class ProduceSkillType
// ===============================

class ProduceSkillType: public SkillType{
private:
	bool pet;
	int maxPets;

public:
	ProduceSkillType();
	virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const {
		descSpeed(str, unit, "ProductionSpeed");
		descEpCost(str, unit);
	}

	bool isPet() const		{return pet;}
	int getMaxPets() const	{return maxPets;}

	virtual SkillClass getClass() const { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::PRODUCE; }
};

// ===============================
// 	class UpgradeSkillType
// ===============================

class UpgradeSkillType: public SkillType{
public:
	UpgradeSkillType() : SkillType(SkillClass::UPGRADE, "Upgrade"){}
	virtual void getDesc(string &str, const Unit *unit) const {
		descSpeed(str, unit, "UpgradeSpeed");
		descEpCost(str, unit);
	}

	virtual SkillClass getClass() const { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::UPGRADE; }
};


// ===============================
// 	class BeBuiltSkillType
// ===============================

class BeBuiltSkillType: public SkillType{
public:
	BeBuiltSkillType() : SkillType(SkillClass::BE_BUILT, "Be built"){}
	virtual void getDesc(string &str, const Unit *unit) const {}

	virtual SkillClass getClass() const { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::BE_BUILT; }
};

// ===============================
// 	class MorphSkillType
// ===============================

class MorphSkillType: public SkillType{
public:
	MorphSkillType() : SkillType(SkillClass::MORPH, "Morph"){}
	virtual void getDesc(string &str, const Unit *unit) const {
		descSpeed(str, unit, "MorphSpeed");
		descEpCost(str, unit);
	}

	virtual SkillClass getClass() const { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::MORPH; }
};

// ===============================
// 	class DieSkillType
// ===============================

class DieSkillType: public SkillType{
private:
	bool fade;

public:
	DieSkillType() : SkillType(SkillClass::DIE, "Die"){}
	bool getFade() const	{return fade;}

	virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const {}

	virtual SkillClass getClass() const { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::DIE; }
};

// ===============================
// 	class SkillFactory
// ===============================

class SkillTypeFactory: public MultiFactory<SkillType>{
private:
	vector<SkillType*> types;
	int idCounter;

public:
	SkillTypeFactory();

	SkillType *newInstance(string classId) {
		SkillType *st = MultiFactory<SkillType>::newInstance(classId);
		st->setId(idCounter++);
		types.push_back(st);
		return st;
	}

	SkillType* getType(int id) {
		if (id < 0 || id >= types.size()) {
			throw runtime_error("Error: Unknown skill type id: " + intToStr(id));
		}
		return types[id];
	}

	int getSkillTypeCount() const { return idCounter; }
};


// ===============================
// 	class AttackSkillPreferences
// ===============================

class AttackSkillPreferences : public XmlBasedFlags<AttackSkillPreference, AttackSkillPreference::COUNT> {
public:
	void load(const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft) {
		XmlBasedFlags<AttackSkillPreference, AttackSkillPreference::COUNT>::load(node, dir, tt, ft, "flag", AttackSkillPreferenceNames);
	}
};

class AttackSkillTypes {
private:
	vector<const AttackSkillType*> types;
	vector<AttackSkillPreferences> associatedPrefs;
	int maxRange;
	Zones zones;
	AttackSkillPreferences allPrefs;

public:
	void init();
	int getMaxRange() const									{return maxRange;}
// const vector<const AttackSkillType*> &getTypes() const	{return types;}
	void getDesc(string &str, const Unit *unit) const;
	bool getZone(Zone zone) const						{return zones.get(zone);}
	bool hasPreference(AttackSkillPreference pref) const	{return allPrefs.get(pref);}
	const AttackSkillType *getPreferredAttack(const Unit *unit, const Unit *target, int rangeToTarget) const;
	const AttackSkillType *getSkillForPref(AttackSkillPreference pref, int rangeToTarget) const {
		assert(types.size() == associatedPrefs.size());
		for (int i = 0; i < types.size(); ++i) {
			if (associatedPrefs[i].get(pref) && types[i]->getMaxRange() >= rangeToTarget) {
				return types[i];
			}
		}
		return NULL;
	}

	void push_back(const AttackSkillType* ast, AttackSkillPreferences pref) {
		types.push_back(ast);
		associatedPrefs.push_back(pref);
	}

	void doChecksum(Checksum &checksum) const {
		for (int i=0; i < types.size(); ++i) {
			checksum.add(types[i]->getName());
		}
	}
};


}}//end namespace

#endif