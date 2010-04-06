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
#include "particle_type.h"

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

class Level: public EnhancementType, public NameIdPair {
private:
	int kills;

public:
	Level() : EnhancementType() {
		const fixed onePointFive = fixed(3) / 2;
		maxHpMult = onePointFive;
		maxEpMult = onePointFive;
		sightMult = fixed(6) / 5;
		armorMult = onePointFive;
		effectStrength = fixed(1) / 10;
	}

	virtual bool load(const XmlNode *prn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const {
		NameIdPair::doChecksum(checksum);
		EnhancementType::doChecksum(checksum);
	}
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

// ===============================
// 	class UnitType
//
///	A unit or building type
// ===============================


class UnitType: public ProducibleType, public UnitStats {
private:
	typedef vector<SkillType*> SkillTypes;
	typedef vector<CommandType*> CommandTypes;
	typedef vector<Resource> StoredResources;
	typedef vector<Level> Levels;
	typedef vector<ParticleSystemType*> particleSystemTypes;
//	typedef vector<PetRule*> PetRules;

private:
	//basic
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

	//REFACTOR: types are limited in the number, and more than just the first command/Skill-TypeOfClass() 
	// is often of interest, let's have vectors per Command/Skill-class for all Command/Skill-Types

	// then, instead of iterating over a vector of all command types and checking their class,
	// we can just get _all_ command types of a class

	//REFACTOR: in trunk, uncomment these.
	//CommandTypes commandTypesByClass[CommandClass::COUNT];
	//SkillTypes skillTypesByClass[SkillClass::COUNT];

	//REFACTOR: use above, remove these
	//OPTIMIZATIONS:
	//store first command type and skill type of each class
	const CommandType *firstCommandTypeOfClass[CommandClass::COUNT];
	const SkillType *firstSkillTypeOfClass[SkillClass::COUNT];
	fixed halfSize;
	fixed halfHeight;

public:
	//creation and loading
	UnitType();
	virtual ~UnitType();
	void preLoad(const string &dir);
	bool load(int id, const string &dir, const TechTree *techTree, const FactionType *factionType);
	virtual void doChecksum(Checksum &checksum) const;

	//get
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
	fixed getHalfSize() const							{return halfSize;}
	fixed getHalfHeight() const							{return halfHeight;}
	bool isMobile () const {
		const SkillType *st = getFirstStOfClass(SkillClass::MOVE);
		return st && st->getSpeed() > 0 ? true: false;
	}
	//cellmap
	bool *cellMap;

	int getStoredResourceCount() const					{return storedResources.size();}
	const Resource *getStoredResource(int i) const		{return &storedResources[i];}
	bool getCellMapCell(int x, int y) const				{
		assert(size * y + x >= 0 && size * y + x < size * size);
		return cellMap[size * y + x];
	}
	bool hasMeetingPoint() const						{return meetingPoint;}
	Texture2D *getMeetingPointImage() const				{return meetingPointImage;}
	StaticSound *getSelectionSound() const				{return selectionSounds.getRandSound();}
	StaticSound *getCommandSound() const				{return commandSounds.getRandSound();}

	int getStore(const ResourceType *rt) const;
	const SkillType *getSkillType(const string &skillName, SkillClass skillClass = SkillClass::COUNT) const;

	const CommandType *getFirstCtOfClass(CommandClass commandClass) const {return firstCommandTypeOfClass[commandClass];}
	const SkillType *getFirstStOfClass(SkillClass skillClass) const {return firstSkillTypeOfClass[skillClass];}
    const HarvestCommandType *getFirstHarvestCommand(const ResourceType *resourceType) const;
	const AttackCommandType *getFirstAttackCommand(Zone zone) const;
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
