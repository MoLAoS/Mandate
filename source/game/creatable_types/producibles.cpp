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
#include "unit.h"
#include "item.h"
#include "world.h"
#include "sim_interface.h"
#include "tech_tree.h"
#include <cassert>
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Glest::Net;
using namespace Glest::Entities;

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class CreatedUnit
// =====================================================

void CreatedUnit::init(const XmlNode *node, const Faction *f) {
	m_type = f->getType()->getUnitType(node->getChildStringValue("type"));
	m_amount = node->getChildIntValue("amount");
	m_amount_plus = node->getChildIntValue("plus");
	m_amount_multiply = node->getChildFloatValue("multiply");
	m_cap = node->getChildIntValue("cap");
}

void CreatedUnit::save(XmlNode *node) const {
	node->addChild("type", m_type->getName());
	node->addChild("amount", m_amount);
	node->addChild("plus", m_amount_plus);
	node->addChild("multiply", m_amount_multiply);
	node->addChild("cap", m_cap);
}

void CreatedUnit::init(const UnitType *ut, int amount, int amount_plus, fixed amount_multiply, int cap) {
    m_type = ut;
    m_amount = amount;
    m_amount_plus = amount_plus;
    m_amount_multiply = amount_multiply;
    m_cap = cap;
}

// =====================================================
// 	class CreatedItem
// =====================================================

void CreatedItem::init(const XmlNode *node, const Faction *f) {
	m_type = f->getType()->getItemType(node->getChildStringValue("type"));
	m_amount = node->getChildIntValue("amount");
	m_amount_plus = node->getChildIntValue("plus");
	m_amount_multiply = node->getChildFloatValue("multiply");
	m_cap = node->getChildIntValue("cap");
}

void CreatedItem::save(XmlNode *node) const {
	node->addChild("type", m_type->getName());
	node->addChild("amount", m_amount);
	node->addChild("plus", m_amount_plus);
	node->addChild("multiply", m_amount_multiply);
	node->addChild("cap", m_cap);
}

void CreatedItem::init(const ItemType *it, int amount, int amount_plus, fixed amount_multiply, int cap) {
    m_type = it;
    m_amount = amount;
    m_amount_plus = amount_plus;
    m_amount_multiply = amount_multiply;
    m_cap = cap;
}

// =====================================================
// 	Helper Classes For Process
// =====================================================

void ResMade::init(const XmlNode *n, const TechTree *tt) {
	ResourceAmount::init(n, tt);
	m_product = n->getChildIntValue("product");
}

void ResMade::init(const ResourceType *rt, string n, int v, int a, fixed b) {
	ResourceAmount::init(rt, v, a, b);
    m_product = 0;
    m_name = n;
}

void ResMade::save(XmlNode *n) const {
	ResourceAmount::save(n);
	n->addChild("product", m_product);
}

void ResCost::init(const XmlNode *n, const TechTree *tt) {
	ResourceAmount::init(n, tt);
}

void ResCost::init(const ResourceType *rt, bool consume, int v, int a, fixed b) {
	ResourceAmount::init(rt, v, a, b);
	m_consume = consume;
}

void ResCost::save(XmlNode *n) const {
	ResourceAmount::save(n);
}

void ItemMade::init(const XmlNode *n, const TechTree *tt) {
}

void ItemMade::init(const ItemType *it, int v, int a, fixed b) {
    CreatedItem::init(it, v, a, b, 0);
}

void ItemMade::save(XmlNode *n) const {
    CreatedItem::save(n);
}

void ResBonus::init() {
    count = 0;
}

// ===============================
// 	class ResourceStore
// ===============================
void ResourceStore::init(string status, const ResourceType *rt, int required) {
    ResourceAmount::init(rt, required, 0, 0);
    m_status = status;
}

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

void ResourceProductionSystem::update(CreatedResource cr, Unit *unit, int timer, TimerStep *timerStep) const {
    const ResourceType *srt;
    Faction *faction = unit->getFaction();
    for (int s = 0; s < unit->getType()->getResourceProductionSystem()->getStoredResourceCount(); ++s) {
        srt = unit->getType()->getResourceProductionSystem()->getStoredResource(s, faction).getType();
        if (srt == cr.getType()) {
            break;
        }
    }
    int cTimeStep = timerStep->currentStep;
    int newStep = cTimeStep + 1;
    timerStep->currentStep = newStep;
    int cRNewTime = timerStep->currentStep;
    if (cRNewTime == timer) {
        if (srt->getClass() == ResourceClass::TECHTREE || srt->getClass() == ResourceClass::TILESET) {
            int balance = cr.getAmount();
            if (cr.getScope() == false) {
                unit->getFaction()->incResourceAmount(srt, balance);
            } else {
                unit->incResourceAmount(srt, balance);
            }
        }
        timerStep->currentStep = 0;
    }
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

void ItemProductionSystem::update(CreatedItem ci, Unit *unit, int timer, TimerStep *timerStep) const {
    Faction *faction = unit->getFaction();
    int cTimeStep = timerStep->currentStep;
    int newStep = cTimeStep + 1;
    timerStep->currentStep = newStep;
    int cRNewTime = timerStep->currentStep;
    if (cRNewTime == timer) {
        int iua = ci.getAmount();
        int iucap = ci.getCap();
        if (iucap != -1) {
            if (iucap < iua) {
                iua = iucap;
            }
        }
        for (int n = 0; n < iua; ++n) {
            if (unit->getItemLimit() > unit->getItemsStored()) {
                Item *newItem = g_world.newItem(unit->getFaction()->getItemCount(), ci.getType(), faction);
                unit->getFaction()->addItem(newItem);
                unit->accessStorageAdd(unit->getFaction()->getItemCount()-1);
            }
        }
        timerStep->currentStep = 0;
    }
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
                int count = processNode->getAttribute("count")->getIntValue();
                int steps = processNode->getAttribute("timer")->getIntValue();
                string range = processNode->getAttribute("scope")->getRestrictedValue();
                bool scope;
                if (range == "local") {
                    scope = true;
                } else {
                    scope = false;
                }
                processes[i].setCount(count);
                processes[i].setScope(scope);
                processTimers[i].init(steps, 0);
                const XmlNode *costsNode = processNode->getChild("costs");
                processes[i].costs.resize(costsNode->getChildCount());
                for (int j = 0; j < processes[i].costs.size(); ++j) {
                const XmlNode *costNode = costsNode->getChild("cost", j);
                string name = costNode->getAttribute("name")->getRestrictedValue();
                int amount = costNode->getAttribute("amount")->getIntValue();
                bool consume = costNode->getAttribute("consume")->getBoolValue();
                processes[i].costs[j].init(techTree->getResourceType(name), consume, amount, 0, 0);
                }
                const XmlNode *bonusesNode = processNode->getChild("bonuses", 0, false);
                if (bonusesNode) {
                processes[i].bonuses.resize(bonusesNode->getChildCount());
                for (int j = 0; j < processes[i].bonuses.size(); ++j) {
                const XmlNode *bonusNode = bonusesNode->getChild("cost", j);
                string costName = bonusNode->getAttribute("cost-name")->getRestrictedValue();
                string prodName = bonusNode->getAttribute("prod-name")->getRestrictedValue();
                int count = bonusNode->getAttribute("count")->getIntValue();
                int amount = bonusNode->getAttribute("amount")->getIntValue();
                int plus = bonusNode->getAttribute("plus")->getIntValue();
                fixed multiply = bonusNode->getAttribute("multiply")->getIntValue();
                bool consume = bonusNode->getAttribute("consume")->getBoolValue();
                processes[i].bonuses[j].count = count;
                processes[i].bonuses[j].cost.init(techTree->getResourceType(costName), consume, amount, 0, 0);
                processes[i].bonuses[j].product.init(techTree->getResourceType(prodName), prodName, 0, plus, multiply);
                }
                }
                try {
                    const XmlNode *productsNode = processNode->getChild("products");
                    processes[i].products.resize(productsNode->getChildCount());
                    for (int k = 0; k < processes[i].products.size(); ++k) {
                        const XmlNode *productNode = productsNode->getChild("product", k);
                        string name = productNode->getAttribute("name")->getRestrictedValue();
                        int amount = productNode->getAttribute("amount")->getIntValue();
                        processes[i].products[k].init(techTree->getResourceType(name), name, amount, 0, 0);
                    }
                } catch (runtime_error e) {
                    g_logger.logXmlError(dir, e.what());
                    loadOk = false;
                }
                try {
                    const XmlNode *itemsNode = processNode->getChild("items", 0, false);
                    if (itemsNode) {
                        processes[i].items.resize(itemsNode->getChildCount());
                        for (int l = 0; l < processes[i].items.size(); ++l) {
                            const XmlNode *itemNode = itemsNode->getChild("item", l);
                            string name = itemNode->getAttribute("name")->getRestrictedValue();
                            int amount = itemNode->getAttribute("amount")->getIntValue();
                            processes[i].items[l].init(factionType->getItemType(name), amount, 0, 0);
                        }
                    }
                } catch (runtime_error e) {
                    g_logger.logXmlError(dir, e.what());
                    loadOk = false;
                }
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

void ProcessProductionSystem::update(Process process, Unit *unit, int timer, TimerStep *timerStep) const {
    vector<int> counts;
    Faction *faction = unit->getFaction();
    int cTimeStep = timerStep->currentStep;
    int newStep = cTimeStep + 1;
    timerStep->currentStep = newStep;
    int cRNewTime = timerStep->currentStep;
    if (cRNewTime == timer) {
        int count = 0;
        int bonusCount = 0;
        for (int t = 0; t < process.costs.size(); ++t) {
            const ResourceType *costsRT = process.costs[t].getType();
            int stockpile = 0;
            if (process.getScope() == false) {
                for (int i = 0; i < unit->getFaction()->sresources.size(); ++i) {
                    if (costsRT == unit->getFaction()->sresources[i].getType()) {
                        stockpile = unit->getFaction()->getSResource(costsRT)->getAmount();
                    }
                }
            } else if (process.getScope() == true) {
                for (int i = 0; i < unit->sresources.size(); ++i) {
                    if (costsRT == unit->sresources[i].getType()) {
                        stockpile = unit->getSResource(costsRT)->getAmount();
                    }
                }
            }
            for (int g = 0; g < process.count; ++g) {
                int expended = process.costs[t].getAmount();
                if (expended > stockpile) {
                    break;
                } else {
                    ++count;
                    stockpile -= expended;
                }
            }
        }
        counts.resize(process.bonuses.size());
        for (int t = 0; t < process.bonuses.size(); ++t) {
            const ResourceType *costsRT = process.bonuses[t].cost.getType();
            int stockpile = 0;
            if (process.getScope() == false) {
                for (int i = 0; i < unit->getFaction()->sresources.size(); ++i) {
                    if (costsRT == unit->getFaction()->sresources[i].getType()) {
                        stockpile = unit->getFaction()->getSResource(costsRT)->getAmount();
                    }
                }
            } else if (process.getScope() == true) {
                for (int i = 0; i < unit->sresources.size(); ++i) {
                    if (costsRT == unit->sresources[i].getType()) {
                        stockpile = unit->getSResource(costsRT)->getAmount();
                    }
                }
            }
            for (int g = 0; g < process.count; ++g) {
                for (int h = 0; h < process.bonuses[t].count; ++h) {
                    int expended = process.bonuses[t].cost.getAmount();
                    if (expended > stockpile) {
                        break;
                    } else {
                        ++counts[t];
                        stockpile -= expended;
                    }
                }
            }
        }
        for (int c = 0; c < process.bonuses.size(); ++c) {
            for (int d = 0; d < counts[c]; ++d) {
                const ResourceType *costsRT = process.bonuses[c].cost.getType();
                for (int i = 0; i < unit->getFaction()->sresources.size(); ++i) {
                    if (costsRT == unit->getFaction()->sresources[i].getType()) {
                        int expended = process.bonuses[c].cost.getAmount();
                        if (process.getScope() == false) {
                            if (process.bonuses[c].cost.getConsume() == true) {
                                unit->getFaction()->incResourceAmount(costsRT, -expended);
                            }
                        } else {
                            if (process.bonuses[c].cost.getConsume() == true) {
                                unit->incResourceAmount(costsRT, -expended);
                            }
                        }
                    }
                }
            }
        }
        for (int g = 0; g < count; ++g) {
            for (int c = 0; c < process.costs.size(); ++c) {
                const ResourceType *costsRT = process.costs[c].getType();
                for (int i = 0; i < unit->getFaction()->sresources.size(); ++i) {
                    if (costsRT == unit->getFaction()->sresources[i].getType()) {
                        int expended = process.costs[c].getAmount();
                        if (process.getScope() == false) {
                            if (process.costs[c].getConsume() == true) {
                                unit->getFaction()->incResourceAmount(costsRT, -expended);
                            }
                        } else {
                            if (process.costs[c].getConsume() == true) {
                                unit->incResourceAmount(costsRT, -expended);
                            }
                        }
                    }
                }
            }
            for (int p = 0; p < process.products.size(); ++p) {
                const ResourceType *productsRT = process.products[p].getType();
                for (int i = 0; i < unit->getFaction()->sresources.size(); ++i) {
                    if (productsRT == unit->getFaction()->sresources[i].getType()) {
                        fixed mult = 1;
                        int produced = process.products[p].getAmount();
                        for (int c = 0; c < process.bonuses.size(); ++c) {
                            if (process.bonuses[c].product.getType() == productsRT) {
                                if (counts[c] > 0) {
                                    mult += process.bonuses[c].product.getAmountMultiply();
                                    --counts[c];
                                }
                            }
                        }
                        produced = (produced * mult).intp();
                        for (int c = 0; c < process.bonuses.size(); ++c) {
                            if (process.bonuses[c].product.getType() == productsRT) {
                                if (counts[c] > 0) {
                                    produced = produced + process.bonuses[c].product.getAmountPlus();
                                    counts[c];
                                }
                            }
                        }
                        if (process.getScope() == false) {
                            unit->getFaction()->incResourceAmount(productsRT, produced);
                        } else {
                            unit->incResourceAmount(productsRT, produced);
                        }
                    }
                }
            }
            for (int t = 0; t < process.items.size(); ++t) {
                const ItemType *itemsIT = process.items[t].getType();
                int items = process.items[t].getAmount();
                if (process.getScope() == false) {
                } else {
                    for (int q = 0; q < items; ++q) {
                        if (unit->getItemLimit() > unit->getItemsStored()) {
                            Item *newItem = g_world.newItem(unit->getFaction()->getItemCount(), itemsIT, faction);
                            unit->getFaction()->addItem(newItem);
                            unit->accessStorageAdd(unit->getFaction()->getItemCount()-1);
                        }
                    }
                }
            }
        }
        timerStep->currentStep = 0;
    }
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
	return unit;
}

Timer UnitProductionSystem::getCreatedUnitTimer(int i, const Faction *f) const {
	Timer timer(createdUnitTimers[i]);
	return timer;
}

void UnitProductionSystem::update(CreatedUnit cu, Unit *unit, int timer, TimerStep *timerStep) const {
    Faction *faction = unit->getFaction();
    int cTimeStep = timerStep->currentStep;
    int newStep = cTimeStep + 1;
    timerStep->currentStep = newStep;
    int cRNewTime = timerStep->currentStep;
    if (cRNewTime == timer) {
        Vec2i locate = unit->getPos();
        int cua = cu.getAmount();
        int cucap = cu.getCap();
        if (cucap != -1) {
            if (cucap < cua) {
                cua = cucap;
            }
        }
        for (int l = 0; l < unit->ownedUnits.size(); ++l) {
            const UnitType *kind = unit->ownedUnits[l].getType();
            if (kind == cu.getType()) {
                int limit = unit->ownedUnits[l].getLimit();
                int owned = unit->ownedUnits[l].getOwned();
                if (owned == limit) {
                    cua = 0;
                } else if (cua > limit - owned) {
                    cua = limit - owned;
                } else {
                }
                break;
            }
        }
        for (int n = 0; n < cua; ++n) {
            Unit *createdUnit = g_world.newUnit(locate, cu.getType(), faction, g_world.getMap(), CardinalDir::NORTH);
            g_world.placeUnit(unit->getCenteredPos(), 10, createdUnit);
            createdUnit->setOwner(unit);
            for (int z = 0; z < unit->ownedUnits.size(); ++z) {
                if (unit->ownedUnits[z].getType() == createdUnit->getType()) {
                    unit->ownedUnits[z].incOwned();
                }
            }
            createdUnit->create();
            createdUnit->born();
            ScriptManager::onUnitCreated(createdUnit);
            g_simInterface.getStats()->produce(unit->getFactionIndex());
        }
        timerStep->currentStep = 0;
    }
}

}}//end namespace
