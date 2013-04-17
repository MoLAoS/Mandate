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

#ifndef _GLEST_GAME_SKILLTYPE_H_
#define _GLEST_GAME_SKILLTYPE_H_

#include "sound.h"
#include "xml_parser.h"
#include "vec.h"
#include "model.h"
#include "lang.h"
#include "flags.h"
#include "factory.h"
#include "abilities.h"
#include <set>
using std::set;

using Shared::Sound::StaticSound;
using Shared::Xml::XmlNode;
using Shared::Math::Vec3f;
using Shared::Graphics::Model;
using Shared::Util::MultiFactory;

#include "effect_type.h"
#include "sound_container.h"
#include "prototypes_enums.h"
#include "simulation_enums.h"
#include "entities_enums.h"
#include "particle_type.h"	// forward decs

namespace Glest {
using namespace Sound;
using Global::Lang;
using Sim::CycleInfo;
using Entities::Unit;

namespace ProtoTypes {
WRAPPED_ENUM( AnimationsStyle,
    SINGLE,
    SEQUENTIAL,
    RANDOM
)
typedef vector<Model *> Animations;
typedef map<SurfaceType, Model*>         AnimationBySurface;
typedef pair<SurfaceType, SurfaceType>   SurfacePair;
typedef map<SurfacePair, Model*>         AnimationBySurfacePair;
// =====================================================
// 	class SoundsAndAnimations
//
///	Class to hold sounds and animations for skills
// =====================================================
class SoundsAndAnimations {
private:
	int animSpeed;
	Animations animations;
	AnimationsStyle animationsStyle;
	AnimationBySurface     m_animBySurfaceMap;
	AnimationBySurfacePair m_animBySurfPairMap;

	SoundContainer sounds;
	float soundStartTime;
public:
    SoundsAndAnimations();
    virtual ~SoundsAndAnimations();

	int getAnimSpeed() const			{return animSpeed;}
	virtual bool isStretchyAnim() const {return false;}
	const Model *getAnimation(SurfaceType from, SurfaceType to) const;
	const Model *getAnimation(SurfaceType st) const;
	const Model *getAnimation() const	{return animations.front();}

	StaticSound *getSound() const		{return sounds.getRandSound();}
	float getSoundStartTime() const		{return soundStartTime;}
	bool hasSounds() const			    {return !sounds.getSounds().empty();}
	Sounds getSounds() const	{return sounds.getSounds();}

	bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct);
};

// =====================================================
// 	class ItemCost
//
///	Class detail the costs of a skill
// =====================================================
class ItemCost {
private:
    int amount;
    const ItemType *type;
public:
    int getAmount() const {return amount;}
    const ItemType *getType() const {return type;}
    void init(int amount, const ItemType *type);
};

typedef vector<ItemCost> ItemCosts;
typedef vector<ResourceAmount> ResCosts;

class SkillCosts {
private:
    ItemCosts itemCosts;
    ResCosts resourceCosts;
    int hpCost;
    int spCost;
    int epCost;
public:
    int getItemCostCount() const {return itemCosts.size();}
    int getResourceCostCount() const {return resourceCosts.size();}

    const ItemCost *getItemCost(int i) const {return &itemCosts[i];}
    const ResourceAmount *getResourceCost(int i) const {return &resourceCosts[i];}

    int getHpCost() const {return hpCost;}
    int getSpCost() const {return spCost;}
    int getEpCost() const {return epCost;}

    bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct);
    void init();
};

// =====================================================
// 	class SkillType
//
///	A basic action that an unit can perform
///@todo revise, these seem to hold data/properties for
/// actions rather than the actions themselves.
// =====================================================
class SkillType : public NameIdPair {
	friend class DynamicTypeFactory<SkillClass, SkillType>;
private:
    int minRange;
    int maxRange;
    SkillCosts skillCosts;
    SoundsAndAnimations soundsAndAnimations;
	const char* typeName;
	int speed;
	bool m_deCloak;
	float startTime;
	UnitParticleSystemTypes eyeCandySystems;
	const CreatableType *m_creatableType;

public:
    const CreatableType *getCreatableType() const {return m_creatableType;}
    const SoundsAndAnimations *getSoundsAndAnimations() const {return &soundsAndAnimations;}
    const SkillCosts *getSkillCosts() const {return &skillCosts;}
	SkillType(const char* typeName);
	virtual ~SkillType();
	virtual bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const = 0;
	virtual void descSpeed(string &str, const Unit *unit, const char* speedType) const;

	CycleInfo calculateCycleTime() const;

	int getMinRange() const				            {return minRange;}
	int getMaxRange() const		                    {return maxRange;}

	virtual SkillClass getClass() const	= 0;
	int getBaseSpeed() const            {return speed;}
	const string &getName() const		{return m_name;}
	float getStartTime() const			{return startTime;}
	bool causesDeCloak() const			{return m_deCloak;}

    virtual fixed getSpeed(const Unit *unit) const	{return speed;}

	unsigned getEyeCandySystemCount() const { return eyeCandySystems.size(); }
	const UnitParticleSystemType* getEyeCandySystem(unsigned i) const {
		assert(i < eyeCandySystems.size());
		return eyeCandySystems[i];
	}
	void setDeCloak(bool v)	{m_deCloak = v;}
	virtual string toString() const	{return Lang::getInstance().get(typeName);}
};

// ===============================
// 	class StopSkillType
// ===============================

class StopSkillType: public SkillType {
public:
	StopSkillType() : SkillType("Stop"){}
	virtual void getDesc(string &str, const Unit *unit) const override {
		Lang &lang= Lang::getInstance();
		str+= lang.get("ReactionSpeed")+": "+ intToStr(getBaseSpeed())+"\n";
	}
	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::STOP; }
};

// ===============================
// 	class MoveSkillType
// ===============================

class MoveSkillType: public SkillType {
private:
	float maxInclination;
	float maxDeclination;
	bool visibleOnly;

public:
	MoveSkillType() : SkillType("Move"), visibleOnly(false) {}
	virtual bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct) override;
	virtual void getDesc(string &str, const Unit *unit) const override {
		descSpeed(str, unit, "WalkSpeed");
	}
	//virtual void doChecksum(Checksum &checksum) const;
	bool getVisibleOnly() const { return visibleOnly; }
	virtual fixed getSpeed(const Unit *unit) const override;
	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::MOVE; }
};

// ===============================
// 	class TargetBasedSkillType
//
/// Base class for both AttackSkillType and CastSpellSkillType
// ===============================
class TargetBasedSkillType: public SkillType {
private:
	Zones zones;
	bool projectile;
	ProjectileType* projectileParticleSystemType;
	SoundContainer projSounds;

	bool splash;
	bool splashDamageAll;
	int splashRadius;
	SplashType* splashParticleSystemType;

	EffectTypes effectTypes;

public:
	TargetBasedSkillType(const char* typeName);
	virtual ~TargetBasedSkillType();
	virtual void doChecksum(Checksum &checksum) const;

	bool getProjectile() const						{return projectile;}
	ProjectileType * getProjParticleType() const	{return projectileParticleSystemType;}
	StaticSound *getProjSound() const				{return projSounds.getRandSound();}

	bool getSplash() const							{return splash;}
	bool getSplashDamageAll() const					{return splashDamageAll;}
	int getSplashRadius() const						{return splashRadius;}
	SplashType * getSplashParticleType() const	    {return splashParticleSystemType;}

	const EffectTypes &getEffectTypes() const	    {return effectTypes;}
	bool hasEffects() const			                {return effectTypes.size() > 0;}

	Zones getZones () const				            {return zones;}
	bool getZone ( const Zone zone ) const		    {return zones.get(zone);}

	virtual bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct) override;
	virtual void getDesc(string &str, const Unit *unit) const override {getDesc(str, unit, "Range");}
	virtual void getDesc(string &str, const Unit *unit, const char* rangeDesc) const;

	void descEffects(string &str, const Unit *unit) const;
    void descRange(string &str, const Unit *unit, const char* rangeDesc) const;
};

// ===============================
// 	class AttackSkillType
// ===============================

class AttackSkillType: public TargetBasedSkillType {
private:
    int cooldown;
	AttackStats attackStats;
    DamageTypes damageTypes;
//	EarthquakeType *earthquakeType;
public:

	AttackSkillType() : TargetBasedSkillType("Attack"), damageTypes(0) /*, earthquakeType(NULL)*/ {}
	virtual ~AttackSkillType();

	virtual bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct) override;
	virtual void getDesc(string &str, const Unit *unit) const override;
	virtual void doChecksum(Checksum &checksum) const override;

	//get
	virtual fixed getSpeed(const Unit *unit) const override;
	const AttackStats *getAttackStats() const		  {return &attackStats;}
	const DamageTypes *getDamageTypes() const         {return &damageTypes;}
	int getDamageTypeCount() const                    {return damageTypes.size();}
	const DamageType *getDamageType(int i) const      {return &damageTypes[i];}
	int getCooldown() const				              {return cooldown;}
//	const EarthquakeType *getEarthquakeType() const	{return earthquakeType;}

	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::ATTACK; }
};

// ===============================
// 	class ConstructSkillType
// ===============================

class ConstructSkillType: public SkillType{
public:
	ConstructSkillType() : SkillType("Construct") {}
	void getDesc(string &str, const Unit *unit) const {
		descSpeed(str, unit, "ConstructSpeed");
	}

	virtual fixed getSpeed(const Unit *unit) const override;
	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::CONSTRUCT; }
};

// ===============================
// 	class BuildSkillType
// ===============================

class BuildSkillType: public SkillType{
public:
	BuildSkillType() : SkillType("Build") {}
	void getDesc(string &str, const Unit *unit) const {
		descSpeed(str, unit, "BuildSpeed");
	}

	virtual fixed getSpeed(const Unit *unit) const override;
	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::BUILD; }
};

// ===============================
// 	class HarvestSkillType
// ===============================

class HarvestSkillType: public SkillType{
public:
	HarvestSkillType() : SkillType("Harvest") {}
	virtual void getDesc(string &str, const Unit *unit) const override {
		// command modifies displayed speed, all handled in HarvestCommandType
	}
	virtual fixed getSpeed(const Unit *unit) const override;
	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::HARVEST; }
};

// ===============================
// 	class TransportSkillType
// ===============================

class TransportSkillType: public SkillType{
public:
	TransportSkillType() : SkillType("Transport") {}
	virtual void getDesc(string &str, const Unit *unit) const override {
	}
	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::TRANSPORT; }
};

// ===============================
// 	class SetStructureSkillType
// ===============================

class SetStructureSkillType: public SkillType{
public:
	SetStructureSkillType() : SkillType("SetStructure") {}
	virtual void getDesc(string &str, const Unit *unit) const override {
	}
	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::SET_STRUCTURE; }
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
	//SplashType *splashParticleSystemType;

public:
	RepairSkillType();
	virtual ~RepairSkillType(){}// { delete splashParticleSystemType; }

	virtual bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct) override;
	virtual void doChecksum(Checksum &checksum) const override;
	virtual void getDesc(string &str, const Unit *unit) const override;

	virtual fixed getSpeed(const Unit *unit) const override;
	int getAmount() const		{return amount;}
	fixed getMultiplier() const	{return multiplier;}
	//SplashType *getSplashParticleSystemType() const	{return splashParticleSystemType;}
	bool isPetOnly() const		{return petOnly;}
	bool isSelfOnly() const		{return selfOnly;}
	bool isSelfAllowed() const	{return selfAllowed;}

	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::REPAIR; }
};

// ===============================
// 	class MaintainSkillType
// ===============================

class MaintainSkillType: public SkillType {
private:
	int amount;
	fixed multiplier;
	bool petOnly;
	bool selfOnly;
	bool selfAllowed;

public:
	MaintainSkillType();
	virtual ~MaintainSkillType(){}

	virtual bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct) override;
	virtual void doChecksum(Checksum &checksum) const override;
	virtual void getDesc(string &str, const Unit *unit) const override;

	virtual fixed getSpeed(const Unit *unit) const override;
	int getAmount() const		{return amount;}
	fixed getMultiplier() const	{return multiplier;}

	bool isPetOnly() const		{return petOnly;}
	bool isSelfOnly() const		{return selfOnly;}
	bool isSelfAllowed() const	{return selfAllowed;}

	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::MAINTAIN; }
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
	virtual bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct) override;
	virtual void doChecksum(Checksum &checksum) const override;
	virtual void getDesc(string &str, const Unit *unit) const override {
		descSpeed(str, unit, "ProductionSpeed");
	}
	virtual fixed getSpeed(const Unit *unit) const override;

	bool isPet() const		{return pet;}
	int getMaxPets() const	{return maxPets;}


	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::PRODUCE; }
};

// ===============================
// 	class UpgradeSkillType
// ===============================

class UpgradeSkillType: public SkillType{
public:
	UpgradeSkillType() : SkillType("Upgrade"){}
	virtual void getDesc(string &str, const Unit *unit) const override {
		descSpeed(str, unit, "UpgradeSpeed");
	}
	virtual fixed getSpeed(const Unit *unit) const override;

	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::UPGRADE; }
};


// ===============================
// 	class BeBuiltSkillType
// ===============================

class BeBuiltSkillType: public SkillType {
private:
	bool  m_stretchy;

public:
	BeBuiltSkillType() : SkillType("Be built"){}
	virtual bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct) override;
	virtual void getDesc(string &str, const Unit *unit) const override {}
	virtual bool isStretchyAnim() const {return m_stretchy;}
	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::BE_BUILT; }
};

// ===============================
// 	class MorphSkillType
// ===============================

class MorphSkillType: public SkillType{
public:
	MorphSkillType() : SkillType("Morph"){}
	virtual void getDesc(string &str, const Unit *unit) const override {
		descSpeed(str, unit, "MorphSpeed");
	}
	virtual fixed getSpeed(const Unit *unit) const override;

	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::MORPH; }
};

// ===============================
// 	class DieSkillType
// ===============================

class DieSkillType: public SkillType{
private:
	bool fade;

public:
	DieSkillType() : SkillType("Die"), fade(0.0f) {}
	bool getFade() const	{return fade;}

	virtual bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct) override;
	virtual void doChecksum(Checksum &checksum) const override;
	virtual void getDesc(string &str, const Unit *unit) const override {}

	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::DIE; }
};

// ===============================
// 	class LoadSkillType
// ===============================

class LoadSkillType: public SkillType{
public:
	LoadSkillType();
	virtual bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct) override;
	virtual void doChecksum(Checksum &checksum) const override;
	virtual void getDesc(string &str, const Unit *unit) const override {
		descSpeed(str, unit, "Speed");
	}

	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::LOAD; }
};

// ===============================
// 	class UnloadSkillType
// ===============================

class UnloadSkillType: public SkillType{
public:
	UnloadSkillType() : SkillType("Unload"){}
	virtual void getDesc(string &str, const Unit *unit) const override {
		descSpeed(str, unit, "Speed");
	}

	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::UNLOAD; }
};

// ===============================
// 	class CastSpellSkillType
// ===============================

class CastSpellSkillType : public TargetBasedSkillType {
public:
	CastSpellSkillType() : TargetBasedSkillType("CastSpell") {}

	virtual void getDesc(string &str, const Unit *unit) const override {
		descSpeed(str, unit, "Speed");
		descEffects(str, unit);
	}

	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::CAST_SPELL; }

};

// ===============================
// 	class BuildSelfSkillType
// ===============================

class BuildSelfSkillType : public SkillType {
private:
	bool  m_stretchy;

public:
	BuildSelfSkillType() : SkillType("BuildSelf") {}

	virtual bool load(const XmlNode *sn, const string &dir, const TechTree *tt, const CreatableType *ct) override;
	virtual void getDesc(string &str, const Unit *unit) const override {
		descSpeed(str, unit, "Speed");
	}
	virtual bool isStretchyAnim() const {return m_stretchy;}
	virtual SkillClass getClass() const override { return typeClass(); }
	static SkillClass typeClass() { return SkillClass::BUILD_SELF; }

};


// ===============================
// 	class ModelFactory
// ===============================

class ModelFactory {
private:
	typedef map<string, Model*> ModelMap;

private:
	ModelMap models;
	int idCounter;

	Model* newInstance(const string &path, int size, int height);

public:
	ModelFactory();
	Model* getModel(const string &path, int size, int height);
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
	const AttackSkillType *getFirstAttackSkill() const {return types[0];}
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
