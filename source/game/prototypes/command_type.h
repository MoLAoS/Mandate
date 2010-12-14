// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//                2008-2009 Daniel Santos
//                2009-2010 James McCulloch
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

#include "prototypes_enums.h"
#include "entities_enums.h"
#include <set>
using std::set;

// All command update methods begin with _PROFILE_COMMAND_UPDATE()
// This allows profiling of command updates to be switched on/off independently
// of other profiling macros
#if 0
#	define _PROFILE_COMMAND_UPDATE() _PROFILE_FUNCTION()
#else
#	define _PROFILE_COMMAND_UPDATE()
#endif

#if defined(LOG_BUILD_COMMAND) && LOG_BUILD_COMMAND
#	define BUILD_LOG(x) STREAM_LOG(x)
#else
#	define BUILD_LOG(x)
#endif

#if defined(LOG_REPAIR_COMMAND) && LOG_REPAIR_COMMAND
#	if LOG_REPAIR_COMMAND > 1
#		define REPAIR_LOG(x) STREAM_LOG(x)
#		define REPAIR_LOG2(x) STREAM_LOG(x)
#	else
#		define REPAIR_LOG(x) STREAM_LOG(x)
#		define REPAIR_LOG2(x)
#	endif
#else
#	define REPAIR_LOG(x)
#	define REPAIR_LOG2(x)
#endif

#if defined(LOG_HARVEST_COMMAND) && LOG_HARVEST_COMMAND
#	define HARVEST_LOG(x) STREAM_LOG(x)
#else
#	define HARVEST_LOG(x)
#endif

using Shared::Util::MultiFactory;
using namespace Glest::Entities;
using Glest::Gui::Clicks;

namespace Glest { namespace ProtoTypes {
using Search::CardinalDir;

// =====================================================
//  class CommandType
//
/// A complex action performed by a unit, composed by skills
// =====================================================

class CommandType : public RequirableType {
	friend class Glest::Sim::CommandTypeFactory;
protected:
	Clicks clicks;
	bool queuable;
	const UnitType *unitType;
	string m_tipKey;

	void setIdAndUnitType(int v, UnitType *ut) { id = v; unitType = ut; }

	string emptyString;

public:
	CommandType(const char* name, Clicks clicks, bool queuable = false);

	virtual void update(Unit *unit) const = 0;
	virtual void getDesc(string &str, const Unit *unit) const = 0;

	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;

	const UnitType* getUnitType() const { return unitType; }

	virtual string toString() const						{return Lang::getInstance().get(name);}
	const string& getTipKey() const						{return m_tipKey;}
	virtual string getTipKey(const string &name) const  {return emptyString;}

	virtual int getProducedCount() const					{return 0;}
	virtual const ProducibleType *getProduced(int i) const{return 0;}

	// candidates for lua callbacks
	/** Apply costs for a command. */
	virtual void apply(Faction *faction, const Command &command) const;
	/** De-Apply costs for a command */
	virtual void undo(Unit *unit, const Command &command) const;
	/** Check if a command can be executed */
	virtual CommandResult check(const Unit *unit, const Command &command) const { return CommandResult::SUCCESS; }
	/** Update the command by one tick. */
	virtual void tick(const Unit *unit, Command &command) const {}
	/** Final actions to finish */
	virtual void finish(Unit *unit, Command &command) const {}
	/** Beginning actions before executing */
	virtual void start(Unit *unit, Command *command) const {}
	//init();

	bool isQueuable() const								{return queuable;}

	//get
	virtual CommandClass getClass() const = 0;
	virtual Clicks getClicks() const					{return clicks;}
	string getDesc(const Unit *unit) const {
		string str;
		//str = name + "\n";
		getDesc(str, unit);
		return str;
	}

protected:
	// static command update helpers... don't really belong here, but it's convenient for now
	///@todo move range checking update helpers to Cartographer (?)
	/// See also: RepairCommandType::repairableInRange()
	static bool unitInRange(const Unit *unit, int range, Unit **rangedPtr,
								const AttackSkillTypes *asts, const AttackSkillType **past);

	static bool attackerInSight(const Unit *unit, Unit **rangedPtr);

	static bool attackableInRange(const Unit *unit, Unit **rangedPtr, 
								const AttackSkillTypes *asts, const AttackSkillType **past);

	static bool attackableInSight(const Unit *unit, Unit **rangedPtr, 
								const AttackSkillTypes *asts, const AttackSkillType **past);

	void replaceDeadReferences(Command &command) const;

public:
	Command* doAutoCommand(Unit *unit) const;
};

// ===============================
//  class MoveBaseCommandType
// ===============================

class MoveBaseCommandType: public CommandType {
protected:
	const MoveSkillType *m_moveSkillType;

public:
	MoveBaseCommandType(const char* name, Clicks clicks) : CommandType(name, clicks), m_moveSkillType(0) {}
	virtual void doChecksum(Checksum &checksum) const {
		CommandType::doChecksum(checksum);
		checksum.add(m_moveSkillType->getName());
	}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const	{m_moveSkillType->getDesc(str, unit);}
	const MoveSkillType *getMoveSkillType() const				{return m_moveSkillType;}

public:
	Command* doAutoFlee(Unit *unit) const;
};

// ===============================
//  class StopBaseCommandType
// ===============================

class StopBaseCommandType: public CommandType {
protected:
	const StopSkillType *m_stopSkillType;

public:
	StopBaseCommandType(const char* name, Clicks clicks) : CommandType(name, clicks), m_stopSkillType(0) {}
	virtual void doChecksum(Checksum &checksum) const {
		CommandType::doChecksum(checksum);
		checksum.add(m_stopSkillType->getName());
	}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const	{m_stopSkillType->getDesc(str, unit);}
	const StopSkillType *getStopSkillType() const				{return m_stopSkillType;}
};

// ===============================
//  class StopCommandType
// ===============================

class StopCommandType: public StopBaseCommandType {
public:
	StopCommandType() : StopBaseCommandType("Stop", Clicks::ONE) {}
	virtual void update(Unit *unit) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::STOP; }
};

// ===============================
//  class MoveCommandType
// ===============================

class MoveCommandType: public MoveBaseCommandType {
public:
	MoveCommandType() : MoveBaseCommandType("Move", Clicks::TWO) {}
	virtual void update(Unit *unit) const;
	virtual void tick(const Unit *unit, Command &command) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::MOVE; }
};

// ===============================
//  class TeleportCommandType
// ===============================

class TeleportCommandType: public MoveBaseCommandType {
public:
	TeleportCommandType() : MoveBaseCommandType("Teleport", Clicks::TWO) {}
	virtual void update(Unit *unit) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::TELEPORT; }
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
	AttackCommandType(const char* name = "Attack", Clicks clicks = Clicks::TWO) :
			MoveBaseCommandType(name, clicks) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const {
		MoveBaseCommandType::doChecksum(checksum);
		AttackCommandTypeBase::doChecksum(checksum);
	}
	virtual void getDesc(string &str, const Unit *unit) const {
		MoveBaseCommandType::getDesc(str, unit);
		AttackCommandTypeBase::getDesc(str, unit);
	}

	bool updateGeneric(Unit *unit, Command *command, const AttackCommandType *act, Unit* target, const Vec2i &targetPos) const;

	virtual void update(Unit *unit) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::ATTACK; }

public:
	Command* doAutoAttack(Unit *unit) const;
};

// =======================================
//  class AttackStoppedCommandType
// =======================================

class AttackStoppedCommandType: public StopBaseCommandType, public AttackCommandTypeBase {
public:
	AttackStoppedCommandType() : StopBaseCommandType("AttackStopped", Clicks::ONE) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const {
		StopBaseCommandType::doChecksum(checksum);
		AttackCommandTypeBase::doChecksum(checksum);
	}
	virtual void getDesc(string &str, const Unit *unit) const {
		AttackCommandTypeBase::getDesc(str, unit);
	}
	virtual void update(Unit *unit) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::ATTACK_STOPPED; }

public:
	Command* doAutoAttack(Unit *unit) const;
};


// ===============================
//  class BuildCommandType
// ===============================

class BuildCommandType: public MoveBaseCommandType {
private:
	const BuildSkillType*	m_buildSkillType;
	vector<const UnitType*> m_buildings;
	map<string, string>		m_tipKeys;
	SoundContainer			m_startSounds;
	SoundContainer			m_builtSounds;

public:
	BuildCommandType() : MoveBaseCommandType("Build", Clicks::TWO), m_buildSkillType(0) {}
	~BuildCommandType();
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const {
		m_buildSkillType->getDesc(str, unit);
	}
	virtual void update(Unit *unit) const;

	// prechecks
	/** @param builtUnitType the unitType to build
	  * @param pos the position to put the unit */
	bool isBlocked(const UnitType *builtUnitType, const Vec2i &pos, CardinalDir facing) const;
	virtual CommandResult check(const Unit *unit, const Command &command) const;
	virtual void undo(Unit *unit, const Command &command) const;

	//get
	const BuildSkillType *getBuildSkillType() const	{return m_buildSkillType;}

	virtual int getProducedCount() const					{return m_buildings.size();}
	virtual const ProducibleType *getProduced(int i) const;

	int getBuildingCount() const					{return m_buildings.size();}
	const UnitType * getBuilding(int i) const		{return m_buildings[i];}

	string getTipKey(const string &name) const  {
		map<string,string>::const_iterator it = m_tipKeys.find(name);
		return it->second;
	}

	StaticSound *getStartSound() const				{return m_startSounds.getRandSound();}
	StaticSound *getBuiltSound() const				{return m_builtSounds.getRandSound();}

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::BUILD; }

private:
	bool hasArrived(Unit *unit, const Command *command, const UnitType *builtUnitType) const;
	void existingBuild(Unit *unit, Command *command, Unit *builtUnit) const;
	/** @returns true if successful */
	bool attemptMoveUnits(const vector<Unit *> &occupants) const;
	void blockedBuild(Unit *unit) const;
	void acceptBuild(Unit *unit, Command *command, const UnitType *builtUnitType) const;
	void continueBuild(Unit *unit, const Command *command, const UnitType *builtUnitType) const;
};


// ===============================
//  class HarvestCommandType
// ===============================

class HarvestCommandType: public MoveBaseCommandType {
private:
	const MoveSkillType*		m_moveLoadedSkillType;
	const HarvestSkillType*		m_harvestSkillType;
	const StopSkillType*		m_stopLoadedSkillType;
	vector<const ResourceType*> m_harvestedResources;
	int							m_maxLoad;
	int							m_hitsPerUnit;

public:
	HarvestCommandType() : MoveBaseCommandType("Harvest", Clicks::TWO)
		, m_moveLoadedSkillType(0), m_harvestSkillType(0), m_stopLoadedSkillType(0)
		, m_maxLoad(0), m_hitsPerUnit(0) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void update(Unit *unit) const;

	//get
	const MoveSkillType *getMoveLoadedSkillType() const		{return m_moveLoadedSkillType;}
	const HarvestSkillType *getHarvestSkillType() const		{return m_harvestSkillType;}
	const StopSkillType *getStopLoadedSkillType() const		{return m_stopLoadedSkillType;}
	int getMaxLoad() const									{return m_maxLoad;}
	int getHitsPerUnit() const								{return m_hitsPerUnit;}
	int getHarvestedResourceCount() const					{return m_harvestedResources.size();}
	const ResourceType* getHarvestedResource(int i) const	{return m_harvestedResources[i];}
	bool canHarvest(const ResourceType *resourceType) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::HARVEST; }
};

// ===============================
//  class RepairCommandType
// ===============================

class RepairCommandType: public MoveBaseCommandType {
private:
	const RepairSkillType* repairSkillType;
	vector<const UnitType*>  repairableUnits;

public:
	RepairCommandType() : MoveBaseCommandType("Repair", Clicks::TWO), repairSkillType(0) {}
	~RepairCommandType() {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void update(Unit *unit) const;
	virtual void tick(const Unit *unit, Command &command) const;

	//get
	const RepairSkillType *getRepairSkillType() const	{return repairSkillType;}
	bool canRepair(const UnitType *unitType) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::REPAIR; }

protected:
	///@todo move to Cartographer, generalise so that the same code can be used for searching 
	// for bad guys to kill/run-from and looking for friendlies to repair.
	static bool repairableInRange(const Unit *unit, Vec2i centre, int centreSize, Unit **rangedPtr, 
			const RepairCommandType *rct, const RepairSkillType *rst, int range, 
			bool allowSelf, bool militaryOnly, bool damagedOnly);

	static bool repairableInRange(const Unit *unit, Unit **rangedPtr, const RepairCommandType *rct,
			int range, bool allowSelf = false, bool militaryOnly = false, bool damagedOnly = true);

	static bool repairableInSight(const Unit *unit, Unit **rangedPtr, const RepairCommandType *rct, bool allowSelf);

public:
	Command* doAutoRepair(Unit *unit) const;
};

// ===============================
//  class ProduceCommandType
// ===============================

class ProduceCommandType: public CommandType {
private:
	const ProduceSkillType* m_produceSkillType;
//	const UnitType *m_producedUnit;
	vector<const UnitType*> m_producedUnits;
	map<string, string>		m_tipKeys;
	SoundContainer			m_finishedSounds;

public:
	ProduceCommandType() : CommandType("Produce", Clicks::ONE, true), m_produceSkillType(0) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void update(Unit *unit) const;
	virtual string getReqDesc() const;
	
	// get
	virtual int getProducedCount() const	{return m_producedUnits.size();}
	virtual const ProducibleType* getProduced(int i) const;

	const UnitType *getProducedUnit(int i) const		{return m_producedUnits[i];}
	int getProducedUnitCount() const		{return m_producedUnits.size();}

	string getTipKey(const string &name) const  {
		map<string,string>::const_iterator it = m_tipKeys.find(name);
		return it->second;
	}

	StaticSound *getFinishedSound() const	{return m_finishedSounds.getRandSound();}

	const ProduceSkillType *getProduceSkillType() const	{return m_produceSkillType;}
	virtual Clicks getClicks() const	{ return m_producedUnits.size() == 1 ? Clicks::ONE : Clicks::TWO; }

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::PRODUCE; }
};

// ===============================
//  class GenerateCommandType
// ===============================

class GenerateCommandType: public CommandType {
private:
	const ProduceSkillType*			m_produceSkillType;
	vector<const GeneratedType*>	m_producibles;
	map<string, string>				m_tipKeys;
	SoundContainer					m_finishedSounds;

public:
	GenerateCommandType() : CommandType("Generate", Clicks::ONE, true), m_produceSkillType(0) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void update(Unit *unit) const;
	virtual string getReqDesc() const;
	StaticSound *getFinishedSound() const	{return m_finishedSounds.getRandSound();}

	//get
	const ProduceSkillType *getProduceSkillType() const	{return m_produceSkillType;}

	virtual int getProducedCount() const	{return m_producibles.size();}
	virtual const ProducibleType *getProduced(int i) const	{return m_producibles[i];}

	string getTipKey(const string &name) const  {
		map<string,string>::const_iterator it = m_tipKeys.find(name);
		return it->second;
	}

	virtual Clicks getClicks() const	{ return m_producibles.size() == 1 ? Clicks::ONE : Clicks::TWO; }

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::GENERATE; }
};

// ===============================
//  class UpgradeCommandType
// ===============================

class UpgradeCommandType: public CommandType {
private:
	const UpgradeSkillType* m_upgradeSkillType;
	const UpgradeType*		m_producedUpgrade;
	SoundContainer			m_finishedSounds;

public:
	UpgradeCommandType() : CommandType("Upgrade", Clicks::ONE, true), m_upgradeSkillType(0), m_producedUpgrade(0)  {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual string getReqDesc() const;
	virtual const ProducibleType *getProduced() const;
	virtual void getDesc(string &str, const Unit *unit) const {
		m_upgradeSkillType->getDesc(str, unit);
		str += "\n" + getProducedUpgrade()->getDesc();
	}
	StaticSound *getFinishedSound() const	{return m_finishedSounds.getRandSound();}
	virtual void update(Unit *unit) const;
	virtual void start(Unit *unit, Command &command) const;

	//get
	virtual int getProducedCount() const	{return 1;}
	virtual const ProducibleType *getProduced(int i) const	{assert(!i); return m_producedUpgrade;}

	const UpgradeSkillType *getUpgradeSkillType() const	{return m_upgradeSkillType;}
	const UpgradeType *getProducedUpgrade() const		{return m_producedUpgrade;}

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::UPGRADE; }

	virtual void apply(Faction *faction, const Command &command) const;
	virtual void undo(Unit *unit, const Command &command) const;
	virtual CommandResult check(const Unit *unit, const Command &command) const;
};

// ===============================
//  class MorphCommandType
// ===============================

class MorphCommandType: public CommandType {
private:
	const MorphSkillType*	m_morphSkillType;
	vector<const UnitType*> m_morphUnits;
	map<string, string>		m_tipKeys;
	int						m_discount;
	SoundContainer			m_finishedSounds;

public:
	MorphCommandType() : CommandType("Morph", Clicks::ONE), m_morphSkillType(0), m_discount(0) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void update(Unit *unit) const;
	virtual string getReqDesc() const;
	
	// get
	virtual int getProducedCount() const	{return m_morphUnits.size();}
	virtual const ProducibleType* getProduced(int i) const;
	
	int getMorphUnitCount() const					{return m_morphUnits.size();}
	const UnitType *getMorphUnit(int i) const		{return m_morphUnits[i];}

	string getTipKey(const string &name) const  {
		map<string,string>::const_iterator it = m_tipKeys.find(name);
		return it->second;
	}

	StaticSound *getFinishedSound() const	{return m_finishedSounds.getRandSound();}

	const MorphSkillType *getMorphSkillType() const	{return m_morphSkillType;}
	Clicks getClicks() const						{return m_morphUnits.size() == 1 ? Clicks::ONE : Clicks::TWO;}
	int getDiscount() const							{return m_discount;}

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::MORPH; }
};

// ===============================
//  class LoadCommandType
// ===============================

class LoadCommandType: public CommandType {
private:
	const MoveSkillType *moveSkillType;
	const LoadSkillType *loadSkillType;
	vector<const UnitType*> m_canLoadList;
	int		m_loadCapacity;
	bool	m_allowProjectiles;
	Vec2f	m_projectileOffsets;
/*  ///@todo: implement these:
	bool m_countCells;	// if true, a size 2 unit occupies 4 slots
	bool m_countSize;	// if true, a height 2 occupies 2 slots
						// if both true, a size 2 height 2 unit would occupy 8
	// OR ... add a unit 'space' param...
*/
public:
	LoadCommandType() 
			: CommandType("Load", Clicks::TWO)
			, loadSkillType(0), moveSkillType(0), m_loadCapacity(0), m_allowProjectiles(false) {
		queuable = true;
	}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void update(Unit *unit) const;
	virtual string getReqDesc() const;
	virtual CommandResult check(const Unit *unit, const Command &command) const;
	virtual void start(Unit *unit, Command *command) const;

	//get
	const MoveSkillType *getMoveSkillType() const	{return moveSkillType;}
	const LoadSkillType *getLoadSkillType() const	{return loadSkillType;}

	int getLoadCapacity() const { return m_loadCapacity; }
	bool canCarry(const UnitType *ut) const {
		return (std::find(m_canLoadList.begin(), m_canLoadList.end(), ut) != m_canLoadList.end());
	}

	bool	areProjectilesAllowed() const	{ return m_allowProjectiles; }
	Vec2f	getProjectileOffset() const		{ return m_projectileOffsets; }

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::LOAD; }
};

// ===============================
//  class UnloadCommandType
// ===============================

class UnloadCommandType: public CommandType {
private:
	const MoveSkillType *moveSkillType;
	const UnloadSkillType *unloadSkillType;

public:
	UnloadCommandType() : CommandType("Unload", Clicks::TWO), unloadSkillType(0), moveSkillType(0) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void update(Unit *unit) const;
	virtual string getReqDesc() const;
	virtual void start(Unit *unit, Command *command) const;

	//get
	const MoveSkillType *getMoveSkillType() const	{return moveSkillType;}
	const UnloadSkillType *getUnloadSkillType() const	{return unloadSkillType;}

	virtual Clicks getClicks() const { return (moveSkillType ? Clicks::TWO : Clicks::ONE); }
	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::UNLOAD; }
};

// ===============================
//  class BeLoadedCommandType
// ===============================

class BeLoadedCommandType : public CommandType {
private:
	const MoveSkillType *moveSkillType;

public:
	BeLoadedCommandType()
			: CommandType("BeLoaded", Clicks::ONE), moveSkillType(0) {}

	void setMoveSkill(const MoveSkillType *moveSkill) { moveSkillType = moveSkill; }
	virtual bool load() {return true;}
	virtual void doChecksum(Checksum &checksum) const {}
	virtual void getDesc(string &str, const Unit *unit) const {}
	virtual void update(Unit *unit) const;
	virtual string getReqDesc() const {return "";}

	//get
	const MoveSkillType *getMoveSkillType() const	{return moveSkillType;}

	virtual Clicks getClicks() const { return Clicks::ONE; }
	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::BE_LOADED; }
};

// ===============================
//  class GuardCommandType
// ===============================

class GuardCommandType: public AttackCommandType {
private:
	int m_maxDistance;

public:
	GuardCommandType(const char* name = "Guard", Clicks clicks = Clicks::TWO)
			: AttackCommandType(name, clicks), m_maxDistance(0) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void update(Unit *unit) const;
	virtual void tick(const Unit *unit, Command &command) const;
	int getMaxDistance() const {return m_maxDistance;}

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::GUARD; }
};

// ===============================
//  class PatrolCommandType
// ===============================

class PatrolCommandType: public GuardCommandType {
public:
	PatrolCommandType() : GuardCommandType("Patrol", Clicks::TWO) {}
	virtual void update(Unit *unit) const;
	virtual void tick(const Unit *unit, Command &command) const;
	virtual void finish(Unit *unit, Command &command) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::PATROL; }
};

// ===============================
//  class GenericCommandType
// ===============================

class CastSpellCommandType: public CommandType {
private:
	const CastSpellSkillType*	m_castSpellSkillType;
	SpellAffect					m_affects;
	SpellStart					m_start;
	bool	m_cycle;

public:
	CastSpellCommandType() : CommandType("CastSpell", Clicks::ONE), m_cycle(false), m_castSpellSkillType(NULL) {}
	virtual void doChecksum(Checksum &checksum) const {
		CommandType::doChecksum(checksum);
		checksum.add(m_castSpellSkillType->getName());
	}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const {
		m_castSpellSkillType->getDesc(str, unit);
	}
	const CastSpellSkillType *getCastSpellSkillType() const			{return m_castSpellSkillType;}

	virtual void update(Unit *unit) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::CAST_SPELL; }
};

// ===============================
//  class SetMeetingPointCommandType
// ===============================

class SetMeetingPointCommandType: public CommandType {
public:
	SetMeetingPointCommandType() : CommandType("SetMeetingPoint", Clicks::TWO) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {return true;}
	virtual void getDesc(string &str, const Unit *unit) const {}
	virtual void update(Unit *unit) const {
		throw std::runtime_error("Set meeting point command in queue. Thats wrong.");
	}
	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::SET_MEETING_POINT; }
	virtual CommandResult check(const Unit *unit, const Command &command) const;
};

// update helper, move somewhere sensible
// =====================================================
// 	class Targets
// =====================================================

/** Utility class for managing multiple targets by distance. */
class Targets : public std::map<Unit*, fixed> {
private:
	Unit *nearest;
	fixed distance;

public:
	Targets() : nearest(0), distance(fixed::max_value()) {}
	void record(Unit *target, fixed dist);
	Unit* getNearest() { return nearest; }
	Unit* getNearestSkillClass(SkillClass sc);
	Unit* getNearestHpRatio(fixed hpRatio);
};

}}//end namespace

#endif
