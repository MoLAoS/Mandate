// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "item_type.h"

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

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class ItemType
// =====================================================

void ItemType::preLoad(const string &dir){
	m_name = basename(dir);
}

bool ItemType::load(const string &dir, const TechTree *techTree, const FactionType *factionType) {
	g_logger.logProgramEvent("Item type: " + dir, true);
	bool loadOk = true;

	m_factionType = factionType;
	string path = dir + "/" + m_name + ".xml";

	name = m_name;

	XmlTree xmlTree;
	try { xmlTree.load(path); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		g_logger.logError("Fatal Error: could not load " + path);
		return false; // bail
	}
	const XmlNode *itemNode;
	try { itemNode = xmlTree.getRootNode(); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}
	const XmlNode *parametersNode;
	try { parametersNode = itemNode->getChild("parameters"); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false; // bail out
	}
	try {
        const XmlNode *typeTagNode = parametersNode->getChild("type-tag");
        if (typeTagNode) {
            typeTag = typeTagNode->getAttribute("type")->getRestrictedValue();
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *tierNode = parametersNode->getChild("tier");
        if (tierNode) {
            qualityTier = tierNode->getAttribute("value")->getIntValue();
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	try {
        const XmlNode *enhancementNode = parametersNode->getChild("enhancement");
        if (!EnhancementType::load(enhancementNode, dir, techTree, factionType)) {
            loadOk = false;
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	// images, requirements, costs...
	if (!ProducibleType::load(parametersNode, dir, techTree, factionType)) {
		loadOk = false;
	}
    // load resistances
	try {
	    const XmlNode *resistancesNode = parametersNode->getChild("resistances", 0, false);
	    if (resistancesNode) {
	        resistances.resize(resistancesNode->getChildCount());
            for (int i = 0; i < resistancesNode->getChildCount(); ++i) {
                const XmlNode *resistanceNode = resistancesNode->getChild("resistance", i);
                string resistanceTypeName = resistanceNode->getAttribute("type")->getRestrictedValue();
                int amount = resistanceNode->getAttribute("value")->getIntValue();
                resistances[i].init(resistanceTypeName, amount);
            }
	    }
	}
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
    // resources processed
    try {
        const XmlNode *processesNode= parametersNode->getChild("processes", 0, false);
        if (processesNode) {
            processes.resize(processesNode->getChildCount());
            processTimers.resize(processesNode->getChildCount());
            for(int i = 0; i < processes.size(); ++i){
                const XmlNode *processNode = processesNode->getChild("process", i);
                string range = processNode->getAttribute("scope")->getRestrictedValue();
                bool scope;
                if (range == "local") {
                    scope = true;
                } else {
                    scope = false;
                }
                processes[i].setScope(scope);
                const XmlNode *costsNode = processNode->getChild("costs");
                processes[i].costs.resize(costsNode->getChildCount());
                for (int j = 0; j < processes[i].costs.size(); ++j) {
                const XmlNode *costNode = costsNode->getChild("cost", j);
                string name = costNode->getAttribute("name")->getRestrictedValue();
                int amount = costNode->getAttribute("amount")->getIntValue();
                processes[i].costs[j].init(techTree->getResourceType(name), name, amount, 0, 0);
                }
                const XmlNode *productsNode = processNode->getChild("products");
                processes[i].products.resize(productsNode->getChildCount());
                for (int k = 0; k < processes[i].products.size(); ++k) {
                const XmlNode *productNode = productsNode->getChild("product", k);
                string name = productNode->getAttribute("name")->getRestrictedValue();
                int amount = productNode->getAttribute("amount")->getIntValue();
                processes[i].products[k].init(techTree->getResourceType(name), name, amount, 0, 0);
                }
                int steps = processNode->getAttribute("timer")->getIntValue();
                processTimers[i].init(steps, 0);
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
    // Resources created
    try {
        const XmlNode *resourcesCreatedNode= parametersNode->getChild("resources-created", 0, false);
        if (resourcesCreatedNode) {
            createdResources.resize(resourcesCreatedNode->getChildCount());
            createdResourceTimers.resize(resourcesCreatedNode->getChildCount());
            for(int i = 0; i < createdResources.size(); ++i){
                const XmlNode *resourceNode = resourcesCreatedNode->getChild("resource", i);
                string range = resourceNode->getAttribute("scope")->getRestrictedValue();
                bool scope;
                if (range == "local") {
                    scope = true;
                } else if (range == "faction") {
                    scope = false;
                }
                createdResources[i].setScope(scope);
                string name = resourceNode->getAttribute("name")->getRestrictedValue();
                int amount = resourceNode->getAttribute("amount")->getIntValue();
                int steps = resourceNode->getAttribute("timer")->getIntValue();
                createdResources[i].init(techTree->getResourceType(name), amount);
                createdResourceTimers[i].init(steps, 0);
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
    // units created
    try {
        const XmlNode *unitsCreatedNode= parametersNode->getChild("units-created", 0, false);
        if (unitsCreatedNode) {
            createdUnits.resize(unitsCreatedNode->getChildCount());
            createdUnitTimers.resize(unitsCreatedNode->getChildCount());
            for(int i = 0; i<createdUnits.size(); ++i){
                const XmlNode *unitNode = unitsCreatedNode->getChild("unit", i);
                string name = unitNode->getAttribute("name")->getRestrictedValue();
                int amount = unitNode->getAttribute("amount")->getIntValue();
                int steps = unitNode->getAttribute("timer")->getIntValue();
                createdUnits[i].init(factionType->getUnitType(name), amount, 0, 0, -1);
                createdUnitTimers[i].init(steps, 0);
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
    // items created
    try {
        const XmlNode *itemsCreatedNode= parametersNode->getChild("items-created", 0, false);
        if (itemsCreatedNode) {
            createdItems.resize(itemsCreatedNode->getChildCount());
            createdItemTimers.resize(itemsCreatedNode->getChildCount());
            for(int i = 0; i<createdItems.size(); ++i){
                const XmlNode *itemNode = itemsCreatedNode->getChild("item", i);
                string name = itemNode->getAttribute("name")->getRestrictedValue();
                int amount = itemNode->getAttribute("amount")->getIntValue();
                int steps = itemNode->getAttribute("timer")->getIntValue();
                createdItems[i].init(factionType->getItemType(name), amount, 0, 0, -1);
                createdItemTimers[i].init(steps, 0);
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
    return loadOk;
}

int ItemType::getCreate(const ResourceType *rt, const Faction *f) const {
    foreach_const (CreatedResources, it, createdResources) {
        if (it->getType() == rt) {
            //Modifier mod = f->getCreateModifier(this, rt);
            return (it->getAmount());
            // * mod.getMultiplier()).intp() + mod.getAddition();
        }
    }
    return 0;
}


ResourceAmount ItemType::getCreatedResource(int i, const Faction *f) const {
	ResourceAmount res(createdResources[i]);
	//Modifier mod = f->getCreateModifier(this, res.getType());
	//res.setAmount((res.getAmount() * mod.getMultiplier()).intp() + mod.getAddition());
	return res;
}

Timer ItemType::getCreatedResourceTimer(int i, const Faction *f) const {
	Timer timer(createdResourceTimers[i]);
	return timer;
}

int ItemType::getProcessing(const ResourceType *rt, const Faction *f) const {
	foreach_const (Processes, it, processes) {
		//if (it->getType() == rt) {
			//Modifier mod = f->getCreateModifier(this, rt);
			//return (it->getAmount() * mod.getMultiplier()).intp() + mod.getAddition();
		//}
	}
	return 0;
}

Process ItemType::getProcess(int i, const Faction *f) const {
	Process proc(processes[i]);
	//Modifier mod = f->getCreateModifier(this, res.getType());
	//proc.setAmount((proc.getAmount() * mod.getMultiplier()).intp() + mod.getAddition());
	return proc;
}

Timer ItemType::getProcessTimer(int i, const Faction *f) const {
	Timer timer(processTimers[i]);
	return timer;
}

int ItemType::getCreateUnit(const UnitType *ut, const Faction *f) const {
	foreach_const (CreatedUnits, it, createdUnits) {
		if (it->getType() == ut) {
			//Modifier mod = f->getCreatedUnitModifier(this, ut);
			return (it->getAmount());
			// * mod.getMultiplier()).intp() + mod.getAddition();
		}
	}
	return 0;
}

CreatedUnit ItemType::getCreatedUnit(int i, const Faction *f) const {
	CreatedUnit unit(createdUnits[i]);
	//Modifier mod = f->getCreatedUnitModifier(this, unit.getType());
	//unit.setAmount((unit.getAmount() * mod.getMultiplier()).intp() + mod.getAddition());
	return unit;
}

Timer ItemType::getCreatedUnitTimer(int i, const Faction *f) const {
	Timer timer(createdUnitTimers[i]);
	return timer;
}

int ItemType::getCreateItem(const ItemType *it, const Faction *f) const {
	foreach_const (CreatedItems, it, createdItems) {
		//if (it->getType() == it) {
			//Modifier mod = f->getCreatedItemModifier(this, ut);
			//return (it->getAmount() * mod.getMultiplier()).intp() + mod.getAddition();
		//}
	}
	return 0;
}

CreatedItem ItemType::getCreatedItem(int i, const Faction *f) const {
	CreatedItem item(createdItems[i]);
	//Modifier mod = f->getCreatedItemModifier(this, item.getType());
	//item.setAmount((item.getAmount() * mod.getMultiplier()).intp() + mod.getAddition());
	return item;
}

Timer ItemType::getCreatedItemTimer(int i, const Faction *f) const {
	Timer timer(createdItemTimers[i]);
	return timer;
}

}}//end namespace
