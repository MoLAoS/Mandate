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

#ifndef _GLEST_GAME_FACTION_H_
#define _GLEST_GAME_FACTION_H_

#include <vector>
#include <map>
#include <cassert>

#include "upgrade.h"
#include "texture.h"
#include "resource.h"
#include "game_constants.h"
#include "element_type.h"
#include "vec.h"
#include "unit_stats_base.h"
#include "prototypes_enums.h"
#include "simulation_enums.h"
#include "upgrade_type.h"
#include "trade_command.h"
#include "mandate_ai_sim.h"
#include "events.h"

using std::map;
using std::vector;

using Shared::Graphics::Texture2D;
using Shared::Math::Vec3f;
using namespace Glest::ProtoTypes;
using Glest::Sim::World;
using Glest::Sim::ControlType;

using namespace Glest::Gui_Mandate;
using namespace Glest::Plan;

namespace Glest { namespace Entities {

class Unit;

// =====================================================
// 	class Product
//
///	Instance of a ProducibleType (Non-unit)
// =====================================================
// Completely unnecessary atm, but we'll probably want to add interesting things
// to it in the future
class Product {
private:
	const GeneratedType *m_type;

public:
	Product(const GeneratedType *gt) : m_type(gt) {}

	const GeneratedType* getType() const { return m_type; }

	// todo... other stuff. Store unit that produced this, other useful things...
};


// =====================================================
// 	factionColours
// =====================================================

extern string factionColourStrings[GameConstants::maxColours];
extern Colour factionColours[GameConstants::maxColours];
extern Colour factionColoursOutline[GameConstants::maxColours];

Vec3f getFactionColour(int ndx);

// =====================================================
// 	class UpgradeStage
// =====================================================

class UpgradeStage : public ProducibleType {
public:
    typedef vector<UpgradeEffect> Upgrades;
    typedef vector<UpgradeUnitCombo> UpgradeUnitMap;
    typedef vector< vector<string> > AffectedUnits;
    typedef vector<string> Names;

	UpgradeUnitMap     m_upgradeMap;
    Names              m_names;
	Upgrades           m_upgrades;
	AffectedUnits      m_unitsAffected;
public:
    mutable int upgradeStage;
    mutable int maxStage;
    const UpgradeType *upgradeType;
	const FactionType *m_factionType;

    int getUpgradeStage() const { return upgradeStage; }
    const UpgradeType *getUpgradeType() const { return upgradeType; }
    void setUpgradeStage(int v) { upgradeStage = v; }
    int getMaxStage() const { return maxStage; }
    void setMaxStage(int v) { maxStage = v; }

	static ProducibleClass typeClass() { return ProducibleClass::UPGRADE; }
    ProducibleClass getClass() const override                       { return typeClass(); }
	const FactionType* getFactionType() const                       { return m_factionType; }

	virtual void doChecksum(Checksum &checksum) const;

    const Statistics *getStatistics(const UnitType *ut, int stage) const;

    UpgradeStage();
    ~UpgradeStage();

    void init(const UpgradeType *upgradeType, int upgradeStage, int maxStage, Names m_names,
              Upgrades m_upgrades, AffectedUnits m_unitsAffected, UpgradeUnitMap m_upgradeMap);
};

// =====================================================
// 	class Faction
//
///	Each of the game players
// =====================================================

class Faction : public NameIdPair {
public:
    typedef vector<Item*>                              Items;
    typedef vector<EventType*>                         EventTypes;
	typedef vector<const ResourceType *>               ResourceTypes;
	typedef map<const ResourceType*, Modifier>         CostModifiers;
	typedef map<const ProducibleType*, CostModifiers>  UnitCostModifiers;
	typedef map<const UnitType*, CostModifiers>        StoreModifiers;
    typedef map<const UnitType*, CostModifiers>        CreateModifiers;
	typedef map<const UnitType*, int>                  UnitTypeCountMap;
	typedef map<const ItemType*, int>                  ItemTypeCountMap;
	typedef map<const UnitType*, Modifier>             CreateUnitModifiers;
	typedef map<const UnitType*, CreateUnitModifiers>  CreatedUnitModifiers;

private:
    typedef vector<StoredResource>	SResources;
    typedef vector<CreatedResource>	CResources;
	typedef vector<Product>			Products;

	UpgradeManager upgradeManager;

	MandateAISim mandateAISim;

    EventTypes eventTypes;
public:
    MandateAISim getMandateAiSim() const {return mandateAISim;}


    SResources    sresources;
    CResources    cresources;

    int getEventTypeCount() const {return eventTypes.size();}
    EventType *getEventType(int i) {return eventTypes[i];}

typedef vector<TradeCommand>    TradeCommands;
               TradeCommands    tradeCommands;


typedef vector<UpgradeStage>    UpgradeStages;
               UpgradeStages    upgradeStages;

    UpgradeStage *getUpgradeStage(const UpgradeType *ut);
	UpgradeStage *getUpgradeStage(int i) {assert(i < upgradeStages.size()); return &upgradeStages[i];}
	int getCurrentStage(const UpgradeType *ut);
	int getUpgradeTypeCount() {return upgradeStages.size();}


private:
	Items items;
	Units units;
	UnitMap unitMap;
	Products products;
	UnitTypeCountMap  m_unitCountMap;  // count of each 'operative' UnitType in factionType.
	ItemTypeCountMap  m_itemCountMap;  // count of each 'operative' ItemType in factionType.

typedef int                 UnitId;
typedef list<UnitId>        UnitIdList;

	// housed unit bits
	UnitIdList	            m_transitingUnits;
	UnitIdList	            m_unitsToTransit;
	UnitIdList	            m_unitsToDetransit;
	UnitId		            m_carrierFaction;

public:
	//-- for carry units
	const UnitIdList& getCarriedUnits() const	{return m_transitingUnits;}
	UnitIdList& getTransitingUnits()			{return m_transitingUnits;}
	UnitIdList& getUnitsToTransit()				{return m_unitsToTransit;}
	UnitIdList& getUnitsToDetransit()			{return m_unitsToDetransit;}
	UnitId getCarrierFaction() const			{return m_carrierFaction;}
	void housedUnitDied(Unit *unit);

typedef list<Command*> Commands;
	Commands commands;

	//bool isVisible() const					{return carried;}
	void setCarried(Faction *host)				{bool carried = (host != 0); m_carrierFaction = (host ? host->m_id : -1);}
	void loadFactionUnitInit(Command *command);
	void unloadFactionUnitInit(Command *command);
	//----

private:

	ControlType control;

	Texture2D *texture;
	Texture2D *m_logoTex;
	const FactionType *factionType;
	UnitCostModifiers m_costModifiers;
	StoreModifiers    m_storeModifiers;
	CreateModifiers   m_createModifiers;
	CreatedUnitModifiers m_createdUnitModifiers;

	int teamIndex;

	typedef vector<int> Allies;
	Allies allies;

	int startLocationIndex;
	int colourIndex;

	bool thisFaction;
	bool defeated;
	time_t lastAttackNotice;
	time_t lastEnemyNotice;
	Vec3f lastEventLoc;

	static ResourceTypes neededResources;

private:
	void buildLogoPixmap();

public:
	void init(const FactionType *factionType, ControlType control, string playerName, TechTree *techTree,
		int factionIndex, int teamIndex, int startLocationIndex, int colourIndex,
		bool thisFaction, bool giveResources);

	void save(XmlNode *node) const;
	void load(const XmlNode *node, World *world, const FactionType *ft, ControlType control, TechTree *tt);

	//get
	const StoredResource *getSResource(const ResourceType *rt) const;
	const StoredResource *getSResource(int i) const  {assert(i < sresources.size()); return &sresources[i];}
	int getStoreAmount(const ResourceType *rt) const;

    const CreatedResource *getCResource(const ResourceType *rt) const;
	const CreatedResource *getCResource(int i) const  {assert(i < cresources.size()); return &cresources[i];}
	int getCreateAmount(const ResourceType *rt) const;

	int getCountOfUnitType(const UnitType *ut) const    { return m_unitCountMap.find(ut)->second; }
	int getCountOfItemType(const ItemType *it) const    { return m_itemCountMap.find(it)->second; }

	const FactionType *getType() const					{return factionType;}
	int getIndex() const								{return m_id;}
	int getTeam() const									{return teamIndex;}
    Allies getAllies() const                            {return allies;}
	bool isDefeated() const								{return defeated;}
	bool getCpuControl() const							{return control >= ControlType::CPU_EASY;}
	bool getCpuUltraControl() const						{return control == ControlType::CPU_ULTRA;}
	bool getCpuEasyControl() const						{return control == ControlType::CPU_EASY;}
	bool getCpuMegaControl() const						{return control == ControlType::CPU_MEGA;}
	ControlType getControlType() const					{return control;}
	Unit *getUnit(int i) const							{assert(units.size() == unitMap.size()); assert(i < units.size()); return units[i];}
	int getUnitCount() const							{return units.size();}
	const Units &getUnits() const						{return units;}
	int getItemCount() const							{return items.size();}
	const Items &getItems() const						{return items;}
	const UpgradeManager *getUpgradeManager() const		{return &upgradeManager;}
	const Texture2D *getTexture() const					{return texture;}
	const Texture2D *getLogoTex() const					{return m_logoTex;}
	int getStartLocationIndex() const					{return startLocationIndex;}
	int getColourIndex() const							{return colourIndex;}
	Colour getColour() const							{return factionColours[colourIndex];}
	Vec3f  getColourV3f() const;
	Vec3f getLastEventLoc() const						{return lastEventLoc;}
	Modifier getCostModifier(const ProducibleType *pt, const ResourceType *rt) const;
    Modifier getStoreModifier(const UnitType *ut, const ResourceType *rt) const;
	Modifier getCreateModifier(const UnitType *ut, const ResourceType *rt) const;
	Modifier getCreatedUnitModifier(const UnitType *ut, const UnitType *sut) const;

	///@todo Remove this!
	static const ResourceTypes &getNeededResources() 	{return neededResources;}

	bool isThisFaction() const							{return thisFaction;}

	// set
	void setDefeated()	{ defeated = true; }

	// upgrades
	void startUpgrade(const UpgradeType *ut, int upgradeStage);
	void cancelUpgrade(const UpgradeType *ut);
	void finishUpgrade(const UpgradeType *ut);

	void applyUpgradeBoosts(Unit *unit) { upgradeManager.addPointBoosts(unit); }

	// cost application
	bool applyCosts(const ProducibleType *p);
	bool applyCosts(const ProducibleType *p, int discount);

	void giveRefund(const ProducibleType *p, int refund);

	void applyStaticCosts(const ProducibleType *p);
	void applyStaticProduction(const ProducibleType *p);
	void deApplyCosts(const ProducibleType *p);
	void deApplyStaticCosts(const ProducibleType *p);
	void deApplyStaticConsumption(const ProducibleType *p);
	void applyCostsOnInterval(const ResourceType *rt);

	// check resource costs
	bool checkCosts(const ProducibleType *pt);
	bool checkCosts(const ProducibleType *pt, int discount);

	// reqs
	bool reqsOk(const RequirableType *rt) const;
	bool reqsOk(const CommandType *ct) const;
	bool reqsOk(const CommandType *ct, const ProducibleType *pt) const;

	// get summary of commmand reqs and costs
	void reportReqsAndCosts(const CommandType *ct, const ProducibleType *pt, CommandCheckResult &out_result) const;
	void reportReqs(const RequirableType *rt, CommandCheckResult &out_result, bool checkDups = false) const;

	// for faction build
	void reportBuildReqsAndCosts(const UnitType *unitType, FactionBuildCheckResult &out_result) const;
	void reportBuildReqs(const UnitType *unitType, FactionBuildCheckResult &out_result, bool checkDups = false) const;

	// diplomacy
	bool isAlly(const Faction *faction)	const			{return teamIndex == faction->getTeam();}
	bool hasBuilding() const;
	bool canSee(const Unit *unit) const;

	// other
	Unit *findUnit(int id) {
		assert(units.size() == unitMap.size());
		UnitMap::iterator it = unitMap.find(id);
		return it == unitMap.end() ? NULL : it->second;
	}

	void add(Unit *unit);
	void remove(Unit *unit);

	void addItem(Item *item);

	void onUnitActivated(const UnitType *ut) { ++m_unitCountMap[ut]; }
	void onUnitMorphed(const UnitType *new_ut, const UnitType *old_ut) {
		assert(m_unitCountMap[old_ut] > 0);
		--m_unitCountMap[old_ut];
		++m_unitCountMap[new_ut];
	}
	void onUnitDeActivated(const UnitType *ut) { --m_unitCountMap[ut]; }

	void addStore(const ResourceType *rt, int amount);
	void addStore(const UnitType *unitType);
	void removeStore(const UnitType *unitType);
	void reEvaluateStore();

	void setLastEventLoc(Vec3f lastEventLoc)	{this->lastEventLoc = lastEventLoc;}
	void attackNotice(const Unit *u);

	// resources
	void incResourceAmount(const ResourceType *rt, int amount);
	void setResourceBalance(const ResourceType *rt, int balance);
	void capResource(const ResourceType *rt);

	// Generated 'products' (non unit producibles)
	void addProduct(const GeneratedType *gt) { products.push_back(Product(gt)); }

private:
	void limitResourcesToStore();
	void resetResourceAmount(const ResourceType *rt);

	void applyTrade(const ResourceType *rt, int tradeAmount);
};

}}//end namespace

#endif
