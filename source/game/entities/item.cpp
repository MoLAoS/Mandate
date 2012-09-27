// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti?o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "item.h"

#include <cassert>

#include "unit.h"
#include "world.h"
#include "upgrade.h"
#include "map.h"
#include "command.h"
#include "object.h"
#include "config.h"
#include "skill_type.h"
#include "core_data.h"
#include "renderer.h"
#include "script_manager.h"
#include "cartographer.h"
#include "game.h"
#include "earthquake_type.h"
#include "sound_renderer.h"
#include "sim_interface.h"
#include "user_interface.h"
#include "route_planner.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Glest::Net;

namespace Glest { namespace Entities {
using namespace ProtoTypes;

// =====================================================
// 	class Item
// =====================================================

	/*currentSteps.resize(type->getCreatedResourceCount());
	for (int i = 0; i < currentSteps.size(); ++i) {
	currentSteps[i].currentStep = 0;
	}

	currentUnitSteps.resize(type->getCreatedUnitCount());
	for (int i = 0; i < currentUnitSteps.size(); ++i) {
	currentUnitSteps[i].currentStep = 0;
	}

	currentProcessSteps.resize(type->getProcessCount());
	for (int i = 0; i < currentProcessSteps.size(); ++i) {
	currentProcessSteps[i].currentStep = 0;
	}*/

string Item::getShortDesc() const {
	stringstream ss;

	return ss.str();
}

string Item::getLongDesc() const {
	Lang &lang = g_lang;
	World &world = g_world;
	string shortDesc = getShortDesc();
	stringstream ss;

	const string factionName = type->getFactionType()->getName();
	int armorBonus = getArmor() - type->getArmor();
	// armor
	ss << endl << lang.get("Armor") << ": " << type->getArmor();
	if (armorBonus) {
		ss << (armorBonus > 0 ? " +" : " ") << armorBonus;
	}
	string armourName = lang.getTechString(type->getArmourType()->getName());
	if (armourName == type->getArmourType()->getName()) {
		armourName = formatString(armourName);
	}
	ss << " (" << armourName << ")";
	if (type->resistances.size() > 0) {
	ss << endl << lang.get("Resistances") << ":";
	for (int i = 0; i < type->resistances.size(); ++i) {
	ss << endl << lang.get(type->resistances[i].getTypeName()) << ": ";
	ss << type->resistances[i].getValue();
	}
	ss << endl;
	}
	// can create resources
	if (type->getCreatedResourceCount() > 0) {
		for (int i = 0; i < type->getCreatedResourceCount(); ++i) {
			ResourceAmount r = type->getCreatedResource(i, getFaction());
			string resName = lang.getTechString(r.getType()->getName());
			Timer tR = type->getCreatedResourceTimer(i, getFaction());
			int cStep = currentSteps[i].currentStep;
			if (resName == r.getType()->getName()) {
				resName = formatString(resName);
			}
			ss << endl << lang.get("Create") << ": ";
			ss << r.getAmount() << " " << resName << " " << lang.get("Timer") << ": " << cStep << "/" << tR.getTimerValue();
		}
	}
	// can process
    if (type->getProcessCount() > 0) {
		for (int i = 0; i < type->getProcessCount(); ++i) {
        ss << endl << lang.get("Process") << ": ";
        Timer tR = type->getProcessTimer(i, getFaction());
        int cStep = currentProcessSteps[i].currentStep;
        ss << endl << lang.get("Timer") << ": " << cStep << "/" << tR.getTimerValue();
        string scope;
        if (type->processes[i].local == true) {
        scope = lang.get("local");
        } else {
        scope = lang.get("faction");
        }
        ss << endl << lang.get("Scope") << ": " << scope;
        ss << endl << lang.get("Costs") << ": ";
            for (int c = 0; c < type->processes[i].costs.size(); ++c) {
            const ResourceType *costsRT = type->processes[i].costs[c].getType();
            string resName = lang.getTechString(costsRT->getName());
            if (resName == costsRT->getName()) {
            resName = formatString(resName);
			}
        ss << endl << type->processes[i].costs[c].getAmount() << " " << resName;
            }
        ss << endl << lang.get("Products") << ": ";
            for (int p = 0; p < type->processes[i].products.size(); ++p) {
            const ResourceType *productsRT = type->processes[i].products[p].getType();
            string resName = lang.getTechString(productsRT->getName());
            if (resName == productsRT->getName()) {
            resName = formatString(resName);
			}
        ss << endl << type->processes[i].products[p].getAmount() << " " << resName;
            }
		}
	}
	// can create units
	if (type->getCreatedUnitCount() > 0) {
		for (int i = 0; i < type->getCreatedUnitCount(); ++i) {
			CreatedUnit u = type->getCreatedUnit(i, getFaction());
			string unitName = lang.getTechString(u.getType()->getName());
			Timer tR = type->getCreatedUnitTimer(i, getFaction());
			int cUStep = currentUnitSteps[i].currentStep;
			if (unitName == u.getType()->getName()) {
				unitName = formatString(unitName);
			}
			ss << endl << lang.get("Create") << ": ";
			ss << u.getAmount() << " " << unitName << "s " <<lang.get("Timer") << ": " << cUStep << "/" << tR.getTimerValue();
		}
	}
	// effects
	//effects.streamDesc(ss);

	return (shortDesc + ss.str());
}

}}//end namespace
