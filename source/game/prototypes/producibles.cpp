// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "producibles.h"

#include "resource.h"
#include "abilities.h"
#include "faction_type.h"
#include "faction.h"
#include "tech_tree.h"
#include <cassert>
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Glest::Net;
using namespace Glest::Entities;

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class ResourceProductionSystem
// =====================================================
bool ResourceProductionSystem::load(const XmlNode *resourceProductionNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
    bool loadOk = true;
    try {
        const XmlNode *resourcesStoredNode = resourceProductionNode->getChild("resources-stored", 0, false);
        if (resourcesStoredNode) {
            storedResources.resize(resourcesStoredNode->getChildCount());
            for(int i=0; i<storedResources.size(); ++i){
                const XmlNode *resourceNode= resourcesStoredNode->getChild("resource", i);
                string name= resourceNode->getAttribute("name")->getRestrictedValue();
                int amount= resourceNode->getAttribute("amount")->getIntValue();
                storedResources[i].init(techTree->getResourceType(name), amount, 0, 0);
            }
        }
    } catch (runtime_error e) {
    g_logger.logXmlError(dir, e.what());
    loadOk = false;
    }
    try {
        const XmlNode *starterResourcesNode = resourceProductionNode->getChild("starter-resources", 0, false);
        if (starterResourcesNode) {
            starterResources.resize(starterResourcesNode->getChildCount());
            for(int i=0; i<starterResources.size(); ++i){
                const XmlNode *resourceNode= starterResourcesNode->getChild("resource", i);
                string name= resourceNode->getAttribute("name")->getRestrictedValue();
                int amount= resourceNode->getAttribute("amount")->getIntValue();
                starterResources[i].init(techTree->getResourceType(name), amount, 0, 0);
            }
        }
    } catch (runtime_error e) {
    g_logger.logXmlError(dir, e.what());
    loadOk = false;
    }
    try {
        const XmlNode *resourcesCreatedNode = resourceProductionNode->getChild("resources-created", 0, false);
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
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
    return loadOk;
}

ResourceAmount ResourceProductionSystem::getCreatedResource(int i, const Faction *f) const {
	ResourceAmount res(createdResources[i]);
	//Modifier mod = f->getCreateModifier(this, res.getType());
	//res.setAmount((res.getAmount() * mod.getMultiplier()).intp() + mod.getAddition());
	return res;
}

Timer ResourceProductionSystem::getCreatedResourceTimer(int i, const Faction *f) const {
	Timer timer(createdResourceTimers[i]);
	return timer;
}

int ResourceProductionSystem::getCreate(const ResourceType *rt, const Faction *f) const {
    foreach_const (CreatedResources, it, createdResources) {
        if (it->getType() == rt) {
            //Modifier mod = f->getCreateModifier(this, rt);
            return (it->getAmount());
            // * mod.getMultiplier()).intp() + mod.getAddition();
        }
    }
    return 0;
}

int ResourceProductionSystem::getStore(const ResourceType *rt, const Faction *f) const {
    foreach_const (StoredResources, it, storedResources) {
        if (it->getType() == rt) {
            //Modifier mod = f->getStoreModifier(this, rt);
            return (it->getAmount());
            //* mod.getMultiplier()).intp() + mod.getAddition();
        }
    }
    return 0;
}

ResourceAmount ResourceProductionSystem::getStoredResource(int i, const Faction *f) const {
    ResourceAmount res(storedResources[i]);
    //Modifier mod = f->getStoreModifier(this, res.getType());
    res.setAmount((res.getAmount()));
    //* mod.getMultiplier()).intp() + mod.getAddition());
    return res;
}

// =====================================================
// 	class ItemProductionSystem
// =====================================================
bool ItemProductionSystem::load(const XmlNode *itemProductionNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
    bool loadOk = true;
    try {
        const XmlNode *itemsCreatedNode = itemProductionNode->getChild("items-created", 0, false);
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
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
    return loadOk;
}

int ItemProductionSystem::getCreateItem(const ItemType *it, const Faction *f) const {
	foreach_const (CreatedItems, it, createdItems) {
		//if (it->getType() == it) {
			//Modifier mod = f->getCreatedItemModifier(this, ut);
			//return (it->getAmount() * mod.getMultiplier()).intp() + mod.getAddition();
		//}
	}
	return 0;
}

CreatedItem ItemProductionSystem::getCreatedItem(int i, const Faction *f) const {
	CreatedItem item(createdItems[i]);
	//Modifier mod = f->getCreatedItemModifier(this, item.getType());
	//item.setAmount((item.getAmount() * mod.getMultiplier()).intp() + mod.getAddition());
	return item;
}

Timer ItemProductionSystem::getCreatedItemTimer(int i, const Faction *f) const {
	Timer timer(createdItemTimers[i]);
	return timer;
}

// =====================================================
// 	class ProcessProductionSystem
// =====================================================
bool ProcessProductionSystem::load(const XmlNode *processProductionNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
    bool loadOk = true;
    try {
        const XmlNode *processesNode = processProductionNode->getChild("processes", 0, false);
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
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
    return loadOk;
}

int ProcessProductionSystem::getProcessing(const ResourceType *rt, const Faction *f) const {
	foreach_const (Processes, it, processes) {
		//if (it->getType() == rt) {
			//Modifier mod = f->getCreateModifier(this, rt);
			//return (it->getAmount() * mod.getMultiplier()).intp() + mod.getAddition();
		//}
	}
	return 0;
}

Process ProcessProductionSystem::getProcess(int i, const Faction *f) const {
	Process proc(processes[i]);
	//Modifier mod = f->getCreateModifier(this, res.getType());
	//proc.setAmount((proc.getAmount() * mod.getMultiplier()).intp() + mod.getAddition());
	return proc;
}

Timer ProcessProductionSystem::getProcessTimer(int i, const Faction *f) const {
	Timer timer(processTimers[i]);
	return timer;
}

// =====================================================
// 	class UnitProductionSystem
// =====================================================
bool UnitProductionSystem::load(const XmlNode *unitProductionNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
    bool loadOk = true;
    try {
        const XmlNode *unitsCreatedNode = unitProductionNode->getChild("units-created", 0, false);
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
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
    return loadOk;
}

int UnitProductionSystem::getCreateUnit(const UnitType *ut, const Faction *f) const {
	foreach_const (CreatedUnits, it, createdUnits) {
		if (it->getType() == ut) {
			//Modifier mod = f->getCreatedUnitModifier(this, ut);
			return (it->getAmount());
			// * mod.getMultiplier()).intp() + mod.getAddition();
		}
	}
	return 0;
}

CreatedUnit UnitProductionSystem::getCreatedUnit(int i, const Faction *f) const {
	CreatedUnit unit(createdUnits[i]);
	//Modifier mod = f->getCreatedUnitModifier(this, unit.getType());
	//unit.setAmount((unit.getAmount() * mod.getMultiplier()).intp() + mod.getAddition());
	return unit;
}

Timer UnitProductionSystem::getCreatedUnitTimer(int i, const Faction *f) const {
	Timer timer(createdUnitTimers[i]);
	return timer;
}

}}//end namespace
