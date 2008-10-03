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

#include "skill_type.h"

#include <cassert>

#include "sound.h"
#include "util.h"
#include "lang.h"
#include "renderer.h"
#include "particle_type.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
// 	class SkillType
// =====================================================

SkillType::~SkillType(){
	deleteValues(sounds.getSounds().begin(), sounds.getSounds().end());
}

void SkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft){
	//name
	name= sn->getChild("name")->getAttribute("value")->getRestrictedValue();

	//ep cost
	epCost= sn->getChild("ep-cost")->getAttribute("value")->getIntValue();

	//speed
	speed= sn->getChild("speed")->getAttribute("value")->getIntValue();

	minRange= sn->getOptionalIntValue("min-range", 1);
	maxRange= sn->getOptionalIntValue("max-range", 1);

	//anim speed
	animSpeed= sn->getChild("anim-speed")->getAttribute("value")->getIntValue();

	//model
	string path= sn->getChild("animation")->getAttribute("path")->getRestrictedValue();
	animation= Renderer::getInstance().newModel(rsGame);
	animation->load(dir + "/" + path);

	//sound
	const XmlNode *soundNode= sn->getChild("sound");
	if(soundNode->getAttribute("enabled")->getBoolValue()){

		soundStartTime= soundNode->getAttribute("start-time")->getFloatValue();

		sounds.resize(soundNode->getChildCount());
		for(int i=0; i<soundNode->getChildCount(); ++i){
			const XmlNode *soundFileNode= soundNode->getChild("sound-file", i);
			string path= soundFileNode->getAttribute("path")->getRestrictedValue();
			StaticSound *sound= new StaticSound();
			sound->load(dir + "/" + path);
			sounds[i]= sound;
		}
	}

	//effects
	const XmlNode *effectsNode= sn->getChild("effects", 0, false);
	if(effectsNode) {
		effectTypes.resize(effectsNode->getChildCount());
		for(int i=0; i<effectsNode->getChildCount(); ++i){
			const XmlNode *effectNode= effectsNode->getChild("effect", i);
			EffectType *effectType = new EffectType();
			effectType->load(effectNode, dir, tt, ft);
			effectTypes[i]= effectType;
		}
	}

	//removing effects
	const XmlNode *removeEffectsNode = sn->getChild("remove-effects", 0, false);
	if(removeEffectsNode) {
		effectsRemoved = removeEffectsNode->getAttribute("count")->getIntValue();
		removeBenificialEffects = removeEffectsNode->getChild("benificial")->getAttribute("value")->getBoolValue();
		removeDetrimentalEffects = removeEffectsNode->getChild("detrimental")->getAttribute("value")->getBoolValue();
		removeAllyEffects = removeEffectsNode->getChild("allies")->getAttribute("value")->getBoolValue();
		removeEnemyEffects = removeEffectsNode->getChild("foes")->getAttribute("value")->getBoolValue();
	} else {
		effectsRemoved = 0;
	}
}

void SkillType::descEffects(string &str, const Unit *unit) const {
}

void SkillType::descRange(string &str, const Unit *unit, const char* rangeDesc) const {
	str+= Lang::getInstance().get(rangeDesc)+": ";
	if(minRange > 1) {
		str += 	intToStr(minRange) + "...";
	}
	str += intToStr(maxRange);
	EnhancementTypeBase::describeModifier(str, unit->getMaxRange(this) - maxRange);
	str+="\n";
}


void SkillType::descEffectsRemoved(string &str, const Unit *unit) const {
	Lang &lang= Lang::getInstance();

	if(effectsRemoved) {
		str+= lang.get("Removes") + " " + intToStr(effectsRemoved) + " ";
		if(removeBenificialEffects) {
			str += lang.get("benificial");
		}
		if(removeDetrimentalEffects) {
			if(removeBenificialEffects) {
				str += " " + lang.get("or") + " ";
			}
			str += lang.get("detrimental");
		}
		str += " " + lang.get("EffectsFrom") + ": ";
		if(removeAllyEffects) {
			str += lang.get("ally") + " ";
		}
		if(removeEnemyEffects) {
			str += lang.get("enemy") + " ";
		}
	}
}

void SkillType::descSpeed(string &str, const Unit *unit, const char* speedType) const {
	str+= Lang::getInstance().get(speedType) + ": " + intToStr(speed);
	EnhancementTypeBase::describeModifier(str, unit->getSpeed(this) - speed);
	str+="\n";
}

string SkillType::skillClassToStr(SkillClass skillClass){
	switch(skillClass){
	case scStop: return "Stop";
	case scMove: return "Move";
	case scAttack: return "Attack";
	case scHarvest: return "Harvest";
	case scRepair: return "Repair";
	case scBuild: return "Build";
	case scDie: return "Die";
	case scBeBuilt: return "Be Built";
	case scProduce: return "Produce";
	case scUpgrade: return "Upgrade";
	case scCastSpell: return "Cast Spell";
	default:
		assert(false);
		return "";
	};
}

string SkillType::fieldToStr(Field field){
	switch(field){
	case fLand: return "Land";
	case fAir: return "Air";
//	case fWater: return "Water";
//	case fSubterranean: return "Subterranean";

		default:
		assert(false);
		return "";
	};
}
// ===============================
// 	class MoveSkillType
// ===============================


// ===============================
// 	class RangedType
// ===============================
/*
RangedType::RangedType() {
	minRange = 0;
	maxRange = 1;
}

void RangedType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft) {
	minRange= sn->getOptionalIntValue("min-range");
	maxRange= sn->getOptionalIntValue("max-range");
}

void RangedType::getDesc(string &str, const Unit *unit, const char* rangeDesc) const {
	str+= Language::getInstance().get(rangeDesc)+": ";
	if(minRange > 1) {
		str += 	intToStr(minRange) + "...";
	}
	str += intToStr(maxRange);
	EnhancementTypeBase::describeModifier(str, unit->getMaxRange(this) - maxRange);
	str+="\n";
}
*/

// =====================================================
// 	class TargetBasedSkillType
// =====================================================

TargetBasedSkillType::TargetBasedSkillType(SkillClass skillClass, const char* typeName)
		: SkillType(skillClass, typeName) {
	startTime = 0.0f;

    projectile= false;
	projectileParticleSystemType = NULL;

	splash = false;
    splashRadius = 0;
	splashParticleSystemType= NULL;
}

TargetBasedSkillType::~TargetBasedSkillType(){
	if(projectileParticleSystemType) {
		delete projectileParticleSystemType;
	}
	if(splashParticleSystemType) {
		delete splashParticleSystemType;
	}
	deleteValues(projSounds.getSounds().begin(), projSounds.getSounds().end());
}

void TargetBasedSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft){
	SkillType::load(sn, dir, tt, ft);

	startTime= sn->getOptionalFloatValue("start-time");

	//fields
	const XmlNode *attackFieldsNode = sn->getChild("attack-fields", 0, false);
	const XmlNode *fieldsNode = sn->getChild("fields", 0, false);
	if(attackFieldsNode && fieldsNode) {
		throw runtime_error("Cannot specify both <attack-fields> and <fields>.");
	}
	if(!attackFieldsNode && !fieldsNode) {
		throw runtime_error("Must specify either <attack-fields> or <fields>.");
	}
	fields.load(fieldsNode ? fieldsNode : attackFieldsNode, dir, tt, ft);

	//projectile
	const XmlNode *projectileNode= sn->getChild("projectile", 0, false);
	if(projectileNode && projectileNode->getAttribute("value")->getBoolValue()) {

		//proj particle
		const XmlNode *particleNode= projectileNode->getChild("particle", 0, false);
		if(particleNode && particleNode->getAttribute("value")->getBoolValue()){
			string path= particleNode->getAttribute("path")->getRestrictedValue();
			projectileParticleSystemType= new ParticleSystemTypeProjectile();
			projectileParticleSystemType->load(dir,  dir + "/" + path);
		}

		//proj sounds
		const XmlNode *soundNode= projectileNode->getChild("sound", 0, false);
		if(soundNode && soundNode->getAttribute("enabled")->getBoolValue()) {
			projSounds.resize(soundNode->getChildCount());
			for(int i=0; i<soundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode= soundNode->getChild("sound-file", i);
				string path= soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				projSounds[i]= sound;
			}
		}
	}

	//splash
	const XmlNode *splashNode= sn->getChild("splash", 0, false);
	if(splashNode && splashNode->getAttribute("value")->getBoolValue()) {
		splash = true;
		splashRadius= splashNode->getChild("radius")->getAttribute("value")->getIntValue();

		//splash particle
		const XmlNode *particleNode= splashNode->getChild("particle", 0, false);
		if(particleNode && particleNode->getAttribute("value")->getBoolValue()){
			string path= particleNode->getAttribute("path")->getRestrictedValue();
			splashParticleSystemType= new ParticleSystemTypeSplash();
			splashParticleSystemType->load(dir,  dir + "/" + path);
		}
	} else {
		splash = false;
	}
}

void TargetBasedSkillType::getDesc(string &str, const Unit *unit, const char* rangeDesc) const {
	Lang &lang= Lang::getInstance();

	descEpCost(str, unit);

	//splash radius
	if(splashRadius){
		str+= lang.get("SplashRadius")+": "+intToStr(splashRadius)+"\n";
	}

	descRange(str, unit, rangeDesc);

	//fields
	str+= lang.get("Fields") + ": ";
	for(int i= 0; i < fCount; i++){
		Field field = static_cast<Field>(i);
		if(fields.get(field)){
			str+= fieldToStr(field) + " ";
		}
	}
	str+="\n";
}

// =====================================================
// 	class AttackSkillType
// =====================================================

void AttackSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft){
	TargetBasedSkillType::load(sn, dir, tt, ft);

	//misc
	attackStrength= sn->getChild("attack-strenght")->getAttribute("value")->getIntValue();
	attackVar= sn->getChild("attack-var")->getAttribute("value")->getIntValue();
	maxRange= sn->getOptionalIntValue("attack-range");
	string attackTypeName= sn->getChild("attack-type")->getAttribute("value")->getRestrictedValue();
	attackType= tt->getAttackType(attackTypeName);
	startTime= sn->getOptionalFloatValue("attack-start-time");

	const XmlNode *attackPctStolenNode= sn->getChild("attack-percent-stolen", 0, false);
	if(attackPctStolenNode) {
		attackPctStolen = attackPctStolenNode->getAttribute("value")->getFloatValue() / 100.0f;
		attackPctVar = attackPctStolenNode->getAttribute("var")->getFloatValue() / 100.0f;
	} else {
		attackPctStolen = 0.0f;
		attackPctVar = 0.0f;
	}
}

void AttackSkillType::getDesc(string &str, const Unit *unit) const {
	Lang &lang= Lang::getInstance();

	//attack strength
	str+= lang.get("AttackStrenght")+": ";
	str+= intToStr(attackStrength - attackVar);
	str+= "...";
	str+= intToStr(attackStrength + attackVar);
	EnhancementTypeBase::describeModifier(str, unit->getAttackStrength(this) - attackStrength);
	str+= " ("+ attackType->getName() +")";
	str+= "\n";

	if(unit->getAttackPctStolen(this) || attackPctVar) {
		str+= lang.get("HealthStolen") + ": ";
		float fhigh = unit->getAttackPctStolen(this) + attackPctVar;
		float flow = unit->getAttackPctStolen(this) - attackPctVar;

		str += floatToStr(flow * 100.0f);
		if(fhigh != flow) {
			str += "% ... ";
			str += floatToStr(fhigh * 100.0f);
		}
		str += "%\n";
	}

	TargetBasedSkillType::getDesc(str, unit, "AttackDistance");

	descSpeed(str, unit, "AttackSpeed");
	descEffects(str, unit);
	descEffectsRemoved(str, unit);
}

// ===============================
// 	class BuildSkillType
// ===============================


// ===============================
// 	class HarvestSkillType
// ===============================


// =====================================================
// 	class DieSkillType
// =====================================================

void DieSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft) {
	SkillType::load(sn, dir, tt, ft);

	fade= sn->getChild("fade")->getAttribute("value")->getBoolValue();
}

// ===============================
// 	class RepairSkillType
// ===============================

RepairSkillType::RepairSkillType() : SkillType(scRepair, "Repair") {
	amount = 0;
	multiplier = 1.0f;
	petOnly = false;
	selfAllowed = true;
	selfOnly = false;
	splashParticleSystemType = NULL;
}

void RepairSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft) {
	minRange = 1;
	maxRange = 1;
	SkillType::load(sn, dir, tt, ft);

	XmlNode *n;

	if((n = sn->getChild("amount", 0, false))) {
		amount = n->getAttribute("value")->getIntValue();
	}

	if((n = sn->getChild("multiplier", 0, false))) {
		multiplier = n->getAttribute("value")->getFloatValue();
	}

	if((n = sn->getChild("pet-only", 0, false))) {
		petOnly = n->getAttribute("value")->getBoolValue();
	}

	if((n = sn->getChild("self-only", 0, false))) {
		selfOnly = n->getAttribute("value")->getBoolValue();
		selfAllowed = true;
	}

	if((n = sn->getChild("self-allowed", 0, false))) {
		selfAllowed = n->getAttribute("value")->getBoolValue();
	}

	if(selfOnly && !selfAllowed) {
		throw runtime_error("Repair skill can't specify self-only as true and self-allowed as false (dork).");
	}

	if(petOnly && selfOnly) {
		throw runtime_error("Repair skill can't specify pet-only with self-only.");
	}

	//splash particle
	const XmlNode *particleNode= sn->getChild("particle", 0, false);
	if(particleNode && particleNode->getAttribute("value")->getBoolValue()){
		string path= particleNode->getAttribute("path")->getRestrictedValue();
		splashParticleSystemType= new ParticleSystemTypeSplash();
		splashParticleSystemType->load(dir,  dir + "/" + path);
	}
}

void RepairSkillType::getDesc(string &str, const Unit *unit) const {
	Lang &lang= Lang::getInstance();
	descRange(str, unit, "MaxRange");

	descSpeed(str, unit, "RepairSpeed");
	if(amount) {
		str+= lang.get("HpRestored")+": "+ intToStr(amount);
	}
	if(maxRange > 1){
		str+= lang.get("Range")+": "+ intToStr(maxRange);
	}
	str+= "\n";
	descEpCost(str, unit);
}

// =====================================================
// 	class ProduceSkillType
// =====================================================

ProduceSkillType::ProduceSkillType() : SkillType(scProduce, "Produce") {
	pet = false;
	maxPets = 0;
}

void ProduceSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft) {
	SkillType::load(sn, dir, tt, ft);

	XmlNode *petNode = sn->getChild("pet", 0, false);
	if(petNode) {
		pet = petNode->getAttribute("value")->getBoolValue();
		maxPets = petNode->getAttribute("max")->getIntValue();
	}
}

// =====================================================
// 	class SkillTypeFactory
// =====================================================

SkillTypeFactory::SkillTypeFactory(){
	registerClass<StopSkillType>("stop");
	registerClass<MoveSkillType>("move");
	registerClass<AttackSkillType>("attack");
	registerClass<BuildSkillType>("build");
	registerClass<BeBuiltSkillType>("be_built");
	registerClass<HarvestSkillType>("harvest");
	registerClass<RepairSkillType>("repair");
	registerClass<ProduceSkillType>("produce");
	registerClass<UpgradeSkillType>("upgrade");
	registerClass<MorphSkillType>("morph");
	registerClass<DieSkillType>("die");
	registerClass<CastSpellSkillType>("cast_spell");

}

SkillTypeFactory &SkillTypeFactory::getInstance(){
	static SkillTypeFactory ctf;
	return ctf;
}

}} //end namespace
