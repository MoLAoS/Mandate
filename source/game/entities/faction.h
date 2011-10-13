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
#include "prototypes_enums.h"
#include "simulation_enums.h"

using std::map;
using std::vector;

using Shared::Graphics::Texture2D;
using Shared::Math::Vec3f;
using namespace Glest::ProtoTypes;
using Glest::Sim::World;
using Glest::Sim::ControlType;

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
// 	class Faction
//
///	Each of the game players
// =====================================================

class Faction : public NameIdPair {
public:
	typedef vector<const ResourceType *>               ResourceTypes;
	typedef map<const ResourceType*, Modifier>         CostModifiers;
	typedef map<const ProducibleType*, CostModifiers>  UnitCostModifiers;
	typedef map<const UnitType*, CostModifiers>        StoreModifiers;
	typedef map<const UnitType*, int>                  UnitTypeCountMap;

private:
    typedef vector<StoredResource>	Resources;
	typedef vector<Product>			Products;

	UpgradeManager upgradeManager;

    Resources resources;
	Units units;
	UnitMap unitMap;
	Products products;
	UnitTypeCountMap  m_unitCountMap;  // count of each 'operative' UnitType in factionType.

	ControlType control;

	Texture2D *texture;
	Texture2D *m_logoTex;
	const FactionType *factionType;
	UnitCostModifiers m_costModifiers;
	StoreModifiers    m_storeModifiers;

	int teamIndex;
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
	const StoredResource *getResource(const ResourceType *rt) const;
	const StoredResource *getResource(int i) const  {assert(i < resources.size()); return &resources[i];}
	int getStoreAmount(const ResourceType *rt) const;

	int getCountOfUnitType(const UnitType *ut) const    { return m_unitCountMap.find(ut)->second; }

	const FactionType *getType() const					{return factionType;}
	int getIndex() const								{return m_id;}
	int getTeam() const									{return teamIndex;}
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
};

}}//end namespace

#endif
