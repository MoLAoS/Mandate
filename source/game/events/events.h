// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_EVENTS_H_
#define _GLEST_GAME_EVENTS_H_

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <time.h>

#include "forward_decs.h"
#include "creatable_type.h"
#include "vec.h"
#include "prototypes_enums.h"
#include "simulation_enums.h"

#include "traits.h"

namespace Glest{ namespace ProtoTypes {
// =====================================================
// 	class Event Helper Classes
// =====================================================
class EventItem {
private:
    const ItemType* itemType;
    int amount;
    bool local;
public:
    const ItemType *getItemType() const {return itemType;}
    int getAmount() const {return amount;}
    bool getScope() const {return local;}
    void init(const ItemType* type, int value, bool scope);
	void getDesc(string &str, const char *pre);
};

class EventUnit {
private:
    const UnitType* unitType;
    int amount;
    bool local;
public:
    const UnitType *getUnitType() const {return unitType;}
    int getAmount() const {return amount;}
    bool getScope() const {return local;}
    void init(const UnitType* type, int value, bool scope);
	void getDesc(string &str, const char *pre);
};

class EventUpgrade {
private:
    const UpgradeType* upgradeType;
    int stage;
public:
    const UpgradeType *getUpgradeType() const {return upgradeType;}
    int getStage() const {return stage;}
    void init(const UpgradeType* type, int value);
	void getDesc(string &str, const char *pre);
};

typedef vector<EventItem> ItemTriggers;
typedef vector<EventUnit> UnitTriggers;
typedef vector<EventUpgrade> UpgradeTriggers;
typedef vector<ResourceAmount> ResourceTriggers;

class Triggers {
private:
    ItemTriggers itemTriggers;
    UnitTriggers unitTriggers;
    UpgradeTriggers upgradeTriggers;
    ResourceTriggers resourceTriggers;
    ResourceTriggers localResTriggers;
public:
    int getItemTriggerCount() const {return itemTriggers.size();}
    const EventItem *getItemTrigger(int i) const {return &itemTriggers[i];}
    int getUnitTriggerCount() const {return unitTriggers.size();}
    const EventUnit *getUnitTrigger(int i) const {return &unitTriggers[i];}
    int getUpgradeTriggerCount() const {return upgradeTriggers.size();}
    const EventUpgrade *getUpgradeTrigger(int i) const {return &upgradeTriggers[i];}
    int getResourceTriggerCount() const {return resourceTriggers.size();}
    const ResourceAmount *getResourceTrigger(int i) const {return &resourceTriggers[i];}
    int getLocalResTriggerCount() const {return localResTriggers.size();}
    const ResourceAmount *getLocalResTrigger(int i) const {return &localResTriggers[i];}
    bool load(const XmlNode *baseNode, const string &dir, const FactionType *ft, bool add=false);
};

// =====================================================
// 	class EventType
// =====================================================
class Choice {
private:
    typedef vector<Modification*> Modifications;
    typedef vector<string> ModifyNames;
    typedef vector<ResourceAmount> Resources;
    typedef vector<EventUpgrade> UpgradeList;
    typedef vector<EventUnit> UnitList;
    typedef vector<EventItem> ItemList;
    typedef vector<Equipment> Gear;
    typedef vector<Trait*> Traits;
    CharacterStats characterStats;
    Modifications modifications;
    ModifyNames modifyNames;
    Resources resources;
    Knowledge knowledge;
    UpgradeList upgrades;
    Traits traits;
    UnitList units;
    ItemList items;
    Gear gear;
    const Texture2D *flavorImage;
    string flavorTitle;
    string flavorText;
    bool sovereign;
public:
    bool getSovCheck() const {return sovereign;}
    const Texture2D *getFlavorImage() const {return flavorImage;}
    string getFlavorTitle() const {return flavorTitle;}
    int getModificationCount() const {return modifications.size();}
    Modification *getModification(int i) const {return modifications[i];}
    int getResourceCount() const {return resources.size();}
    const ResourceAmount *getResource(int i) const {return &resources[i];}
    int getEquipmentCount() {return gear.size();}
    Equipment getEquipment(int i) {return gear[i];}
    int getUpgradesCount() {return upgrades.size();}
    EventUpgrade getUpgrade(int i) {return upgrades[i];}
    int getTraitCount() {return traits.size();}
    Trait *getTrait(int i) {return traits[i];}
    int getUnitCount() {return units.size();}
    EventUnit getUnit(int i) {return units[i];}
    int getItemCount() {return items.size();}
    EventItem getItem(int i) {return items[i];}
	bool load(const XmlNode *baseNode, const string &dir, const TechTree *techTree, const FactionType *factionType);
	void getDesc(string &str, const char *pre);
	void processChoice(Faction *faction);
};

class EventType : public DisplayableType {
private:
    typedef vector<Choice> Choices;
    Choices choices;
    const FactionType* m_factionType;
    string name;
    Triggers triggers;
public:
    int getChoiceCount() const {return choices.size();}
    const Choice *getChoice(int i) const {return &choices[i];}
    Choice *getChoice(int i) {return &choices[i];}
	const FactionType* getFactionType() const { return m_factionType; }
    string getEventName() const {return name;}
	void preLoad(const string &dir);
	bool load(const string &dir, const TechTree *techTree, const FactionType *factionType);
	bool checkTriggers(Faction *faction) const;
	static EventClass typeClass() { return EventClass::EVENT; }
};

}}//end namespace

#endif
