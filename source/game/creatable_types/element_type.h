// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_ELEMENTTYPE_H_
#define _GLEST_GAME_ELEMENTTYPE_H_

#include <vector>
#include <map>
#include <set>
#include <string>

#include "texture.h"
#include "xml_parser.h"
#include "checksum.h"
#include "factory.h"
#include "prototypes_enums.h"
#include "resource.h"
#include "forward_decs.h"

using std::vector;
using std::map;
using std::set;
using std::string;

using namespace Shared::Xml;
using Shared::Graphics::Texture2D;
using Shared::Util::SingleTypeFactory;

namespace Glest {

namespace Entities {
	typedef vector<Unit*> Units;
	typedef map<int, Unit*> UnitMap;
	typedef vector<Faction*> Factions;
	typedef map<int, Faction*> FactionMap;
	typedef set<Unit*> MutUnitSet;
}

namespace ProtoTypes {

using namespace Entities;

typedef const XmlNode* XmlNodePtr;

/** modifier pair (static addition and multiplier) */
struct Modifier {
	int    m_addition;
	fixed  m_multiplier;

	int   getAddition()   const { return m_addition;   }
	fixed getMultiplier() const { return m_multiplier; }

	Modifier() : m_addition(0), m_multiplier(1) {}
	Modifier(int add, fixed mult) : m_addition(add), m_multiplier(mult) {}
	Modifier(const Modifier &that) : m_addition(that.m_addition), m_multiplier(that.m_multiplier) {}
};
//typedef pair<int, fixed> Modifier;


// =====================================================
// 	class NameIdPair
//
/// Base class for anything that has both a name and id
// =====================================================

class NameIdPair {
	friend class Sim::DynamicTypeFactory<ProducibleClass, ProducibleType>;
	friend class Sim::DynamicTypeFactory<SkillClass, SkillType>;
	friend class Sim::DynamicTypeFactory<CmdClass, CommandType>;
	friend class Sim::DynamicTypeFactory<EffectClass, EffectType>;

protected:
	int    m_id;   // id
	string m_name; // name

protected:
	void setId(int v) { m_id = v; }
	void setName(const string &name) { m_name = name; }

public:
	static const int invalidId = -1;

public:
	NameIdPair() : m_id(invalidId) {}
	NameIdPair(int id, const char *name) : m_id(id), m_name(name) {}
	virtual ~NameIdPair() {}

	int getId() const					{return m_id;}
	string getName() const				{return m_name;}

	virtual void doChecksum(Checksum &checksum) const;
};

// =====================================================
// 	class DisplayableType
//
/// Base class for anything that has a name, id and a portrait.
// =====================================================

class DisplayableType : public NameIdPair {
protected:
	Texture2D *image;	//portrait

public:
	DisplayableType() : image(NULL) {}
	DisplayableType(int id, const char *name, Texture2D *image) :
			NameIdPair(id, name), image(image) {}
	virtual ~DisplayableType() {};

	virtual bool load(const XmlNode *baseNode, const string &dir);

	//get
	const Texture2D *getImage() const	{return image;}
};


// =====================================================
// 	class RequirableType
//
///	Base class for anything that has requirements
// =====================================================
class ItemReq {
private:
    const ItemType* itemType;
    int amount;
public:
    const ItemType *getItemType() const {return itemType;}
    int getAmount() const {return amount;}
    void init(const ItemType* type, int value);
};

class UnitReq {
private:
    const UnitType* unitType;
    int amount;
public:
    const UnitType *getUnitType() const {return unitType;}
    int getAmount() const {return amount;}
    void init(const UnitType* type, int value);
};

class UpgradeReq {
private:
    const UpgradeType* upgradeType;
    int stage;
public:
    const UpgradeType *getUpgradeType() const {return upgradeType;}
    int getStage() const {return stage;}
    void init(const UpgradeType* type, int value);
};

class RequirableType: public DisplayableType {
public:
	typedef vector<const UnitReq> UnitReqs;
	typedef vector<const ItemReq> ItemReqs;
	typedef vector<const UpgradeReq> UpgradeReqs;

protected:
	UnitReqs unitReqs;			//needed units
	ItemReqs itemReqs;			//needed items
	UpgradeReqs upgradeReqs;	//needed upgrades
	int subfactionsReqs;		//bitmask of subfactions producable is restricted to

public:
	RequirableType() : DisplayableType(), subfactionsReqs(0) {}
	RequirableType(int id, const char *name, Texture2D *image) :
			DisplayableType(id, name, image), subfactionsReqs(0) {}

	virtual void doChecksum(Checksum &checksum) const;

	//get
	int getUpgradeReqCount() const						{return upgradeReqs.size();}
	int getUnitReqCount() const							{return unitReqs.size();}
	int getItemReqCount() const							{return itemReqs.size();}
	const UpgradeReq getUpgradeReq(int i) const		    {return upgradeReqs[i];}
	const UnitReq getUnitReq(int i) const				{return unitReqs[i];}
	const ItemReq getItemReq(int i) const				{return itemReqs[i];}
	int getSubfactionsReqs() const						{return subfactionsReqs;}
	bool isAvailableInSubfaction(int subfaction) const	{return (bool)(1 << subfaction & subfactionsReqs);}

    //other
    string getReqDesc(const Faction *f, const FactionType *ft) const;
	virtual bool load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft);
};

// =====================================================
// 	class ProducibleType
//
///	Base class for anything that can be produced
// =====================================================

class ProducibleType : public RequirableType {
public:
	typedef vector<ResourceAmount> Costs;

protected:
	Costs costs;
    Costs localCosts;
    Texture2D *cancelImage;
	int productionTime;
	int advancesToSubfaction;
	bool advancementIsImmediate;

public:
    ProducibleType();
	virtual ~ProducibleType();

    // get
	int getCostCount() const						{return costs.size();}
	//const ResourceAmount *getCost(int i) const			{return &costs[i];}
	//const ResourceAmount *getCost(const ResourceType *rt) const {
	//	for(int i = 0; i < costs.size(); ++i){
	//		if(costs[i].getType() == rt){
	//			return &costs[i];
	//		}
	//	}
	//	return NULL;
	//}
	ResourceAmount getCost(int i, const Faction *f) const;
	ResourceAmount getCost(const ResourceType *rt, const Faction *f) const;

	int getLocalCostCount() const						{return localCosts.size();}
	ResourceAmount getLocalCost(int i, const Faction *f) const;
	ResourceAmount getLocalCost(const ResourceType *rt, const Faction *f) const;

	int getProductionTime() const					{return productionTime;}
	const Texture2D *getCancelImage() const			{return cancelImage;}
	int getAdvancesToSubfaction() const				{return advancesToSubfaction;}
	bool isAdvanceImmediately() const				{return advancementIsImmediate;}

    // varios
	virtual bool load(const XmlNode *baseNode, const string &dir, const TechTree *techTree, const FactionType *factionType);

	virtual void doChecksum(Checksum &checksum) const;
	string getReqDesc(const Faction *f, const FactionType *ft) const;

	virtual ProducibleClass getClass() const = 0;
};

class GeneratedType : public ProducibleType {
private:
	const CommandType *m_commandType;

public:
	static ProducibleClass typeClass() { return ProducibleClass::GENERATED; }

public:
	GeneratedType() : m_commandType(NULL) {}

	const CommandType* getCommandType() const { return m_commandType; }
	void setCommandType(const CommandType *ct) { m_commandType = ct; }

	ProducibleClass getClass() const override { return typeClass(); }
};

class UnitReqResult {
private:
	const UnitType     *m_unitType;
	bool                m_ok;

public:
	UnitReqResult(const UnitType *unitType, bool ok) : m_unitType(unitType) , m_ok(ok) {}
	bool isRequirementMet() const { return m_ok; }
	const UnitType*    getUnitType() const { return m_unitType; }
};

class UpgradeReqResult {
private:
	const UpgradeType  *m_upgradeType;
	bool                m_ok;

public:
	UpgradeReqResult(const UpgradeType *upgradeType, bool ok) : m_upgradeType(upgradeType), m_ok(ok) {}
	bool isRequirementMet() const { return m_ok; }
	const UpgradeType* getUpgradeType() const { return m_upgradeType; }
};

class ResourceCostResult {
private:
	const ResourceType  *m_type;  // resource type of this cost
	int                  m_cost;  // required amount
	int                  m_store; // amount faction has

public:
	ResourceCostResult(const ResourceType *resourcType, int cost, int store)
		: m_type(resourcType), m_cost(cost), m_store(store) {}

	bool isCostMet() const { return m_cost <= m_store; }
	int  getCost() const { return m_cost; }
	int  getDifference() const { assert (m_cost > m_store); return m_cost - m_store; }
	const ResourceType* getResourceType() const { return m_type; }
};

class ResourceMadeResult {
private:
	const ResourceType  *m_type;  // resource type of this cost
	int                  m_made;  // amount made

public:
	ResourceMadeResult(const ResourceType *resourcType, int made)
		: m_type(resourcType), m_made(made) {}

	int  getAmount() const { return m_made; }
	const ResourceType* getResourceType() const { return m_type; }
};

typedef vector<UnitReqResult>      UnitReqResults;
typedef vector<UpgradeReqResult>   UpgradeReqResults;
typedef vector<ResourceCostResult> ResourceCostResults;
typedef vector<ResourceMadeResult> ResourceMadeResults;

struct CommandCheckResult {
	const CommandType    *m_commandType;
	const ProducibleType *m_producibleType;
	UnitReqResults        m_unitReqResults;
	UpgradeReqResults     m_upgradeReqResults;
	ResourceCostResults   m_resourceCostResults;
	ResourceMadeResults   m_resourceMadeResults;
	bool                  m_upgradedAlready;
	bool                  m_upgradingAlready;
	bool                  m_partiallyUpgraded;
	bool                  m_availableInSubFaction;
};

}}//end namespace

#endif
