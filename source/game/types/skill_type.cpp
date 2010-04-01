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

#include "pch.h"
#include "skill_type.h"

#include <cassert>

#include "sound.h"
#include "util.h"
#include "lang.h"
#include "renderer.h"
#include "particle_type.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "map.h"
#include "earthquake_type.h"
#include "network_message.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
// 	class SkillType
// =====================================================

SkillType::SkillType(SkillClass skillClass, const char* typeName) 
		: NameIdPair()
		, skillClass(skillClass)
		, effectTypes()
		, epCost(0)
		//, speed(0.f)
		//, animSpeed(0.f)
		, animations()
		, animationsStyle(AnimationsStyle::SINGLE)
		, sounds()
		, soundStartTime(0.f)
		, typeName(typeName)
		, minRange(0)
		, maxRange(0)
		, effectsRemoved(0)
		, removeBenificialEffects(false)
		, removeDetrimentalEffects(false)
		, removeAllyEffects(false)
		, removeEnemyEffects(false) {
}

SkillType::~SkillType(){
	deleteValues(sounds.getSounds().begin(), sounds.getSounds().end());
}

void SkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft){

	name = sn->getChildStringValue("name");
	epCost = sn->getOptionalIntValue("ep-cost");
	speed = sn->getChildIntValue("speed");
	minRange = sn->getOptionalIntValue("min-range", 1);
	maxRange = sn->getOptionalIntValue("max-range", 1);
	animSpeed = sn->getChildIntValue("anim-speed");

	//model
	const XmlNode *animNode = sn->getChild("animation");
	if (animNode->getAttribute("path")) { // single animation, lagacy style
		string path= sn->getChild("animation")->getAttribute("path")->getRestrictedValue();
		animations.push_back(Renderer::getInstance().newModel(rsGame));
		animations.back()->load(dir + "/" + path);
		animationsStyle = AnimationsStyle::SINGLE;
	} else { // multi-anim, new style
		for (int i=0; i < animNode->getChildCount(); ++i) {
			string path = animNode->getChild("anim-file", i)->getAttribute("path")->getRestrictedValue();
			animations.push_back(Renderer::getInstance().newModel(rsGame));
			animations.back()->load(dir + "/" + path);
		}
		animationsStyle = AnimationsStyle::RANDOM;
	}

	//sound
	const XmlNode *soundNode= sn->getChild("sound", 0, false);
	if(soundNode && soundNode->getBoolAttribute("enabled")) {
		soundStartTime= soundNode->getAttribute("start-time")->getFloatValue();
		for(int i=0; i<soundNode->getChildCount(); ++i){
			const XmlNode *soundFileNode= soundNode->getChild("sound-file", i);
			string path= soundFileNode->getAttribute("path")->getRestrictedValue();
			StaticSound *sound= new StaticSound();
			sound->load(dir + "/" + path);
			sounds.add(sound);
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
		effectsRemoved = removeEffectsNode->getIntAttribute("count");
		removeBenificialEffects = removeEffectsNode->getChildBoolValue("benificial");
		removeDetrimentalEffects = removeEffectsNode->getChildBoolValue("detrimental");
		removeAllyEffects = removeEffectsNode->getChildBoolValue("allies");
		removeEnemyEffects = removeEffectsNode->getChildBoolValue("foes");
	} else {
		effectsRemoved = 0;
	}

	startTime= sn->getOptionalFloatValue("start-time");

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
			for(int i=0; i<soundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode= soundNode->getChild("sound-file", i);
				string path= soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				projSounds.add(sound);
			}
		}
	}

	//splash damage and/or special effects
	const XmlNode *splashNode = sn->getChild("splash", 0, false);
	splash = splashNode && splashNode->getBoolValue();
	if (splash) {
		splashDamageAll = splashNode->getOptionalBoolValue("damage-all", true);
		splashRadius= splashNode->getChild("radius")->getAttribute("value")->getIntValue();

		//splash particle
		const XmlNode *particleNode= splashNode->getChild("particle", 0, false);
		if(particleNode && particleNode->getAttribute("value")->getBoolValue()){
			string path= particleNode->getAttribute("path")->getRestrictedValue();
			splashParticleSystemType= new ParticleSystemTypeSplash();
			splashParticleSystemType->load(dir,  dir + "/" + path);
		}
	}
}

void SkillType::doChecksum(Checksum &checksum) const {
	checksum.add<SkillClass>(skillClass);
	foreach_const (EffectTypes, it, effectTypes) {
		(*it)->doChecksum(checksum);
	}
	checksum.add(name);
	checksum.add<int>(epCost);
	checksum.add<int>(speed);
	checksum.add<int>(animSpeed);
	checksum.add<int>(minRange);
	checksum.add<int>(maxRange);
	checksum.add<int>(effectsRemoved);
	checksum.add<bool>(removeBenificialEffects);
	checksum.add<bool>(removeDetrimentalEffects);
	checksum.add<bool>(removeAllyEffects);
	checksum.add<bool>(removeEnemyEffects);
	checksum.add<float>(startTime);
	checksum.add<bool>(projectile);
	checksum.add<bool>(splash);
	checksum.add<bool>(splashDamageAll);
	checksum.add<int>(splashRadius);

}

void SkillType::descEffects(string &str, const Unit *unit) const {
	for(EffectTypes::const_iterator i = effectTypes.begin(); i != effectTypes.end(); ++i) {
		str += "Effect: ";
		(*i)->getDesc(str);
	}
}

void SkillType::descRange(string &str, const Unit *unit, const char* rangeDesc) const {
	str+= Lang::getInstance().get(rangeDesc)+": ";
	if(minRange > 1) {
		str += 	intToStr(minRange) + "...";
	}
	str += intToStr(maxRange);
	EnhancementType::describeModifier(str, unit->getMaxRange(this) - maxRange);
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
	EnhancementType::describeModifier(str, unit->getSpeed(this) - speed);
	str+="\n";
}

CycleInfo SkillType::calculateCycleTime() const {
	static const float speedModifier = 1.f / GameConstants::speedDivider / float(WORLD_FPS);
	if (skillClass == SkillClass::MOVE) {
		return CycleInfo(-1, -1);
	}
	float progressSpeed = getSpeed() * speedModifier;
	float animProgressSpeed = getAnimSpeed() * speedModifier;
	int skillFrames = int(1.0000001f / progressSpeed);
	int animFrames = int(1.0000001f / animProgressSpeed);
	int attackOffset = -1;
	if (skillClass == SkillClass::ATTACK) {
		attackOffset = int(startTime / animProgressSpeed);
		if (!attackOffset) attackOffset = 1;
		assert(attackOffset > 0 && attackOffset < 256);
	}
	int soundOffset = -1;
	if (!sounds.getSounds().empty()) {
		soundOffset = int(soundStartTime / animProgressSpeed);
		if (soundOffset < 1) ++soundOffset;
		assert(soundOffset > 0);
	}
	assert(skillFrames > 0 && animFrames > 0);
	return CycleInfo(skillFrames, animFrames, soundOffset, attackOffset);
}


/** obsolete??? use SkillClassNames[sc] ?? */
string SkillType::skillClassToStr(SkillClass skillClass){
	switch(skillClass){
		case SkillClass::STOP: return "Stop";
		case SkillClass::MOVE: return "Move";
		case SkillClass::ATTACK: return "Attack";
		case SkillClass::HARVEST: return "Harvest";
		case SkillClass::REPAIR: return "Repair";
		case SkillClass::BUILD: return "Build";
		case SkillClass::DIE: return "Die";
		case SkillClass::BE_BUILT: return "Be Built";
		case SkillClass::PRODUCE: return "Produce";
		case SkillClass::UPGRADE: return "Upgrade";
		case SkillClass::CAST_SPELL: return "Cast Spell";
		default:
			assert(false);
			return "";
	};
}

/** obsolete??? use FieldNames[f] ?? */
string SkillType::fieldToStr(Zone field){
	switch (field) {
		case Zone::SURFACE_PROP: return "SurfaceProp";
		case Zone::LAND: return "Surface";
		case Zone::AIR: return "Air";
			//	case fSubsurface: return "Subsurface";

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
	EnhancementType::describeModifier(str, unit->getMaxRange(this) - maxRange);
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

	//fields
	const XmlNode *attackFieldsNode = sn->getChild("attack-fields", 0, false);
	const XmlNode *fieldsNode = sn->getChild("fields", 0, false);
	if(attackFieldsNode && fieldsNode) {
		throw runtime_error("Cannot specify both <attack-fields> and <fields>.");
	}
	if(!attackFieldsNode && !fieldsNode) {
		throw runtime_error("Must specify either <attack-fields> or <fields>.");
	}
	zones.load(fieldsNode ? fieldsNode : attackFieldsNode, dir, tt, ft);
}

void TargetBasedSkillType::doChecksum(Checksum &checksum) const {
	SkillType::doChecksum(checksum);
	foreach_enum (Zone, z) {
		checksum.add<bool>(zones.get(z));
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
	str+= lang.get("Zones") + ": ";
	for(int i= 0; i < Zone::COUNT; i++){
		Zone zone = enum_cast<Zone>(i);
		if(zones.get(zone)){
			str+= fieldToStr(zone) + " ";
		}
	}
	str+="\n";
}

// =====================================================
// 	class AttackSkillType
// =====================================================

AttackSkillType::~AttackSkillType() { 
	delete earthquakeType; 
}

void AttackSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft){
	TargetBasedSkillType::load(sn, dir, tt, ft);

	//misc
	if (sn->getOptionalChild("attack-strenght")) { // support vanilla-glest typo
		attackStrength = sn->getChild("attack-strenght")->getAttribute("value")->getIntValue();
	} else {
		attackStrength = sn->getChild("attack-strength")->getAttribute("value")->getIntValue();
	}
	attackVar= sn->getChild("attack-var")->getAttribute("value")->getIntValue();
	maxRange= sn->getOptionalIntValue("attack-range");
	string attackTypeName= sn->getChild("attack-type")->getAttribute("value")->getRestrictedValue();
	attackType= tt->getAttackType(attackTypeName);
	startTime= sn->getOptionalFloatValue("attack-start-time");

	const XmlNode *attackPctStolenNode= sn->getChild("attack-percent-stolen", 0, false);
	if(attackPctStolenNode) {
		attackPctStolen = attackPctStolenNode->getAttribute("value")->getFixedValue() / 100;
		attackPctVar = attackPctStolenNode->getAttribute("var")->getFixedValue() / 100;
	} else {
		attackPctStolen = 0;
		attackPctVar = 0;
	}

	earthquakeType = NULL;
	XmlNode *earthquakeNode = sn->getChild("earthquake", 0, false);
	if(earthquakeNode) {
		earthquakeType = new EarthquakeType(float(attackStrength), attackType);
		earthquakeType->load(earthquakeNode, dir, tt, ft);
	}
}

void AttackSkillType::doChecksum(Checksum &checksum) const {
	TargetBasedSkillType::doChecksum(checksum);

	checksum.add(attackStrength);
	checksum.add(attackVar);
	checksum.add(attackPctStolen);
	checksum.add(attackPctVar);
	attackType->doChecksum(checksum);
	
	// earthquakeType ??
}

void AttackSkillType::getDesc(string &str, const Unit *unit) const {
	Lang &lang= Lang::getInstance();

	//attack strength
	str+= lang.get("AttackStrength")+": ";
	str+= intToStr(attackStrength - attackVar);
	str+= "...";
	str+= intToStr(attackStrength + attackVar);
	EnhancementType::describeModifier(str, unit->getAttackStrength(this) - attackStrength);
	str+= " ("+ attackType->getName() +")";
	str+= "\n";
/*
	if(unit->getAttackPctStolen(this) != 0 || attackPctVar != 0) {
		str+= lang.get("HealthStolen") + ": ";
		fixed fhigh = unit->getAttackPctStolen(this) + attackPctVar;
		fixed flow = unit->getAttackPctStolen(this) - attackPctVar;

		str += Conversion::toStr(flow * 100);
		if(fhigh != flow) {
			str += "% ... ";
			str += Conversion::toStr(fhigh * 100);
		}
		str += "%\n";
	}
*/
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

// ===============================
// 	class DieSkillType
// ===============================

void DieSkillType::doChecksum(Checksum &checksum) const {
	SkillType::doChecksum(checksum);
	checksum.add<bool>(fade);
}

void DieSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft) {
	SkillType::load(sn, dir, tt, ft);

	fade= sn->getChild("fade")->getAttribute("value")->getBoolValue();
}

// ===============================
// 	class RepairSkillType
// ===============================

RepairSkillType::RepairSkillType() : SkillType(SkillClass::REPAIR, "Repair"), splashParticleSystemType(NULL) {
	amount = 0;
	multiplier = 1;
	petOnly = false;
	selfAllowed = true;
	selfOnly = false;
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
		multiplier = n->getAttribute("value")->getFixedValue();
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

void RepairSkillType::doChecksum(Checksum &checksum) const {
	SkillType::doChecksum(checksum);
	checksum.add(amount);
	checksum.add(multiplier);
	checksum.add(petOnly);
	checksum.add(selfOnly);
	checksum.add(selfAllowed);
}

void RepairSkillType::getDesc(string &str, const Unit *unit) const {
	Lang &lang= Lang::getInstance();
//	descRange(str, unit, "MaxRange");

	descSpeed(str, unit, "RepairSpeed");
	if(amount) {
		str+= "\n" + lang.get("HpRestored")+": "+ intToStr(amount);
	}
	if(maxRange > 1){
		str+= "\n" + lang.get("Range")+": "+ intToStr(maxRange);
	}
	str+= "\n";
	descEpCost(str, unit);
}

// =====================================================
// 	class ProduceSkillType
// =====================================================

ProduceSkillType::ProduceSkillType() : SkillType(SkillClass::PRODUCE, "Produce") {
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

void ProduceSkillType::doChecksum(Checksum &checksum) const {
	SkillType::doChecksum(checksum);
	checksum.add<bool>(pet);
	checksum.add<int>(maxPets);
}

// ===============================
// 	class FallDownSkillType
// ===============================

FallDownSkillType::FallDownSkillType(const SkillType *model) : SkillType(SkillClass::FALL_DOWN, "Fall down") {
    speed = model->getSpeed();
    animSpeed = model->getAnimSpeed();
    animations.push_back((Model *)model->getAnimation());
	agility = 0.5f;
}

void FallDownSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft) {
	SkillType::load(sn, dir, tt, ft);

	agility = sn->getChildFloatValue("agility");
}

void FallDownSkillType::doChecksum(Checksum &checksum) const {
	SkillType::doChecksum(checksum);
	checksum.add<float>(agility);
}
// ===============================
// 	class GetUpSkillType
// ===============================

GetUpSkillType::GetUpSkillType(const SkillType *model) : SkillType(SkillClass::GET_UP, "Get up") {
    speed = 50;//model->getSpeed();
    animSpeed = model->getAnimSpeed();
    animations.push_back((Model *)model->getAnimation());
}

void GetUpSkillType::load(const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft) {
	SkillType::load(sn, dir, tt, ft);
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
	registerClass<FallDownSkillType>("fall_down");
	registerClass<GetUpSkillType>("get_up");
}

SkillTypeFactory &SkillTypeFactory::getInstance(){
	static SkillTypeFactory ctf;
	return ctf;
}

}} //end namespace
