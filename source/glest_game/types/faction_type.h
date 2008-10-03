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

#ifndef _GLEST_GAME_FACTIONTYPE_H_
#define _GLEST_GAME_FACTIONTYPE_H_

#include "unit_type.h"
#include "upgrade_type.h"
#include "sound.h"

using Shared::Sound::StrSound;
using Shared::Sound::StaticSound;

namespace Glest{ namespace Game{

// =====================================================
// 	class SubfactionType
//
// =====================================================
/*
// TODO: Impelement
class SubfactionType{
private:
	string name;
	SoundContainer *advancementNotice;
};
*/

// =====================================================
// 	class FactionType
//
///	Each of the possible factions the user can select
// =====================================================

class FactionType{
private:
	typedef pair<const UnitType*, int> PairPUnitTypeInt;
	typedef vector<UnitType> UnitTypes;
	typedef vector<UpgradeType> UpgradeTypes;
	typedef vector<PairPUnitTypeInt> StartingUnits;
	typedef vector<Resource> Resources;
	typedef vector<string> Subfactions;

private:
    string name;
    UnitTypes unitTypes;
    UpgradeTypes upgradeTypes;
	StartingUnits startingUnits;
	Resources startingResources;
	Subfactions subfactions;
	StrSound *music;
	SoundContainer *attackNotice;
	SoundContainer *enemyNotice;
	int attackNoticeDelay;
	int enemyNoticeDelay;

public:
	//init
	FactionType();
    void load(const string &dir, const TechTree *techTree, Checksum* checksum);
	~FactionType();

    //get
	string getName() const								{return name;}
	int getUnitTypeCount() const						{return unitTypes.size();}
	int getUpgradeTypeCount() const						{return upgradeTypes.size();}
	int getSubfactionCount() const						{return subfactions.size();}
	const UnitType *getUnitType(int i) const			{return &unitTypes[i];}
	const UpgradeType *getUpgradeType(int i) const		{return &upgradeTypes[i];}
	const string &getSubfaction(int i) const			{return subfactions[i];}
	int getSubfactionIndex(const string &name) const;
	StrSound *getMusic() const							{return music;}
	int getStartingUnitCount() const					{return startingUnits.size();}
	const UnitType *getStartingUnit(int i) const		{return startingUnits[i].first;}
	int getStartingUnitAmount(int i) const				{return startingUnits[i].second;}
	const SoundContainer *getAttackNotice() const		{return attackNotice;}
	const SoundContainer *getEnemyNotice() const		{return enemyNotice;}
	int getAttackNoticeDelay() const					{return attackNoticeDelay;}
	int getEnemyNoticeDelay() const						{return enemyNoticeDelay;}

	const UnitType *getUnitType(const string &name) const;
	const UpgradeType *getUpgradeType(const string &name) const;
	int getStartingResourceAmount(const ResourceType *resourceType) const;
};

}}//end namespace

#endif
