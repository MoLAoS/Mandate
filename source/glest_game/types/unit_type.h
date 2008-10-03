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

#ifndef _GLEST_GAME_UNITTYPE_H_
#define _GLEST_GAME_UNITTYPE_H_

#include "element_type.h"
#include "command_type.h"
#include "damage_multiplier.h"
#include "sound_container.h"
#include "checksum.h"
#include "unit_stats_base.h"

namespace Glest{ namespace Game{

using Shared::Sound::StaticSound;
using Shared::Util::Checksum;

class UpgradeType;
class UnitType;
class ResourceType;
class TechTree;
class FactionType;

// ===============================
// 	class Level
// ===============================

class Level: public EnhancementTypeBase{
private:
	string name;
	int kills;

public:
	Level() : EnhancementTypeBase() {
		maxHpMult = 1.5f;
		maxEpMult = 1.5f;
		sightMult = 1.2f;
		armorMult = 1.5f;
		effectStrength = 0.1f;
	}

	virtual void load(const XmlNode *prn, const string &dir, const TechTree *tt, const FactionType *ft);
	const string &getName() const	{return name;}
	int getKills() const			{return kills;}
};

/*
class PetRule {
private:
	const UnitType *type;
	int count;

public:
	PetRule();
	virtual void load(const XmlNode *prn, const string &dir, const TechTree *tt, const FactionType *ft);
};
*/

enum UnitClass{
	ucWarrior,
	ucWorker,
	ucBuilding
};

// ===============================
// 	class UnitType
//
///	A unit or building type
// ===============================


class UnitType: public ProducibleType, public UnitStatsBase {
private:
    typedef vector<SkillType*> SkillTypes;
    typedef vector<CommandType*> CommandTypes;
    typedef vector<Resource> StoredResources;
	typedef vector<Level> Levels;
//	typedef vector<PetRule*> PetRules;

private:
	//basic
	int id;
	bool multiBuild;
    bool multiSelect;

	//sounds
    SoundContainer selectionSounds;
    SoundContainer commandSounds;

	//info
    SkillTypes skillTypes;
    CommandTypes commandTypes;
    StoredResources storedResources;
	Levels levels;
//	PetRules petRules;
	Emanations emanations;

	//meeting point
	bool meetingPoint;
	Texture2D *meetingPointImage;

    //OPTIMIZATION: store first command type and skill type of each class
	const CommandType *firstCommandTypeOfClass[ccCount];
    const SkillType *firstSkillTypeOfClass[scCount];

public:
	//creation and loading
    UnitType();
    virtual ~UnitType();
	void preLoad(const string &dir);
    void load(int id, const string &dir, const TechTree *techTree, const FactionType *factionType, Checksum* checksum);

	//get
	int getId() const									{return id;}
	bool getMultiSelect() const							{return multiSelect;}

	const SkillType *getSkillType(int i) const			{return skillTypes[i];}
	const CommandType *getCommandType(int i) const		{return commandTypes[i];}
	const CommandType *getCommandType(const string &name) const;
	const Level *getLevel(int i) const					{return &levels[i];}
	int getSkillTypeCount() const						{return skillTypes.size();}
	int getCommandTypeCount() const						{return commandTypes.size();}
	int getLevelCount() const							{return levels.size();}
//	const PetRules &getPetRules() const					{return petRules;}
	const Emanations &getEmanations() const				{return emanations;}
	bool isMultiBuild() const							{return multiBuild;}

	//cellmap
	bool *cellMap;

	int getStoredResourceCount() const					{return storedResources.size();}
	const Resource *getStoredResource(int i) const		{return &storedResources[i];}
	bool getCellMapCell(int x, int y) const				{return cellMap[size*y+x];}
	bool hasMeetingPoint() const						{return meetingPoint;}
	Texture2D *getMeetingPointImage() const				{return meetingPointImage;}
	StaticSound *getSelectionSound() const				{return selectionSounds.getRandSound();}
	StaticSound *getCommandSound() const				{return commandSounds.getRandSound();}

	int getStore(const ResourceType *rt) const;
	const SkillType *getSkillType(const string &skillName, SkillClass skillClass = scCount) const;
	const SkillType *getFirstStOfClass(SkillClass skillClass) const;
    const CommandType *getFirstCtOfClass(CommandClass commandClass) const;
    const HarvestCommandType *getFirstHarvestCommand(const ResourceType *resourceType) const;
	const AttackCommandType *getFirstAttackCommand(Field field) const;
	const RepairCommandType *getFirstRepairCommand(const UnitType *repaired) const;

	//has
    bool hasCommandType(const CommandType *commandType) const;
	bool hasCommandClass(CommandClass commandClass) const;
    bool hasSkillType(const SkillType *skillType) const;
    bool hasSkillClass(SkillClass skillClass) const;
	bool hasCellMap() const								{return cellMap!=NULL;}

	//is
	bool isOfClass(UnitClass uc) const;

	//find
	const CommandType* findCommandTypeById(int id) const;

private:
    void computeFirstStOfClass();
    void computeFirstCtOfClass();
};


}}//end namespace


#endif
