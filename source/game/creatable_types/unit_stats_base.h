// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_UNIT_STAT_BASE_H_
#define _GLEST_GAME_UNIT_STAT_BASE_H_

#include <cassert>
#include <stdexcept>
#include "vec.h"
#include "element_type.h"
#include "fixed.h"
#include "xml_parser.h"
#include "conversion.h"
#include "util.h"
#include "flags.h"
#include "game_constants.h"
#include "prototypes_enums.h"

using std::runtime_error;

using namespace Shared::Graphics;
using namespace Shared::Xml;
using namespace Shared::Util;
using namespace Glest::Sim;

namespace Glest { namespace ProtoTypes {

class UpgradeType;

/** Fields of travel, and indirectly zone of occupance */
class Fields : public XmlBasedFlags<Field, Field::COUNT> {
public:
	void load(const XmlNode *node, const string &dir) {
		XmlBasedFlags<Field, Field::COUNT>::load(node, dir, "field", FieldNames);
	}
};

///@todo remove the need for this hacky crap
inline Field dominantField(const Fields &fields) {
	Field f = Field::INVALID;
	if (fields.get(Field::LAND)) f = Field::LAND;
	else if (fields.get(Field::AIR)) f = Field::AIR;
	else if (fields.get(Field::STAIR)) f = Field::STAIR;
	else if (fields.get(Field::WALL)) f = Field::WALL;
	if (fields.get(Field::AMPHIBIOUS)) f = Field::AMPHIBIOUS;
	else if (fields.get(Field::ANY_WATER)) f = Field::ANY_WATER;
	else if (fields.get(Field::DEEP_WATER)) f = Field::DEEP_WATER;
	return f;
}

/** Zones of attack (air, surface, etc.) */
class Zones : public XmlBasedFlags<Zone, Zone::COUNT> {
public:
	void load(const XmlNode *node, const string &dir) {
		XmlBasedFlags<Zone, Zone::COUNT>::load(node, dir, "field", ZoneNames);
	}
};

// ==============================================================
// 	enum Property & class UnitProperties
// ==============================================================

/** Properties that can be applied to a unit. */
class UnitProperties : public XmlBasedFlags<Property, Property::COUNT>{
private:
	//static const char *names[pCount];

public:
	void load(const XmlNode *node, const string &dir) {
		XmlBasedFlags<Property, Property::COUNT>::load(node, dir, "property", PropertyNames);
	}
};

// ===============================
// 	class Stat
// ===============================
/** values and functions for all uses of a given stat */
class Stat {
private:
    int value;
    fixed valueMult;
    int layerAdd;
    fixed layerMult;
public:
    Stat() : value(0), valueMult(1), layerAdd(0), layerMult(0) {}

    int getValue() const {return value;}
    fixed getValueMult() const {return valueMult;}
    int getLayerAdd() const {return layerAdd;}
    fixed getLayerMult() const {return layerMult;}

    void setValue(int change) {value = change;}
    void setValueMult(fixed change) {valueMult = change;}
    void setLayerAdd(int change) {layerAdd = change;}
    void setLayerMult(fixed change) {layerMult = change;}

    void incValue(int change) {value += change;}
    void incValueMult(fixed change) {valueMult += change;}
    void incLayerAdd(int change) {layerAdd += change;}
    void incLayerMult(fixed change) {layerMult += change;}

    void getDesc(string &str, const char *pre, string name) const;

    void formatModifier(string &str, const char *pre, string label, int value, fixed multiplier, int layer, fixed layerMult) const;
	void doChecksum(Checksum &checksum) const;
    void init(int value, fixed valueMult, int layerAdd, fixed layerMult);
    bool load(const XmlNode *baseNode);
    void save(XmlNode *node) const;
    void reset();
    void modify();
    bool isEmpty() const;
	void clampMultipliers();
    void sanitiseStat(int safety);
};

// ===============================
// 	class StatGroup
// ===============================
/** health, shield, energy, mana, stamina */
class StatGroup {
private:
    string name;
    Stat maxStat;
    Stat regenStat;
    Stat boostStat;
public:
    StatGroup() { reset(); }
    string getName() const {return name;}

    Stat *getMaxStat()   {return &maxStat;}
    Stat *getRegenStat() {return &regenStat;}
    Stat *getBoostStat() {return &boostStat;}

    const Stat *getMaxStat() const {return &maxStat;}
    const Stat *getRegenStat() const {return &regenStat;}
    const Stat *getBoostStat() const {return &boostStat;}

    void setName(string newName) {name = newName;}
    void getDesc(string &str, const char *pre) const;
	virtual void doChecksum(Checksum &checksum) const;

	void reset();
	void modify();
	bool isEmpty() const;
	bool load(const XmlNode *baseNode, const string &dir);
	void save(XmlNode *node) const;
	void addStatic(const StatGroup *rp, fixed strength = 1);
	void addMultipliers(const StatGroup *rp, fixed strength = 1);
	void applyMultipliers(const StatGroup *rp);
	void clampMultipliers();
	void sanitiseStatGroup();
	void sum(const StatGroup *statGroup) {
		addStatic(statGroup);
		addMultipliers(statGroup);
	}
};

typedef vector<StatGroup> StatGroups;

class ResourcePools {
private:
    StatGroup health;

	StatGroups resources;
	StatGroups defenses;

    Stat maxCp;

public:
	ResourcePools() { reset(); }
	virtual ~ResourcePools() {}

	virtual void doChecksum(Checksum &checksum) const;

    StatGroup *getHealth()            {return &health;}
    StatGroup *getResource(int i)     {return &resources[i];}
    StatGroup *getDefense(int i)      {return &defenses[i];}
	Stat *getMaxCp()                  {return &maxCp;}

	const StatGroup *getHealth() const {return &health;}
    int getResourceCount() const {return resources.size();}
    const StatGroup *getResource(int i) const {return &resources[i];}
    int getDefenseCount() const {return defenses.size();}
    const StatGroup *getDefense(int i) const {return &defenses[i];}
	const Stat *getMaxCp() const {return &maxCp;}

    void addResources(StatGroups statGroups);
    void addDefenses(StatGroups statGroups);

    void getDesc(string &str, const char *pre) const;

	void reset();
	void modify();
	bool isEmpty() const;
	bool load(const XmlNode *baseNode, const string &dir);
	void save(XmlNode *node) const;
	void addStatic(const ResourcePools *rp, fixed strength = 1);
	void addMultipliers(const ResourcePools *rp, fixed strength = 1);
	void applyMultipliers(const ResourcePools *rp);
	void clampMultipliers();
	void sanitiseResourcePools();
	void sum(const ResourcePools *rPools) {
		addStatic(rPools);
		addMultipliers(rPools);
	}
};

// ===============================
// 	class ProductionSpeeds
// ===============================
class ProductionSpeeds {
private:
	Stat prodSpeed;
	Stat repairSpeed;
	Stat harvestSpeed;
public:
	ProductionSpeeds() { reset(); }
	virtual ~ProductionSpeeds() {}

	virtual void doChecksum(Checksum &checksum) const;

	Stat *getProdSpeed()    {return &prodSpeed;}
	Stat *getRepairSpeed() 	{return &repairSpeed;}
	Stat *getHarvestSpeed() {return &harvestSpeed;}

	const Stat *getProdSpeed() const	{return &prodSpeed;}
	const Stat *getRepairSpeed() const	{return &repairSpeed;}
	const Stat *getHarvestSpeed() const {return &harvestSpeed;}

    void getDesc(string &str, const char *pre) const;

	virtual void reset();
	void modify();
	bool isEmpty() const;
	bool load(const XmlNode *parametersNode, const string &dir);
	virtual void save(XmlNode *node) const;
	void addStatic(const ProductionSpeeds *ps, fixed strength = 1);
	void addMultipliers(const ProductionSpeeds *ps, fixed strength = 1);
	void applyMultipliers(const ProductionSpeeds *ps);
	void clampMultipliers();
	void sanitiseProductionSpeeds();
	void sum(const ProductionSpeeds *pSpeeds) {
		addStatic(pSpeeds);
		addMultipliers(pSpeeds);
	}
};

// ===============================
// 	class AttackStats
// ===============================
class AttackStats {
private:
	Stat attackRange;
	Stat attackSpeed;
    Stat attackStrength;
    Stat attackPotency;
public:
	AttackStats() { reset(); }
	virtual ~AttackStats() {}

	virtual void doChecksum(Checksum &checksum) const;

	Stat *getAttackRange() 	{return &attackRange;}
	Stat *getAttackSpeed() 	{return &attackSpeed;}
	Stat *getAttackStrength() {return &attackStrength;}
	Stat *getAttackPotency()  {return &attackPotency;}

	const Stat *getAttackRange() const	    {return &attackRange;}
	const Stat *getAttackSpeed() const	    {return &attackSpeed;}
	const Stat *getAttackStrength() const	{return &attackStrength;}
	const Stat *getAttackPotency() const    {return &attackPotency;}

    void getDesc(string &str, const char *pre) const;

	virtual void reset();
	void modify();
	bool isEmpty() const;
	bool load(const XmlNode *parametersNode, const string &dir);
	void save(XmlNode *node) const;
	void addStatic(const AttackStats *as, fixed strength = 1);
	void addMultipliers(const AttackStats *as, fixed strength = 1);
	void applyMultipliers(const AttackStats *as);
	void clampMultipliers();
	void sanitiseAttackStats();
	void sum(const AttackStats *aStats) {
		addStatic(aStats);
		addMultipliers(aStats);
	}
};

// ===============================
// 	class UnitStats
// ===============================
class UnitStats {
private:
    Stat sight;
	Stat expGiven;
	Stat morale;
	Stat moveSpeed;
	Stat effectStrength;
public:
	UnitStats() { reset(); }
	virtual ~UnitStats() {}

	virtual void doChecksum(Checksum &checksum) const;

	Stat *getSight() 			    {return &sight;}
	Stat *getExpGiven() 		    {return &expGiven;}
	Stat *getMorale() 			    {return &morale;}
	Stat *getMoveSpeed() 	        {return &moveSpeed;}
	Stat *getEffectStrength() 	    {return &effectStrength;}

	const Stat *getSight() const			{return &sight;}
	const Stat *getExpGiven() const		    {return &expGiven;}
	const Stat *getMorale() const			{return &morale;}
	const Stat *getMoveSpeed() const		{return &moveSpeed;}
	const Stat *getEffectStrength() const	{return &effectStrength;}

    void getDesc(string &str, const char *pre) const;

	virtual void reset();
	void modify();
	bool isEmpty() const;
	bool load(const XmlNode *parametersNode, const string &dir);
	void save(XmlNode *node) const;
	void addStatic(const UnitStats *us, fixed strength = 1);
	void addMultipliers(const UnitStats *us, fixed strength = 1);
	void applyMultipliers(const UnitStats *us);
	void clampMultipliers();
	void sanitiseUnitStats();
};

// ===============================
// 	class EnhancementType
// ===============================
class EnhancementType {
private:
    ResourcePools resourcePools;
    ProductionSpeeds productionSpeeds;
    AttackStats attackStats;
    UnitStats unitStats;
public:
	EnhancementType() { reset(); }

    ResourcePools *getResourcePools()       {return &resourcePools;}
    ProductionSpeeds *getProductionSpeeds() {return &productionSpeeds;}
    AttackStats *getAttackStats()           {return &attackStats;}
    UnitStats *getUnitStats()               {return &unitStats;}

    const ResourcePools *getResourcePools() const {return &resourcePools;}
    const ProductionSpeeds *getProductionSpeeds() const {return &productionSpeeds;}
    const AttackStats *getAttackStats() const {return &attackStats;}
    const UnitStats *getUnitStats() const {return &unitStats;}

	void addStatic(const EnhancementType *e, fixed strength = 1);
	void addMultipliers(const EnhancementType *e, fixed strength = 1);
	void applyMultipliers(const EnhancementType *e);
	void clampMultipliers();

    void sanitiseEnhancement();
	void reset();
	void modify();
	bool isEmpty() const;
	void getDesc(string &str, const char *prefix) const;
	bool load(const XmlNode *baseNode, const string &dir);
	virtual void doChecksum(Checksum &checksum) const;
	void save(XmlNode *node) const;
	static void describeModifier(string &str, int value) {
		if (value != 0) {
			if (value > 0) {
				str += "+";
			}
			str += intToStr(value);
		}
	}
	void sum(const EnhancementType *enh) {
		addStatic(enh);
		addMultipliers(enh);
	}
};

}}//end namespace

#endif
