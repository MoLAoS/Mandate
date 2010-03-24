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


#ifndef _GLEST_GAME_COMMANDTYPE_H_
#define _GLEST_GAME_COMMANDTYPE_H_

#include "element_type.h"
#include "resource_type.h"
#include "lang.h"
#include "skill_type.h"
#include "factory.h"
#include "xml_parser.h"
#include "sound_container.h"
#include "skill_type.h"
#include "upgrade_type.h"
#include "game_constants.h"

namespace Glest { namespace Game {

using Shared::Util::MultiFactory;

class UnitUpdater;
class Unit;
class UnitType;
class TechTree;
class FactionType;


class AttackSkillPreferences : public XmlBasedFlags<AttackSkillPreference, AttackSkillPreference::COUNT> {
public:
	void load(const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft) {
		XmlBasedFlags<AttackSkillPreference, AttackSkillPreference::COUNT>::load(node, dir, tt, ft, "flag", AttackSkillPreferenceNames);
	}
};

class AttackSkillTypes {
private:
	vector<const AttackSkillType*> types;
	vector<AttackSkillPreferences> associatedPrefs;
	int maxRange;
	Zones zones;
	AttackSkillPreferences allPrefs;

public:
	void init();
	int getMaxRange() const									{return maxRange;}
// const vector<const AttackSkillType*> &getTypes() const	{return types;}
	void getDesc(string &str, const Unit *unit) const;
	bool getZone(Zone zone) const						{return zones.get(zone);}
	bool hasPreference(AttackSkillPreference pref) const	{return allPrefs.get(pref);}
	const AttackSkillType *getPreferredAttack(const Unit *unit, const Unit *target, int rangeToTarget) const;
	const AttackSkillType *getSkillForPref(AttackSkillPreference pref, int rangeToTarget) const {
		assert(types.size() == associatedPrefs.size());
		for (int i = 0; i < types.size(); ++i) {
			if (associatedPrefs[i].get(pref) && types[i]->getMaxRange() >= rangeToTarget) {
				return types[i];
			}
		}
		return NULL;
	}

	void push_back(const AttackSkillType* ast, AttackSkillPreferences pref) {
		types.push_back(ast);
		associatedPrefs.push_back(pref);
	}

	void doChecksum(Checksum &checksum) const {
		for (int i=0; i < types.size(); ++i) {
			checksum.addString(types[i]->getName());
		}
	}
};

// =====================================================
//  class CommandType
//
/// A complex action performed by a unit, composed by skills
// =====================================================

class CommandType: public RequirableType {
protected:
	CommandClass cc;
	Clicks clicks;
	bool queuable;
	const UnitType *unitType;
	int unitTypeIndex;

private:
	static int nextId;
	static int getNextId() {
		return nextId++;
	}

public:
	CommandType(const char* name, CommandClass cc, Clicks clicks, bool queuable = false);

	virtual void update(UnitUpdater *unitUpdater, Unit *unit) const;
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void setUnitTypeAndIndex(const UnitType *unitType, int unitTypeIndex);
	virtual void getDesc(string &str, const Unit *unit) const = 0;
	virtual string toString() const						{return Lang::getInstance().get(name);}
	virtual const ProducibleType *getProduced() const	{return NULL;}
	bool isQueuable() const								{return queuable;}
	const UnitType *getUnitType() const					{return unitType;}
	int getUnitTypeIndex() const						{return unitTypeIndex;}
	

	//get
	CommandClass getClass() const						{assert(this); return cc;}
	Clicks getClicks() const							{return clicks;}
	string getDesc(const Unit *unit) const {
		string str;
		str = name + "\n";
		getDesc(str, unit);
		return str;
	}
};

// ===============================
//  class MoveBaseCommandType
// ===============================

class MoveBaseCommandType: public CommandType {
protected:
	const MoveSkillType *moveSkillType;

public:
	MoveBaseCommandType(const char* name, CommandClass commandTypeClass, Clicks clicks) :
			CommandType(name, commandTypeClass, clicks) {}
	virtual void doChecksum(Checksum &checksum) const {
		CommandType::doChecksum(checksum);
		checksum.addString(moveSkillType->getName());
	}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const	{moveSkillType->getDesc(str, unit);}
	const MoveSkillType *getMoveSkillType() const				{return moveSkillType;}
};

// ===============================
//  class StopBaseCommandType
// ===============================

class StopBaseCommandType: public CommandType {
protected:
	const StopSkillType *stopSkillType;

public:
	StopBaseCommandType(const char* name, CommandClass commandTypeClass, Clicks clicks) :
			CommandType(name, commandTypeClass, clicks) {}
	virtual void doChecksum(Checksum &checksum) const {
		CommandType::doChecksum(checksum);
		checksum.addString(stopSkillType->getName());
	}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const	{stopSkillType->getDesc(str, unit);}
	const StopSkillType *getStopSkillType() const				{return stopSkillType;}
};

// ===============================
//  class StopCommandType
// ===============================

class StopCommandType: public StopBaseCommandType {
public:
	StopCommandType() : StopBaseCommandType("Stop", CommandClass::STOP, Clicks::ONE) {}
};

// ===============================
//  class MoveCommandType
// ===============================

class MoveCommandType: public MoveBaseCommandType {
public:
	MoveCommandType() : MoveBaseCommandType("Move", CommandClass::MOVE, Clicks::TWO) {}
};

// ===============================
//  class AttackCommandTypeBase
// ===============================

class AttackCommandTypeBase {
protected:
	AttackSkillTypes attackSkillTypes;

public:
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType *ut);
	virtual void getDesc(string &str, const Unit *unit) const {attackSkillTypes.getDesc(str, unit);}
	virtual void doChecksum(Checksum &checksum) const {
		attackSkillTypes.doChecksum(checksum);
	}
// const AttackSkillType *getAttackSkillType() const	{return attackSkillTypes.begin()->first;}
// const AttackSkillType *getAttackSkillType(Field field) const;
	const AttackSkillTypes *getAttackSkillTypes() const	{return &attackSkillTypes;}
};

// ===============================
//  class AttackCommandType
// ===============================

class AttackCommandType: public MoveBaseCommandType, public AttackCommandTypeBase {

public:
	AttackCommandType(const char* name = "Attack", CommandClass commandTypeClass = CommandClass::ATTACK, Clicks clicks = Clicks::TWO) :
			MoveBaseCommandType(name, commandTypeClass, clicks) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const {
		MoveBaseCommandType::doChecksum(checksum);
		AttackCommandTypeBase::doChecksum(checksum);
	}
	virtual void getDesc(string &str, const Unit *unit) const {
		AttackCommandTypeBase::getDesc(str, unit);
		MoveBaseCommandType::getDesc(str, unit);
	}
};

// =======================================
//  class AttackStoppedCommandType
// =======================================

class AttackStoppedCommandType: public StopBaseCommandType, public AttackCommandTypeBase {
public:
	AttackStoppedCommandType() : StopBaseCommandType("AttackStopped", CommandClass::ATTACK_STOPPED, Clicks::ONE) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const {
		StopBaseCommandType::doChecksum(checksum);
		AttackCommandTypeBase::doChecksum(checksum);
	}
	virtual void getDesc(string &str, const Unit *unit) const {
		AttackCommandTypeBase::getDesc(str, unit);
	}
};


// ===============================
//  class BuildCommandType
// ===============================

class BuildCommandType: public MoveBaseCommandType {
private:
	const BuildSkillType* buildSkillType;
	vector<const UnitType*> buildings;
	SoundContainer startSounds;
	SoundContainer builtSounds;

public:
	BuildCommandType() : MoveBaseCommandType("Build", CommandClass::BUILD, Clicks::TWO) {}
	~BuildCommandType();
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const {
		buildSkillType->getDesc(str, unit);
	}

	//get
	const BuildSkillType *getBuildSkillType() const	{return buildSkillType;}
	int getBuildingCount() const					{return buildings.size();}
	const UnitType * getBuilding(int i) const		{return buildings[i];}
	StaticSound *getStartSound() const				{return startSounds.getRandSound();}
	StaticSound *getBuiltSound() const				{return builtSounds.getRandSound();}
};


// ===============================
//  class HarvestCommandType
// ===============================

class HarvestCommandType: public MoveBaseCommandType {
private:
	const MoveSkillType *moveLoadedSkillType;
	const HarvestSkillType *harvestSkillType;
	const StopSkillType *stopLoadedSkillType;
	vector<const ResourceType*> harvestedResources;
	int maxLoad;
	int hitsPerUnit;

public:
	HarvestCommandType() : MoveBaseCommandType("Harvest", CommandClass::HARVEST, Clicks::TWO) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;

	//get
	const MoveSkillType *getMoveLoadedSkillType() const		{return moveLoadedSkillType;}
	const HarvestSkillType *getHarvestSkillType() const		{return harvestSkillType;}
	const StopSkillType *getStopLoadedSkillType() const		{return stopLoadedSkillType;}
	int getMaxLoad() const									{return maxLoad;}
	int getHitsPerUnit() const								{return hitsPerUnit;}
	int getHarvestedResourceCount() const					{return harvestedResources.size();}
	const ResourceType* getHarvestedResource(int i) const	{return harvestedResources[i];}
	bool canHarvest(const ResourceType *resourceType) const;
};


// ===============================
//  class RepairCommandType
// ===============================

class RepairCommandType: public MoveBaseCommandType {
private:
	const RepairSkillType* repairSkillType;
	vector<const UnitType*>  repairableUnits;

public:
	RepairCommandType() : MoveBaseCommandType("Repair", CommandClass::REPAIR, Clicks::TWO) {}
	~RepairCommandType();
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;

	//get
	const RepairSkillType *getRepairSkillType() const	{return repairSkillType;}
	bool isRepairableUnitType(const UnitType *unitType) const;
};


// ===============================
//  class ProduceCommandType
// ===============================

class ProduceCommandType: public CommandType {
private:
	const ProduceSkillType* produceSkillType;
	const UnitType *producedUnit;

public:
	ProduceCommandType() : CommandType("Produce", CommandClass::PRODUCE, Clicks::ONE, true) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;

	virtual string getReqDesc() const;
	virtual const ProducibleType *getProduced() const;

	//get
	const ProduceSkillType *getProduceSkillType() const	{return produceSkillType;}
	const UnitType *getProducedUnit() const				{return producedUnit;}
};


// ===============================
//  class UpgradeCommandType
// ===============================

class UpgradeCommandType: public CommandType {
private:
	const UpgradeSkillType* upgradeSkillType;
	const UpgradeType* producedUpgrade;

public:
	UpgradeCommandType() : CommandType("Upgrade", CommandClass::UPGRADE, Clicks::ONE, true) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual string getReqDesc() const;
	virtual const ProducibleType *getProduced() const;
	virtual void getDesc(string &str, const Unit *unit) const {
		upgradeSkillType->getDesc(str, unit);
		str += "\n" + getProducedUpgrade()->getDesc();
	}

	//get
	const UpgradeSkillType *getUpgradeSkillType() const	{return upgradeSkillType;}
	const UpgradeType *getProducedUpgrade() const		{return producedUpgrade;}
};

// ===============================
//  class MorphCommandType
// ===============================

class MorphCommandType: public CommandType {
private:
	const MorphSkillType* morphSkillType;
	const UnitType* morphUnit;
	int discount;

public:
	MorphCommandType() : CommandType("Morph", CommandClass::MORPH, Clicks::ONE) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual string getReqDesc() const;
	virtual const ProducibleType *getProduced() const;

	//get
	const MorphSkillType *getMorphSkillType() const	{return morphSkillType;}
	const UnitType *getMorphUnit() const			{return morphUnit;}
	int getDiscount() const							{return discount;}
};


// ===============================
//  class CastSpellCommandType
// ===============================

class CastSpellCommandType: public MoveBaseCommandType {
private:
	const CastSpellSkillType* castSpellSkillType;

public:
	CastSpellCommandType() : MoveBaseCommandType("CastSpell", CommandClass::CAST_SPELL, Clicks::TWO) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const;
	const CastSpellSkillType * getCastSpellSkillType() const	{return castSpellSkillType;}
};

// ===============================
//  class GuardCommandType
// ===============================

class GuardCommandType: public AttackCommandType {
private:
	int maxDistance;

public:
	GuardCommandType(const char* name = "Guard", CommandClass commandTypeClass = CommandClass::GUARD, Clicks clicks = Clicks::TWO) :
			AttackCommandType(name, commandTypeClass, clicks) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	int getMaxDistance() const {return maxDistance;}
};

// ===============================
//  class PatrolCommandType
// ===============================

class PatrolCommandType: public GuardCommandType {
public:
	PatrolCommandType() : GuardCommandType("Patrol", CommandClass::PATROL, Clicks::TWO) {}
};


// ===============================
//  class SetMeetingPointCommandType
// ===============================

class SetMeetingPointCommandType: public CommandType {
public:
	SetMeetingPointCommandType() :
			CommandType("SetMeetingPoint", CommandClass::SET_MEETING_POINT, Clicks::TWO) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {return true;}
	virtual void getDesc(string &str, const Unit *unit) const {}
};

// ===============================
//  class CommandFactory
// ===============================

class CommandTypeFactory: public MultiFactory<CommandType> {
private:
	CommandTypeFactory();

public:
	static CommandTypeFactory &getInstance();
};

}}//end namespace

#endif
