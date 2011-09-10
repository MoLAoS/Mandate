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
#include "logger.h"

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

#if defined(LOG_WORLD_EVENTS) && LOG_WORLD_EVENTS
#	define COMMAND_LOG(f, l, c, x)                      \
		if (g_logger.shouldLogCmdEvent(f, c, l)) {      \
			stringstream ss;                            \
			ss << "Faction ndx:" << f << " CmdClass::"  \
				<< CmdClassNames[c] << ": " << x;   \
			g_logger.logWorldEvent(ss.str());           \
		}
#	define BUILD_LOG(u, x)     COMMAND_LOG(u->getFaction()->getIndex(), 1, CmdClass::BUILD, x)
#	define REPAIR_LOG(u, x)    COMMAND_LOG(u->getFaction()->getIndex(), 1, CmdClass::REPAIR, x)
#	define REPAIR_LOG2(u, x)   COMMAND_LOG(u->getFaction()->getIndex(), 2, CmdClass::REPAIR, x)
#	define HARVEST_LOG(u, x)   COMMAND_LOG(u->getFaction()->getIndex(), 1, CmdClass::HARVEST, x)
#	define HARVEST_LOG2(u, x)  COMMAND_LOG(u->getFaction()->getIndex(), 2, CmdClass::HARVEST, x)
#else
#	define BUILD_LOG(u, x)
#	define REPAIR_LOG(u, x)
#	define REPAIR_LOG2(u, x)
#	define HARVEST_LOG(u, x)
#	define HARVEST_LOG2(u, x)
#endif

using Shared::Util::MultiFactory;
using namespace Glest::Entities;
using Glest::Gui::Clicks;

namespace Glest { namespace ProtoTypes {
using Search::CardinalDir;


class CmdDescriptor {
public:
	virtual void setHeader(const string &header) = 0;
	virtual void setTipText(const string &mainText) = 0;
	virtual void addElement(const string &msg) = 0;
	virtual void addItem(const DisplayableType *dt, const string &msg) = 0;
	virtual void addReq(const DisplayableType *dt, bool ok, const string &msg) = 0;
};

typedef const ProducibleType* ProdTypePtr;

// =====================================================
//  class CommandType
//
/// A complex action performed by a unit, composed by skills
// =====================================================

class CommandType : public RequirableType {
	friend class Glest::Sim::DynamicTypeFactory<CmdClass, CommandType>;
	friend class Glest::Sim::PrototypeFactory;

protected:
	Clicks          clicks;
	bool            queuable;
	const UnitType *unitType;
	string          m_tipKey;
	string          m_tipHeaderKey;
	int             energyCost;
	bool            m_display;

	void setId(int v) { m_id = v; }
	void setUnitType(const UnitType *ut) { unitType = ut; }

	string emptyString;

public:
	CommandType(const char* name, Clicks clicks, bool queuable = false);

	// describe(), and virtual helpers descSkills() and subDesc()
	void describe(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const;
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const = 0;
	virtual void subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {}

	// old skool desc()
	virtual void getDesc(string &str, const Unit *unit) const = 0;
	virtual string toString() const;

	// load & checksum
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;

	/** tip key for this command, from the tip attribute of the command name node */
	const string& getTipKey() const						{return m_tipKey;}
	
	/** tip key for a producible this command makes, from the tip attribute of the producible node */
	virtual string getTipKey(const string &name) const  {return emptyString;}

	/** sub-heading, for two-click production commands, is the heading used on the tip for a 
	  * 'sub-selection', if the lang entry this points to included '%s' it will be replaced
	  * by the translated producible name */
	virtual string getSubHeader() const                 {return m_tipHeaderKey;}

	// Production
	/** return number of things (ProducibleType derivitives) this command can make.
	  * CommandTypes that make stuff MUST override this method */
	virtual int getProducedCount() const            { return 0; }
	/** return the thing (ProducibleType derivitive) this command makes at index i.
	  * CommandTypes that make stuff MUST override this method */
	virtual ProdTypePtr getProduced(int i) const    { return 0; }

	// candidates for lua callbacks
	/** Check if a command can be executed */
	virtual CmdResult check(const Unit *unit, const Command &command) const { return CmdResult::SUCCESS; }
	/** Apply costs for a command. */
	virtual void apply(Faction *faction, const Command &command) const;
	/** De-Apply costs for a command */
	virtual void undo(Unit *unit, const Command &command) const;
	/** Update the command by one frame. */
	///@todo virtual void update(Unit*, Command&) const = 0
	virtual void update(Unit *unit) const = 0;
	/** Update the command by one tick (40 frames). */
	virtual void tick(const Unit *unit, Command &command) const {}
	/** Final actions to finish */
	virtual void finish(Unit *unit, Command &command) const {}
	/** Beginning actions before executing */
	virtual void start(Unit *unit, Command *command) const {}
	//init();

	bool isQueuable() const								{return queuable;}
	bool isInvisible() const                            {return !m_display;}

	// get
	const UnitType*       getUnitType() const   { return unitType; }
	int                   getEnergyCost() const	{ return energyCost; }
	bool                  getArrowDetails(const Command *cmd, Vec3f &out_target, Vec3f &out_color) const;
	virtual Vec3f         getArrowColor() const {return Vec3f(1.f, 1.f, 0.f);}
	virtual Clicks        getClicks() const     {return clicks;}
	string getDesc(const Unit *unit) const {
		string str;
		//str = name + "\n";
		getDesc(str, unit);
		if (energyCost) {
			str += "\n" + g_lang.get("EnergyCost") + ": " + intToStr(energyCost);
		}
		return str;
	}
	virtual CmdClass getClass() const = 0;

	Command* doAutoCommand(Unit *unit) const;

protected:
	void replaceDeadReferences(Command &command) const;

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

	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override {}
	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::STOP; }
};

// ===============================
//  class MoveCommandType
// ===============================

class MoveCommandType: public MoveBaseCommandType {
public:
	MoveCommandType() : MoveBaseCommandType("Move", Clicks::TWO) {}
	virtual void update(Unit *unit) const;
	virtual void tick(const Unit *unit, Command &command) const;

	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual Vec3f getArrowColor() const {return Vec3f(0.f, 1.f, 0.f);}
	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::MOVE; }
};

// ===============================
//  class TeleportCommandType
// ===============================

class TeleportCommandType: public MoveBaseCommandType {
public:
	TeleportCommandType() : MoveBaseCommandType("Teleport", Clicks::TWO) {}
	virtual void update(Unit *unit) const;

	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::TELEPORT; }
	virtual CmdResult check(const Unit *unit, const Command &command) const;
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
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;

	bool updateGeneric(Unit *unit, Command *command, const AttackCommandType *act, Unit* target, const Vec2i &targetPos) const;

	virtual void update(Unit *unit) const;

	virtual Vec3f getArrowColor() const {return Vec3f(1.f, 0.f, 0.f);}
	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::ATTACK; }

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
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual void update(Unit *unit) const;

	virtual Vec3f getArrowColor() const {return Vec3f(1.f, 0.f, 0.f);}
	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::ATTACK_STOPPED; }

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
	virtual void subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const override;
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual void getDesc(string &str, const Unit *unit) const {
		m_buildSkillType->getDesc(str, unit);
	}
	virtual void update(Unit *unit) const;

	// prechecks
	/** @param builtUnitType the unitType to build
	  * @param pos the position to put the unit */
	bool isBlocked(const UnitType *builtUnitType, const Vec2i &pos, CardinalDir facing) const;
	virtual CmdResult check(const Unit *unit, const Command &command) const;
	virtual void undo(Unit *unit, const Command &command) const;

	//get
	const BuildSkillType *getBuildSkillType() const	{return m_buildSkillType;}

	virtual int getProducedCount() const					{return m_buildings.size();}
	virtual ProdTypePtr getProduced(int i) const;

	bool canBuild(const UnitType *ut) const		{
		return std::find(m_buildings.begin(), m_buildings.end(), ut) != m_buildings.end();
	}

	int getBuildingCount() const					{return m_buildings.size();}
	const UnitType * getBuilding(int i) const		{return m_buildings[i];}

	string getTipKey(const string &name) const  {
		map<string,string>::const_iterator it = m_tipKeys.find(name);
		return it->second;
	}

	StaticSound *getStartSound() const				{return m_startSounds.getRandSound();}
	StaticSound *getBuiltSound() const				{return m_builtSounds.getRandSound();}

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::BUILD; }

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
	virtual void subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const override;
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
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

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::HARVEST; }
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
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual void subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const override;
	virtual void update(Unit *unit) const;
	virtual void tick(const Unit *unit, Command &command) const;
	virtual CmdResult check(const Unit *unit, const Command &command) const;

	//get
	const RepairSkillType *getRepairSkillType() const	{return repairSkillType;}
	bool canRepair(const UnitType *unitType) const;

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::REPAIR; }

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
	vector<int>             m_producedNumbers;
	map<string, string>		m_tipKeys;
	SoundContainer			m_finishedSounds;

public:
	ProduceCommandType() : CommandType("Produce", Clicks::ONE, true), m_produceSkillType(0) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const override;
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual void update(Unit *unit) const;
	virtual string getReqDesc(const Faction *f) const;
	
	int getProducedNumber(const UnitType*) const;

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

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::PRODUCE; }
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
	virtual void subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const override;
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual void update(Unit *unit) const;
	virtual string getReqDesc(const Faction *f) const;
	StaticSound *getFinishedSound() const	{return m_finishedSounds.getRandSound();}

	//get
	const ProduceSkillType *getProduceSkillType() const	{return m_produceSkillType;}

	virtual int getProducedCount() const	{return m_producibles.size();}
	virtual ProdTypePtr getProduced(int i) const	{return m_producibles[i];}

	string getTipKey(const string &name) const  {
		map<string,string>::const_iterator it = m_tipKeys.find(name);
		return it->second;
	}

	virtual Clicks getClicks() const	{ return m_producibles.size() == 1 ? Clicks::ONE : Clicks::TWO; }

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::GENERATE; }
};

// ===============================
//  class UpgradeCommandType
// ===============================

class UpgradeCommandType: public CommandType {
private:
	const UpgradeSkillType*     m_upgradeSkillType;
	vector<const UpgradeType*>  m_producedUpgrades;
	SoundContainer			    m_finishedSounds;
	map<string, string>		    m_tipKeys;

public:
	UpgradeCommandType() : CommandType("Upgrade", Clicks::ONE, true), m_upgradeSkillType(0) { }
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual string getReqDesc(const Faction *f) const;
	//virtual ProdTypePtr getProduced() const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const override;
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	StaticSound *getFinishedSound() const	{return m_finishedSounds.getRandSound();}
	virtual void update(Unit *unit) const;
	virtual void start(Unit *unit, Command &command) const;

	// get
	virtual int getProducedCount() const	{return m_producedUpgrades.size();}
	virtual ProdTypePtr getProduced(int i) const {
		ASSERT_RANGE(i, m_producedUpgrades.size());
		return m_producedUpgrades[i];
	}

	const UpgradeSkillType *getUpgradeSkillType() const	{return m_upgradeSkillType;}

	const UpgradeType *getProducedUpgrade(int i) const		{return m_producedUpgrades[i];}

	string getTipKey(const string &name) const  {
		map<string,string>::const_iterator it = m_tipKeys.find(name);
		return it->second;
	}

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::UPGRADE; }

	virtual void apply(Faction *faction, const Command &command) const;
	virtual void undo(Unit *unit, const Command &command) const;
	virtual CmdResult check(const Unit *unit, const Command &command) const;
};

// ===============================
//  class MorphCommandType
// ===============================

class MorphCommandType : public CommandType {
protected:
	const MorphSkillType*	m_morphSkillType;
	vector<const UnitType*> m_morphUnits;
	map<string, string>		m_tipKeys;
	int						m_discount;
	int						m_refund;
	SoundContainer			m_finishedSounds;

protected:
	MorphCommandType(const char* name);

private:
	void updateNormal(Unit *unit) const;
	//void updatePlaced(Unit *unit) const;

public:
	MorphCommandType();
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void update(Unit *unit) const;
	virtual string getReqDesc(const Faction *f) const;
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual void subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const override;
	
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
	int getRefund() const                           {return m_refund;}

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::MORPH; }
};

// ===============================
//  class TransformCommandType
// ===============================

class TransformCommandType : public MorphCommandType {
protected:
	const MoveSkillType*	m_moveSkillType;
	Vec2i                   m_position; // cell offset from 'buildPos' to go to before morphing
	float                   m_rotation;
	HpPolicy                m_hpPolicy;

public:
	TransformCommandType();

	HpPolicy getHpPolicy() const { return m_hpPolicy; }

	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;

	virtual void update(Unit *unit) const;
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;

	const MoveSkillType *getMoveSkillType() const {return m_moveSkillType;}
	virtual CmdClass getClass() const         { return typeClass(); }

	static CmdClass typeClass()               { return CmdClass::TRANSFORM; }
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
	virtual string getReqDesc(const Faction *f) const;
	virtual void subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const override;
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual CmdResult check(const Unit *unit, const Command &command) const;
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

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::LOAD; }
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
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual void update(Unit *unit) const;
	virtual string getReqDesc(const Faction *f) const;
	virtual void start(Unit *unit, Command *command) const;

	//get
	const MoveSkillType *getMoveSkillType() const	{return moveSkillType;}
	const UnloadSkillType *getUnloadSkillType() const	{return unloadSkillType;}

	virtual Clicks getClicks() const { return (moveSkillType ? Clicks::TWO : Clicks::ONE); }
	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::UNLOAD; }
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
	virtual string getReqDesc(const Faction *f) const {return "";}

	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override {}

	//get
	const MoveSkillType *getMoveSkillType() const	{return moveSkillType;}

	virtual Clicks getClicks() const { return Clicks::ONE; }
	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::BE_LOADED; }
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
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::GUARD; }
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
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::PATROL; }
};

// ===============================
//  class GenericCommandType
// ===============================

class CastSpellCommandType: public CommandType {
private:
	const CastSpellSkillType*	m_castSpellSkillType;
	SpellAffect					m_affects;
	bool	m_cycle;

public:
	CastSpellCommandType() : CommandType("CastSpell", Clicks::ONE), m_cycle(false), m_castSpellSkillType(NULL) {}
	virtual void doChecksum(Checksum &checksum) const {
		CommandType::doChecksum(checksum);
		checksum.add(m_castSpellSkillType->getName());
	}
	virtual Clicks getClicks() const { return (m_affects == SpellAffect::SELF ? Clicks::ONE : Clicks::TWO); }
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const {
		m_castSpellSkillType->getDesc(str, unit);
	}
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	const CastSpellSkillType *getCastSpellSkillType() const			{return m_castSpellSkillType;}

	SpellAffect	getSpellAffects() const  { return m_affects; }

	virtual void update(Unit *unit) const;

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::CAST_SPELL; }
};

// ===============================
//  class BuildSelfCommandType
// ===============================

class BuildSelfCommandType : public CommandType {
private:
	const BuildSelfSkillType *m_buildSelfSkill;
	bool                      m_allowRepair;

public:
	BuildSelfCommandType() : CommandType("build-self", Clicks::TWO), m_buildSelfSkill(0) {}

	bool allowRepair() const { return m_allowRepair; }

	virtual void getDesc(string &str, const Unit *unit) const {}
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void update(Unit *unit) const;
	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::BUILD_SELF; }
};

// ===============================
//  class SetMeetingPointCommandType
// ===============================

class SetMeetingPointCommandType: public CommandType {
public:
	SetMeetingPointCommandType() : CommandType("SetMeetingPoint", Clicks::TWO) { m_display = false; }
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {return true;}
	virtual void getDesc(string &str, const Unit *unit) const {}
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override {}
	virtual void update(Unit *unit) const {
		throw std::runtime_error("Set meeting point command in queue. Thats wrong.");
	}
	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::SET_MEETING_POINT; }
	virtual CmdResult check(const Unit *unit, const Command &command) const;
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
