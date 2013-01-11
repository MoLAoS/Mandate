// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
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
Item::Item(CreateParams params)
		: id(params.ident)
		, type(params.type)
		, faction(params.faction) {
    currentSteps.resize(type->getResourceProductionSystem().getCreatedResourceCount());
	for (int i = 0; i < currentSteps.size(); ++i) {
	currentSteps[i].currentStep = 0;
	}

	currentUnitSteps.resize(type->getUnitProductionSystem().getCreatedUnitCount());
	for (int i = 0; i < currentUnitSteps.size(); ++i) {
	currentUnitSteps[i].currentStep = 0;
	}

	currentItemSteps.resize(type->getItemProductionSystem().getCreatedItemCount());
	for (int i = 0; i < currentItemSteps.size(); ++i) {
	currentItemSteps[i].currentStep = 0;
	}

	currentProcessSteps.resize(type->getProcessProductionSystem().getProcessCount());
	for (int i = 0; i < currentProcessSteps.size(); ++i) {
	currentProcessSteps[i].currentStep = 0;
	}
}

Item::~Item() {
	if (!g_program.isTerminating() && World::isConstructed()) {
	}
}

void Item::init(int ident, const ItemType *newType, Faction *f) {
    type = newType;
    faction = f;
    id = ident;

    currentSteps.resize(type->getResourceProductionSystem().getCreatedResourceCount());
	for (int i = 0; i < currentSteps.size(); ++i) {
	currentSteps[i].currentStep = 0;
	}

	currentUnitSteps.resize(type->getUnitProductionSystem().getCreatedUnitCount());
	for (int i = 0; i < currentUnitSteps.size(); ++i) {
	currentUnitSteps[i].currentStep = 0;
	}

	currentItemSteps.resize(type->getItemProductionSystem().getCreatedItemCount());
	for (int i = 0; i < currentItemSteps.size(); ++i) {
	currentItemSteps[i].currentStep = 0;
	}

	currentProcessSteps.resize(type->getProcessProductionSystem().getProcessCount());
	for (int i = 0; i < currentProcessSteps.size(); ++i) {
	currentProcessSteps[i].currentStep = 0;
	}
}

string Item::getShortDesc() const {
	stringstream ss;

	return ss.str();
}

string Item::getLongDesc() const {
	Lang &lang = g_lang;
	World &world = g_world;
	string shortDesc = getShortDesc();
	string str;
	stringstream ss;

	const string factionName = type->getFactionType()->getName();

	ss << "Weapon Class: " << getType()->getTypeTag();
	ss << endl << "Quality Tier: " << getType()->getQualityTier();
	ss << endl << "Bonuses: ";
	getDesc(str, "\n");
    ss << str;
	if (type->getResistances()->size() > 0) {
	ss << endl << lang.get("Resistances") << ":";
	for (int i = 0; i < type->getResistances()->size(); ++i) {
	ss << endl << lang.get(type->getResistance(i)->getTypeName()) << ": ";
	ss << type->getResistance(i)->getValue();
	}
	ss << endl;
	}
	// can create resources
	if (type->getResourceProductionSystem().getCreatedResourceCount() > 0) {
		for (int i = 0; i < type->getResourceProductionSystem().getCreatedResourceCount(); ++i) {
			ResourceAmount r = type->getResourceProductionSystem().getCreatedResource(i, getFaction());
			string resName = lang.getTechString(r.getType()->getName());
			Timer tR = type->getResourceProductionSystem().getCreatedResourceTimer(i, getFaction());
			int cStep = currentSteps[i].currentStep;
			if (resName == r.getType()->getName()) {
				resName = formatString(resName);
			}
			ss << endl << lang.get("Create") << ": ";
			ss << r.getAmount() << " " << resName << " " << lang.get("Timer") << ": " << cStep << "/" << tR.getTimerValue();
		}
	}
	// can process
    if (type->getProcessProductionSystem().getProcessCount() > 0) {
		for (int i = 0; i < type->getProcessProductionSystem().getProcessCount(); ++i) {
        ss << endl << lang.get("Process") << ": ";
        Timer tR = type->getProcessProductionSystem().getProcessTimer(i, getFaction());
        int cStep = currentProcessSteps[i].currentStep;
        ss << endl << lang.get("Timer") << ": " << cStep << "/" << tR.getTimerValue();
        string scope;
        if (type->getProcessProductionSystem().getProcesses()[i].getScope() == true) {
        scope = lang.get("local");
        } else {
        scope = lang.get("faction");
        }
        ss << endl << lang.get("Scope") << ": " << scope;
        ss << endl << lang.get("Costs") << ": ";
            for (int c = 0; c < type->getProcessProductionSystem().getProcesses()[i].costs.size(); ++c) {
            const ResourceType *costsRT = type->getProcessProductionSystem().getProcesses()[i].costs[c].getType();
            string resName = lang.getTechString(costsRT->getName());
            if (resName == costsRT->getName()) {
            resName = formatString(resName);
			}
        ss << endl << type->getProcessProductionSystem().getProcesses()[i].costs[c].getAmount() << " " << resName;
            }
        ss << endl << lang.get("Products") << ": ";
            for (int p = 0; p < type->getProcessProductionSystem().getProcesses()[i].products.size(); ++p) {
            const ResourceType *productsRT = type->getProcessProductionSystem().getProcesses()[i].products[p].getType();
            string resName = lang.getTechString(productsRT->getName());
            if (resName == productsRT->getName()) {
            resName = formatString(resName);
			}
        ss << endl << type->getProcessProductionSystem().getProcesses()[i].products[p].getAmount() << " " << resName;
            }
        ss << endl << lang.get("Items") << ": ";
            for (int t = 0; t < type->getProcessProductionSystem().getProcesses()[i].items.size(); ++t) {
            const ItemType *itemsIT = type->getProcessProductionSystem().getProcesses()[i].items[t].getType();
            string itemName = lang.getTechString(itemsIT->getName());
            if (itemName == itemsIT->getName()) {
            itemName = formatString(itemName);
			}
        ss << endl << type->getProcessProductionSystem().getProcesses()[i].items[t].getAmount() << " " << itemName;
            }
		}
	}
	// can create units
	if (type->getUnitProductionSystem().getCreatedUnitCount() > 0) {
		for (int i = 0; i < type->getUnitProductionSystem().getCreatedUnitCount(); ++i) {
			CreatedUnit u = type->getUnitProductionSystem().getCreatedUnit(i, getFaction());
			string unitName = lang.getTechString(u.getType()->getName());
			Timer tR = type->getUnitProductionSystem().getCreatedUnitTimer(i, getFaction());
			int cUStep = currentUnitSteps[i].currentStep;
			if (unitName == u.getType()->getName()) {
				unitName = formatString(unitName);
			}
			ss << endl << lang.get("Create") << ": ";
			ss << u.getAmount() << " " << unitName << "s " <<lang.get("Timer") << ": " << cUStep << "/" << tR.getTimerValue();
		}
	}
	// can create units
	if (type->getItemProductionSystem().getCreatedItemCount() > 0) {
		for (int i = 0; i < type->getItemProductionSystem().getCreatedItemCount(); ++i) {
			CreatedItem item = type->getItemProductionSystem().getCreatedItem(i, getFaction());
			string itemName = lang.getTechString(item.getType()->getName());
			Timer tR = type->getItemProductionSystem().getCreatedItemTimer(i, getFaction());
			int cIStep = currentItemSteps[i].currentStep;
			if (itemName == item.getType()->getName()) {
				itemName = formatString(itemName);
			}
			ss << endl << lang.get("Create") << ": ";
			ss << item.getAmount() << " " << itemName << "s " <<lang.get("Timer") << ": " << cIStep << "/" << tR.getTimerValue();
		}
	}

	if (type->getOwnedUnits().size() > 0) {
		for (int i = 0; i < type->getOwnedUnits().size(); ++i) {
			UnitsOwned uo = type->getOwnedUnits()[i];
			const UnitType *uot = uo.getType();
			string unitName = lang.getTechString(uot->getName());
			int owned = uo.getOwned();
			int limit = uo.getLimit();
			if (unitName == uo.getType()->getName()) {
				unitName = formatString(unitName);
			}
			ss << endl << lang.get("Owned") << ": ";
			ss << endl << unitName << ": " << uo.getLimit();
		}
	}
	// effects
	//effects.streamDesc(ss);

	return (ss.str());
}

// =====================================================
//  class ItemFactory
// =====================================================
Item* ItemFactory::newItem(int ident, const ItemType* type, Faction *faction) {
	//Item item;
	//item.init(ident, type, faction);
	//items.push_back(item);
	//return &items[items.size()-1];
	Item::CreateParams params(ident, type, faction);
	Item *item = new Item(params);
	return item;
}

}}//end namespace
