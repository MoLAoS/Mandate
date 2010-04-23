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
// 	class EffectType
// =====================================================

EffectType::EffectType() : lightColour(0.0f) {
	bias = EffectBias::NEUTRAL;
	stacking = EffectStacking::STACK;

	duration = 0;
	chance = 100;
	light = false;
	particleSystemType = NULL;
	sound = NULL;
	soundStartTime = 0.0f;
	loopSound = false;
	damageType = NULL;
}

bool EffectType::load(const XmlNode *en, const string &dir, const TechTree *tt, const FactionType *ft) {
	string tmp;
	const XmlAttribute *attr;
	const XmlNode *node;

	bool loadOk = true;
	//name
	try { name = en->getAttribute("name")->getRestrictedValue(); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	//bigtime hack (REFACTOR: Move to EffectTypeFactory)
	id = const_cast<TechTree*>(tt)->addEffectType(this);

	//bias
	try {
		tmp = en->getAttribute("bias")->getRestrictedValue();
		bias = EffectBiasNames.match(tmp.c_str());
		if (bias == EffectBias::INVALID) {
			throw runtime_error("Not a valid value for bias: " + tmp + ": " + dir);
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//stacking
	try {
		tmp = en->getAttribute("stacking")->getRestrictedValue();
		stacking = EffectStackingNames.match(tmp.c_str());
		if (stacking == EffectStacking::INVALID) {
			throw runtime_error("Not a valid value for stacking: " + tmp + ": " + dir);
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//target (default all)
	try {
		attr = en->getAttribute("target", false);
		if(attr) {
			tmp = attr->getRestrictedValue();
			if (tmp == "ally") {
				flags.set(EffectTypeFlag::ALLY, true);
			} else if (tmp == "foe") {
				flags.set(EffectTypeFlag::FOE, true);
			} else if (tmp == "pet") {
				flags.set(EffectTypeFlag::ALLY, true);
				flags.set(EffectTypeFlag::PETS_ONLY, true);
			} else if (tmp == "all") {
				flags.set(EffectTypeFlag::ALLY, true);
				flags.set(EffectTypeFlag::FOE, true);
			} else {
				throw runtime_error("Not a valid value for units-effected: " + tmp + ": " + dir);
			}
		} else {
			//default value
			flags.set(EffectTypeFlag::ALLY, true);
			flags.set(EffectTypeFlag::FOE, true);
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//chance (default 100%)
	try {
		attr = en->getAttribute("chance", false);
		if (attr) {
			chance = attr->getFixedValue();
		} else {
			chance = 100;
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//duration
	try { duration = en->getAttribute("duration")->getIntValue(); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//damageType (default NULL)
	try {
		attr = en->getAttribute("damage-type", false);
		if(attr) {
			damageType = tt->getAttackType(attr->getRestrictedValue());
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//display (default true)
	try {
		attr = en->getAttribute("display", false);
		if(attr) {
			display = attr->getBoolValue();
		} else {
			display = true;
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//image (default NULL)
	try {
		attr = en->getAttribute("image", false);
		if(attr && !(attr->getRestrictedValue() == "")) {
			image = Renderer::getInstance().newTexture2D(rsGame);
			image->load(dir + "/" + attr->getRestrictedValue());
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//flags
	const XmlNode *flagsNode = en->getChild("flags", 0, false);
	if(flagsNode) {
		flags.load(flagsNode, dir, tt, ft);
	}

	EnhancementType::load(en, dir, tt, ft);

	//light & lightColour
	try {
		const XmlNode *lightNode = en->getChild("light", 0, false);
		if(lightNode) {
			light = lightNode->getAttribute("enabled")->getBoolValue();

			if(light) {
				lightColour.x = lightNode->getAttribute("red")->getFloatValue(0.f, 1.f);
				lightColour.y = lightNode->getAttribute("green")->getFloatValue(0.f, 1.f);
				lightColour.z = lightNode->getAttribute("blue")->getFloatValue(0.f, 1.f);
			}
		} else {
			light = false;
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	//particle
	try {
		const XmlNode *particleNode = en->getChild("particle", 0, false);
		if(particleNode && particleNode->getAttribute("value")->getBoolValue()) {
			string path = particleNode->getAttribute("path")->getRestrictedValue();
			//	   	particleSystemType = new ParticleSystemType();
			//	   	particleSystemType->load(en,  dir + "/" + path);
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//sound
	try { 
		const XmlNode *soundNode = en->getChild("sound", 0, false);
		if(soundNode && soundNode->getAttribute("enabled")->getBoolValue()) {
			soundStartTime = soundNode->getAttribute("start-time")->getFloatValue();
			loopSound = soundNode->getAttribute("loop")->getBoolValue();
			string path = soundNode->getAttribute("path")->getRestrictedValue();
			sound = new StaticSound();
			sound->load(dir + "/" + path);
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//recourse
	try {
		const XmlNode *recourseEffectsNode = en->getChild("recourse-effects", 0, false);
		if(recourseEffectsNode) {
			recourse.resize(recourseEffectsNode->getChildCount());
			for(int i = 0; i < recourseEffectsNode->getChildCount(); ++i) {
				const XmlNode *recourseEffectNode = recourseEffectsNode->getChild("effect", i);
				EffectType *effectType = new EffectType();
				effectType->load(recourseEffectNode, dir, tt, ft);
				recourse[i] = effectType;
			}
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	if(hpRegeneration >= 0 && damageType) {
		damageType = NULL;
	}
	return loadOk;
}

void EffectType::doChecksum(Checksum &checksum) const {
	NameIdPair::doChecksum(checksum);
	EnhancementType::doChecksum(checksum);

	checksum.add(bias);
	checksum.add(stacking);
	checksum.add(effectflags);

	checksum.add(duration);
	checksum.add(chance);
	checksum.add(light);
	checksum.add(lightColour);

	checksum.add(soundStartTime);
	checksum.add(loopSound);
	
	foreach_const (EffectTypes, it, recourse) {
		(*it)->doChecksum(checksum);
	}
	foreach_enum (EffectTypeFlag, f) {
		checksum.add(flags.get(f));
	}
	checksum.add(damageType->getName());
	checksum.add(display);
}

void EffectType::getDesc(string &str) const {
	if (!display) {
		return;
	}

	str += getName();

	// effected units
	if (isEffectsPetsOnly() || !isEffectsFoe() || !isEffectsAlly()) {
		str += "\n\tEffects: ";
		if (isEffectsPetsOnly()) {
			str += "pets only";
		} else if (isEffectsAlly()) {
			str += "ally only";
		} else {
			assert(isEffectsFoe());
			str += "foe only";
		}
	}

	if (chance != 100) {
		str += "\n\tChance: " + intToStr(chance.intp()) + "%\n\t";
	}

	str += "\n\tDuration: ";
	if (isPermanent()) {
		str += "permenant";
	} else {
		str += intToStr(duration);
	}

	if (damageType) {
		str += "\n\tDamage Type: " + damageType->getName();
	}

	EnhancementType::getDesc(str, "\n\t");
	str += "\n";
}

// This is a prototype, shoud be called EmanationType...
// =====================================================
//  class Emanation
// =====================================================

bool Emanation::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	EffectType::load(n, dir, tt, ft);

	//radius
	try { radius = n->getAttribute("radius")->getIntValue(); }
	catch (runtime_error e) {
		Logger::getErrorLog().add ( dir, e.what () );
		return false;
	}
	return true;
}

void Emanation::doChecksum(Checksum &checksum) const {
	EffectType::doChecksum(checksum);
	checksum.add(radius);
}

}}//end namespace
