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

#include "pch.h"
#include "effect_type.h"
#include "renderer.h"
#include "tech_tree.h"
#include "logger.h"

#include "leak_dumper.h"

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

bool EffectType::load(const XmlNode *effectNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	string tmp;
	const XmlAttribute *attr;
	const XmlNode *node;

	bool loadOk = true;
	//name
	try { name = effectNode->getAttribute("name")->getRestrictedValue(); }
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}
	//bigtime hack
	id = ((TechTree*)tt)->addEffectType(this);

	//bias
	try {
		tmp = effectNode->getAttribute("bias")->getRestrictedValue();
		if (tmp == "detrimental") 
			bias = ebDetrimental;
		else if (tmp == "neutral")
			bias = ebNeutral;
		else if (tmp == "benificial")
			bias = ebBenificial;
		else
			throw runtime_error("Not a valid value for bias: " + tmp + ": " + dir);
	} catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}

	//stacking
	try {
		tmp = effectNode->getAttribute("stacking")->getRestrictedValue();
		if(tmp == "stack")
			stacking = esStack;
		else if(tmp == "extend")
			stacking = esExtend;
		else if(tmp == "overwrite")
			stacking = esOverwrite;
		else if(tmp == "reject")
			stacking = esReject;
		else
			throw runtime_error("Not a valid value for stacking: " + tmp + ": " + dir);
	} catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}

	//target (default all)
	try {
		attr = effectNode->getAttribute("target", false);
		if(attr) {
			tmp = attr->getRestrictedValue();

			if(tmp == "ally")
				flags.set(etfAlly, true);
			else if(tmp == "foe")
				flags.set(etfFoe, true);
			else if(tmp == "pet") {
				flags.set(etfAlly, true);
				flags.set(eftPetsOnly, true);
			}
			else if(tmp == "all") {
				flags.set(etfAlly, true);
				flags.set(etfFoe, true);
			} else {
				throw runtime_error("Not a valid value for units-effected: " + tmp + ": " + dir);
			}
		} else {
			//default value
			flags.set(etfAlly, true);
			flags.set(etfFoe, true);
		}
	} catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}

	//chance (default 100%)
	try {
		attr = effectNode->getAttribute("chance", false);
		if(attr)
			chance = attr->getFloatValue();
		else
			chance = 100.0f;
	} catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}

	//duration
	try { duration = effectNode->getAttribute("duration")->getIntValue(); }
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}

	//damageType (default NULL)
	try {
		attr = effectNode->getAttribute("damage-type", false);
		if(attr) {
			damageType = tt->getAttackType(attr->getRestrictedValue());
		}
	}
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}

	//display (default true)
	try {
		attr = effectNode->getAttribute("display", false);
		if(attr) {
			display = attr->getBoolValue();
		} else {
			display = true;
		}
	} catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}

	//image (default NULL)
	try {
		attr = effectNode->getAttribute("image", false);
		if(attr && !(attr->getRestrictedValue() == "")) {
			image = Renderer::getInstance().newTexture2D(rsGame);
			image->load(dir + "/" + attr->getRestrictedValue());
		}
	} catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}

	//flags
	const XmlNode *flagsNode = effectNode->getChild("flags", 0, false);
	if(flagsNode) {
		flags.load(flagsNode, dir, tt, ft);
	}

	EnhancementTypeBase::load(effectNode, dir, tt, ft);

	//light & lightColor
	try {
		const XmlNode *lightNode = effectNode->getChild("light", 0, false);
		if(lightNode) {
			light = lightNode->getAttribute("enabled")->getBoolValue();

			if(light) {
				lightColor.x = lightNode->getAttribute("red")->getFloatValue(0.f, 1.f);
				lightColor.y = lightNode->getAttribute("green")->getFloatValue(0.f, 1.f);
				lightColor.z = lightNode->getAttribute("blue")->getFloatValue(0.f, 1.f);
			}
		} else {
			light = false;
		}
	}
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}
	//particle
	try {
		const XmlNode *particleNode = effectNode->getChild("particle", 0, false);
		if(particleNode && particleNode->getAttribute("value")->getBoolValue()) {
			string path = particleNode->getAttribute("path")->getRestrictedValue();
			//	   	particleSystemType = new ParticleSystemType();
			//	   	particleSystemType->load(effectNode,  dir + "/" + path);
		}
	} catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}

	//sound
	try { 
		const XmlNode *soundNode = effectNode->getChild("sound", 0, false);
		if(soundNode && soundNode->getAttribute("enabled")->getBoolValue()) {
			soundStartTime = soundNode->getAttribute("start-time")->getFloatValue();
			loopSound = soundNode->getAttribute("loop")->getBoolValue();
			string path = soundNode->getAttribute("path")->getRestrictedValue();
			sound = new StaticSound();
			sound->load(dir + "/" + path);
		}
	} catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}

	//recourse
	try {
		const XmlNode *recourseEffectsNode = effectNode->getChild("recourse-effects", 0, false);
		if(recourseEffectsNode) {
			recourse.resize(recourseEffectsNode->getChildCount());
			for(int i = 0; i < recourseEffectsNode->getChildCount(); ++i) {
				const XmlNode *recourseEffectNode = recourseEffectsNode->getChild("effect", i);
				EffectType *effectType = new EffectType();
				effectType->load(recourseEffectNode, dir, tt, ft);
				recourse[i] = effectType;
			}
		}
	} catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}
	if(hpRegeneration >= 0 && damageType) {
		damageType = NULL;
	}
	return loadOk;
}

void EffectType::getDesc(string &str) const {
	if(!display) {
		return;
	}

	str += getName();

	// effected units
	if(isEffectsPetsOnly() || !isEffectsFoe() || !isEffectsAlly()) {
		str += "\n\tEffects: ";
		if(isEffectsPetsOnly()) {
			str += "pets only";
		} else if(isEffectsAlly()) {
			str += "ally only";
		} else {
			assert(isEffectsFoe());
			str += "foe only";
		}
	}

	if(chance != 100.0f) {
		str += "\n\tChance: " + intToStr((int)chance) + "%\n\t";
	}

	str += "\n\tDuration: ";
	if(isPermanent()) {
		str += "permenant";
	} else {
		str += intToStr(duration);
	}

	if(damageType) {
		str += "\n\tDamage Type: " + damageType->getName();
	}

	EnhancementTypeBase::getDesc(str, "\n\t");
	str += "\n";
}


// =====================================================
//  class Emanation
// =====================================================

bool Emanation::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	EffectType::load(n, dir, tt, ft);

	//radius
   try { radius = n->getAttribute("radius")->getIntValue(); }
   catch ( runtime_error e ) {
      Logger::getErrorLog().add ( dir, e.what () );
      return false;
   }
   return true;
}

}}//end namespace
