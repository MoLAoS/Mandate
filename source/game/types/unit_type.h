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
#include "factory.h"

namespace Glest{ namespace Game{

using Shared::Sound::StaticSound;
using Shared::Util::Checksum;
using Shared::Util::SingleFactory;

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
class UnitTypeFactory;
// ===============================
// 	class UnitType
//
///	A unit or building type
// ===============================

class UnitType : public ProducibleType, public UnitStats {
	friend class UnitTypeFactory;
private:
	typedef vector<SkillType*> SkillTypes;
	typedef vector<CommandType*> CommandTypes;
	typedef vector<Resource> StoredResources;
	typedef vector<Level> Levels;
	typedef vector<ParticleSystemType*> particleSystemTypes;
//	typedef vector<PetRule*> PetRules;
	//typedef map<int, const CommandType*> CommandTypeMap;

private:
	//basic
	bool multiBuild;
	bool multiSelect;

	//sounds
	SoundContainer selectionSounds;
	SoundContainer commandSounds;

	//info
	StoredResources storedResources;
	Levels levels;
//	PetRules petRules;
	Emanations emanations;

	//meeting point
	bool meetingPoint;
	Texture2D *meetingPointImage;

	CommandTypes commandTypes;
	CommandTypes commandTypesByClass[CommandClass::COUNT]; // command types mapped by CommandClass

	SkillTypes skillTypes;
	SkillTypes skillTypesByClass[SkillClass::COUNT];

	//REFACTOR: use above, remove these
	//OPTIMIZATIONS:
	//store first command type and skill type of each class
	//const CommandType *firstCommandTypeOfClass[CommandClass::COUNT];
	//const SkillType *firstSkillTypeOfClass[SkillClass::COUNT];
	fixed halfSize;
	fixed halfHeight;

public:
	//creation and loading
	UnitType();
	virtual ~UnitType();
	void preLoad(const string &dir);
	bool load(const string &dir, const TechTree *techTree, const FactionType *factionType);
	virtual void doChecksum(Checksum &checksum) const;

	//get
	bool getMultiSelect() const							{return multiSelect;}

	const SkillType *getSkillType(int i) const			{return skillTypes[i];}
	
	int getCommandTypeCount() const						{return commandTypes.size();}
	const CommandType *getCommandType(int i) const		{return commandTypes[i];}
	const CommandType *getCommandType(const string &name) const;
	
	template <typename ConcreteType>
	int getCommandTypeCount() const {
		return commandTypesByClass[ConcreteType::typeClass()].size();
	}
	template <typename ConcreteType>
	const ConcreteType* getCommandType(int i) const {
		return static_cast<const ConcreteType*>(commandTypesByClass[ConcreteType::typeClass()][i]);
	}
	const CommandTypes& getCommandTypes(CommandClass cc) const {
		return commandTypesByClass[cc];
	}
	const CommandType *getFirstCtOfClass(CommandClass cc) const {
		return commandTypesByClass[cc].empty() ? 0 : commandTypesByClass[cc].front();
	}
    const HarvestCommandType *getHarvestCommand(const ResourceType *resourceType) const;
	const AttackCommandType *getAttackCommand(Zone zone) const;
	const RepairCommandType *getRepairCommand(const UnitType *repaired) const;

	const Level *getLevel(int i) const					{return &levels[i];}
	int getSkillTypeCount() const						{return skillTypes.size();}
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
	bool getCellMapCell(int x, int y) const				{
		assert(size * y + x >= 0 && size * y + x < size * size);
		return cellMap[size * y + x];
	}

	// resources
	int getStoredResourceCount() const					{return storedResources.size();}
	const Resource *getStoredResource(int i) const		{return &storedResources[i];}
	int getStore(const ResourceType *rt) const;

	// meeting point
	bool hasMeetingPoint() const						{return meetingPoint;}
	Texture2D *getMeetingPointImage() const				{return meetingPointImage;}

	// sounds
	StaticSound *getSelectionSound() const				{return selectionSounds.getRandSound();}
	StaticSound *getCommandSound() const				{return commandSounds.getRandSound();}

	const SkillType *getSkillType(const string &skillName, SkillClass skillClass = SkillClass::COUNT) const;
	const SkillType *getFirstStOfClass(SkillClass sc) const {
		return skillTypesByClass[sc].empty() ? 0 : skillTypesByClass[sc].front();
	}

	// has
	bool hasCommandType(const CommandType *ct) const;
	bool hasCommandClass(CommandClass cc) const { return !commandTypesByClass[cc].empty(); }
    bool hasSkillType(const SkillType *skillType) const;
    bool hasSkillClass(SkillClass skillClass) const;
	bool hasCellMap() const								{return cellMap!=NULL;}

	// is
	bool isOfClass(UnitClass uc) const;

	// find
	// this is only used to convert NetworkCOmmand to Command, replace with a single map in CommandTypeFactory,
	// which will be taking control of ids... see comments in command_type.h [above decl. of resetIdCounter()]
	//const CommandType* findCommandTypeById(int id) const {
	//	CommandTypeMap::const_iterator it = commandTypeMap.find(id);
	//	return (it != commandTypeMap.end() ? it->second : 0);
	//}

private:
    void sortSkillTypes();
    void sortCommandTypes();
};


// ===============================
//  class UnitTypeFactory
// ===============================

class UnitTypeFactory: private SingleTypeFactory<UnitType> {
private:
	int idCounter;
	vector<UnitType *> types;

public:
	UnitTypeFactory() : idCounter(0) { }

	UnitType *newInstance() {
		UnitType *ut = SingleTypeFactory<UnitType>::newInstance();
		ut->setId(idCounter++);
		types.push_back(ut);
		return ut;
	}

	UnitType* getType(int id) {
		if (id < 0 || id >= types.size()) {
			throw runtime_error("Error: Unknown unit type id: " + intToStr(id));
		}
		return types[id];
	}
};

}}//end namespace


#endif
