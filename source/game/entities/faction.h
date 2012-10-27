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

private:
	typedef vector<UpgradeEffect> Enhancements;
	typedef map<const UnitType*, const UpgradeEffect*> EnhancementMap;
	typedef vector< vector<string> > AffectedUnits;
    typedef vector<string> Names;

	EnhancementMap     m_enhancementMap;
	const FactionType *m_factionType;

public:
    Names              m_names;
    Enhancements       m_enhancements;
	AffectedUnits      m_unitsAffected;


    mutable int upgradeStage;
    mutable int maxStage;
    const UpgradeType *upgradeType;

    int getUpgradeStage() const { return upgradeStage; }
    const UpgradeType *getUpgradeType() const { return upgradeType; }
    void setUpgradeStage(int v) { upgradeStage = v; }
    int getMaxStage() const { return maxStage; }
    void setMaxStage(int v) { maxStage = v; }

	static ProducibleClass typeClass() { return ProducibleClass::UPGRADE; }
    ProducibleClass getClass() const override                       { return typeClass(); }
	const FactionType* getFactionType() const                       { return m_factionType; }
	const EnhancementType* getEnhancement(const UnitType *ut) const;
	Modifier getCostModifier(const UnitType *ut, const ResourceType *rt) const;
	Modifier getStoreModifier(const UnitType *ut, const ResourceType *rt) const;
    Modifier getCreateModifier(const UnitType *ut, const ResourceType *rt) const;

	bool isAffected(const UnitType *unitType) const {
		return m_enhancementMap.find(unitType) != m_enhancementMap.end();
	}

	virtual void doChecksum(Checksum &checksum) const;

    UpgradeStage();
    ~UpgradeStage();

    void init(const UpgradeType *upgradeType, int upgradeStage, int maxStage, Names m_names,
        const Enhancements m_enhancements, AffectedUnits m_unitsAffected, EnhancementMap m_enhancementMap);
};

// =====================================================
// 	class Faction
//
///	Each of the game players
// =====================================================

class Faction : public NameIdPair {
public:
    typedef vector<Item>                               Items;
	typedef vector<const ResourceType *>               ResourceTypes;
	typedef map<const ResourceType*, Modifier>         CostModifiers;
	typedef map<const ProducibleType*, CostModifiers>  UnitCostModifiers;
	typedef map<const UnitType*, CostModifiers>        StoreModifiers;
    typedef map<const UnitType*, CostModifiers>        CreateModifiers;
	typedef map<const UnitType*, int>                  UnitTypeCountMap;
	typedef map<const UnitType*, Modifier>             CreateUnitModifiers;
	typedef map<const UnitType*, CreateUnitModifiers>  CreatedUnitModifiers;

private:
    typedef vector<StoredResource>	SResources;
    typedef vector<CreatedResource>	CResources;
	typedef vector<Product>			Products;

	UpgradeManager upgradeManager;

	MandateAISim mandateAISim;

public:
    MandateAISim getMandateAiSim() const {return mandateAISim;}

    SResources    sresources;
    CResources    cresources;

typedef vector<TradeCommand>    TradeCommands;
               TradeCommands    tradeCommands;


typedef vector<UpgradeStage>    UpgradeStages;
               UpgradeStages    upgradeStages;

    UpgradeStage *getUpgradeStage(const UpgradeType *ut);
	UpgradeStage *getUpgradeStage(int i) {assert(i < upgradeStages.size()); return &upgradeStages[i];}
	int getCurrentStage(const UpgradeType *ut);

	Items items;

private:
	Units units;
	UnitMap unitMap;
	Products products;
	UnitTypeCountMap  m_unitCountMap;  // count of each 'operative' UnitType in factionType.

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
	int subfaction;			// the current subfaction index starting at zero
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
	const UpgradeManager *getUpgradeManager() const		{return &upgradeManager;}
	const Texture2D *getTexture() const					{return texture;}
	const Texture2D *getLogoTex() const					{return m_logoTex;}
	int getStartLocationIndex() const					{return startLocationIndex;}
	int getColourIndex() const							{return colourIndex;}
	Colour getColour() const							{return factionColours[colourIndex];}
	Vec3f  getColourV3f() const;
	int getSubfaction() const							{return subfaction;}
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
	void startUpgrade(const UpgradeType *ut);
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

	// subfaction checks
	bool isAvailable(const CommandType *ct) const;
	bool isAvailable(const CommandType *ct, const ProducibleType *pt) const;
	bool isAvailable(const RequirableType *rt) const	{return rt->isAvailableInSubfaction(subfaction);}

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

    void addCreate(const ResourceType *rt, int amount);
	void addCreate(const UnitType *unitType);
	void removeCreate(const UnitType *unitType);
	void reEvaluateCreate();

	void setLastEventLoc(Vec3f lastEventLoc)	{this->lastEventLoc = lastEventLoc;}
	void attackNotice(const Unit *u);

	void advanceSubfaction(int subfaction);
	void checkAdvanceSubfaction(const ProducibleType *pt, bool finished);

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
