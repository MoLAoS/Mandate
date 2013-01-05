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

void FactionBuild::subDesc(const Faction *faction, TradeDescriptor *callback, const UnitType *ut) const {
    //Lang &lang = g_lang;
    //callback->addElement("Build: ");
    //callback->addItem(ut, getUnitType()->getName());
}

void FactionBuild::describe(const Faction *faction, TradeDescriptor *callback, const UnitType *ut) const {
	//callback->setHeader("");
	//callback->setTipText("");
	//callback->addItem(ut, "");
	subDesc(faction, callback, ut);
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
