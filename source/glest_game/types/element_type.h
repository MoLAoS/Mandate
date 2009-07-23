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
#include <string>


#include "texture.h"
#include "resource.h"
#include "xml_parser.h"

using std::vector;
using std::map;
using std::string;


using Shared::Graphics::Texture2D;

namespace Glest { namespace Game {

using namespace Shared::Xml;

//class UpgradeType;
class TechTree;
class FactionType;
class UnitType;
class UpgradeType;
class DisplayableType;
class ResourceType;
class Unit;
class Faction;

typedef vector<Unit*> Units;
typedef map<int, Unit*> UnitMap;
typedef vector<Faction*> Factions;
typedef map<int, Faction*> FactionMap;

// =====================================================
// 	class NameIdPair
//
/// Base class for anything that has both a name and id 
// =====================================================

class NameIdPair {
protected:
	int id;				//id
	string name;		//name

public:
	static const int invalidId = -1;

public:
	NameIdPair() : id(invalidId) {}
	NameIdPair(int id, const char *name) : id(id), name(name) {}
	virtual ~NameIdPair() {}

	int getId() const					{return id;}
	string getName() const				{return name;}
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
private:
	typedef vector<const UnitType*> UnitReqs;
	typedef vector<const UpgradeType*> UpgradeReqs;

protected:
	UnitReqs unitReqs;			//needed units
	UpgradeReqs upgradeReqs;	//needed upgrades
	int subfactionsReqs;		//bitmask of subfactions producable is restricted to

public:
	RequirableType() : DisplayableType(), subfactionsReqs(0) {}
	RequirableType(int id, const char *name, Texture2D *image) :
			DisplayableType(id, name, image), subfactionsReqs(0) {}
	//get
	int getUpgradeReqCount() const						{return upgradeReqs.size();}
	int getUnitReqCount() const							{return unitReqs.size();}
	const UpgradeType *getUpgradeReq(int i) const		{return upgradeReqs[i];}
	const UnitType *getUnitReq(int i) const				{return unitReqs[i];}
	int getSubfactionsReqs() const						{return subfactionsReqs;}
	bool isAvailableInSubfaction(int subfaction) const	{return (bool)(1 << subfaction & subfactionsReqs);}

    //other
    virtual string getReqDesc() const;
	virtual bool load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft);
};

// =====================================================
// 	class ProducibleType
//
///	Base class for anything that can be produced
// =====================================================

class ProducibleType: public RequirableType {
private:
	typedef vector<Resource> Costs;

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
	const Resource *getCost(int i) const			{return &costs[i];}
	const Resource *getCost(const ResourceType *rt) const {
		for(int i = 0; i < costs.size(); ++i){
			if(costs[i].getType() == rt){
				return &costs[i];
			}
		}
		return NULL;
	}
	int getProductionTime() const					{return productionTime;}
	const Texture2D *getCancelImage() const			{return cancelImage;}
	int getAdvancesToSubfaction() const				{return advancesToSubfaction;}
	bool isAdvanceImmediately() const				{return advancementIsImmediate;}

    //varios
    void checkCostStrings(TechTree *techTree);
	virtual bool load(const XmlNode *baseNode, const string &dir, const TechTree *techTree, const FactionType *factionType);

	virtual string getReqDesc() const;
};

}}//end namespace

#endif
