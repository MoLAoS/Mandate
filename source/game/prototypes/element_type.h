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
	friend class Sim::DynamicTypeFactory<CommandClass, CommandType>;
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

class RequirableType: public DisplayableType {
protected:
	typedef vector<const UnitType*> UnitReqs;
	typedef vector<const UpgradeType*> UpgradeReqs;

	UnitReqs unitReqs;			//needed units
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
	const UpgradeType *getUpgradeReq(int i) const		{return upgradeReqs[i];}
	const UnitType *getUnitReq(int i) const				{return unitReqs[i];}
	int getSubfactionsReqs() const						{return subfactionsReqs;}
	bool isAvailableInSubfaction(int subfaction) const	{return (bool)(1 << subfaction & subfactionsReqs);}

    //other
    virtual string getReqDesc(const Faction *f) const;
	virtual bool load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft);
};

// =====================================================
// 	class ProducibleType
//
///	Base class for anything that can be produced
// =====================================================

class ProducibleType : public RequirableType {
private:
	typedef vector<ResourceAmount> Costs;

protected:
	Costs costs;
    Texture2D *cancelImage;
	int productionTime;
	int advancesToSubfaction;
	bool advancementIsImmediate;

public:
    ProducibleType();
	virtual ~ProducibleType();

    //get
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

	int getProductionTime() const					{return productionTime;}
	const Texture2D *getCancelImage() const			{return cancelImage;}
	int getAdvancesToSubfaction() const				{return advancesToSubfaction;}
	bool isAdvanceImmediately() const				{return advancementIsImmediate;}

    //varios
    void checkCostStrings(TechTree *techTree);
	virtual bool load(const XmlNode *baseNode, const string &dir, const TechTree *techTree, const FactionType *factionType);

	virtual void doChecksum(Checksum &checksum) const;
	virtual string getReqDesc(const Faction *f) const;

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

}}//end namespace

#endif
