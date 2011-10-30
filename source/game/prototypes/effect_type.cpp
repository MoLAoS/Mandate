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

using Glest::Util::Logger;

namespace Glest { namespace ProtoTypes {
using namespace Graphics;
// =====================================================
// 	class EffectType
// =====================================================

EffectType::EffectType() : lightColour(0.0f) {
	bias = EffectBias::NEUTRAL;
	stacking = EffectStacking::STACK;

	duration = 0;
	chance = 100;
	light = false;
	sound = NULL;
	soundStartTime = 0.0f;
	loopSound = false;
	damageType = NULL;
	factionType = 0;
	display = true;
}

bool EffectType::load(const XmlNode *en, const string &dir, const TechTree *tt, const FactionType *ft) {
	factionType = ft;
	string tmp;
	const XmlAttribute *attr;
	bool loadOk = true;
	
	try { // name
		m_name = en->getAttribute("name")->getRestrictedValue(); 
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	// bigtime hack (REFACTOR: Move to EffectTypeFactory)
	m_id = const_cast<TechTree*>(tt)->addEffectType(this);

	try { // bias
		tmp = en->getAttribute("bias")->getRestrictedValue();
		bias = EffectBiasNames.match(tmp.c_str());
		if (bias == EffectBias::INVALID) {
			if (tmp == "benificial") { // support old typo/spelling error
				bias = EffectBias::BENEFICIAL;
			} else {
				throw runtime_error("Not a valid value for bias: " + tmp + ": " + dir);
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // stacking
		tmp = en->getAttribute("stacking")->getRestrictedValue();
		stacking = EffectStackingNames.match(tmp.c_str());
		if (stacking == EffectStacking::INVALID) {
			throw runtime_error("Not a valid value for stacking: " + tmp + ": " + dir);
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // target (default all)
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
				throw runtime_error("Not a valid value for effect target: '" + tmp + "' : " + dir);
			}
		} else { // default value
			flags.set(EffectTypeFlag::ALLY, true);
			flags.set(EffectTypeFlag::FOE, true);
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // chance (default 100%)
		attr = en->getAttribute("chance", false);
		if (attr) {
			chance = attr->getFixedValue();
		} else {
			chance = 100;
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // duration
		duration = en->getAttribute("duration")->getIntValue();
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // damageType (default NULL)
		attr = en->getAttribute("damage-type", false);
		if(attr) {
			damageType = tt->getAttackType(attr->getRestrictedValue());
		}
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // display (default true)
		attr = en->getAttribute("display", false);
		if(attr) {
			display = attr->getBoolValue();
		} else {
			display = true;
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // image (default NULL)
		attr = en->getAttribute("image", false);
		if(attr && !(attr->getRestrictedValue() == "")) {
			image = g_renderer.getTexture2D(ResourceScope::GAME, dir + "/" + attr->getRestrictedValue());
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	const XmlNode *affectNode = en->getChild("affect", 0, false);
	if (affectNode) {
		affectTag = affectNode->getRestrictedValue();
	} else {
		affectTag = "";
	}

	// flags
	const XmlNode *flagsNode = en->getChild("flags", 0, false);
	if (flagsNode) {
		flags.load(flagsNode, dir, tt, ft);
	}

	EnhancementType::load(en, dir, tt, ft);

	try { // light & lightColour
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
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // particle systems
		const XmlNode *particlesNode = en->getOptionalChild("particle-systems");
		if (particlesNode) {
			for (int i=0; i < particlesNode->getChildCount(); ++i) {
				const XmlNode *particleFileNode = particlesNode->getChild("particle-file", i);
				string path = particleFileNode->getAttribute("path")->getRestrictedValue();
				UnitParticleSystemType *unitParticleSystemType = new UnitParticleSystemType();
				unitParticleSystemType->load(dir,  dir + "/" + path);
				particleSystems.push_back(unitParticleSystemType);
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // sound
		const XmlNode *soundNode = en->getChild("sound", 0, false);
		if(soundNode && soundNode->getAttribute("enabled")->getBoolValue()) {
			soundStartTime = soundNode->getAttribute("start-time")->getFloatValue();
			loopSound = soundNode->getAttribute("loop")->getBoolValue();
			string path = soundNode->getAttribute("path")->getRestrictedValue();
			sound = new StaticSound();
			sound->load(dir + "/" + path);
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // recourse
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
		g_logger.logXmlError(dir, e.what ());
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
	if (damageType) {
		checksum.add(damageType->getName());
	}
	checksum.add(display);
}

void EffectType::getDesc(string &str) const {
	if (!display) {
		return;
	}

	str += g_lang.getFactionString(getFactionType()->getName(), getName());

	// effected units
	if (isEffectsPetsOnly() || isEffectsFoe() || isEffectsAlly()) {
		str += "\n   " + g_lang.get("Affects") + ": ";
		if (isEffectsPetsOnly()) {
			str += g_lang.get("PetsOnly");
		} else if (isEffectsAlly()) {
			str += g_lang.get("AllyOnly");
		} else {
			assert(isEffectsFoe());
			str += g_lang.get("FoeOnly");
		}
	}

	if (chance != 100) {
		str += "\n   " + g_lang.get("Chance") + ": " + intToStr(chance.intp()) + "%";
	}

	str += "\n   " + g_lang.get("Duration") + ": ";
	if (isPermanent()) {
		str += g_lang.get("Permenant");
	} else {
		str += intToStr(duration);
	}

	if (damageType) {
		str += "\n   " + g_lang.get("DamageType") + ": " + g_lang.getTechString(damageType->getName());
	}

	EnhancementType::getDesc(str, "\n   ");
	str += "\n";
}

// =====================================================
//  class EmanationType
// =====================================================

bool EmanationType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	EffectType::load(n, dir, tt, ft);

	//radius
	try { radius = n->getAttribute("radius")->getIntValue(); }
	catch (runtime_error e) {
		g_logger.logError ( dir, e.what () );
		return false;
	}
	return true;
}

void EmanationType::doChecksum(Checksum &checksum) const {
	EffectType::doChecksum(checksum);
	checksum.add(radius);
}

}}//end namespace
