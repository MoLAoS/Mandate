// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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
#include "damage_multiplier.h"

using std::set;
using std::string;

namespace Glest { namespace ProtoTypes {

class EffectType;
class MusicPlaylistType;


// =====================================================
// 	class TechTree
//
///	A set of factions and resources
// =====================================================

class TechTree{
private:
	typedef vector<ResourceType> ResourceTypes;
	typedef vector<FactionType> FactionTypes;
	typedef vector<ArmourType> ArmorTypes;
	typedef vector<AttackType> AttackTypes;
	typedef vector<EffectType*> EffectTypes;
	typedef map<string, ResourceType*> ResourceTypeMap;
	typedef map<string, FactionType*> FactionTypeMap;
	typedef map<string, ArmourType*> ArmorTypeMap;
	typedef map<string, AttackType*> AttackTypeMap;
	typedef map<string, EffectType*> EffectTypeMap;

private:
	string name;
    string desc;
    ResourceTypes resourceTypes;
    FactionTypes factionTypes;
	ArmorTypes armorTypes;
	AttackTypes attackTypes;
	EffectTypes effectTypes;

    ResourceTypeMap resourceTypeMap;
    FactionTypeMap factionTypeMap;
	ArmorTypeMap armorTypeMap;
	AttackTypeMap attackTypeMap;
	EffectTypeMap effectTypeMap;

	DamageMultiplierTable damageMultiplierTable;

    MusicPlaylistType *m_playlist;


public:
	bool preload(const string &dir, const set<string> &factionNames);
	bool load(const string &dir, const set<string> &factionNames);

	void doChecksumResources(Checksum &checksum) const;
	void doChecksumDamageMult(Checksum &checksum) const;
	void doChecksumFaction(Checksum &checksum, int ndx) const;
	void doChecksum(Checksum &checksum) const;

    TechTree(): m_playlist(0) {}
	~TechTree();

	string getName() const			{ return name; }

	// get count
	int getResourceTypeCount() const							{return resourceTypes.size();}
	int getFactionTypeCount() const								{return factionTypes.size();}
	int getArmorTypeCount() const								{return armorTypes.size();}
	int getAttackTypeCount() const								{return attackTypes.size();}
	int getEffectTypeCount() const								{return effectTypes.size();}

	// get by index
	const ResourceType *getResourceType(int i) const			{return &resourceTypes[i];}
	const FactionType *getFactionType(int i) const				{return &factionTypes[i];}
	const ArmourType *getArmourType(int i) const					{return &armorTypes[i];}
	const AttackType *getAttackType(int i) const				{return &attackTypes[i];}
	const EffectType *getEffectType(int i) const				{return effectTypes[i];}

	// get by name
	const ResourceType *getResourceType(const string &name) const {
		ResourceTypeMap::const_iterator i = resourceTypeMap.find(name);
		if(i != resourceTypeMap.end()) {
			return i->second;
		} else {
		   throw runtime_error("Resource Type not found: " + name);
		}
	}

	const FactionType *getFactionType(const string &name) const {
		FactionTypeMap::const_iterator i = factionTypeMap.find(name);
		if(i != factionTypeMap.end()) {
			return i->second;
		} else {
		   throw runtime_error("Faction Type not found: " + name);
		}
	}
	const ArmourType *getArmourType(const string &name) const {
		ArmorTypeMap::const_iterator i = armorTypeMap.find(name);
		if(i != armorTypeMap.end()) {
			return i->second;
		} else {
		   throw runtime_error("Armor Type not found: " + name);
		}
	}

	const AttackType *getAttackType(const string &name) const {
		AttackTypeMap::const_iterator i = attackTypeMap.find(name);
		if(i != attackTypeMap.end()) {
			return i->second;
		} else {
		   throw runtime_error("Attack Type not found: " + name);
		}
	}

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
	fixed getDamageMultiplier(const AttackType *att, const ArmourType *art) const {
		return damageMultiplierTable.getDamageMultiplier(att, art);
	}
    MusicPlaylistType *getMusicPlaylist() const                 { return m_playlist; }

	// misc
	int addEffectType(EffectType *et);
};

}} //end namespace

#endif
