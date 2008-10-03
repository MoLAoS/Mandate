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
#include "vec.h"
#include "model.h"
#include "xml_parser.h"
#include "util.h"
#include "unit_stats_base.h"
#include "damage_multiplier.h"
#include "element_type.h"
#include "effect_type.h"
#include "factory.h"
#include "sound_container.h"
#include "lang.h"

using Shared::Sound::StaticSound;
using Shared::Xml::XmlNode;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Model;

namespace Glest{ namespace Game{

using Shared::Util::MultiFactory;

class ParticleSystemTypeProjectile;
class ParticleSystemTypeSplash;
class FactionType;
class TechTree;
class EnhancementTypeBase;
class Unit;

enum SkillClass{
    scStop,
    scMove,
    scAttack,
    scBuild,
    scHarvest,
    scRepair,
    scBeBuilt,
    scProduce,
    scUpgrade,
	scMorph,
	scDie,
	scCastSpell,

    scCount
};

// =====================================================
// 	class SkillType
//
///	A basic action that an unit can perform
// =====================================================

class SkillType{
protected:
    SkillClass skillClass;
	EffectTypes effectTypes;
	string name;
	int epCost;
    int speed;
    int animSpeed;
    Model *animation;
    SoundContainer sounds;
	float soundStartTime;
	const char* typeName;
	int minRange;
	int maxRange;

	int effectsRemoved;
	bool removeBenificialEffects;
	bool removeDetrimentalEffects;
	bool removeAllyEffects;
	bool removeEnemyEffects;

public:
    //varios
	SkillType(SkillClass skillClass, const char* typeName) {
		this->skillClass = skillClass;
		this->typeName = typeName;
	}
    virtual ~SkillType();
    virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const = 0;
	void descEffects(string &str, const Unit *unit) const;
	void descEffectsRemoved(string &str, const Unit *unit) const;
	void descSpeed(string &str, const Unit *unit, const char* speedType) const;
	void descRange(string &str, const Unit *unit, const char* rangeDesc) const;

	void descEpCost(string &str, const Unit *unit) const {
		if(epCost){
			str+= Lang::getInstance().get("EpCost") + ": " + intToStr(epCost) + "\n";
		}
	}

    //get
	const string &getName() const		{return name;}
	SkillClass getClass() const			{return skillClass;}
	const EffectTypes &getEffectTypes() const	{return effectTypes;}
	bool isHasEffects() const			{return effectTypes.size() > 0;}
	int getEpCost() const				{return epCost;}
	int getSpeed() const				{return speed;}
	int getAnimSpeed() const			{return animSpeed;}
	const Model *getAnimation() const	{return animation;}
	StaticSound *getSound() const		{return sounds.getRandSound();}
	float getSoundStartTime() const		{return soundStartTime;}
	int getMaxRange() const				{return maxRange;}
	int getMinRange() const				{return minRange;}

	//other
	virtual string toString() const		{return Lang::getInstance().get(typeName);}
	static string skillClassToStr(SkillClass skillClass);
	static string fieldToStr(Field field);

	// get removing effects
	int getEffectsRemoved() const			{return effectsRemoved;}
	bool isRemoveBenificialEffects() const	{return removeBenificialEffects;}
	bool isRemoveDetrimentalEffects() const	{return removeDetrimentalEffects;}
	bool isRemoveAllyEffects() const		{return removeAllyEffects;}
	bool isRemoveEnemyEffects() const		{return removeEnemyEffects;}

};

// ===============================
// 	class StopSkillType
// ===============================

class StopSkillType: public SkillType{
public:
    StopSkillType() : SkillType(scStop, "Stop"){}
	virtual void getDesc(string &str, const Unit *unit) const {
		Lang &lang= Lang::getInstance();
		str+= lang.get("ReactionSpeed")+": "+ intToStr(speed)+"\n";
		descEpCost(str, unit);
	}
};

// ===============================
// 	class MoveSkillType
// ===============================

class MoveSkillType: public SkillType{
public:
    MoveSkillType() : SkillType(scMove, "Move"){}
	virtual void getDesc(string &str, const Unit *unit) const {
		descSpeed(str, unit, "WalkSpeed");
		descEpCost(str, unit);
	}
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
	Fields fields;
	float startTime;

	bool projectile;
	ParticleSystemTypeProjectile* projectileParticleSystemType;
	SoundContainer projSounds;

	bool splash;
	int splashRadius;
	ParticleSystemTypeSplash* splashParticleSystemType;

public:
    TargetBasedSkillType(SkillClass skillClass, const char* typeName);
    virtual ~TargetBasedSkillType();
    virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const	{getDesc(str, unit, "Range");}
	virtual void getDesc(string &str, const Unit *unit, const char* rangeDesc) const;

	Fields getFields() const				{return fields;}
	bool getField(Field field) const		{return fields.get(field);}
	float getStartTime() const				{return startTime;}

	//get proj
	bool getProjectile() const									{return projectile;}
	ParticleSystemTypeProjectile * getProjParticleType() const	{return projectileParticleSystemType;}
	StaticSound *getProjSound() const							{return projSounds.getRandSound();}

	//get splash
	bool getSplash() const										{return splash;}
	int getSplashRadius() const									{return splashRadius;}
	ParticleSystemTypeSplash * getSplashParticleType() const	{return splashParticleSystemType;}
};

// ===============================
// 	class AttackSkillType
// ===============================

class AttackSkillType: public TargetBasedSkillType{
private:
    int attackStrength;
    int attackVar;
	float attackPctStolen;
    float attackPctVar;
	const AttackType *attackType;

public:
    AttackSkillType() : TargetBasedSkillType(scAttack, "Attack"), attackType(NULL) {}
    virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const;

	//get
	int getAttackStrength() const				{return attackStrength;}
	int getAttackVar() const					{return attackVar;}
	float getAttackPctStolen() const			{return attackPctStolen;}
	float getAttackPctVar() const				{return attackPctVar;}
	const AttackType *getAttackType() const		{return attackType;}
};

// ===============================
// 	class BuildSkillType
// ===============================

class BuildSkillType: public SkillType{
public:
    BuildSkillType() : SkillType(scBuild, "Build") {}
	void getDesc(string &str, const Unit *unit) const {
		descSpeed(str, unit, "BuildSpeed");
		descEpCost(str, unit);
	}
};

// ===============================
// 	class HarvestSkillType
// ===============================

class HarvestSkillType: public SkillType{
public:
    HarvestSkillType() : SkillType(scHarvest, "Harvest") {}
	virtual void getDesc(string &str, const Unit *unit) const {}
};

// ===============================
// 	class RepairSkillType
// ===============================

class RepairSkillType: public SkillType {
private:
	int amount;
	float multiplier;
	bool petOnly;
	bool selfOnly;
	bool selfAllowed;
	ParticleSystemTypeSplash *splashParticleSystemType;

public:
    RepairSkillType();
	virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const;

	int getAmount() const		{return amount;}
	float getMultiplier() const	{return multiplier;}
	ParticleSystemTypeSplash *getSplashParticleSystemType() const	{return splashParticleSystemType;}
	bool isPetOnly() const		{return petOnly;}
	bool isSelfOnly() const		{return selfOnly;}
	bool isSelfAllowed() const	{return selfAllowed;}
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
	virtual void getDesc(string &str, const Unit *unit) const {
		descSpeed(str, unit, "ProductionSpeed");
		descEpCost(str, unit);
	}

	bool isPet() const		{return pet;}
	int getMaxPets() const	{return maxPets;}
};

// ===============================
// 	class UpgradeSkillType
// ===============================

class UpgradeSkillType: public SkillType{
public:
    UpgradeSkillType() : SkillType(scUpgrade, "Upgrade"){}
	virtual void getDesc(string &str, const Unit *unit) const {
		descSpeed(str, unit, "UpgradeSpeed");
		descEpCost(str, unit);
	}
};


// ===============================
// 	class BeBuiltSkillType
// ===============================

class BeBuiltSkillType: public SkillType{
public:
    BeBuiltSkillType() : SkillType(scBeBuilt, "Be built"){}
	virtual void getDesc(string &str, const Unit *unit) const {}
};

// ===============================
// 	class MorphSkillType
// ===============================

class MorphSkillType: public SkillType{
public:
    MorphSkillType() : SkillType(scMorph, "Morph"){}
	virtual void getDesc(string &str, const Unit *unit) const {
		descSpeed(str, unit, "MorphSpeed");
		descEpCost(str, unit);
	}
};

// ===============================
// 	class DieSkillType
// ===============================

class DieSkillType: public SkillType{
private:
	bool fade;

public:
    DieSkillType() : SkillType(scDie, "Die"){}
    bool getFade() const	{return fade;}

	virtual void load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const {}
};

// ===============================
// 	class CastSpellSkillType
// ===============================

class CastSpellSkillType: public TargetBasedSkillType{
public:
    CastSpellSkillType() : TargetBasedSkillType(scCastSpell, "Cast spell"){}
	virtual void getDesc(string &str, const Unit *unit) const {}
};

// ===============================
// 	class SkillFactory
// ===============================

class SkillTypeFactory: public MultiFactory<SkillType>{
private:
	SkillTypeFactory();
public:
	static SkillTypeFactory &getInstance();
};

}}//end namespace

#endif
