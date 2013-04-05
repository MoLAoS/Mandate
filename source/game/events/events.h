// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_EVENTS_H_
#define _GLEST_GAME_EVENTS_H_

#include "traits.h"
#include "world.h"

namespace Glest{ namespace ProtoTypes {
// =====================================================
// 	class Event Helper Classes
// =====================================================
class Item {
private:
    ItemType* itemType;
    int amount;
public:
    ItemType *getItemType() {return itemType;}
    int getAmount() {return amount;}
    void init(const ItemType* type, int value);
};

class Unit {
private:
    UnitType* unitType;
    int amount;
public:
    UnitType *getUnitType() {return unitType;}
    int getAmount() {return amount;}
    void init(const UnitType* type, int value);
};

class Upgrade {
private:
    UpgradeType* upgradeType;
    int stage;
public:
    UpgradeType *getUpgradeType() {return upgradeType;}
    int getStage() {return stage;}
    void init(const UpgradeType* type, int value);
};

class Triggers {
private:
public:

};

// =====================================================
// 	class Events
// =====================================================
class Event : public RequirableType {
private:
    typedef vector<Modification*> Modifications;
    typedef vector<string> ModifyNames;
    typedef vector<ResourceAmount> Resources;
    typedef vector<Upgrade> UpgradeList;
    typedef vector<Unit> UnitList;
    typedef vector<Item> ItemList;
    typedef vector<Equipment> Gear;
    typedef vector<Trait*> Traits;
    Modifications modifications;
    ModifyNames modifyNames;
    Resources resources;
    UpgradeList upgrades;
    Traits traits;
    UnitList units;
    ItemList items;
    Gear gear;
    const FactionType* m_factionType;
    string name;
public:
    int getModificationCount() {return modifications.size();}
    Modification *getModification(int i) {return modifications[i];}
    int getResourceCount() {return resources.size();}
    ResourceAmount getResource(int i) {return resources[i];}
    int getEquipmentCount() {return gear.size();}
    Equipment getEquipment(int i) {return gear[i];}
    int getUpgradesCount() {return upgrades.size();}
    Upgrade getUpgrade(int i) {return upgrades[i];}
    int getTraitCount() {return traits.size();}
    Trait *getTrait(int i) {return traits[i];}
    int getUnitCount() {return units.size();}
    Unit getUnit(int i) {return units[i];}
    int getItemCount() {return items.size();}
    Item getItem(int i) {return items[i];}
	const FactionType* getFactionType() const { return m_factionType; }
    string getEventName() const {return name;}
	void preLoad(const string &dir);
	bool load(const string &dir, const TechTree *techTree, const FactionType *factionType);
};

}}//end namespace

#endif
