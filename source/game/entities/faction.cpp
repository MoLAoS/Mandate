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
		resources.resize(techTree->getResourceTypeCount());
		for (int i = 0; i < techTree->getResourceTypeCount(); ++i) {
			const ResourceType *rt = techTree->getResourceType(i);
			int resourceAmount= giveResources? factionType->getStartingResourceAmount(rt): 0;
			resources[i].init(rt, resourceAmount);
		}
		for (int i=0; i < factionType->getUnitTypeCount(); ++i) {
			const UnitType *ut = factionType->getUnitType(i);
			for (int j=0; j < techTree->getResourceTypeCount(); ++j) {
				const ResourceType *rt = techTree->getResourceType(j);
				m_costModifiers[ut][rt] = Modifier(0, 1);
				m_storeModifiers[ut][rt] = Modifier(0, 1);
			}
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
	for (int i = 0; i < resources.size(); ++i) {
		XmlNode *resourceNode = n->addChild("resource");
		resourceNode->addChild("type", resources[i].getType()->getName());
		resourceNode->addChild("amount", resources[i].getAmount());
		resourceNode->addChild("storage", resources[i].getStorage());
	}

	n = node->addChild("units");
	for (Units::const_iterator i = units.begin(); i != units.end(); ++i) {
		(*i)->save(n->addChild("unit"));
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
	resources.resize(n->getChildCount());
	for (int i = 0; i < n->getChildCount(); ++i) {
		XmlNode *resourceNode = n->getChild("resource", i);
		const ResourceType *rt = tt->getResourceType(resourceNode->getChildStringValue("type"));
		resources[i].init(rt, resourceNode->getChildIntValue("amount"));
		resources[i].setStorage(resourceNode->getChildIntValue("storage"));
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
}

// ================== get ==================

const StoredResource *Faction::getResource(const ResourceType *rt) const {
	for (int i = 0; i < resources.size(); ++i) {
		if (rt == resources[i].getType()) {
			return &resources[i];
		}
	}
	assert(false);
	return NULL;
}

int Faction::getStoreAmount(const ResourceType *rt) const {
	for (int i = 0; i < resources.size(); ++i) {
		if (rt == resources[i].getType()) {
			return resources[i].getStorage();
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
	upgradeManager.finishUpgrade(ut);

	for (int i = 0; i < getUnitCount(); ++i) {
		getUnit(i)->applyUpgrade(ut);
	}

	// update unit cost & store modifiers
	const TechTree *tt = g_world.getTechTree();
	for (int i=0; i < factionType->getUnitTypeCount(); ++i) {
		const UnitType *unitType = factionType->getUnitType(i);
		for (int j=0; j < tt->getResourceTypeCount(); ++j) {
			const ResourceType *resType = tt->getResourceType(j);
			Modifier mod = ut->getCostModifier(unitType, resType);
			m_costModifiers[unitType][resType].m_addition += mod.getAddition();
			m_costModifiers[unitType][resType].m_multiplier += (mod.getMultiplier() - 1);

			mod = ut->getStoreModifier(unitType, resType);
			m_storeModifiers[unitType][resType].m_addition += mod.getAddition();
			m_storeModifiers[unitType][resType].m_multiplier += (mod.getMultiplier() - 1);
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
		if (!upgradeManager.isUpgraded(rt->getUpgradeReq(i))) {
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
	
	if ((pt && !reqsOk(pt)) || !isAvailable(ct)) {
		return false;
	}

	if (ct->getClass() == CmdClass::UPGRADE && pt) {
		const UpgradeCommandType *uct = static_cast<const UpgradeCommandType*>(ct);
		const UpgradeType *ut = static_cast<const UpgradeType*>(pt);
		if (upgradeManager.isUpgradingOrUpgraded(ut)) {
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
		const UpgradeType *ut = rt->getUpgradeReq(i);
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
		} else {
			out_result.m_upgradedAlready = false;
			out_result.m_upgradingAlready = false;
		}
		reportReqs(pt, out_result, true);
		for (int i=0; i < pt->getCostCount(); ++i) {
			ResourceAmount res = pt->getCost(i, this);
			if (res.getAmount() < 0) {
				out_result.m_resourceMadeResults.push_back(ResourceMadeResult(res.getType(), -res.getAmount()));
			} else {
				int stored = getResource(res.getType())->getAmount();
				out_result.m_resourceCostResults.push_back(ResourceCostResult(res.getType(), res.getAmount(), stored));
			}
		}
		if (out_result.m_availableInSubFaction) { // don't overwrite false
			out_result.m_availableInSubFaction = isAvailable(pt);
		}
	} else {
		out_result.m_upgradedAlready = false;
		out_result.m_upgradingAlready = false;
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
			incResourceAmount(rt, -(cost));
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
		cost = (cost * ratio).intp();

		if ((cost > 0 || rt->getClass() != ResourceClass::STATIC) && rt->getClass() != ResourceClass::CONSUMABLE) {
			incResourceAmount(rt, -(cost));
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
				if (getResource(resource.getType())->getAmount() < 0) {
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

	//for each unit cost check if enough resources
	for (int i = 0; i < pt->getCostCount(); ++i) {
		ResourceAmount ra = pt->getCost(i, this);
		const ResourceType *rt = ra.getType();		
		int cost = ra.getAmount();

		if (cost > 0) {
			int available = getResource(rt)->getAmount();

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
			int available = getResource(rt)->getAmount();

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
	for (int i = 0; i < resources.size(); ++i) {
		StoredResource *r = &resources[i];
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
	for (int i = 0; i < resources.size(); ++i) {
		StoredResource *r = &resources[i];
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
			for (int j=0; j < ut->getStoredResourceCount(); ++j) {
				ResourceAmount res = ut->getStoredResource(j, this);
				storeMap[res.getType()] += res.getAmount();
			}
		}
	}
	for (int j = 0; j < resources.size(); ++j) {
		if (resources[j].getType()->getClass() != ResourceClass::STATIC) {
			resources[j].setStorage(storeMap[resources[j].getType()]);
		}
	}
}

void Faction::addStore(const ResourceType *rt, int amount) {
	for (int j = 0; j < resources.size(); ++j) {
		if (resources[j].getType() == rt) {
			resources[j].setStorage(resources[j].getStorage() + amount);
		}
	}
}

void Faction::addStore(const UnitType *unitType) {
	for (int i = 0; i < unitType->getStoredResourceCount(); ++i) {
		ResourceAmount r = unitType->getStoredResource(i, this);
		for (int j = 0; j < resources.size(); ++j) {
			if (resources[j].getType() == r.getType()) {
				resources[j].setStorage(resources[j].getStorage() + r.getAmount());
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
	for (int i = 0; i < resources.size(); ++i) {
		StoredResource *r = &resources[i];
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
	for (int i = 0; i < resources.size(); ++i) {
		StoredResource *r = &resources[i];
		if (r->getType()->getClass() != ResourceClass::STATIC && r->getAmount() > r->getStorage()) {
			r->setAmount(r->getStorage());
		}
	}
}

void Faction::resetResourceAmount(const ResourceType *rt) {
	for (int i = 0; i < resources.size(); ++i) {
		if (resources[i].getType() == rt) {
			resources[i].setAmount(0);
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

}}//end namespace
