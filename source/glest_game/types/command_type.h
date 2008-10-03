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

namespace Glest{ namespace Game{

using Shared::Util::MultiFactory;

class UnitUpdater;
class Unit;
class UnitType;
class TechTree;
class FactionType;

enum CommandClass{
	ccStop,
	ccMove,
	ccAttack,
	ccAttackStopped,
	ccBuild,
	ccHarvest,
	ccRepair,
	ccProduce,
	ccUpgrade,
	ccMorph,
	ccCastSpell,
	ccGuard,
	ccPatrol,
	ccSetMeetingPoint,

	ccCount,
	ccNull
};

enum Clicks{
	cOne,
	cTwo
};

enum AttackSkillPreference {
	aspWheneverPossible,
	aspAtMaxRange,
	aspOnLarge,
	aspOnBuilding,
	aspWhenDamaged,

	aspCount
};

class AttackSkillPreferences : public XmlBasedFlags<AttackSkillPreference, aspCount>{
private:
	static const char *names[aspCount];

public:
	void load(const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft) {
		XmlBasedFlags<AttackSkillPreference, aspCount>::load(node, dir, tt, ft, "flag", names);
	}
};

class AttackSkillTypes {
private:
	vector<const AttackSkillType*> types;
	vector<AttackSkillPreferences> associatedPrefs;
	int maxRange;
	Fields fields;
	AttackSkillPreferences allPrefs;

public:
	void init();
	int getMaxRange() const									{return maxRange;}
//	const vector<const AttackSkillType*> &getTypes() const	{return types;}
	void getDesc(string &str, const Unit *unit) const;
	bool getField(Field field) const						{return fields.get(field);}
	bool hasPreference(AttackSkillPreference pref) const	{return allPrefs.get(pref);}
	const AttackSkillType *getPreferredAttack(const Unit *unit, const Unit *target, int rangeToTarget) const;
	const AttackSkillType *getSkillForPref(AttackSkillPreference pref, int rangeToTarget) const {
		assert(types.size() == associatedPrefs.size());
		for(int i = 0; i < types.size(); ++i) {
			if(associatedPrefs[i].get(pref) && types[i]->getMaxRange() >= rangeToTarget) {
				return types[i];
			}
		}
		return NULL;
	}

	void push_back(const AttackSkillType* ast, AttackSkillPreferences pref) {
		types.push_back(ast);
		associatedPrefs.push_back(pref);
	}
};

// =====================================================
// 	class CommandType
//
///	A complex action performed by a unit, composed by skills
// =====================================================

class CommandType: public RequirableType{
protected:
	CommandClass commandTypeClass;
	Clicks clicks;
	int id;
	const char* typeName;
	bool queuable;

public:
	static const int invalidId= -1;

public:
	CommandType(CommandClass commandTypeClass, Clicks clicks, const char* typeName, bool queuable = false);
	virtual void update(UnitUpdater *unitUpdater, Unit *unit) const;
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
	virtual void getDesc(string &str, const Unit *unit) const= 0;
	virtual string toString() const						{return Lang::getInstance().get(typeName);}
	virtual const ProducibleType *getProduced() const	{return NULL;}
	bool isQueuable() const								{return queuable;}
	string getDesc(const Unit *unit) const {
		string str;
		str= name + "\n";
		getDesc(str, unit);
		return str;
	}

	//get
	CommandClass getClass() const;
	Clicks getClicks() const		{return clicks;}
	int getId() const				{return id;}
};

// ===============================
// 	class MoveBaseCommandType
// ===============================

class MoveBaseCommandType: public CommandType{
protected:
	const MoveSkillType *moveSkillType;

public:
	MoveBaseCommandType(CommandClass commandTypeClass, Clicks clicks, const char* typeName) :
		CommandType(commandTypeClass, clicks, typeName){}
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
	virtual void getDesc(string &str, const Unit *unit) const {
		moveSkillType->getDesc(str, unit);
	}

	const MoveSkillType *getMoveSkillType() const	{return moveSkillType;}
};

// ===============================
// 	class StopBaseCommandType
// ===============================

class StopBaseCommandType: public CommandType{
protected:
	const StopSkillType *stopSkillType;

public:
	StopBaseCommandType(CommandClass commandTypeClass, Clicks clicks, const char* typeName) :
		CommandType(commandTypeClass, clicks, typeName){}
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
	virtual void getDesc(string &str, const Unit *unit) const {
		stopSkillType->getDesc(str, unit);
	}

	const StopSkillType *getStopSkillType() const	{return stopSkillType;}
};

// ===============================
// 	class StopCommandType
// ===============================

class StopCommandType: public StopBaseCommandType{
public:
	StopCommandType() : StopBaseCommandType(ccStop, cOne, "Stop"){}
};

// ===============================
// 	class MoveCommandType
// ===============================

class MoveCommandType: public MoveBaseCommandType{
public:
	MoveCommandType() : MoveBaseCommandType(ccMove, cTwo, "Move"){}
};

// ===============================
// 	class AttackCommandTypeBase
// ===============================

class AttackCommandTypeBase {
protected:
	AttackSkillTypes attackSkillTypes;

public:
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
	virtual void getDesc(string &str, const Unit *unit) const {attackSkillTypes.getDesc(str, unit);}

//	const AttackSkillType *getAttackSkillType() const		{return attackSkillTypes.begin()->first;}
//	const AttackSkillType *getAttackSkillType(Field field) const;
	const AttackSkillTypes *getAttackSkillTypes() const		{return &attackSkillTypes;}
};

// ===============================
// 	class AttackCommandType
// ===============================

class AttackCommandType: public MoveBaseCommandType, public AttackCommandTypeBase {

public:
	AttackCommandType(CommandClass commandTypeClass = ccAttack, Clicks clicks = cTwo, const char* typeName = "Attack") :
			MoveBaseCommandType(commandTypeClass, clicks, typeName){}
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
	virtual void getDesc(string &str, const Unit *unit) const{
		AttackCommandTypeBase::getDesc(str, unit);
		MoveBaseCommandType::getDesc(str, unit);
	}
};

// =======================================
// 	class AttackStoppedCommandType
// =======================================

class AttackStoppedCommandType: public StopBaseCommandType, public AttackCommandTypeBase {
public:
	AttackStoppedCommandType() : StopBaseCommandType(ccAttackStopped, cOne, "AttackStopped"){}
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
	virtual void getDesc(string &str, const Unit *unit) const {
		AttackCommandTypeBase::getDesc(str, unit);
	}
};


// ===============================
// 	class BuildCommandType
// ===============================

class BuildCommandType: public MoveBaseCommandType{
private:
	const BuildSkillType* buildSkillType;
	vector<const UnitType*> buildings;
	SoundContainer startSounds;
	SoundContainer builtSounds;

public:
	BuildCommandType() : MoveBaseCommandType(ccBuild, cTwo, "Build"){}
	~BuildCommandType();
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
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
// 	class HarvestCommandType
// ===============================

class HarvestCommandType: public MoveBaseCommandType{
private:
	const MoveSkillType *moveLoadedSkillType;
	const HarvestSkillType *harvestSkillType;
	const StopSkillType *stopLoadedSkillType;
	vector<const ResourceType*> harvestedResources;
	int maxLoad;
	int hitsPerUnit;

public:
	HarvestCommandType() : MoveBaseCommandType(ccHarvest, cTwo, "Harvest"){}
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
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
// 	class RepairCommandType
// ===============================

class RepairCommandType: public MoveBaseCommandType{
private:
	const RepairSkillType* repairSkillType;
	vector<const UnitType*>  repairableUnits;

public:
	RepairCommandType() : MoveBaseCommandType(ccRepair, cTwo, "Repair"){}
	~RepairCommandType();
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
	virtual void getDesc(string &str, const Unit *unit) const;

	//get
	const RepairSkillType *getRepairSkillType() const		{return repairSkillType;}
	bool isRepairableUnitType(const UnitType *unitType) const;
};


// ===============================
// 	class ProduceCommandType
// ===============================

class ProduceCommandType: public CommandType{
private:
	const ProduceSkillType* produceSkillType;
	const UnitType *producedUnit;

public:
	ProduceCommandType() : CommandType(ccProduce, cOne, "Produce", true){}
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
	virtual void getDesc(string &str, const Unit *unit) const;

	virtual string getReqDesc() const;
	virtual const ProducibleType *getProduced() const;

	//get
	const ProduceSkillType *getProduceSkillType() const	{return produceSkillType;}
	const UnitType *getProducedUnit() const				{return producedUnit;}
};


// ===============================
// 	class UpgradeCommandType
// ===============================

class UpgradeCommandType: public CommandType{
private:
	const UpgradeSkillType* upgradeSkillType;
	const UpgradeType* producedUpgrade;

public:
	UpgradeCommandType() : CommandType(ccUpgrade, cOne, "Upgrade", true){}
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
	virtual string getReqDesc() const;
	virtual const ProducibleType *getProduced() const;
	virtual void getDesc(string &str, const Unit *unit) const {
		upgradeSkillType->getDesc(str, unit);
		str+= "\n"+getProducedUpgrade()->getDesc();
	}

	//get
	const UpgradeSkillType *getUpgradeSkillType() const		{return upgradeSkillType;}
	const UpgradeType *getProducedUpgrade() const			{return producedUpgrade;}
};

// ===============================
// 	class MorphCommandType
// ===============================

class MorphCommandType: public CommandType{
private:
	const MorphSkillType* morphSkillType;
	const UnitType* morphUnit;
	int discount;

public:
	MorphCommandType() : CommandType(ccMorph, cOne, "Morph"){}
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual string getReqDesc() const;
	virtual const ProducibleType *getProduced() const;

	//get
	const MorphSkillType *getMorphSkillType() const		{return morphSkillType;}
	const UnitType *getMorphUnit() const				{return morphUnit;}
	int getDiscount() const								{return discount;}
};


// ===============================
// 	class CastSpellCommandType
// ===============================

class CastSpellCommandType: public MoveBaseCommandType{
private:
	const CastSpellSkillType* castSpellSkillType;

public:
	CastSpellCommandType() : MoveBaseCommandType(ccCastSpell, cTwo, "CastSpell"){}
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
		virtual void getDesc(string &str, const Unit *unit) const;
	const CastSpellSkillType * getCastSpellSkillType() const{return castSpellSkillType;}
};

// ===============================
// 	class GuardCommandType
// ===============================

class GuardCommandType: public AttackCommandType{
private:
	int maxDistance;

public:
	GuardCommandType(CommandClass commandTypeClass = ccGuard, Clicks clicks = cTwo, const char* typeName = "Guard") :
			AttackCommandType(commandTypeClass, clicks, typeName){}
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut);
	int getMaxDistance() const							{return maxDistance;}
};

// ===============================
// 	class PatrolCommandType
// ===============================

class PatrolCommandType: public GuardCommandType{
private:

public:
	PatrolCommandType() : GuardCommandType(ccPatrol, cTwo, "Patrol"){}
};


// ===============================
// 	class SetMeetingPointCommandType
// ===============================

class SetMeetingPointCommandType: public CommandType{
public:
	SetMeetingPointCommandType() :
		CommandType(ccSetMeetingPoint, cOne, "SetMeetingPoint"){}
	virtual void load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){}
	virtual void getDesc(string &str, const Unit *unit) const {}

	static const SetMeetingPointCommandType *getInstance() {
		static SetMeetingPointCommandType singleton;
		return &singleton;
	}
};

// ===============================
// 	class CommandFactory
// ===============================

class CommandTypeFactory: public MultiFactory<CommandType>{
private:
	CommandTypeFactory();

public:
	static CommandTypeFactory &getInstance();
};

}}//end namespace

#endif
