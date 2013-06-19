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
//#include "network_message.h"
#include "sim_interface.h"
#include "character_creator.h"
#include "main_menu.h"
#include "menu_state_character_creator.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class SoundsAndAnimations
// =====================================================
SoundsAndAnimations::SoundsAndAnimations()
		: animationsStyle(AnimationsStyle::SINGLE)
		, soundStartTime(0.f)
		, animSpeed(0) {
}

SoundsAndAnimations::~SoundsAndAnimations(){
	deleteValues(sounds.getSounds());
}

void SoundsAndAnimations::addAnimation(string load, int s, int h) {
	ModelFactory &modelFactory = g_world.getModelFactory();
    animations.push_back(modelFactory.getModel(cleanPath(load), s, h));
}

bool SoundsAndAnimations::load(const XmlNode *sn, const string &dir, const CreatableType *ct){
    bool loadOk = true;
	animSpeed = sn->getChildIntValue("anim-speed");
	ModelFactory &modelFactory = g_world.getModelFactory();
	const XmlNode *animNode = sn->getChild("animation");
	const XmlAttribute *animPathAttrib = animNode->getAttribute("path", false);
	if (animPathAttrib) { // single animation, lagacy style
		string path = dir + "/" + animPathAttrib->getRestrictedValue();
        const XmlAttribute *animLoadAttrib = animNode->getAttribute("load", false);
        bool loadBool = false;
        if (animLoadAttrib) {
            loadBool = animLoadAttrib->getBoolValue();
        }
        if (loadBool) {
            loadValue = "/" + animPathAttrib->getRestrictedValue();
        } else {
            if (g_program.getState()->isGameState()) {
                animations.push_back(modelFactory.getModel(cleanPath(path), ct->getSize(), ct->getHeight()));
            }
        }
		animationsStyle = AnimationsStyle::SINGLE;
	} else { // multi-anim or anim-by-surface-type, new style
		for (int i=0; i < animNode->getChildCount(); ++i) {
			XmlNodePtr node = animNode->getChild(i);
			string path = node->getAttribute("path")->getRestrictedValue();
			path = dir + "/" + path;
			Model *model = modelFactory.getModel(cleanPath(path), ct->getSize(), ct->getHeight());
			if (node->getName() == "anim-file") {
				animations.push_back(model);
			} else if (node->getName() == "surface") {
				string type = node->getAttribute("type")->getRestrictedValue();
				SurfaceType st = SurfaceTypeNames.match(type);
				if (st != SurfaceType::INVALID) {
					m_animBySurfaceMap[st] = model;
				} else {
					throw runtime_error("SurfaceType '" + type + "' invalid.");
				}
			} else if (node->getName() == "surface-change") {
				string fromType = node->getAttribute("from")->getRestrictedValue();
				string toType = node->getAttribute("to")->getRestrictedValue();
				SurfaceType from = SurfaceTypeNames.match(fromType);
				SurfaceType to = SurfaceTypeNames.match(toType);
				if (from != SurfaceType::INVALID && to != SurfaceType::INVALID) {
					m_animBySurfPairMap[make_pair(from, to)] = model;
				} else {
					string msg;
					if (from == SurfaceType::INVALID && to == SurfaceType::INVALID) {
						msg = "SurfaceType '" + fromType + "' and '" + toType + "' both invalid.";
					} else if (from != SurfaceType::INVALID) {
						msg = "SurfaceType '" + fromType + "' invalid.";
					} else {
						msg = "SurfaceType '" + toType + "' invalid.";
					}
					throw runtime_error(msg);
				}
			} else {
				g_logger.logXmlError(path,
					("Warning: In 'animation' node, child node '" + node->getName() + "' unknown.").c_str());
			}
		}
		if (animations.size() > 1) {
			animationsStyle = AnimationsStyle::RANDOM;
		}
		if (!m_animBySurfaceMap.empty() || !m_animBySurfPairMap.empty()) {
			if (m_animBySurfaceMap.size() != SurfaceType::COUNT) {
			}
		}
	}
	XmlNodePtr soundNode= sn->getChild("sound", 0, false);
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
	return loadOk;
}

void SoundsAndAnimations::save(XmlNode *node) const {

}

const Model* SoundsAndAnimations::getAnimation(SurfaceType from, SurfaceType to) const {
	AnimationBySurfacePair::const_iterator it = m_animBySurfPairMap.find(make_pair(from, to));
	if (it != m_animBySurfPairMap.end()) {
		return it->second;
	}
	return getAnimation(to);
}

const Model* SoundsAndAnimations::getAnimation(SurfaceType st) const {
	AnimationBySurface::const_iterator it = m_animBySurfaceMap.find(st);
	if (it != m_animBySurfaceMap.end()) {
		return it->second;
	}
	return getAnimation();
}

// =====================================================
// 	class ItemCost && StatCost
// =====================================================
void ItemCost::init(int amount, const ItemType *type) {
    this->type = type;
    this->amount = amount;
}

void ItemCost::save(XmlNode *node) const {
    node->addAttribute("type", type->getName());
    node->addAttribute("amount", amount);
}

void StatCost::init(int amount, string name) {
    this->name = name;
    this->amount = amount;
}

void StatCost::save(XmlNode *node) const {
    node->addAttribute("name", name);
    node->addAttribute("amount", amount);
}

// =====================================================
// 	class SkillCosts
// =====================================================
void SkillCosts::init() {
    hpCost = 0;
}

bool SkillCosts::load(const XmlNode *sn, const string &dir, const FactionType *ft) {
    bool loadOk = true;
    hpCost = 0;
    levelReq = 0;
    const XmlNode *hpCostNode = sn->getChild("hp-cost", 0, false);
    if (hpCostNode) {
        hpCost = hpCostNode->getAttribute("amount")->getIntValue();
    }
    const XmlNode *levelReqNode = sn->getChild("level-req", 0, false);
    if (levelReqNode) {
        levelReq = levelReqNode->getAttribute("level")->getIntValue();
    }
    const XmlNode *resourcesNode = sn->getChild("res-pools", 0, false);
    if (resourcesNode) {
        resources.resize(resourcesNode->getChildCount());
        for (int i = 0; i < resourcesNode->getChildCount(); ++i) {
            const XmlNode *costNode = resourcesNode->getChild("cost", i);
            string name = costNode->getAttribute("name")->getRestrictedValue();
            int amount = costNode->getAttribute("amount")->getIntValue();
            resources[i].init(amount, name);
        }
    }
    const XmlNode *defensesNode = sn->getChild("def-pools", 0, false);
    if (defensesNode) {
        defenses.resize(defensesNode->getChildCount());
        for (int i = 0; i < defensesNode->getChildCount(); ++i) {
            const XmlNode *costNode = defensesNode->getChild("cost", i);
            string name = costNode->getAttribute("name")->getRestrictedValue();
            int amount = costNode->getAttribute("amount")->getIntValue();
            defenses[i].init(amount, name);
        }
    }
    const XmlNode *itemCostsNode = sn->getChild("item-costs", 0, false);
    if (itemCostsNode) {
        itemCosts.resize(itemCostsNode->getChildCount());
        for (int i = 0; i < itemCostsNode->getChildCount(); ++i) {
            const XmlNode *costNode = itemCostsNode->getChild("cost", i);
            string typeString = costNode->getAttribute("type")->getRestrictedValue();
            int amount = costNode->getAttribute("amount")->getIntValue();
            const ItemType *itemType = ft->getItemType(typeString);
            itemCosts[i].init(amount, itemType);
        }
    }
    const XmlNode *resourceCostsNode = sn->getChild("resource-costs", 0, false);
    if (resourceCostsNode) {
        resourceCosts.resize(resourceCostsNode->getChildCount());
        for (int i = 0; i < resourceCostsNode->getChildCount(); ++i) {
            const XmlNode *costNode = resourceCostsNode->getChild("cost", i);
            string typeString = costNode->getAttribute("type")->getRestrictedValue();
            int amount = costNode->getAttribute("amount")->getIntValue();
            const TechTree *tt = 0;
            if (&g_world) {
                tt = g_world.getTechTree();
            } else {
                if (g_program.getState()->isCCState()) {
                    MainMenu *charMenu = static_cast<MainMenu*>(g_program.getState());
                    MenuStateCharacterCreator *charState = static_cast<MenuStateCharacterCreator*>(charMenu->getState());
                    CharacterCreator *charCreator = charState->getCharacterCreator();
                    tt = charCreator->getTechTree();
                }
            }
            const ResourceType *resourceType = tt->getResourceType(typeString);
            resourceCosts[i].init(resourceType, amount, 0, 0);
        }
    }
    return loadOk;
}

void SkillCosts::getDesc(CmdDescriptor *callback) const {
    if (getHpCost() > 0) {
        string name = g_lang.getTranslatedTechName("Health");
        string msg = name + " (" + intToStr(getHpCost()) + ")";
        callback->addElement(msg);
    }
    if (getResourceCount() > 0) {
        callback->addElement(g_lang.get("Costs") + ":");
        for (int i = 0; i < getResourceCount(); ++i) {
            const StatCost *iCost = getResource(i);
            string name = g_lang.getTranslatedTechName(iCost->getName());
            string msg = name + " (" + intToStr(iCost->getAmount()) + ")";
            callback->addElement(msg);
        }
    }
    if (getResourceCostCount() > 0) {
        callback->addElement(g_lang.get("Costs") + ":");
        for (int i = 0; i < getResourceCostCount(); ++i) {
            const ResourceAmount *res = getResourceCost(i);
            string name = g_lang.getTranslatedTechName(res->getType()->getName());
            string msg = "Local| " + name + " (" + intToStr(res->getAmount()) + ")";
            callback->addReq(res->getType(), true, msg);
        }
    }
    if (getItemCostCount() > 0) {
        callback->addElement(g_lang.get("Costs") + ":");
        for (int i = 0; i < getItemCostCount(); ++i) {
            const ItemCost *iCost = getItemCost(i);
            string name = g_lang.getTranslatedTechName(iCost->getType()->getName());
            string msg = "Local| " + name + " (" + intToStr(iCost->getAmount()) + ")";
            callback->addReq(iCost->getType(), true, msg);
        }
    }
}

void SkillCosts::save(XmlNode *node) const {
    XmlNode *n;
    n = node->addChild("hp-cost");
    n->addAttribute("value", hpCost);
    n = node->addChild("level-req");
    n->addAttribute("level", levelReq);
    n = node->addChild("res-pools");
    for (int i = 0; i < resources.size(); ++i) {
        resources[i].save(n->addChild("cost"));
    }
    n = node->addChild("def-pools");
    for (int i = 0; i < defenses.size(); ++i) {
        defenses[i].save(n->addChild("cost"));
    }
    n = node->addChild("item-costs");
    for (int i = 0; i < itemCosts.size(); ++i) {
        itemCosts[i].save(n->addChild("cost"));
    }
    n = node->addChild("resource-costs");
    for (int i = 0; i < resourceCosts.size(); ++i) {
        resourceCosts[i].save(n->addChild("cost"));
    }
}

// =====================================================
// 	class SkillType
// =====================================================
SkillType::SkillType(const char* typeName)
		: NameIdPair(-1, "default")
		, m_deCloak(false)
		, startTime(0.f)
		, minRange(0)
		, maxRange(0)
		, typeName(typeName)
		, m_creatableType(0) {
}

SkillType::~SkillType(){
	deleteValues(eyeCandySystems);
}

bool SkillType::load(const XmlNode *sn, const string &dir, const FactionType *ft, const CreatableType *ct){
    bool loadOk = true;
	m_creatableType = ct;
    if (ft == 0) {
        const XmlAttribute *factionAttribute = sn->getAttribute("faction");
        string faction = factionAttribute->getRestrictedValue();
        if (&g_world) {
            ft = g_world.getTechTree()->getFactionType(faction);
        } else {
            if (g_program.getState()->isCCState()) {
                MainMenu *charMenu = static_cast<MainMenu*>(g_program.getState());
                MenuStateCharacterCreator *charState = static_cast<MenuStateCharacterCreator*>(charMenu->getState());
                CharacterCreator *charCreator = charState->getCharacterCreator();
                const TechTree *tt = charCreator->getTechTree();
                ft = tt->getFactionType(faction);
            }
        }
    }
    const XmlNode *nameNode = sn->getChild("name");
	m_name = nameNode->getRestrictedValue();
	//m_name = sn->getChildStringValue("name");
	speed = sn->getChildIntValue("speed");

    const XmlNode *minRangeNode = sn->getChild("min-range", 0, false);
	if(minRangeNode) {
		minRange = minRangeNode->getAttribute("value")->getIntValue();
	}

    const XmlNode *maxRangeNode = sn->getChild("max-range", 0, false);
	if(maxRangeNode) {
		maxRange = maxRangeNode->getAttribute("value")->getIntValue();
	}

    const XmlNode *skillCostsNode = sn->getChild("skill-costs", 0, false);
    if (skillCostsNode) {
        if(!skillCosts.load(skillCostsNode, dir, ft)) {
           loadOk = false;
        }
    } else {
        skillCosts.init();
    }

    const XmlNode *soundsAndAnimationsNode = sn->getChild("sounds-animations", 0, false);
    if (soundsAndAnimationsNode) {
        if(!soundsAndAnimations.load(soundsAndAnimationsNode, dir, ct)) {
           loadOk = false;
        }
    }
	// particle systems
	const XmlNode *particlesNode = sn->getOptionalChild("particle-systems");
	if (particlesNode) {
		for (int i=0; i < particlesNode->getChildCount(); ++i) {
			const XmlNode *particleFileNode = particlesNode->getChild("particle", i);
			string path = particleFileNode->getAttribute("path")->getRestrictedValue();
			UnitParticleSystemType *unitParticleSystemType = new UnitParticleSystemType();
			unitParticleSystemType->load(dir,  dir + "/" + path);
			eyeCandySystems.push_back(unitParticleSystemType);
		}
	} else { // megaglest style?
		particlesNode = sn->getOptionalChild("particles");
		if (particlesNode) {
			bool particleEnabled = particlesNode->getAttribute("value")->getBoolValue();
			if (particleEnabled) {
				for (int i=0; i < particlesNode->getChildCount(); ++i) {
					const XmlNode *particleFileNode = particlesNode->getChild("particle-file", i);
					string path= particleFileNode->getAttribute("path")->getRestrictedValue();
					UnitParticleSystemType *unitParticleSystemType = new UnitParticleSystemType();
					unitParticleSystemType->load(dir,  dir + "/" + path);
					eyeCandySystems.push_back(unitParticleSystemType);
				}
			}
		}
	}
	startTime = sn->getOptionalFloatValue("start-time");
	return loadOk;
}

void SkillType::save(XmlNode *node) const {
}

void SkillType::doChecksum(Checksum &checksum) const {
	checksum.add<SkillClass>(getClass());
	checksum.add(m_name);
	checksum.add(speed);
	checksum.add(startTime);
}

void SkillType::descSpeed(string &str, const Unit *unit, const char* speedType) const {
	str += g_lang.get(speedType) + ": " + intToStr(speed);
	int uSpeed = 0;
	if (unit) {
        uSpeed = unit->getSpeed(this);
	}
	EnhancementType::describeModifier(str, uSpeed - speed);
	str += "\n";
}

CycleInfo SkillType::calculateCycleTime() const {
	const SkillClass skillClass = getClass();
	static const float speedModifier = 1.f / GameConstants::speedDivider / float(WORLD_FPS);
	if (skillClass == SkillClass::MOVE) {
		return CycleInfo(-1, -1);
	}
	float progressSpeed = getBaseSpeed() * speedModifier;

	int skillFrames = int(1.0000001f / progressSpeed);
	int animFrames = 1;
	int soundOffset = -1;
	int systemOffset = -1;

	if (getSoundsAndAnimations()->getAnimSpeed() != 0) {
		float animProgressSpeed = getSoundsAndAnimations()->getAnimSpeed() * speedModifier;
		animFrames = int(1.0000001f / animProgressSpeed);

		if (skillClass == SkillClass::ATTACK || skillClass == SkillClass::CAST_SPELL) {
			systemOffset = clamp(int(startTime / animProgressSpeed), 1, animFrames - 1);
			assert(systemOffset > 0 && systemOffset < 256);
		}
		if (!getSoundsAndAnimations()->getSounds().empty()) {
			soundOffset = clamp(int(getSoundsAndAnimations()->getSoundStartTime() / animProgressSpeed), 1, animFrames - 1);
			assert(soundOffset > 0);
		}
	}
	if (skillFrames < 1) {
		stringstream ss;
		string name;
		if (m_creatableType != 0) {
            name = m_creatableType->getName();
		}
		ss << "Error: CreatableType '" << name << "', SkillType '" << getName()
			<< ", skill speed is too fast, cycle calculation clamped to one frame.";
		g_logger.logError(ss.str());
		skillFrames = 1;
	}
	assert(animFrames > 0);
	return CycleInfo(skillFrames, animFrames, soundOffset, systemOffset);
}

// =====================================================
// 	class MoveSkillType
// =====================================================

bool MoveSkillType::load(const XmlNode *sn, const string &dir, const FactionType *ft, const CreatableType *ct){
    bool loadOk = true;
    if (ft == 0) {
        const XmlAttribute *factionAttribute = sn->getAttribute("faction");
        string faction = factionAttribute->getRestrictedValue();
        if (&g_world) {
            ft = g_world.getTechTree()->getFactionType(faction);
        } else {
            if (g_program.getState()->isCCState()) {
                MainMenu *charMenu = static_cast<MainMenu*>(g_program.getState());
                MenuStateCharacterCreator *charState = static_cast<MenuStateCharacterCreator*>(charMenu->getState());
                CharacterCreator *charCreator = charState->getCharacterCreator();
                const TechTree *tt = charCreator->getTechTree();
                ft = tt->getFactionType(faction);
            }
        }
    }
	loadOk = SkillType::load(sn, dir, ft, ct);

	XmlNode *visibleOnlyNode = sn->getOptionalChild("visible-only");
	if (visibleOnlyNode) {
		visibleOnly = visibleOnlyNode->getAttribute("value")->getBoolValue();
	}
	return loadOk;
}

fixed MoveSkillType::getSpeed(const Unit *unit) const {
    if (!unit) {
        return getBaseSpeed();
    }
	return getBaseSpeed() * unit->getStatistics()->getEnhancement()->getUnitStats()->getMoveSpeed()->getValueMult() +
	unit->getStatistics()->getEnhancement()->getUnitStats()->getMoveSpeed()->getValue();
}

// =====================================================
// 	class TargetBasedSkillType
// =====================================================

TargetBasedSkillType::TargetBasedSkillType(const char* typeName)
		: effectTypes()
		, projectile(false)
		, projectileParticleSystemType(0)
		, splash(false)
		, splashDamageAll(false)
		, splashRadius(0)
		, splashParticleSystemType(0)
		, SkillType(typeName) {}

TargetBasedSkillType::~TargetBasedSkillType(){
	deleteValues(projSounds.getSounds());
	delete projectileParticleSystemType;
	delete splashParticleSystemType;
}

bool TargetBasedSkillType::load(const XmlNode *sn, const string &dir, const FactionType *ft, const CreatableType *ct){
    bool loadOk = true;
    if (ft == 0) {
        const XmlAttribute *factionAttribute = sn->getAttribute("faction");
        string faction = factionAttribute->getRestrictedValue();
        if (&g_world) {
            ft = g_world.getTechTree()->getFactionType(faction);
        } else {
            if (g_program.getState()->isCCState()) {
                MainMenu *charMenu = static_cast<MainMenu*>(g_program.getState());
                MenuStateCharacterCreator *charState = static_cast<MenuStateCharacterCreator*>(charMenu->getState());
                CharacterCreator *charCreator = charState->getCharacterCreator();
                const TechTree *tt = charCreator->getTechTree();
                ft = tt->getFactionType(faction);
            }
        }
    }
	SkillType::load(sn, dir, ft, ct);

	//effects
	const XmlNode *effectsNode = sn->getChild("effects", 0, false);
	if (effectsNode) {
		effectTypes.resize(effectsNode->getChildCount());
		for(int i=0; i < effectsNode->getChildCount(); ++i) {
			const XmlNode *effectNode = effectsNode->getChild("effect", i);
			EffectType *effectType = new EffectType();
			effectType->load(effectNode, dir);
			effectTypes[i] = effectType;
		}
	}
	//projectile
	const XmlNode *projectileNode = sn->getChild("projectile", 0, false);
	if (projectileNode && projectileNode->getAttribute("value")->getBoolValue()) {

		//proj particle
		const XmlNode *particleNode= projectileNode->getChild("particle", 0, false);
		if(particleNode && particleNode->getAttribute("value")->getBoolValue()){
			string path= particleNode->getAttribute("path")->getRestrictedValue();
			projectileParticleSystemType = new ProjectileType();
			projectileParticleSystemType->load(dir,  dir + "/" + path);
			projectile = true;
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
			splashParticleSystemType= new SplashType();
			splashParticleSystemType->load(dir,  dir + "/" + path);
		}
	}

	//fields
	const XmlNode *attackFieldsNode = sn->getChild("attack-fields", 0, false);
	const XmlNode *fieldsNode = sn->getChild("fields", 0, false);
	if(attackFieldsNode && fieldsNode) {
		throw runtime_error("Cannot specify both <attack-fields> and <fields>.");
	}
	if(!attackFieldsNode && !fieldsNode) {
		throw runtime_error("Must specify either <attack-fields> or <fields>.");
	}
	zones.load(fieldsNode ? fieldsNode : attackFieldsNode, dir);
	return loadOk;
}

void TargetBasedSkillType::doChecksum(Checksum &checksum) const {
	SkillType::doChecksum(checksum);
	checksum.add(projectile);
	checksum.add(splash);
	checksum.add(splashDamageAll);
	checksum.add(splashRadius);
	foreach_enum (Zone, z) {
		checksum.add<bool>(zones.get(z));
	}
}

void TargetBasedSkillType::getDesc(string &str, const Unit *unit, const char* rangeDesc) const {
	Lang &lang= Lang::getInstance();
	//splash radius
	if (splashRadius) {
		str += lang.get("SplashRadius") + ": " + intToStr(splashRadius) + "\n";
	}
	descRange(str, unit, rangeDesc);
	//fields
	str+= lang.get("Zones") + ": ";
	foreach_enum (Zone, z) {
		if (zones.get(z)) {
			str += string(lang.get(ZoneNames[z])) + " ";
		}
	}
}

void TargetBasedSkillType::descEffects(string &str, const Unit *unit) const {
	for(EffectTypes::const_iterator i = effectTypes.begin(); i != effectTypes.end(); ++i) {
		str += g_lang.get("Effect") + ": ";
		(*i)->getDesc(str);
	}
}

void TargetBasedSkillType::descRange(string &str, const Unit *unit, const char* rangeDesc) const {
	str+= Lang::getInstance().get(rangeDesc)+": ";
	if(getMinRange() > 1) {
		str += 	intToStr(getMinRange()) + "...";
	}
	str += intToStr(getMaxRange());
	int uRange = 0;
	if (unit) {
        uRange = unit->getMaxRange(this);
	}
	EnhancementType::describeModifier(str, uRange - getMaxRange());
	str+="\n";
}

// =====================================================
// 	class AttackSkillType
// =====================================================
bool AttackLevel::load(const XmlNode *attackLevelNode, const string &dir) {
    bool loadOk = true;
    cooldown = 0;
	if (attackLevelNode->getOptionalChild("cooldown")) {
        cooldown = attackLevelNode->getOptionalChild("cooldown")->getAttribute("time")->getIntValue();
	}

	const XmlNode *statisticsNode = attackLevelNode->getChild("statistics", 0, false);
	if (statisticsNode) {
        if (!statistics.load(statisticsNode, dir)) {
            loadOk = false;
        }
	}
	return loadOk;
}

AttackSkillType::~AttackSkillType() {
//	delete earthquakeType;
}

bool AttackSkillType::load(const XmlNode *sn, const string &dir, const FactionType *ft, const CreatableType *ct){
    bool loadOk = true;
	loadOk = TargetBasedSkillType::load(sn, dir, ft, ct);
    const XmlNode *attackLevelsNode = sn->getChild("attack-levels");
    levels.resize(attackLevelsNode->getChildCount());
    for (int i = 0; i < attackLevelsNode->getChildCount(); ++i) {
        const XmlNode *attackLevelNode = attackLevelsNode->getChild("attack-level", i);
       levels[i].load(attackLevelNode, dir);
    }

	//misc
#ifdef EARTHQUAKE_CODE
	earthquakeType = NULL;
	XmlNode *earthquakeNode = sn->getChild("earthquake", 0, false);
	if(earthquakeNode) {
		earthquakeType = new EarthquakeType(float(0));
		earthquakeType->load(earthquakeNode, dir, ft);
	}
#endif
    return loadOk;
}

void AttackSkillType::doChecksum(Checksum &checksum) const {
	TargetBasedSkillType::doChecksum(checksum);
	// earthquakeType ??
}

void AttackSkillType::getDesc(string &str, const Unit *unit) const {
	Lang &lang= Lang::getInstance();
	const AttackLevel *aLevel = &levels[skillLevel];
    descSpeed(str, unit, "AttackSpeed");
    str += "Cooldown: ";
    str += intToStr(aLevel->getCooldown());
    str += "\n";
    if (aLevel->getStatistics()->getDamageTypeCount() > 0) {
        str += lang.get("Damage")+": ";
        str += "\n";
        for (int i = 0; i < aLevel->getStatistics()->getDamageTypeCount(); ++i) {
            int uDamage = 0;
            if (unit) {
                uDamage = unit->getStatistics()->getDamageType(i)->getValue();
            }
            if (aLevel->getStatistics()->getDamageType(i)->getValue() + uDamage > 0) {
                str += lang.get(aLevel->getStatistics()->getDamageType(i)->getTypeName())+": ";
                str += intToStr(aLevel->getStatistics()->getDamageType(i)->getValue() + uDamage);
            }
        }
        str += "\n";
    } else {
        throw runtime_error(intToStr(aLevel->getStatistics()->getDamageTypeCount()));
    }
    TargetBasedSkillType::getDesc(str, unit, "AttackDistance");
    descEffects(str, unit);
}

fixed AttackSkillType::getSpeed(const Unit *unit) const {
	return getBaseSpeed() * unit->getStatistics()->getEnhancement()->getAttackStats()->getAttackSpeed()->getValueMult() +
	unit->getStatistics()->getEnhancement()->getAttackStats()->getAttackSpeed()->getValue();
}

// ===============================
// 	class BuildSkillType
// ===============================

fixed BuildSkillType::getSpeed(const Unit *unit) const {
	return getBaseSpeed() * unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getRepairSpeed()->getValueMult() +
	unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getRepairSpeed()->getValue();
}

// ===============================
// 	class ConstructSkillType
// ===============================

fixed ConstructSkillType::getSpeed(const Unit *unit) const {
	return getBaseSpeed() * unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getRepairSpeed()->getValueMult() +
	unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getRepairSpeed()->getValue();
}

// ===============================
// 	class HarvestSkillType
// ===============================

fixed HarvestSkillType::getSpeed(const Unit *unit) const {
	return getBaseSpeed() * unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getHarvestSpeed()->getValueMult() +
	unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getHarvestSpeed()->getValue();
}

// ===============================
// 	class TransportSkillType
// ===============================

// ===============================
// 	class SetStructureSkillType
// ===============================

// ===============================
// 	class DieSkillType
// ===============================

void DieSkillType::doChecksum(Checksum &checksum) const {
	SkillType::doChecksum(checksum);
	checksum.add<bool>(fade);
}

bool DieSkillType::load(const XmlNode *sn, const string &dir, const FactionType *ft, const CreatableType *ct) {
    bool loadOk = true;
	loadOk = SkillType::load(sn, dir, ft, ct);

	fade= sn->getChild("fade")->getAttribute("value")->getBoolValue();
	return loadOk;
}

// ===============================
// 	class RepairSkillType
// ===============================

RepairSkillType::RepairSkillType() : SkillType("Repair") {
	amount = 0;
	multiplier = 1;
	petOnly = false;
	selfAllowed = true;
	selfOnly = false;
}

bool RepairSkillType::load(const XmlNode *sn, const string &dir, const FactionType *ft, const CreatableType *ct){
    bool loadOk = true;
	loadOk = SkillType::load(sn, dir, ft, ct);

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
	return loadOk;
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
	descSpeed(str, unit, "RepairSpeed");

	if(amount) {
		str+= "\n" + lang.get("HpRestored")+": "+ intToStr(amount);
	}
	if(getMaxRange() > 1){
		str+= "\n" + lang.get("Range")+": "+ intToStr(getMaxRange());
	}
}

fixed RepairSkillType::getSpeed(const Unit *unit) const {
	return getBaseSpeed() * unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getRepairSpeed()->getValueMult() +
	unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getRepairSpeed()->getValue();
}

// ===============================
// 	class MaintainSkillType
// ===============================

MaintainSkillType::MaintainSkillType() : SkillType("Maintain") {
	amount = 0;
	multiplier = 1;
	petOnly = false;
	selfAllowed = true;
	selfOnly = false;
}

bool MaintainSkillType::load(const XmlNode *sn, const string &dir, const FactionType *ft, const CreatableType *ct){
    bool loadOk = true;
	loadOk = SkillType::load(sn, dir, ft, ct);

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
		throw runtime_error("Maintain skill can't specify pet-only with self-only.");
	}
	return loadOk;
}

void MaintainSkillType::doChecksum(Checksum &checksum) const {
	SkillType::doChecksum(checksum);
	checksum.add(amount);
	checksum.add(multiplier);
	checksum.add(petOnly);
	checksum.add(selfOnly);
	checksum.add(selfAllowed);
}

void MaintainSkillType::getDesc(string &str, const Unit *unit) const {
	Lang &lang= Lang::getInstance();
	descSpeed(str, unit, "MaintainSpeed");

	if(amount) {
		str+= "\n" + lang.get("HpRestored")+": "+ intToStr(amount);
	}
	if(getMaxRange() > 1){
		str+= "\n" + lang.get("Range")+": "+ intToStr(getMaxRange());
	}
}

fixed MaintainSkillType::getSpeed(const Unit *unit) const {
	return getBaseSpeed() * unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getRepairSpeed()->getValueMult() +
	unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getRepairSpeed()->getValue();
}

// =====================================================
// 	class ProduceSkillType
// =====================================================

ProduceSkillType::ProduceSkillType() : SkillType("Produce") {
	pet = false;
	maxPets = 0;
}

bool ProduceSkillType::load(const XmlNode *sn, const string &dir, const FactionType *ft, const CreatableType *ct){
    bool loadOk = true;
	loadOk = SkillType::load(sn, dir, ft, ct);

	XmlNode *petNode = sn->getChild("pet", 0, false);
	if(petNode) {
		pet = petNode->getAttribute("value")->getBoolValue();
		maxPets = petNode->getAttribute("max")->getIntValue();
	}
	return loadOk;
}

void ProduceSkillType::doChecksum(Checksum &checksum) const {
	SkillType::doChecksum(checksum);
	checksum.add<bool>(pet);
	checksum.add<int>(maxPets);
}

fixed ProduceSkillType::getSpeed(const Unit *unit) const {
	return getBaseSpeed() * unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getProdSpeed()->getValueMult() +
	unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getProdSpeed()->getValue();
}

// =====================================================
// 	class UpgradeSkillType
// =====================================================

fixed UpgradeSkillType::getSpeed(const Unit *unit) const {
	return getBaseSpeed() * unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getProdSpeed()->getValueMult() +
	unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getProdSpeed()->getValue();
}

// =====================================================
// 	class MorphSkillType
// =====================================================

fixed MorphSkillType::getSpeed(const Unit *unit) const {
	return getBaseSpeed() * unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getProdSpeed()->getValueMult() +
	unit->getStatistics()->getEnhancement()->getProductionSpeeds()->getProdSpeed()->getValue();
}

// =====================================================
// 	class LoadSkillType
// =====================================================

LoadSkillType::LoadSkillType() : SkillType("Load") {
}

bool LoadSkillType::load(const XmlNode *sn, const string &dir, const FactionType *ft, const CreatableType *ct){
    bool loadOk = true;
	loadOk = SkillType::load(sn, dir, ft, ct);
	return loadOk;
}

void LoadSkillType::doChecksum(Checksum &checksum) const {
	SkillType::doChecksum(checksum);
}

// =====================================================
// 	class BeBuiltSkillType
// =====================================================

bool BeBuiltSkillType::load(const XmlNode *sn, const string &dir, const FactionType *ft, const CreatableType *ct){
    bool loadOk = true;
	loadOk = SkillType::load(sn, dir, ft, ct);
	m_stretchy = sn->getOptionalBoolValue("anim-stretch", false);
	return loadOk;
}

// =====================================================
// 	class BuildSelfSkillType
// =====================================================

bool BuildSelfSkillType::load(const XmlNode *sn, const string &dir, const FactionType *ft, const CreatableType *ct){
    bool loadOk = true;
	loadOk = SkillType::load(sn, dir, ft, ct);
	m_stretchy = sn->getOptionalBoolValue("anim-stretch", false);
	return loadOk;
}

// =====================================================
// 	class ModelFactory
// =====================================================

ModelFactory::ModelFactory(){}

Model* ModelFactory::newInstance(const string &path, int size, int height) {
	assert(models.find(path) == models.end());
	Model *model = g_renderer.newModel(ResourceScope::GAME);
	model->load(path, size, height);
	while (mediaErrorLog.hasError()) {
		MediaErrorLog::ErrorRecord record = mediaErrorLog.popError();
		g_logger.logMediaError("", record.path, record.msg.c_str());
	}
	models[path] = model;
	return model;
}

Model* ModelFactory::getModel(const string &path, int size, int height) {
	string cleanedPath = cleanPath(path);
	ModelMap::iterator it = models.find(cleanedPath);
	if (it != models.end()) {
		return it->second;
	}
	return newInstance(cleanedPath, size, height);
}

// =====================================================
// 	class AttackSkillTypes & enum AttackSkillPreferenceFlags
// =====================================================
void AttackSkillTypes::doChecksum(Checksum &checksum) const {
    for (int i=0; i < types.size(); ++i) {
        if (types[i] != 0) {
            checksum.add(types[i]->getName());
        } else {
            checksum.add("default");
        }
    }
}

void AttackSkillTypes::init() {
	maxRange = 0;

	assert(types.size() == associatedPrefs.size());
	if (types[0] != 0) {
        for(int i = 0; i < types.size(); ++i) {
            if(types[i]->getMaxRange() > maxRange) {
                maxRange = types[i]->getMaxRange();
            }
            zones.flags |= types[i]->getZones().flags;
            allPrefs.flags |= associatedPrefs[i].flags;
        }
    }
}

void AttackSkillTypes::getDesc(string &str, const Unit *unit) const {
	if(types.size() == 1) {
		types[0]->getDesc(str, unit);
	} else {
		str += Lang::getInstance().get("Attacks") + ": ";
		for(int i = 0; i < types.size(); ++i) {
			if (i) {
				str += ", ";
			}
			str += types[i]->getName();
		}
	}
}

const AttackSkillType *AttackSkillTypes::getPreferredAttack(
		const Unit *unit, const Unit *target, int rangeToTarget) const {
	const AttackSkillType *ast = NULL;

	if (types.size() == 1) {
		ast = types[0];
		return unit->getMaxRange(ast) >= rangeToTarget ? ast : NULL;
	}

	// a skill for use when damaged gets 1st priority.
	if (hasPreference(AttackSkillPreference::WHEN_DAMAGED) && unit->isDamaged()) {
		return getSkillForPref(AttackSkillPreference::WHEN_DAMAGED, rangeToTarget);
	}

	//If a skill in this collection is specified as use whenever possible and
	//the target resides in a field that skill can attack, we will only use that
	//skill if in range and return NULL otherwise.
	if (hasPreference(AttackSkillPreference::WHENEVER_POSSIBLE)) {
		ast = getSkillForPref(AttackSkillPreference::WHENEVER_POSSIBLE, 0);
		assert(ast);
		if (ast->getZone(target->getCurrZone())) {
			return unit->getMaxRange(ast) >= rangeToTarget ? ast : NULL;
		}
		ast = NULL;
	}

	//if(hasPreference(AttackSkillPreference::ON_BUILDING) && unit->getType()->isOfClass(UnitClass::BUILDING)) {
	//	ast = getSkillForPref(AttackSkillPreference::ON_BUILDING, rangeToTarget);
	//}

	//if(!ast && hasPreference(AttackSkillPreference::ON_LARGE) && unit->getType()->getSize() > 1) {
	//	ast = getSkillForPref(AttackSkillPreference::ON_LARGE, rangeToTarget);
	//}

	//still haven't found an attack skill then use the 1st that's in range
	if(!ast) {
		for(int i = 0; i < types.size(); ++i) {
			if(unit->getMaxRange(types[i]) >= rangeToTarget && types[i]->getZone(target->getCurrZone())) {
				ast = types[i];
				break;
			}
		}
	}

	return ast;
}

}} //end namespace
