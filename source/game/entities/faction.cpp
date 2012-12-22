// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "faction.h"

#include <algorithm>
#include <cassert>
#include <time.h>

#include "resource_type.h"
#include "unit.h"
#include "util.h"
#include "sound_renderer.h"
#include "renderer.h"
#include "tech_tree.h"
#include "world.h"

#include "program.h"
#include "sim_interface.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Entities {

using ProtoTypes::UnitType;

string factionColourStrings[GameConstants::maxColours] = {
	"red", "green", "blue", "yellow",
	"cyan", "orange", "pink", "white",
	"maroon", "forest", "navy", "mint",
	"teal", "brown", "purple", "sky"
};

// faction colours, in RGBA format
Colour factionColours[GameConstants::maxColours] = {

	Colour(0xFFu, 0x00u, 0x00u, 0xFFu), // red
	Colour(0x55u, 0xFFu, 0x25u, 0xFFu), // green
	Colour(0x3Fu, 0x3Fu, 0xFFu, 0xFFu), // blue
	Colour(0xFFu, 0xF5u, 0x35u, 0xFFu), // yellow

	Colour(0x5Cu, 0xFFu, 0xFFu, 0xFFu), // cyan
	Colour(0xF0u, 0x70u, 0x00u, 0xFFu), // orange
	Colour(0xFFu, 0x21u, 0xB5u, 0xFFu), // pink
	Colour(0xFFu, 0xFFu, 0xFFu, 0xFFu), // white

	Colour(0x77u, 0x00u, 0x00u, 0xFFu), // maroon
	Colour(0x00u, 0x67u, 0x00u, 0xFFu), // forest
	Colour(0x00u, 0x00u, 0x64u, 0xFFu), // navy
	Colour(0x75u, 0xFFu, 0x75u, 0xFFu), // mint

	Colour(0x00u, 0x77u, 0x77u, 0xFFu), // teal
	Colour(0x82u, 0x5Eu, 0x00u, 0xFFu), // brown
	Colour(0x7Bu, 0x00u, 0xA1u, 0xFFu), // purple
	Colour(0x93u, 0x93u, 0xFFu, 0xFFu)  // sky
};

Colour factionColoursOutline[GameConstants::maxColours] = {

	Colour(0xA8u, 0x00u, 0x00u, 0xFFu), // red
	Colour(0x36u, 0xA1u, 0x17u, 0xFFu), // green
	Colour(0x2Au, 0x2Au, 0xAAu, 0xFFu), // blue
	Colour(0x9Fu, 0x99u, 0x21u, 0xFFu), // yellow

	Colour(0x3Cu, 0xA7u, 0xA7u, 0xFFu), // cyan
	Colour(0xB7u, 0x5Bu, 0x00u, 0xFFu), // orange
	Colour(0x9Eu, 0x14u, 0x70u, 0xFFu), // pink
	Colour(0xAAu, 0xAAu, 0xAAu, 0xFFu), // white

	Colour(0x96u, 0x1Eu, 0x1Eu, 0xFFu), // maroon
	Colour(0x3Cu, 0x9Bu, 0x3Eu, 0xFFu), // forest
	Colour(0x3Cu, 0x3Cu, 0x77u, 0xFFu), // navy
	Colour(0x58u, 0xC1u, 0x58u, 0xFFu), // mint

	Colour(0x63u, 0xA7u, 0xA7u, 0xFFu), // teal
	Colour(0x86u, 0x6Du, 0x2Bu, 0xFFu), // brown
	Colour(0x8Au, 0x3Eu, 0xA1u, 0xFFu), // purple
	Colour(0x64u, 0x64u, 0xACu, 0xFFu), // sky
};

Vec3f getFactionColour(int ndx) {
	Colour &c = factionColours[ndx];
	return Vec3f(c.r / 255.f, c.g / 255.f, c.b / 255.f);
}

Vec3f  Faction::getColourV3f() const {
	Colour &c = factionColours[colourIndex];
	return Vec3f(c.r / 255.f, c.g / 255.f, c.b / 255.f);
}

// =====================================================
//  class UpgradeStage
// =====================================================
    UpgradeStage::UpgradeStage()
    {}

    UpgradeStage::~UpgradeStage() {}

void UpgradeStage::init(const UpgradeType *upgradeType, int upgradeStage, int maxStage, Names m_names,
        Enhancements m_enhancements, AffectedUnits m_unitsAffected, EnhancementMap m_enhancementMap) {
    this->upgradeType = upgradeType;
    this->upgradeStage = upgradeStage;
    this->maxStage = maxStage;
    this->m_names = m_names;
    this->m_enhancements = m_enhancements;
    this->m_unitsAffected = m_unitsAffected;
    this->m_enhancementMap = m_enhancementMap;
    }

const EnhancementType* UpgradeStage::getEnhancement(const UnitType *ut) const {
	EnhancementMap::const_iterator it = m_enhancementMap.find(ut);
	if (it != m_enhancementMap.end()) {
		return it->second->getEnhancement();
	}
	return 0;
}

Modifier UpgradeStage::getCostModifier(const UnitType *ut, const ResourceType *rt) const {
	EnhancementMap::const_iterator uit = m_enhancementMap.find(ut);
	if (uit != m_enhancementMap.end()) {
		ResModifierMap::const_iterator rit = uit->second->m_costModifiers.find(rt);
		if (rit != uit->second->m_costModifiers.end()) {
			return rit->second;
		}
	}
	return Modifier(0, 1);
}

Modifier UpgradeStage::getStoreModifier(const UnitType *ut, const ResourceType *rt) const {
	EnhancementMap::const_iterator uit = m_enhancementMap.find(ut);
	if (uit != m_enhancementMap.end()) {
		ResModifierMap::const_iterator rit = uit->second->m_storeModifiers.find(rt);
		if (rit != uit->second->m_storeModifiers.end()) {
			return rit->second;
		}
	}
	return Modifier(0, 1);
}

Modifier UpgradeStage::getCreateModifier(const UnitType *ut, const ResourceType *rt) const {
	EnhancementMap::const_iterator uit = m_enhancementMap.find(ut);
	if (uit != m_enhancementMap.end()) {
		ResModifierMap::const_iterator rit = uit->second->m_createModifiers.find(rt);
		if (rit != uit->second->m_createModifiers.end()) {
			return rit->second;
		}
	}
	return Modifier(0, 1);
}

void UpgradeStage::doChecksum(Checksum &checksum) const {
	ProducibleType::doChecksum(checksum);
	foreach_const (Enhancements, it, m_enhancements) {
		it->m_enhancement.doChecksum(checksum);
	}
	///@todo resource mods

	// iterating over a std::map is not the same as std::set !!
	vector<int> enhanceIds;
	foreach_const (EnhancementMap, it, m_enhancementMap) {
		enhanceIds.push_back(it->first->getId());
		///@todo add EnhancementType index (it->second->getEnhancement()) ?
	}
	// sort first
	std::sort(enhanceIds.begin(), enhanceIds.end());
	foreach (vector<int>, it, enhanceIds) {
		checksum.add(*it);
	}
}

// =====================================================
//  class Faction
// =====================================================
Faction::ResourceTypes Faction::neededResources;

void Faction::init(const FactionType *factionType, ControlType control, string playerName, TechTree *techTree,
					int factionIndex, int teamIndex, int startLocationIndex, int colourIndex,
					bool thisFaction, bool giveResources) {
	this->control = control;
	this->factionType = factionType;
	this->m_name = playerName;
	this->startLocationIndex = startLocationIndex;
	this->m_id = factionIndex;
	this->teamIndex = teamIndex;
	this->colourIndex = colourIndex;
	this->thisFaction = thisFaction;
	this->subfaction = 0;
	this->lastAttackNotice = 0;
	this->lastEnemyNotice = 0;
	this->defeated = false;
	lastEventLoc.x = -1.0f;  // -1 x indicates uninitialized, no last event

	texture = 0;
	m_logoTex = 0;

	for (int i=0; i < factionType->getUnitTypeCount(); ++i) {
		m_unitCountMap[factionType->getUnitType(i)] = 0;
	}

	if (factionIndex != -1) { // !Glestimals
		sresources.resize(techTree->getResourceTypeCount());
		for (int i = 0; i < techTree->getResourceTypeCount(); ++i) {
			const ResourceType *rt = techTree->getResourceType(i);
			int resourceAmount= giveResources ? factionType->getStartingResourceAmount(rt) : 0;
			sresources[i].init(rt, resourceAmount);
		}

		for (int i=0; i < factionType->getUnitTypeCount(); ++i) {
			const UnitType *ut = factionType->getUnitType(i);
			for (int j=0; j < techTree->getResourceTypeCount(); ++j) {
				const ResourceType *rt = techTree->getResourceType(j);
				m_costModifiers[ut][rt] = Modifier(0, 1);
				m_storeModifiers[ut][rt] = Modifier(0, 1);
			}
		}

		tradeCommands.resize(sresources.size()*4);
		int init = 0;
		int inita[] = {100, 500, 1000, 2000};
		for (int i = 0; i < sresources.size(); ++i) {
			const ResourceType *rt = sresources[i].getType();
			for (int j = 0; j < 4; ++j) {
			int value = inita[j];
			tradeCommands[init].init(value, rt, Clicks::TWO);
			++init;
			}
		}

        World *world = &g_world;
		mandateAISim.init(world, this);

        upgradeStages.resize(factionType->getUpgradeTypeCount());
		for (int i = 0; i < factionType->getUpgradeTypeCount(); ++i) {
			const UpgradeType *ut = factionType->getUpgradeType(i);
			upgradeStages[i].init(ut, 0, ut->maxStage, ut->m_names, ut->m_enhancements,
            ut->m_unitsAffected, ut->m_enhancementMap);
		}

		texture = g_renderer.newTexture2D(ResourceScope::GAME);
		Pixmap2D *pixmap = texture->getPixmap();
		pixmap->init(1, 1, 3);
		pixmap->setPixel(0, 0, factionColours[colourIndex].ptr());

		if (thisFaction && (factionType->getLogoTeamColour() || factionType->getLogoRgba())) {
			buildLogoPixmap();
		}
	}
}

void Faction::buildLogoPixmap() {
	m_logoTex = g_renderer.newTexture2D(ResourceScope::GAME);
	Pixmap2D *pixmap = m_logoTex->getPixmap();
	pixmap->init(256, 256, 4);

	const Pixmap2D *teamPixmap = factionType->getLogoTeamColour();
	if (teamPixmap) { // team-colour
		Vec3f baseColour = Vec3f(factionColours[colourIndex]) / 255.f;
		for (int y = 0; y < 256; ++y) {
			for (int x = 0; x < 256; ++x) {
				Vec4f pixel = teamPixmap->getPixel4f(x, y);
				float lum = (pixel.r + pixel.g + pixel.b) / 3.f;
				Vec4f val(baseColour.r * lum, baseColour.g * lum, baseColour.b * lum, pixel.a);
				pixmap->setPixel(x, y, val);
			}
		}
	}
	const Pixmap2D *rgbaPixmap = factionType->getLogoRgba();
	if (rgbaPixmap) {
		if (!teamPixmap) { // just copy
			for (int y = 0; y < 256; ++y) {
				for (int x = 0; x < 256; ++x) {
					pixmap->setPixel(x, y, rgbaPixmap->getPixel4f(x, y));
				}
			}
		} else { // dodgy blend...
			for (int y = 0; y < 256; ++y) {
				for (int x = 0; x < 256; ++x) {
					Vec4f current = pixmap->getPixel4f(x, y);
					Vec4f incoming = rgbaPixmap->getPixel4f(x, y);

					Vec4f result = (current * (1.f - incoming.a)) + (incoming * incoming.a);
					result.a = std::max(current.a, incoming.a);

					pixmap->setPixel(x, y, result);
				}
			}
		}
	}
}

void Faction::save(XmlNode *node) const {
	XmlNode *n;

	node->addChild("id", m_id);
	node->addChild("name", m_name);
	node->addChild("teamIndex", teamIndex);
	node->addChild("startLocationIndex", startLocationIndex);
	node->addChild("colourIndex", colourIndex);
	node->addChild("thisFaction", thisFaction);
	node->addChild("subfaction", subfaction);
//	node->addChild("lastEventLoc", lastEventLoc);
	upgradeManager.save(node->addChild("upgrades"));

	n = node->addChild("resources");
	for (int i = 0; i < sresources.size(); ++i) {
		XmlNode *resourceNode = n->addChild("resource");
		resourceNode->addChild("type", sresources[i].getType()->getName());
		resourceNode->addChild("amount", sresources[i].getAmount());
		resourceNode->addChild("storage", sresources[i].getStorage());
	}

	n = node->addChild("units");
	for (Units::const_iterator i = units.begin(); i != units.end(); ++i) {
		(*i)->save(n->addChild("unit"));
	}

	n = node->addChild("upgradeStages");
	for (int i = 0; i < upgradeStages.size(); ++i) {
	    XmlNode *upgradeStageNode = n->addChild("upgradeStage");
		upgradeStageNode->addChild("type", upgradeStages[i].getUpgradeType()->getName());
		upgradeStageNode->addChild("amount", upgradeStages[i].getUpgradeStage());
	}



}

void Faction::load(const XmlNode *node, World *world, const FactionType *ft, ControlType control, TechTree *tt) {
	XmlNode *n;
	Map *map = world->getMap();

	this->factionType = ft;
	this->control = control;
	this->lastAttackNotice = 0;
	this->lastEnemyNotice = 0;

	m_id = node->getChildIntValue("id");
	m_name = node->getChildStringValue("name");
	teamIndex = node->getChildIntValue("teamIndex");
	startLocationIndex = node->getChildIntValue("startLocationIndex");
	thisFaction = node->getChildBoolValue("thisFaction");
	subfaction = node->getChildIntValue("subfaction");
	time_t lastAttackNotice = 0;
	time_t lastEnemyNotice = 0;
//	lastEventLoc = node->getChildVec3fValue("lastEventLoc");

	upgradeManager.load(node->getChild("upgrades"), this);

	n = node->getChild("resources");
	sresources.resize(n->getChildCount());
	for (int i = 0; i < n->getChildCount(); ++i) {
		XmlNode *resourceNode = n->getChild("resource", i);
		const ResourceType *rt = tt->getResourceType(resourceNode->getChildStringValue("type"));
		sresources[i].init(rt, resourceNode->getChildIntValue("amount"));
		sresources[i].setStorage(resourceNode->getChildIntValue("storage"));
	}

    n = node->getChild("resources");
	cresources.resize(n->getChildCount());
	for (int i = 0; i < n->getChildCount(); ++i) {
		XmlNode *resourceNode = n->getChild("resource", i);
		const ResourceType *rt = tt->getResourceType(resourceNode->getChildStringValue("type"));
		cresources[i].init(rt, resourceNode->getChildIntValue("amount"));
	}

    n = node->getChild("upgradeStages");
	upgradeStages.resize(n->getChildCount());
	for (int i = 0; i < n->getChildCount(); ++i) {
		XmlNode *upgradeStageNode = n->getChild("upgradeStage", i);
		const UpgradeType *ut = ft->getUpgradeType(upgradeStageNode->getChildStringValue("type"));
        upgradeStages[i].init(ut, 0, ut->maxStage, ut->m_names,
        ut->m_enhancements, ut->m_unitsAffected, ut->m_enhancementMap);
    }

	n = node->getChild("units");
	units.reserve(n->getChildCount());
	assert(units.empty() && unitMap.empty());
	for (int i = 0; i < n->getChildCount(); ++i) {
		g_world.newUnit(n->getChild("unit", i), this, map, tt);
	}
	subfaction = node->getChildIntValue("subfaction"); // reset in case unit construction changed it
	colourIndex = node->getChildIntValue("colourIndex");

	texture = g_renderer.newTexture2D(ResourceScope::GAME);
	Pixmap2D *pixmap = texture->getPixmap();
	pixmap->init(1, 1, 3);
	pixmap->setPixel(0, 0, factionColours[colourIndex].ptr());

	assert(units.size() == unitMap.size());

    /*for (int i = 0; i < getType()->getGuiFileNamesCount(); ++i) {
	    Rocket::Core::String fileName;
	    for(int j = 0; j < getType()->getGuiFileName(i).size(); j++) {
            char c = getType()->getGuiFileName(i)[j];
            fileName += c;
        }
        Rocket::Core::ElementDocument* document = world->getGame().getContext()->LoadDocument(fileName);
        if (document != NULL) {
            document->Show();
            document->RemoveReference();
        }
	}*/
}

// ================== get ==================
const StoredResource *Faction::getSResource(const ResourceType *rt) const {
	for (int i = 0; i < sresources.size(); ++i) {
		if (rt == sresources[i].getType()) {
			return &sresources[i];
		}
	}
	assert(false);
	return NULL;
}

int Faction::getStoreAmount(const ResourceType *rt) const {
	for (int i = 0; i < sresources.size(); ++i) {
		if (rt == sresources[i].getType()) {
			return sresources[i].getStorage();
		}
	}
	assert(false);
	return 0;
}

const CreatedResource *Faction::getCResource(const ResourceType *rt) const {
	for (int i = 0; i < cresources.size(); ++i) {
		if (rt == cresources[i].getType()) {
			return &cresources[i];
		}
	}
	assert(false);
	return NULL;
}

int Faction::getCreateAmount(const ResourceType *rt) const {
	for (int i = 0; i < cresources.size(); ++i) {
		if (rt == cresources[i].getType()) {
		}
	}
	assert(false);
	return 0;
}

UpgradeStage *Faction::getUpgradeStage(const UpgradeType *ut) {
	for (int i = 0; i < upgradeStages.size(); ++i) {
		if (ut == upgradeStages[i].getUpgradeType()) {
			return &upgradeStages[i];
		}
	}
	assert(false);
	return NULL;
}

int Faction::getCurrentStage(const UpgradeType *ut) {
	for (int i = 0; i < upgradeStages.size(); ++i) {
		if (ut == upgradeStages[i].getUpgradeType()) {
			return upgradeStages[i].getUpgradeStage();
		}
	}
	assert(false);
	return 0;
}

// ==================== upgrade manager ====================

void Faction::startUpgrade(const UpgradeType *ut) {
	upgradeManager.startUpgrade(ut, m_id);
}

void Faction::cancelUpgrade(const UpgradeType *ut) {
	upgradeManager.cancelUpgrade(ut);
}

void Faction::finishUpgrade(const UpgradeType *ut) {
	upgradeManager.finishUpgrade(ut, this);

	for (int i = 0; i < getUnitCount(); ++i) {
		getUnit(i)->applyUpgrade(ut);
	}

    upgradeManager.wrapUpdateUpgrade(ut, this);

    const UpgradeStage *us = getUpgradeStage(ut);

	// update unit cost & store modifiers
	const TechTree *tt = g_world.getTechTree();
	for (int i=0; i < factionType->getUnitTypeCount(); ++i) {
		const UnitType *unitType = factionType->getUnitType(i);
		for (int j=0; j < tt->getResourceTypeCount(); ++j) {
			const ResourceType *resType = tt->getResourceType(j);
			Modifier mod = us->getCostModifier(unitType, resType);
			m_costModifiers[unitType][resType].m_addition += mod.getAddition();
			m_costModifiers[unitType][resType].m_multiplier += (mod.getMultiplier() - 1);

			mod = us->getStoreModifier(unitType, resType);
			m_storeModifiers[unitType][resType].m_addition += mod.getAddition();
			m_storeModifiers[unitType][resType].m_multiplier += (mod.getMultiplier() - 1);

			mod = us->getCreateModifier(unitType, resType);
			m_createModifiers[unitType][resType].m_addition += mod.getAddition();
			m_createModifiers[unitType][resType].m_multiplier += (mod.getMultiplier() - 1);
		}
	}

	// update store caps
	reEvaluateStore();
}

Modifier Faction::getCostModifier(const ProducibleType *pt, const ResourceType *rt) const {
	UnitCostModifiers::const_iterator it = m_costModifiers.find(pt);
	if (it != m_costModifiers.end()) {
		CostModifiers::const_iterator rit = it->second.find(rt);
		if (rit != it->second.end()) {
			return rit->second;
		}
	}
	return Modifier(0, 1);
}

Modifier Faction::getStoreModifier(const UnitType *ut, const ResourceType *rt) const {
	StoreModifiers::const_iterator it = m_storeModifiers.find(ut);
	if (it != m_storeModifiers.end()) {
		CostModifiers::const_iterator rit = it->second.find(rt);
		if (rit != it->second.end()) {
			return rit->second;
		}
	}
	return Modifier(0, 1);
}

Modifier Faction::getCreateModifier(const UnitType *ut, const ResourceType *rt) const {
	CreateModifiers::const_iterator it = m_createModifiers.find(ut);
	if (it != m_createModifiers.end()) {
		CostModifiers::const_iterator rit = it->second.find(rt);
		if (rit != it->second.end()) {
			return rit->second;
		}
	}
	return Modifier(0, 1);
}

Modifier Faction::getCreatedUnitModifier(const UnitType *ut, const UnitType *sut) const {
	CreatedUnitModifiers::const_iterator it = m_createdUnitModifiers.find(ut);
	if (it != m_createdUnitModifiers.end()) {
		CreateUnitModifiers::const_iterator rit = it->second.find(sut);
		if (rit != it->second.end()) {
			return rit->second;
		}
	}
	return Modifier(0, 1);
}

// ==================== reqs ====================

/** Checks if all required units and upgrades are present for a RequirableType */
bool Faction::reqsOk(const RequirableType *rt) const {
	// required units
	for (int i = 0; i < rt->getUnitReqCount(); ++i) {
		if (!getCountOfUnitType(rt->getUnitReq(i))) {
			return false;
		}
	}
	// required upgrades
	for (int i = 0; i < rt->getUpgradeReqCount(); ++i) {
		if (upgradeManager.isUpgraded(rt->getUpgradeReq(i).reqType)) {
		} else if (upgradeManager.isPartial(rt->getUpgradeReq(i).reqType)) {
		    Faction *f;
		    for (int j = 0; j < g_world.getFactionCount(); ++j) {
		        if (this == g_world.getFaction(j)) {
                    f = g_world.getFaction(j);
		        }
		    }
		    int stage = f->getCurrentStage(rt->getUpgradeReq(i).reqType);
            if (rt->getUpgradeReq(i).stage == stage) {

            } else {
                return false;
            }
		} else {
            return false;
		}
	}
	// available in subfaction ?
	return rt->isAvailableInSubfaction(subfaction);
}

/** Checks if all required units and upgrades are present for a CommandType
  * @return true if the Command can be executed, or if is a multi-tier Produce/Morph/Generate
  */
bool Faction::reqsOk(const CommandType *ct) const {
	if (ct->getClass() == CmdClass::MOVE) {
		DEBUG_HOOK();
	}
	if (ct->getProducedCount() == 1) {
		return reqsOk(ct, ct->getProduced(0));
	} else {
		return reqsOk(ct, 0);
	}
}

/** Checks if all required units and upgrades are present for a CommandType & a ProducibleType
  * @return true if the Command can be executed and (optionaly) Producible produced
  */
bool Faction::reqsOk(const CommandType *ct, const ProducibleType *pt) const {

	if (ct->getClass() == CmdClass::SET_MEETING_POINT
	|| ct->getClass() == CmdClass::BE_LOADED) {
		return true;
	}

	if (ct->getClass() == CmdClass::CREATE_SQUAD
	|| ct->getClass() == CmdClass::EXPAND_SQUAD) {
		return true;
	}

	if ((pt && !reqsOk(pt)) || !isAvailable(ct)) {
		return false;
	}

	if (ct->getClass() == CmdClass::UPGRADE && pt) {
		const UpgradeCommandType *uct = static_cast<const UpgradeCommandType*>(ct);
		const UpgradeType *ut = static_cast<const UpgradeType*>(pt);
		const Faction *f = this;
		if (upgradeManager.isUpgradingOrUpgraded(ut, f)) {
			return false;
		}
	}

	return reqsOk(static_cast<const RequirableType*>(ct));
}

/** Checks sub-faction availability of a command @return true if available */
bool Faction::isAvailable(const CommandType *ct) const {
	if (ct->getProducedCount() == 1) {
		return isAvailable(ct, ct->getProduced(0));
	} else {
		return isAvailable(ct, 0);
	}
}

/** Checks sub-faction availability of a command and a producible @return true if available */
bool Faction::isAvailable(const CommandType *ct, const ProducibleType *pt) const {
	if (!ct->isAvailableInSubfaction(subfaction)) {
		return false;
	}
	// If this command is producing or building anything, we need to make sure
	// that producable is also available.
	if (pt) {
		if (pt->isAvailableInSubfaction(subfaction)) {
			return true;
		} else {
			return false;
		}
	}
	return true;
}

void Faction::reportReqs(const RequirableType *rt, CommandCheckResult &out_result, bool checkDups) const {
	// required units
	for (int i = 0; i < rt->getUnitReqCount(); ++i) {
		const UnitType *ut = rt->getUnitReq(i);
		if (checkDups) {
			bool duplicate = false;
			foreach (UnitReqResults, it, out_result.m_unitReqResults) {
				if (it->getUnitType() == ut) {
					duplicate = true;
					break;
				}
			}
			if (duplicate) {
				continue;
			}
		}
		bool ok = getCountOfUnitType(ut);
		out_result.m_unitReqResults.push_back(UnitReqResult(ut, ok));
	}
	// required upgrades
	for (int i = 0; i < rt->getUpgradeReqCount(); ++i) {
		const UpgradeType *ut = rt->getUpgradeReq(i).reqType;
		if (checkDups) {
			bool duplicate = false;
			foreach (UpgradeReqResults, it, out_result.m_upgradeReqResults) {
				if (it->getUpgradeType() == ut) {
					duplicate = true;
					break;
				}
			}
			if (duplicate) {
				continue;
			}
		}
		bool ok = upgradeManager.isUpgraded(ut);
		out_result.m_upgradeReqResults.push_back(UpgradeReqResult(ut, ok));
	}
}

void Faction::reportReqsAndCosts(const CommandType *ct, const ProducibleType *pt, CommandCheckResult &out_result) const {
	out_result.m_commandType = ct;
	out_result.m_producibleType = pt;
	reportReqs(ct, out_result);
	out_result.m_availableInSubFaction = isAvailable(ct);
	if (pt) {
		if (pt->getClass() == ProducibleClass::UPGRADE) {
			const UpgradeType *ut = static_cast<const UpgradeType*>(pt);
			out_result.m_upgradedAlready = upgradeManager.isUpgraded(ut);
			out_result.m_upgradingAlready = upgradeManager.isUpgrading(ut);
			out_result.m_partiallyUpgraded = upgradeManager.isPartial(ut);
		} else {
			out_result.m_upgradedAlready = false;
			out_result.m_upgradingAlready = false;
			out_result.m_partiallyUpgraded = false;
		}
		reportReqs(pt, out_result, true);
		for (int i=0; i < pt->getCostCount(); ++i) {
			ResourceAmount res = pt->getCost(i, this);
			if (res.getAmount() < 0) {
				out_result.m_resourceMadeResults.push_back(ResourceMadeResult(res.getType(), -res.getAmount()));
			} else {
				int stored = getSResource(res.getType())->getAmount();
				out_result.m_resourceCostResults.push_back(ResourceCostResult(res.getType(), res.getAmount(), stored));
			}
		}
		if (out_result.m_availableInSubFaction) { // don't overwrite false
			out_result.m_availableInSubFaction = isAvailable(pt);
		}
	} else {
		out_result.m_upgradedAlready = false;
		out_result.m_upgradingAlready = false;
		out_result.m_partiallyUpgraded = false;
	}
}

// ================== cost application ==================

/// apply costs except static production (start building/production)
bool Faction::applyCosts(const ProducibleType *p) {

	if (!checkCosts(p)) {
		return false;
	}

	//for each unit cost spend it
	//pass 2, decrease resources, except negative static costs (ie: farms)
	for (int i = 0; i < p->getCostCount(); ++i) {
		ResourceAmount ra = p->getCost(i, this);
		const ResourceType *rt = ra.getType();
        int cost = ra.getAmount();
		if ((cost > 0 || rt->getClass() != ResourceClass::STATIC) && rt->getClass() != ResourceClass::CONSUMABLE) {
		    int addition = ra.getAmountPlus();
		    fixed multiplier = ra.getAmountMultiply();
		    int plus = m_costModifiers[p][rt].m_addition;
		    fixed multiply = m_costModifiers[p][rt].m_multiplier;
			cost = addition + cost;
			if (multiplier.intp() != 1) {
                cost = (cost * multiplier).intp();
			}
			cost = cost + plus;
			if (multiply.intp() != 1) {
                cost = (cost * multiply).intp();
			}
			incResourceAmount(rt, -cost);
		}

	}
	return true;
}

/// apply costs (with a discount) except static production (start building/production)
bool Faction::applyCosts(const ProducibleType *pt, int discount) {
	if (!checkCosts(pt, discount)) {
		return false;
	}

	fixed ratio = (fixed(100) - (discount / fixed(100))) / 100;

	//for each unit cost spend it
	//pass 2, decrease resources, except negative static costs (ie: farms)
	for (int i = 0; i < pt->getCostCount(); ++i) {
		ResourceAmount ra = pt->getCost(i, this);
		const ResourceType *rt = ra.getType();
		int cost = ra.getAmount();
		int modCost = (cost * ratio).intp();
		if ((cost > 0 || rt->getClass() != ResourceClass::STATIC) && rt->getClass() != ResourceClass::CONSUMABLE) {
            int addition = ra.getAmountPlus();
		    fixed multiplier = ra.getAmountMultiply();
			modCost = (m_costModifiers[pt][rt].m_addition + addition + cost) * multiplier.intp();
			incResourceAmount(rt, -(modCost));
		}
	}
	return true;

}

/// give refund (when a morph ends)
void Faction::giveRefund(const ProducibleType *p, int refund) {
	//increase resources
	for (int i = 0; i < p->getCostCount(); ++i) {
		ResourceAmount ra = p->getCost(i, this);
		const ResourceType *rt = ra.getType();
		int cost = ra.getAmount();
		if ((cost > 0 || rt->getClass() != ResourceClass::STATIC) && rt->getClass() != ResourceClass::CONSUMABLE) {
			incResourceAmount(rt, cost * refund / 100);
		}
	}
}

//apply static production (for starting units)
void Faction::applyStaticCosts(const ProducibleType *p) {

	//decrease static resources
	for (int i = 0; i < p->getCostCount(); ++i) {
		ResourceAmount ra = p->getCost(i, this);
		const ResourceType *rt = ra.getType();
		if (rt->getClass() == ResourceClass::STATIC) {
			int cost = ra.getAmount();
			if (cost > 0) {
				incResourceAmount(rt, -cost);
			}
		}
	}
}

//apply static production (when a mana source is done)
void Faction::applyStaticProduction(const ProducibleType *p) {

	//decrease static resources
	for (int i = 0; i < p->getCostCount(); ++i) {
		ResourceAmount ra = p->getCost(i, this);
		const ResourceType *rt = ra.getType();
		if (rt->getClass() == ResourceClass::STATIC) {
			int cost = ra.getAmount();
			if (cost < 0) {
				incResourceAmount(rt, -cost);
			}
		}
	}
}

//deapply all costs except static production (usually when a building is cancelled)
void Faction::deApplyCosts(const ProducibleType *p) {

	//increase resources
	for (int i = 0; i < p->getCostCount(); ++i) {
		ResourceAmount ra = p->getCost(i, this);
		const ResourceType *rt = ra.getType();
		int cost = ra.getAmount();
		if ((cost > 0 || rt->getClass() != ResourceClass::STATIC) && rt->getClass() != ResourceClass::CONSUMABLE) {
			incResourceAmount(rt, cost);
		}
	}
}

//deapply static costs (usually when a unit dies)
void Faction::deApplyStaticCosts(const ProducibleType *p) {

	//decrease resources
	for (int i = 0; i < p->getCostCount(); ++i) {
		ResourceAmount ra = p->getCost(i, this);
		const ResourceType *rt = ra.getType();
		if (rt->getClass() == ResourceClass::STATIC && rt->getRecoupCost()) {
			int cost = ra.getAmount();
			incResourceAmount(rt, cost);
		}
	}
}

//deapply static costs, but not negative costs, for when building gets killed
void Faction::deApplyStaticConsumption(const ProducibleType *p) {
	//decrease resources
	for (int i=0; i < p->getCostCount(); ++i) {
		ResourceAmount ra = p->getCost(i, this);
		const ResourceType *rt = ra.getType();
		if (rt->getClass() == ResourceClass::STATIC) {
			int cost = ra.getAmount();
			if (cost > 0) {
				incResourceAmount(rt, cost);
			}
		}
	}
}

// apply resource on interval for a cosumable resouce
void Faction::applyCostsOnInterval(const ResourceType *rt) {
	assert(rt->getClass() == ResourceClass::CONSUMABLE);
	if (!ScriptManager::getPlayerModifiers(m_id)->getConsumeEnabled()) {
		return;
	}
	// increment consumables
	for (int j = 0; j < getUnitCount(); ++j) {
		Unit *unit = getUnit(j);
		if (unit->isOperative()) {
			const ResourceAmount resource = unit->getType()->getCost(rt, this);
			if (resource.getType() && resource.getAmount() < 0) {
				incResourceAmount(resource.getType(), -resource.getAmount());
			}
		}
	}

	//decrement consumables
	for (int j = 0; j < getUnitCount(); ++j) {
		Unit *unit = getUnit(j);
		if (unit->isOperative()) {
			const ResourceAmount resource = unit->getType()->getCost(rt, this);
			if (resource.getType() && resource.getAmount() > 0) {
				incResourceAmount(resource.getType(), -resource.getAmount());

				//decrease unit hp
				///@todo: Implement rules for specifying what happens when you're consumable
				//      demand exceeds supply & stores.
				if (getSResource(resource.getType())->getAmount() < 0) {
					resetResourceAmount(resource.getType());
					if (unit->decHp(unit->getType()->getMaxHp() / 3)) {
						World::getCurrWorld()->doKill(unit, unit);
					} else {
						StaticSound *sound = unit->getType()->getFirstStOfClass(SkillClass::DIE)->getSound();
						if (sound != NULL && thisFaction) {
							SoundRenderer::getInstance().playFx(sound);
						}
					}
				}
			}
		}
	}
	// limit to store
	capResource(rt);
}

bool Faction::checkCosts(const ProducibleType *pt) {
	bool ok = true;
	neededResources.clear();
	for (int i = 0; i < pt->getCostCount(); ++i) {
		ResourceAmount ra = pt->getCost(i, this);
		const ResourceType *rt = ra.getType();
		int cost = ra.getAmount();
		if (cost > 0) {
			int available = getSResource(rt)->getAmount();
			if (cost > available) {
				ok = false;
				neededResources.push_back(rt);
			}
		}
	}
	return ok;
}

bool Faction::checkCosts(const ProducibleType *pt, int discount) {
	bool ok = true;
	neededResources.clear();
	fixed ratio = (fixed(100) - (discount / fixed(100))) / 100;

	//for each unit cost check if enough resources
	for (int i = 0; i < pt->getCostCount(); ++i) {
		ResourceAmount ra = pt->getCost(i, this);
		const ResourceType *rt = ra.getType();
		int cost = (ra.getAmount() * ratio).intp();

		if (cost > 0) {
			int available = getSResource(rt)->getAmount();

			if (cost > available) {
				ok = false;
				neededResources.push_back(rt);
			}
		}
	}

	return ok;
}

// ================== diplomacy ==================

bool Faction::hasBuilding() const {
	for (int i = 0; i < getUnitCount(); ++i) {
		Unit *unit = getUnit(i);
		if (unit->isBuilding()) {
			return true;
		}
	}
	return false;
}

bool Faction::canSee(const Unit *unit) const {
	Map &map = g_map;
	if (unit->isCarried()) {
		return false;
	}
	if (unit->isGarrisoned()) {
		return false;
	}
	if (isAlly(unit->getFaction())) {
		return true;
	}
	Vec2i tPos = Map::toTileCoords(unit->getCenteredPos());
	if (unit->isCloaked()) {
		int cloakGroup = unit->getType()->getCloakType()->getCloakGroup();
		if (!g_cartographer.canDetect(teamIndex, cloakGroup, tPos)) {
			return false;
		}
	}
	if (map.getTile(tPos)->isVisible(teamIndex)) {
		return true;
	}
	if (unit->isTargetUnitVisible(teamIndex)) {
		return true;
	}
	return false;
}

// ================== misc ==================

void Faction::incResourceAmount(const ResourceType *rt, int amount) {
	for (int i = 0; i < sresources.size(); ++i) {
		StoredResource *r = &sresources[i];
		if (r->getType() == rt) {
			r->setAmount(r->getAmount() + amount);
			if (r->getType()->getClass() != ResourceClass::STATIC
			&& r->getType()->getClass() != ResourceClass::CONSUMABLE
			&& r->getAmount() > getStoreAmount(rt)) {
				r->setAmount(getStoreAmount(rt));
			}
			return;
		}
	}
	assert(false);
}

void Faction::setResourceBalance(const ResourceType *rt, int balance) {
	if (!ScriptManager::getPlayerModifiers(m_id)->getConsumeEnabled()) {
		return;
	}
	for (int i = 0; i < sresources.size(); ++i) {
		StoredResource *r = &sresources[i];
		if (r->getType() == rt) {
			r->setBalance(balance);
			return;
		}
	}
	assert(false);
}

void Faction::add(Unit *unit) {
//	LOG_NETWORK( "Faction: " + intToStr(id) + " unit added Id: " + intToStr(unit->getId()) );
	units.push_back(unit);
	unitMap[unit->getId()] = unit;
}

void Faction::remove(Unit *unit) {
	Units::iterator it = std::find(units.begin(), units.end(), unit);
	assert(it != units.end());
	units.erase(it);
	unitMap.erase(unit->getId());
	assert(units.size() == unitMap.size());
}

void Faction::addItem(Item *item) {
    items.push_back(item);
}

void Faction::reEvaluateStore() {
	typedef map<const ResourceType*, int> StorageMap;
	StorageMap storeMap;
	const TechTree *tt = g_world.getTechTree();
	for (int i=0; i < tt->getResourceTypeCount(); ++i) {
		const ResourceType *rt = tt->getResourceType(i);
		if (rt->getClass() != ResourceClass::STATIC) {
			storeMap[rt] = 0;
		}
	}
	foreach_const (Units, it, units) {
		// don't want the resources of dead units to be included
		if (!(*it)->isDead()) {
			const UnitType *ut = (*it)->getType();
			for (int j=0; j < ut->getResourceProductionSystem().getStoredResourceCount(); ++j) {
				ResourceAmount res = ut->getResourceProductionSystem().getStoredResource(j, this);
				storeMap[res.getType()] += res.getAmount();
			}
		}
	}
	for (int j = 0; j < sresources.size(); ++j) {
		if (sresources[j].getType()->getClass() != ResourceClass::STATIC) {
			sresources[j].setStorage(storeMap[sresources[j].getType()]);
		}
	}
}

void Faction::addStore(const ResourceType *rt, int amount) {
	for (int j = 0; j < sresources.size(); ++j) {
		if (sresources[j].getType() == rt) {
			sresources[j].setStorage(sresources[j].getStorage() + amount);
		}
	}
}

void Faction::addStore(const UnitType *unitType) {
	for (int i = 0; i < unitType->getResourceProductionSystem().getStoredResourceCount(); ++i) {
		ResourceAmount r = unitType->getResourceProductionSystem().getStoredResource(i, this);
		for (int j = 0; j < sresources.size(); ++j) {
			if (sresources[j].getType() == r.getType()) {
				sresources[j].setStorage(sresources[j].getStorage() + r.getAmount());
			}
		}
	}
}

void Faction::removeStore(const UnitType *unitType) {
	reEvaluateStore();
	limitResourcesToStore();
}

void Faction::capResource(const ResourceType *rt) {
	RUNTIME_CHECK(rt->getClass() == ResourceClass::CONSUMABLE);
	for (int i = 0; i < sresources.size(); ++i) {
		StoredResource *r = &sresources[i];
		if (r->getType() == rt) {
			if (r->getAmount() > getStoreAmount(rt)) {
				r->setAmount(getStoreAmount(rt));
			}
			return;
		}
	}
	assert(false);
}

void Faction::limitResourcesToStore() {
	for (int i = 0; i < sresources.size(); ++i) {
		StoredResource *r = &sresources[i];
		if (r->getType()->getClass() != ResourceClass::STATIC && r->getAmount() > r->getStorage()) {
			r->setAmount(r->getStorage());
		}
	}
}

void Faction::resetResourceAmount(const ResourceType *rt) {
	for (int i = 0; i < sresources.size(); ++i) {
		if (sresources[i].getType() == rt) {
			sresources[i].setAmount(0);
			return;
		}
	}
	assert(false);
}

/**
 * Plays an auditory notification of being attacked, if it hasn't been done
 * recently
 */
void Faction::attackNotice(const Unit *u) {
	if (factionType->getAttackNotice()) {
		time_t curTime;
		time(&curTime);

		if (curTime >= lastAttackNotice + factionType->getAttackNoticeDelay()) {
			lastAttackNotice = curTime;
			RUNTIME_CHECK(!u->isCarried());
			RUNTIME_CHECK(!u->isGarrisoned());
			lastEventLoc = u->getCurrVector();
			StaticSound *sound = factionType->getAttackNotice()->getRandSound();
			if (sound) {
				g_soundRenderer.playFx(sound);
			}
		}
	}
	g_userInterface.getMinimap()->addAttackNotice(u->getCenteredPos());
}

void Faction::advanceSubfaction(int subfaction) {
	this->subfaction = subfaction;
	//FIXME: We should probably play a sound, display a banner-type notice or
	//something.
}

void Faction::checkAdvanceSubfaction(const ProducibleType *pt, bool finished) {
	int advance = pt->getAdvancesToSubfaction();
	if (advance != -1 && subfaction != advance) {
		bool immediate = pt->isAdvanceImmediately();
		if ((immediate && !finished) || (!immediate && finished)) {
			advanceSubfaction(advance);
		}
	}
}

void Faction::applyTrade(const ResourceType *rt, int tradeAmount) {
    incResourceAmount(rt, tradeAmount);
}

}}//end namespace
