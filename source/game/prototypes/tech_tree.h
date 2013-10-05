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

#ifndef _GLEST_GAME_TECHTREE_H_
#define _GLEST_GAME_TECHTREE_H_

#include <map>
#include <set>
#include "util.h"
#include "resource_type.h"
#include "faction_type.h"

using std::set;
using std::string;

namespace Glest { namespace ProtoTypes {

class EffectType;

// =====================================================
// 	class TechTree
//
///	A set of factions and resources
// =====================================================

class TechTree{
private:
	typedef vector<ResourceType> ResourceTypes;
	typedef vector<FactionType> FactionTypes;
	typedef vector<EffectType*> EffectTypes;
	typedef map<string, ResourceType*> ResourceTypeMap;
	typedef map<string, FactionType*> FactionTypeMap;
	typedef map<string, EffectType*> EffectTypeMap;

private:
	string name;
    string desc;
    ResourceTypes resourceTypes;
    FactionTypes factionTypes;
	EffectTypes effectTypes;
	DamageTypes damageTypes;
	DamageTypes resistances;

	CraftResources resourceStats;
	CraftStats weaponStats;
	CraftStats armorStats;
	CraftStats accessoryStats;

    ResourceTypeMap resourceTypeMap;
    FactionTypeMap factionTypeMap;
	EffectTypeMap effectTypeMap;

public:
    int getWeaponStatCount() const {return weaponStats.size();}
    const CraftStat *getWeaponStat(int i) const {return &weaponStats[i];}
    int getArmorStatCount() const {return armorStats.size();}
    const CraftStat *getArmorStat(int i) const {return &armorStats[i];}
    int getAccessoryStatCount() const {return accessoryStats.size();}
    const CraftStat *getAccessoryStat(int i) const {return &accessoryStats[i];}
    int getResourceStatCount() const {return resourceStats.size();}
    const CraftRes *getResourceStat(int i) const {return &resourceStats[i];}

    CraftStat *getWeaponStat(int i) {return &weaponStats[i];}
    CraftStat *getArmorStat(int i) {return &armorStats[i];}
    CraftStat *getAccessoryStat(int i) {return &accessoryStats[i];}
    CraftRes *getResourceStat(int i) {return &resourceStats[i];}

	bool preload(const string &dir, const set<string> &factionNames);
	bool load(const string &dir, const set<string> &factionNames);

	void doChecksumResources(Checksum &checksum) const;
	void doChecksumFaction(Checksum &checksum, int ndx) const;
	void doChecksum(Checksum &checksum) const;

	~TechTree();

	string getName() const			{ return name; }

	// get count
	int getResourceTypeCount() const						{return resourceTypes.size();}
	int getFactionTypeCount() const							{return factionTypes.size();}
	int getEffectTypeCount() const							{return effectTypes.size();}
	int getDamageTypeCount() const							{return damageTypes.size();}
	int getResistanceCount() const							{return resistances.size();}

	// get by index
	const ResourceType *getResourceType(int i) const		{return &resourceTypes[i];}
	const FactionType *getFactionType(int i) const			{return &factionTypes[i];}
	const EffectType *getEffectType(int i) const			{return effectTypes[i];}
	DamageType getDamageType(int i) const			        {return damageTypes[i];}
	DamageType getResistance(int i) const			        {return resistances[i];}

	// get by name
	const ResourceType *getResourceType(const string &name) const;
	ResourceType *getResourceType(const string &name);

	const FactionType *getFactionType(const string &name) const;
	FactionType *getFactionType(const string &name);

	const EffectType *getEffectType(const string &name) const {
		EffectTypeMap::const_iterator i = effectTypeMap.find(name);
		if(i != effectTypeMap.end()) {
			return i->second;
		} else {
		   throw runtime_error("Effect Type not found: " + name);
		}
	}
	// other getters
    const ResourceType *getTechResourceType(int i) const;
    const ResourceType *getFirstTechResourceType() const;
    const string &getDesc() const								{return desc;}

	// misc
	int addEffectType(EffectType *et);
};

}} //end namespace

#endif
