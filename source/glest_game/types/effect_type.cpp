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

#include "effect_type.h"
#include "renderer.h"
#include "tech_tree.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class EffectTypeFlags
// =====================================================

const char *EffectTypeFlags::names[etfCount] = {
	"effects-ally",				// etfAlly
	"effects-foe",				// etfFoe
	"not-effects-normal-units", // etfNoNormalUnits
	"effects-buildings",		// etfBuildings
	"pets-only",				// eftPetsOnly
 	"effects-non-living",		// etfNonLiving
	"apply-splash-strength",	// etfScaleSplashStrength
	"ends-with-source",			// etfEndsWhenSourceDies
	"recourse-ends-with-target",// etfRecourseEndsWithRoot
	"permanent",				// etfPermanent
	"allow-negative-speed",		// etfAllowNegativeSpeed
	"tick-immediately",			// etfTickImmediately
	"damaged",					// etfAIDamaged
	"ranged",					// etfAIRanged
	"melee",					// etfAIMelee
	"worker",					// etfAIWorker
	"building",					// etfAIBuilding
	"heavy",					// etfAIHeavy
	"scout",					// etfAIScout
	"combat",					// etfAICombat
	"use-sparingly",			// etfAIUseSparingly
	"use-liberally"				// etfAIUseLiberally
};

// =====================================================
// 	class EffectType
// =====================================================

EffectType::EffectType() : lightColor(0.0f) {
	//DisplayableType members
	name = "";
	image = NULL;

	bias = ebNeutral;
	stacking = esStack;

	duration = 0;
	chance = 100.0f;
	light = false;
	particleSystemType = NULL;
	sound = NULL;
	soundStartTime = 0.0f;
	loopSound = false;
	damageType = NULL;
}

void EffectType::load(const XmlNode *effectNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	string tmp;
	const XmlAttribute *attr;
	const XmlNode *node;

	//name
	name= effectNode->getAttribute("name")->getRestrictedValue();

	//bigtime hack
	((TechTree*)tt)->addEffectType(this);

	//bias
	tmp= effectNode->getAttribute("bias")->getRestrictedValue();
	if(tmp=="detrimental"){
		bias= ebDetrimental;
	}else if(tmp=="neutral"){
		bias= ebNeutral;
	}else if(tmp=="benificial"){
		bias= ebBenificial;
	}else{
		throw runtime_error("Not a valid value for bias: " + tmp + ": " + dir);
	}

	//stacking
	tmp= effectNode->getAttribute("stacking")->getRestrictedValue();
	if(tmp=="stack"){
		stacking= esStack;
	}else if(tmp=="extend"){
		stacking= esExtend;
	}else if(tmp=="overwrite"){
		stacking= esOverwrite;
	}else if(tmp=="reject"){
		stacking= esReject;
	}else{
		throw runtime_error("Not a valid value for stacking: " + tmp + ": " + dir);
	}

	//target (default all)
	attr = effectNode->getAttribute("target", false);
	if(attr) {
		tmp= attr->getRestrictedValue();
		if(tmp=="ally"){
			flags.set(etfAlly, true);
		}else if(tmp=="foe"){
			flags.set(etfFoe, true);
		}else if(tmp=="pet"){
			flags.set(etfAlly, true);
			flags.set(eftPetsOnly, true);
		}else if(tmp=="all"){
			flags.set(etfAlly, true);
			flags.set(etfFoe, true);
		}else{
			throw runtime_error("Not a valid value for units-effected: " + tmp + ": " + dir);
		}
	} else {
		//default value
		flags.set(etfAlly, true);
		flags.set(etfFoe, true);
	}

	//chance (default 100%)
	attr = effectNode->getAttribute("chance", false);
	if(attr) {
		chance = attr->getFloatValue();
	} else {
		chance = 100.0f;
	}

	//duration
	duration= effectNode->getAttribute("duration")->getIntValue();

	//damageType (default NULL)
	attr = effectNode->getAttribute("damage-type", false);
	if(attr) {
		damageType= tt->getAttackType(attr->getRestrictedValue());
	}

	//display (default true)
	attr = effectNode->getAttribute("display", false);
	if(attr) {
		display= attr->getBoolValue();
	} else {
		display= true;
	}

	//image (default NULL)
	attr = effectNode->getAttribute("image", false);
	if(attr && !(attr->getRestrictedValue() == "")) {
		image = Renderer::getInstance().newTexture2D(rsGame);
		image->load(dir + "/" + attr->getRestrictedValue());
	}

	//flags
	const XmlNode *flagsNode = effectNode->getChild("flags", 0, false);
	if(flagsNode) {
		flags.load(flagsNode, dir, tt, ft);
	}

	EnhancementTypeBase::load(effectNode, dir, tt, ft);

	//light & lightColor
	const XmlNode *lightNode= effectNode->getChild("light", 0, false);
	if(lightNode) {
		light= lightNode->getAttribute("enabled")->getBoolValue();
		if(light){
			lightColor.x= lightNode->getAttribute("red")->getFloatValue(0.f, 1.f);
			lightColor.y= lightNode->getAttribute("green")->getFloatValue(0.f, 1.f);
			lightColor.z= lightNode->getAttribute("blue")->getFloatValue(0.f, 1.f);
		}
	} else {
		light = false;
	}

	//particle
	const XmlNode *particleNode= effectNode->getChild("particle", 0, false);
	if(particleNode && particleNode->getAttribute("value")->getBoolValue()) {
		string path= particleNode->getAttribute("path")->getRestrictedValue();
		particleSystemType= new ParticleSystemType();
		particleSystemType->load(effectNode,  dir + "/" + path);
	}

	//sound
	const XmlNode *soundNode= effectNode->getChild("sound", 0, false);
	if(soundNode && soundNode->getAttribute("enabled")->getBoolValue()) {
		soundStartTime= soundNode->getAttribute("start-time")->getFloatValue();
		loopSound= soundNode->getAttribute("loop")->getBoolValue();
		string path= soundNode->getAttribute("path")->getRestrictedValue();
		sound= new StaticSound();
		sound->load(dir + "/" + path);
	}

	//recourse
	const XmlNode *recourseEffectsNode= effectNode->getChild("recourse-effects", 0, false);
	if(recourseEffectsNode) {
		recourse.resize(recourseEffectsNode->getChildCount());
		for(int i=0; i<recourseEffectsNode->getChildCount(); ++i){
			const XmlNode *recourseEffectNode= recourseEffectsNode->getChild("effect", i);
			EffectType *effectType = new EffectType();
			effectType->load(recourseEffectNode, dir, tt, ft);
			recourse[i]= effectType;
		}
	}

	if(hpRegeneration >= 0 && damageType) {
		damageType = NULL;
	}
}

string &EffectType::getDesc(string &str) const {
	return str;
}


// =====================================================
// 	class Emanation
// =====================================================

void Emanation::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	EffectType::load(n, dir, tt, ft);

	//radius
	radius = n->getAttribute("radius")->getIntValue();
}

}}//end namespace
