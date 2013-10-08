// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "faction_command.h"
#include "unit_type.h"
#include "util.h"
#include "sound_renderer.h"
#include "renderer.h"
#include "tech_tree.h"
#include "world.h"
#include "program.h"
#include "sim_interface.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Gui_Mandate {

// =====================================================
// 	class FactionBuild
// =====================================================

void FactionBuild::init(const UnitType* ut, Clicks cl) {
    unitType = ut;
    clicks = cl;
    stringstream ss;
    m_tipKey = "Build " + ut->getName();
    m_tipHeaderKey = ut->getName();
    name = ut->getName();
}

void FactionBuild::describe(const Faction *faction, TradeDescriptor *callback, const UnitType *ut) const {
    Lang &lang = g_lang;
    string header, tip;
    string factionName = ut->getFactionType()->getName();
    if (!lang.lookUp(ut->getName(), factionName, header)) {
        header = formatString(ut->getName());
    }
	callback->setHeader(header);
	callback->setTipText("");
	FactionBuildCheckResult factionBuildCheckResult;
	faction->reportBuildReqsAndCosts(ut, factionBuildCheckResult);

	vector<ItemReqResult> &itemReqs = factionBuildCheckResult.m_itemReqResults;
    vector<UnitReqResult> &unitReqs = factionBuildCheckResult.m_unitReqResults;
    vector<UpgradeReqResult> &upgradeReqs = factionBuildCheckResult.m_upgradeReqResults;
    if (!unitReqs.empty() || !upgradeReqs.empty()) {
        callback->addElement(g_lang.get("Reqs") + ":");
        if (!unitReqs.empty()) {
            foreach (vector<UnitReqResult>, it, factionBuildCheckResult.m_unitReqResults) {
                string name = g_lang.getTranslatedFactionName(factionName, it->getUnitType()->getName());
                callback->addReq(it->isRequirementMet(), it->getUnitType(), name);
            }
        }
        if (!upgradeReqs.empty()) {
            foreach (vector<UpgradeReqResult>, it, factionBuildCheckResult.m_upgradeReqResults) {
                string name = g_lang.getTranslatedFactionName(factionName, it->getUpgradeType()->getName());
                callback->addReq(it->isRequirementMet(), it->getUpgradeType(), name);
            }
        }
    }
    if (!factionBuildCheckResult.m_resourceCostResults.empty()) {
        callback->addElement(g_lang.get("Costs") + ":");
        foreach (ResourceCostResults, it, factionBuildCheckResult.m_resourceCostResults) {
            string name = g_lang.getTranslatedTechName(it->getResourceType()->getName());
            string msg = name + " (" + intToStr(it->getCost()) + ")";
            if (!it->isCostMet(0)) {
                msg += " [-" + intToStr(it->getDifference(0)) + "]";
            }
            callback->addReq(it->isCostMet(0), it->getResourceType(), msg);
        }
    }
    if (!factionBuildCheckResult.m_resourceMadeResults.empty()) {
        callback->addElement(g_lang.get("Generated") + ":");
        foreach (ResourceMadeResults, it, factionBuildCheckResult.m_resourceMadeResults) {
            string name = g_lang.getTranslatedTechName(it->getResourceType()->getName());
            string msg = name + " (" + intToStr(it->getAmount()) + ")";
            callback->addItem(it->getResourceType(), msg);
        }
    }
}

void FactionBuild::build(const Faction *faction, const Vec2i &pos) const {
    Faction *fac = faction->getUnit(0)->getFaction();
    Map *map = g_world.getMap();
    if (map->canOccupy(pos, unitType->getField(), unitType, CardinalDir::NORTH)) {
        map->clearFoundation(unitType->foundation, pos, unitType->getSize(), unitType->getField());
        Unit *builtUnit = NULL;
        if (fac->applyCosts(unitType)) {
            builtUnit = g_world.newUnit(pos, unitType, fac, map, CardinalDir::NORTH);
            builtUnit->create();
        }
    } else {
        if (fac->getIndex() == g_world.getThisFactionIndex()) {
            g_console.addStdMessage("BuildingNoPlace");
        }
    }
}

}}//end namespace
