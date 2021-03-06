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

#ifndef _GLEST_GAME_UNIT_H_
#define _GLEST_GAME_UNIT_H_

//#include <map>

#include "model.h"
#include "upgrade_type.h"
#include "particle.h"
#include "skill_type.h"
#include "item.h"
#include "effect.h"
#include "unit_type.h"
#include "settlement.h"
#include "stats.h"
#include "math_util.h"
#include "timer.h"
#include "logger.h"
#include "factory.h"
#include "type_factories.h"
#include "game_particle.h"

#include "prototypes_enums.h"
#include "simulation_enums.h"
#include "entities_enums.h"

namespace Glest { namespace Sim {
	class SkillCycleTable;
}}

namespace Glest { namespace Entities {

using namespace Shared::Math;
using namespace Shared::Graphics;
using Shared::Platform::Chrono;
using Shared::Util::SingleTypeFactory;

using namespace Hierarchy;
using namespace ProtoTypes;
using Sim::Map;

class Unit;
class UnitFactory;

WRAPPED_ENUM( AutoCmdFlag,
	REPAIR,
	MAINTAIN,
	ATTACK,
	FLEE
)

// AutoCmdState : describes an auto command category state of
// the selection (auto-repair, auto-attack & auto-flee)
WRAPPED_ENUM( AutoCmdState,
	NONE,
	ALL_ON,
	ALL_OFF,
	MIXED
)

class Vec2iList : public list<Vec2i> {
public:
	void read(const XmlNode *node);
	void write(XmlNode *node) const;
};

ostream& operator<<(ostream &stream,  Vec2iList &vec);

// =====================================================
// 	class UnitPath
// =====================================================
/** Holds the next cells of a Unit movement
  * @extends std::list<Shared::Math::Vec2i>
  */
class UnitPath : public Vec2iList {
	friend class Unit;
private:
	static const int maxBlockCount = 10; /**< number of frames to wait on a blocked path */

private:
	int blockCount;		/**< number of frames this path has been blocked */

	void clear()			{list<Vec2i>::clear(); blockCount = 0;} /**< clear the path		*/

public:
	UnitPath() : blockCount(0) {} /**< Construct path object */
	bool isBlocked()		{return blockCount >= maxBlockCount;} /**< is this path blocked	   */
	bool empty()			{return list<Vec2i>::empty();}	/**< is path empty				  */
	int  size()				{return list<Vec2i>::size();}	/**< size of path				 */
	void resetBlockCount()	{blockCount = 0; }
	void incBlockCount()	{blockCount++;}		   /**< increment block counter			   */
	void push(Vec2i &pos)	{push_front(pos);}	  /**< push onto front of path			  */
	Vec2i peek()			{return front();}	 /**< peek at the next position			 */
	void pop()				{erase(begin());}	/**< pop the next position off the path */

	int getBlockCount() const { return blockCount; }

	void read(const XmlNode *node);
	void write(XmlNode *node) const;
};

class WaypointPath : public Vec2iList {
public:
	WaypointPath() {}
	void push(const Vec2i &pos/*, float dist*/)	{ push_front(pos); }
	Vec2i peek() const					{return front();}
	//float waypointToGoal() const		{ return front().second; }
	void pop()							{erase(begin());}
	void condense();
};

typedef Unit*               UnitPtr;
typedef const Unit*         ConstUnitPtr;
typedef vector<Unit*>       UnitVector;
typedef vector<const Unit*> ConstUnitVector;
typedef set<const Unit*>    UnitSet;
typedef list<Unit*>         UnitList;
typedef list<UnitId>        UnitIdList;

typedef vector<StatCost>    Pools;
// ===============================
// 	class Unit
//
///	A game unit or building
// ===============================

/** Represents a single game unit or building. The Unit class inherits from
  * Statistics as a mechanism to maintain a cache of it's current
  * stat values. These values are only recalculated when an effect is added
  * or removed, an upgrade is applied or the unit levels up or garrisons a unit. */
class Unit {
	friend class EntityFactory<Unit>;

public:
	typedef list<Command*> Commands;
	//typedef list<UnitId> Pets;

	typedef vector<BonusPower> BonusPowers;
	typedef vector<ProductionSystemTimers> BonusPowerTimers;

	typedef vector<UnitsOwned> OwnedUnits;
	typedef vector<string> BuffsApplied;
	typedef vector<Attacker> Attackers;
    typedef vector<Modification> Modifications;

/**< system for localized resources */
private:
    typedef vector<StoredResource> SResources;
public:
    SResources sresources;

    const StoredResource *getSResource(const ResourceType *rt) const;
	const StoredResource *getSResource(int i) const  {assert(i < sresources.size()); return &sresources[i];}
	int getStoreAmount(const ResourceType *rt) const;
    void incResourceAmount(const ResourceType *rt, int amount);
	void setResourceBalance(const ResourceType *rt, int balance);
	void addStore(const ResourceType *rt, int amount);
	void addStore(const UnitType *unitType);

/**< system for localized resources */

/**< system for items */
private:
    typedef vector<Equipment> Storage;
    typedef vector<int> StoredItems;
    typedef vector<const ItemType*> Requisitions;

    StoredItems equippedItems;
    StoredItems storedItems;
    int itemLimit;
    int itemsStored;
    Storage storage;
    Storage equipment;
    Requisitions requisitions;
    Requisitions addOns;

public:
    int getAddOnCount() const {return addOns.size();}
    const ItemType *getAddOn(int i) const {return addOns[i];}

    int getRequisitionCount() const {return requisitions.size();}
    const ItemType *getRequisition(int i) const {return requisitions[i];}
    void addRequisition(const ItemType *requisition) {requisitions.push_back(requisition);}
    void removeRequisition(int i) {requisitions.erase(requisitions.begin() + i);}
    void clearRequisitions() {requisitions.clear();}
    int getItemLimit() const {return itemLimit;}
    int getItemsStored() const {return itemsStored;}
    void setItemLimit(int expand) {itemLimit = itemLimit + expand;}
    void setItemsStored(int amount) {itemsStored = itemsStored + amount;}
	int getStoredItemCount() const {return storedItems.size();}
	StoredItems getStoredItems() const {return storedItems;}
	Item *getStoredItem(int i) const {return getFaction()->getItems()[storedItems[i]];}
	void clearStoredItems() {storedItems.clear();}
    void accessStorageAdd(int ident);
    void accessStorageRemove(int ident);
    void accessStorageExchange(Unit *unit);
    StoredItems getEquippedItems() const {return equippedItems;}
    Item *getEquippedItem(int i) const {return getFaction()->getItems()[equippedItems[i]];}
    void equipItem(int ident);
    void unequipItem(int ident);
    void consumeItem(int ident);
    const Equipment *getStorage(int i) const {return &storage[i];}
    Equipment *getStorage(int i) {return &storage[i];}
    int getStorageSize() const {return storage.size();}
    const Equipment *getEquipment(int i) const {return &equipment[i];}
    int getEquipmentSize() const {return equipment.size();}
/**< system for items */

    void computeTax();

private:
	typedef vector<Timer> CreatedUnitTimers;
	CreatedUnitTimers createdUnitTimers;

	typedef vector<CreatedUnit> CreatedUnits;
	CreatedUnits createdUnits;
public:
	int getUnitTimerCount()             {return createdUnitTimers.size();}
    Timer *getCreatedUnitTimer(int i)   {return &createdUnitTimers[i];}

	int getCreatedUnitCount()           {return createdUnits.size();}
    CreatedUnit *getCreatedUnit(int i)  {return &createdUnits[i];}
private:
    Statistics statistics;
    Traits traits;
    Specialization *specialization;
    Actions actions;

    CharacterStats characterStats;
    Knowledge knowledge;

	CraftResources resourceStats;
	CraftStats weaponStats;
	CraftStats armorStats;
	CraftStats accessoryStats;

    typedef vector<Trait*> HeroClasses;
    HeroClasses heroClasses;

    bool character;

    bool changeType(const UnitType *unitType);

public:
	int getHeroClassesCount()       {return heroClasses.size();}
    Trait *getHeroClass(int i)  {return heroClasses[i];}
    void addHeroClass(Trait* heroClass) {heroClasses.push_back(heroClass);}

    int getWeaponStatCount() const {return weaponStats.size();}
    const CraftStat *getWeaponStat(int  i) const {return &weaponStats[i];}
    int getArmorStatCount() const {return armorStats.size();}
    const CraftStat *getArmorStat(int i) const {return &armorStats[i];}
    int getAccessoryStatCount() const {return accessoryStats.size();}
    const CraftStat *getAccessoryStat(int i) const {return &accessoryStats[i];}
    int getResourceStatCount() const {return resourceStats.size();}
    const CraftRes *getResourceStat(int i) const {return &resourceStats[i];}

    bool isCharacter() {return character;}
    void addKnowledge(const Knowledge *newKnowledge) {knowledge.sum(newKnowledge);}
    void addCharacterStats(const CharacterStats *newCharacterStats) {characterStats.sum(newCharacterStats);}

    const CharacterStats *getCharacterStats() const {return &characterStats;}
    const Knowledge *getKnowledge() const {return &knowledge;}
    CharacterStats *getCharacterStats() {return &characterStats;}
    Knowledge *getKnowledge() {return &knowledge;}

    Modifications modifications;
    EffectTypes effectTypes;
    bool dayCycle;

    const Statistics *getStatistics() const {return &statistics;}
    Statistics *getStatistics() {return &statistics;}
    void initSkillsAndCommands();

    CitizenModifier totalModifier;
    void generateCitizen();
    void educateCitizen(const CitizenModifier *educationModifier, int developmentLevel, Unit *createdUnit);
    int getDevelopmentLevel();
    void addCrafts(int finesse);
    const CitizenModifier *getCitizenModifiers();
    void assignCitizen(Unit *assignedUnit, const UnitType *newType);
    void assignLaborer(Unit *assignedUnit);

    void generateItem(Item *item);
private:
	// basic stats
	int id;					/**< unique identifier  */
	int hp;					/**< current hit points */
    int cp;					/**< current capture points */
    Pools resPools;         /**< current resource pools */
    Pools defPools;         /**< current resource pools */
	int loadCount;			/**< current 'load' (resources carried) */
	int deadCount;			/**< how many frames this unit has been dead */
	int progress2;			/**< 'secondary' skill progress counter (progress for Production) */
	int kills;				/**< number of kills */
	int exp;                /**< amount of experience */

	bool existing;

	/**< new system to enable walls */
	Zone zone;
	Field field;

	string currentFocus;
	Unit *goalStructure;
	string goalReason;
public:
    Trait *currentResearch;

    Field getField() const		  {return field;}
	void setField(Field newField) { field = newField; }
	Zone getZone() const		  {return zone;}
	void setZone(Zone newZone)    { zone = newZone; }
	/**< new system to enable walls */

	string getCurrentFocus() const {return currentFocus;}
	void setCurrentFocus(string newFocus) {currentFocus = newFocus;}
	Unit *getGoalStructure() const {return goalStructure;}
	void setGoalStructure(Unit *unit) {goalStructure = unit;}
	string getGoalReason() const {return goalReason;}
	void setGoalReason(string string) {goalReason = string;}
	void shop();

    int getTraitCount() {return traits.size();}
    Trait *getTrait(int i) {return traits[i];}
    Actions *getActions() {return &actions;}

	Attackers attackers;

	UnitDirection previousDirection;

	ProductionSystemTimers productionSystemTimers;
	BonusPowerTimers bonusPowerTimers;

    CurrentStep currentCommandCooldowns; /**< current timer step for skill cooldowns */
    CurrentStep currentAiUpdate; /**< current timer step for skill cooldowns */

    ProductionRoute productionRoute;
    Settlement settlement;

    Unit *owner;
    vector<Unit*> controlledUnits;
    int getControlledUnitCount() const {return controlledUnits.size();}
    Unit *getControlledUnit(int i) {return controlledUnits[i];}
    void addControlledUnit(Unit *unit) {controlledUnits.push_back(unit);}
    void setOwner(Unit *unit);
    OwnedUnits ownedUnits;

    BuffsApplied buffNames;

    int taxedGold;

    int taxRate;

    bool garrisonTest;

    bool bonusObject;

private:
	// housed unit bits
	UnitIdList	m_carriedUnits;
	UnitIdList	m_unitsToCarry;
	UnitIdList	m_unitsToUnload;
	UnitId		m_carrier;

    // garrisoned unit list
	UnitIdList	m_garrisonedUnits;
	UnitIdList	m_unitsToGarrison;
	UnitIdList	m_unitsToDegarrison;
	UnitId		m_garrison;

	// engine info
	int lastAnimReset;			/**< the frame the current animation cycle was started */
	int nextAnimReset;			/**< the frame the next animation cycle will begin */
	int lastCommandUpdate;		/**< the frame this unit last updated its command */
	int nextCommandUpdate;		/**< the frame next command update will occur */
	int systemStartFrame;		/**< the frame the unit will start an attack or spell system */
	int soundStartFrame;		/**< the frame the sound for the current skill should be started */

	// target info
	UnitId targetRef;
	Field targetField;				/**< Field target travels in @todo replace with Zone ? */
	bool faceTarget;				/**< If true and target is set, we continue to face target. */
	bool useNearestOccupiedCell;	/**< If true, targetPos is set to target->getNearestOccupiedCell() */

	// position info
	Vec2i pos;						/**< Current position */
	Vec2i lastPos;					/**< The last position before current */
	Vec2i nextPos;					/**< Position unit is moving into next. Note: this will almost always == pos */
	Vec2i targetPos;				/**< Position of the target, or the cell of the target we care about. */
	Vec3f targetVec;				/**< 3D position of target (centre) */
	Vec2i meetingPos;				/**< Cell position of metting point */

	// rotation info
	float lastRotation;				/**< facing last frame, in degrees */
	float targetRotation;			/**< desired facing, in degrees*/
	float rotation;					/**< current facing, in degrees */
	CardinalDir m_facing;			/**< facing for buildings */

	const Level *level;				/**< current Level */
	int levelNumber;

	const UnitType *type;			/**< the UnitType of this unit */
	const ResourceType *loadType;	/**< the type if resource being carried */

	const SkillType *currSkill;		/**< the SkillType currently being executed */

	// some flags
	bool toBeUndertaken;			/**< awaiting a date with the grim reaper */

	bool m_autoCmdEnable[AutoCmdFlag::COUNT];
	//bool autoRepairEnabled;			/**< is auto repair enabled */

	bool carried;					/**< is the unit being carried */
	bool garrisoned;                /**< is the unit being garrisoned */
	//bool visible;

	bool	m_cloaked;

	bool	m_cloaking, m_deCloaking;
	float	m_cloakAlpha;

	// this should go somewhere else
	float highlight;				/**< alpha for selection circle effects */

	Effects effects;				/**< Effects (spells, etc.) currently effecting unit. */
	Effects effectsCreated;			/**< All effects created by this unit. */
	Statistics totalUpgrade;	/**< All stat changes from upgrades, level ups, garrisoned units and effects */

	// is this really needed here? maybe keep faction (but change to an index), ditch map
	Faction *faction;
	Map *map;

	// paths
	UnitPath unitPath;
	WaypointPath waypointPath;

	// command queue
	Commands commands;

	// eye candy particle systems
	ParticleSystem *fire;
	UnitParticleSystems skillParticleSystems;
	UnitParticleSystems effectParticleSystems;

	// scripting stuff
	int commandCallback;		// for script 'command callbacks'
	int hp_below_trigger;		// if non-zero, call the Trigger manager when HP falls below this
	int hp_above_trigger;		// if non-zero, call the Trigger manager when HP rises above this
    int cp_below_trigger;		// if non-zero, call the Trigger manager when CP falls below this
    int cp_above_trigger;		// if non-zero, call the Trigger manager when CP rises above this
	bool attacked_trigger;

public:
    bool checkSkillCosts(const CommandType *ct);
    bool applySkillCosts(const CommandType *ct);
	bool applyCosts(const CommandType *ct, const ProducibleType *pt);
	void applyStaticCosts(const ProducibleType *p);
	bool checkCosts(const CommandType *ct, const ProducibleType *pt);

	MEMORY_CHECK_DECLARATIONS(Unit)

	// signals
	typedef sigslot::signal<Unit*>	UnitSignal;

	UnitSignal		Created;	  /**< fires when a unit is created		   */
	UnitSignal		Born;		 /**< fires when a unit is 'born'		  */
	//UnitSignal		Moving;		/**< fires just before a unit is moved in cells	*/
	//UnitSignal		Moved;	   /**< fires after a unit has moved in cells	*/
	//UnitSignal		Morphing; /**<  */
	//UnitSignal		Morphed; /**<  */
	UnitSignal		StateChanged; /**< command changed / availability changed, etc (for gui) */
	UnitSignal		Died;	/**<  */

public:
	struct CreateParams {
		Vec2i pos;
		const UnitType *type;
		Faction *faction;
		Map *map;
		CardinalDir face;
		Unit* master;

		CreateParams(const Vec2i &pos, const UnitType *type, Faction *faction, Map *map,
				CardinalDir face = CardinalDir::NORTH, Unit* master = NULL)
			: pos(pos), type(type), faction(faction), map(map), face(face), master(master) { }
	};

	struct LoadParams {
		const XmlNode *node;
		Faction *faction;
		Map *map;
		const TechTree *tt;
		bool putInWorld;

		LoadParams(const XmlNode *node, Faction *faction, Map *map, const TechTree *tt, bool putInWorld = true)
			: node(node), faction(faction), map(map), tt(tt), putInWorld(putInWorld) {}
	};

private:
	Unit(CreateParams params);
	Unit(LoadParams params);

	virtual ~Unit();

	void setId(int v) { id = v; }

	void checkEffectParticles();
	void checkEffectCloak();
	void startSkillParticleSystems();

public:
	void save(XmlNode *node) const;

	//queries
	int getId() const							{return id;}
	Field getCurrField() const					{return getField();}
	Zone getCurrZone() const					{return getZone();}
	int getLoadCount() const					{return loadCount;}
	int getSize() const							{return type->getSize();}
	float getProgress() const;
	float getAnimProgress() const;
	float getHightlight() const					{return highlight;}
	int getProgress2() const					{return progress2;}

	int getNextCommandUpdate() const			{ return nextCommandUpdate; }
	int getLastCommandUpdate() const			{ return lastCommandUpdate; }
	int getNextAnimReset() const				{ return nextAnimReset; }
	int getSystemStartFrame() const				{ return systemStartFrame; }
	int getSoundStartFrame() const				{ return soundStartFrame; }

	int getFactionIndex() const					{return faction->getIndex();}
	int getTeam() const							{return faction->getTeam();}
	int getHp() const							{return hp;}
	int getResourcePoolCount() const            {return resPools.size();}
	const StatCost *getResource(int i) const    {return &resPools[i];}
	int getDefensePoolCount() const             {return defPools.size();}
	const StatCost *getDefense(int i) const     {return &defPools[i];}
    int getCp() const							{return cp;}


	int getProductionPercent() const;
	float getHpRatio() const					{return clamp(float(hp) / getStatistics()->getEnhancement()->getResourcePools()->
                                                 getHealth()->getMaxStat()->getValue(), 0.f, 1.f);}
	fixed getHpRatioFixed() const				{ return fixed(hp) / getStatistics()->getEnhancement()->getResourcePools()->getHealth()->getMaxStat()->getValue(); }
    float getCpRatio() const					{return clamp(float(cp) / getStatistics()->getEnhancement()->getResourcePools()->getMaxCp()->getValue(), 0.f, 1.f);}
	bool getToBeUndertaken() const				{return toBeUndertaken;}
	UnitId getTarget() const					{return targetRef;}
	Vec2i getNextPos() const					{return nextPos;}
	Vec2i getTargetPos() const					{return targetPos;}
	Vec3f getTargetVec() const					{return targetVec;}
	Field getTargetField() const				{return targetField;}
	Vec2i getMeetingPos() const					{return meetingPos;}
	Faction *getFaction() const					{return faction;}
	const ResourceType *getLoadType() const		{return loadType;}

	bool reqsOk(const RequirableType *rt) const;

	const UnitType *getType() const				{return type;}
	const SkillType *getCurrSkill() const		{return currSkill;}
	const Statistics *getTotalUpgrade() const	{return &totalUpgrade;}
	float getRotation() const					{return rotation;}
	Vec2f getVerticalRotation() const			{return Vec2f(0.f);}
	ParticleSystem *getFire() const				{return fire;}
	int getKills() const						{return kills;}
	int getExp() const						    {return exp;}
	virtual void setExp(int v)                  { exp = v; }
	const Level *getLevel() const				{return level;}
	int getLevelNumber() const                  {return levelNumber;}
	const Level *getNextLevel() const;
	string getFullName() const;
	const UnitPath *getPath() const				{return &unitPath;}
	UnitPath *getPath()							{return &unitPath;}
	WaypointPath *getWaypointPath()				{return &waypointPath;}
	const WaypointPath *getWaypointPath() const {return &waypointPath;}
	int getBaseSpeed(const SkillType *st) const {return st->getBaseSpeed();}
	int getSpeed(const SkillType *st) const;
	int getBaseSpeed() const					{return getBaseSpeed(currSkill);}
	int getSpeed() const						{return getSpeed(currSkill);}
	const Emanations &getEmanations() const		{return type->getEmanations();}
	const Commands &getCommands() const			{return commands;}
	const RepairCommandType *getRepairCommandType(const Unit *u) const;
	int getDeadCount() const					{return deadCount;}
	void setModelFacing(CardinalDir value);
	CardinalDir getModelFacing() const			{ return m_facing; }
	int getCloakGroup() const                   { return type->getCloakType()->getCloakGroup(); }

	//-- for carry units
	const UnitIdList& getCarriedUnits() const	{return m_carriedUnits;}
	UnitIdList& getCarriedUnits()				{return m_carriedUnits;}
	UnitIdList& getUnitsToCarry()				{return m_unitsToCarry;}
	UnitIdList& getUnitsToUnload()				{return m_unitsToUnload;}
	UnitId getCarrier() const					{return m_carrier;}

	const UnitIdList& getGarrisonedUnits() const	{return m_garrisonedUnits;}
	UnitIdList& getGarrisonedUnits()				{return m_garrisonedUnits;}
	UnitIdList& getUnitsToGarrison()				{return m_unitsToGarrison;}
	UnitIdList& getUnitsToDegarrison()				{return m_unitsToDegarrison;}
	UnitId getGarrison() const					    {return m_garrison;}

	void housedUnitDied(Unit *unit);

	//bool isVisible() const					{return carried;}
	void setCarried(Unit *host)				{carried = (host != 0); m_carrier = (host ? host->getId() : -1);}
	void setGarrisoned(Unit *host)			{garrisoned = (host != 0); m_garrison = (host ? host->getId() : -1);}
	void loadUnitInit(Command *command);
	void unloadUnitInit(Command *command);
	void garrisonUnitInit(Command *command);
	void degarrisonUnitInit(Command *command);
	//----

	///@todo move to a helper of ScriptManager, connect signals...
	void setCommandCallback();
	void clearCommandCallback()					{ commandCallback = 0; }
	const int getCommandCallback() const	{ return commandCallback; }
	void setHPBelowTrigger(int i)				{ hp_below_trigger = i; }
	void setHPAboveTrigger(int i)				{ hp_above_trigger = i; }
	void setCPBelowTrigger(int i)				{ cp_below_trigger = i; }
	void setCPAboveTrigger(int i)				{ cp_above_trigger = i; }
	void setAttackedTrigger(bool val)			{ attacked_trigger = val; }
	bool getAttackedTrigger() const				{ return attacked_trigger; }

	/**
	 * Returns the maximum range (attack range, spell range, etc.) for this unit
	 * using the supplied skill after all modifications due to upgrades &
	 * effects.
	 */
	int getMaxRange(const TargetBasedSkillType *tbst) const {
		switch(tbst->getClass()) {
			case SkillClass::ATTACK:
				return (tbst->getMaxRange() * getStatistics()->getEnhancement()->getAttackStats()->getAttackRange()->getValueMult() +
                getStatistics()->getEnhancement()->getAttackStats()->getAttackRange()->getValue()).intp();
			case SkillClass::CAST_SPELL:
				return (tbst->getMaxRange() * getStatistics()->getEnhancement()->getAttackStats()->getAttackRange()->getValueMult() +
                getStatistics()->getEnhancement()->getAttackStats()->getAttackRange()->getValue()).intp();
			default:
				return tbst->getMaxRange();
		}
	}

	int getMaxRange(const AttackSkillTypes *asts) const {
		return (asts->getMaxRange() * getStatistics()->getEnhancement()->getAttackStats()->getAttackRange()->getValueMult() +
          getStatistics()->getEnhancement()->getAttackStats()->getAttackRange()->getValue()).intp();
	}

	// pos
	Vec2i getPos() const				{return pos;}//carried ? carrier->getPos() : pos;}
	Vec2i getLastPos() const			{return lastPos;}
	Vec2i getCenteredPos() const		{return Vec2i(type->getHalfSize().intp()) + getPos();}
	fixedVec2 getFixedCenteredPos() const	{ return fixedVec2(pos.x + type->getHalfSize(), pos.y + type->getHalfSize()); }
	Vec2i getNearestOccupiedCell(const Vec2i &from) const;

	// alpha (for cloaking or putrefacting)
	float getCloakAlpha() const			{return m_cloakAlpha;}
	float getRenderAlpha() const;

	// is
	bool isIdle() const					{return currSkill->getClass() == SkillClass::STOP;}
	bool isMoving() const				{return currSkill->getClass() == SkillClass::MOVE;}
	bool isMobile ()					{ return type->isMobile(); }
	bool isCarried() const				{return carried;}
	bool isGarrisoned() const           {return garrisoned;}
	bool isHighlighted() const			{return highlight > 0.f;}
	bool isPutrefacting() const			{return deadCount;}
	bool isAlly(const Unit *unit) const	{return faction->isAlly(unit->getFaction());}
	bool isInteresting(InterestingUnitType iut) const;
	bool isAutoCmdEnabled(AutoCmdFlag f) const	{return m_autoCmdEnable[f];}
	bool isCloaked() const					{return m_cloaked;}
	bool renderCloaked() const			{return m_cloaked || m_cloaking || m_deCloaking;}
	bool isOfClass(UnitClass uc) const { return type->isOfClass(uc); }
	bool isTargetUnitVisible(int teamIndex) const;
	bool isExisting() const {return existing;}
	bool isActive() const;
	bool isBuilding() const;
	bool isDead() const					{return !hp;}
	bool isAlive() const				{return hp;}
	bool isDamaged() const				{return hp < getStatistics()->getEnhancement()->getResourcePools()->getHealth()->getMaxStat()->getValue();}
	bool isOperative() const			{return isAlive() && isBuilt();}
	bool isBeingBuilt() const			{
		return currSkill->getClass() == SkillClass::BE_BUILT
			|| currSkill->getClass() == SkillClass::BUILD_SELF;
	}
	bool isBuilt() const				{return !isBeingBuilt();}
	// set
	void setCurrSkill(const SkillType *currSkill);
	void setCurrSkill(SkillClass sc)					{setCurrSkill(getType()->getActions()->getFirstStOfClass(sc));}
	void setLoadCount(int loadCount)					{this->loadCount = loadCount;}
	void setLoadType(const ResourceType *loadType)		{this->loadType = loadType;}

	void setProgress2(int progress2)					{this->progress2 = progress2;}
	void setPos(const Vec2i &pos);
	void setNextPos(const Vec2i &nextPos)				{this->nextPos = nextPos; targetRef = -1;}
	void setTargetPos(const Vec2i &targetPos)			{this->targetPos = targetPos; targetRef = -1;}
	void setTarget(const Unit *unit, bool faceTarget = true, bool useNearestOccupiedCell = true);
	void setTargetVec(const Vec3f &targetVec)			{this->targetVec = targetVec;}
	void setMeetingPos(const Vec2i &meetingPos)			{this->meetingPos = meetingPos;}
	void setAutoCmdEnable(AutoCmdFlag f, bool v);

	//render related
	const Model *getCurrentModel() const;
	//const Model *getCurrentModel() const				{return currSkill->getAnimation();}

	Vec3f getCurrVector() const							{
		return getCurrVectorFlat() + Vec3f(0.f, type->getHalfHeight().toFloat(), 0.f);
	}
	//Vec3f getCurrVectorFlat() const;
	// this is a heavy use function so it's inlined even though it isn't exactly small
	Vec3f getCurrVectorFlat() const {
		Vec2i pos = getPos();
		Vec3f v(float(pos.x),  computeHeight(pos), float(pos.y));
		if (currSkill->getClass() == SkillClass::MOVE) {
			v = Vec3f(float(lastPos.x), computeHeight(lastPos), float(lastPos.y)).lerp(getProgress(), v);
		}
		const float &halfSize = type->getHalfSize().toFloat();
		v.x += halfSize;
		v.z += halfSize;
		return v;
	}

	Vec3f getCurrVectorSink() const {
		Vec3f currVec = getCurrVectorFlat();

		// let dead units start sinking before they go away
		int framesUntilDead = GameConstants::maxDeadCount - getDeadCount();
		if(framesUntilDead <= 200 && !type->isOfClass(UnitClass::BUILDING)) {
			float baseline = logf(20.125f) / 5.f;
			float adjust = logf((float)framesUntilDead / 10.f + 0.125f) / 5.f;
			currVec.y += adjust - baseline;
		}

		return currVec;
	}

	//command related
	const CommandType *getFirstAvailableCt(CmdClass commandClass) const;
	bool anyCommand() const								{return !commands.empty();}

	/** return current command, assert that there is always one command */
	Command *getCurrCommand() const {
		assert(!commands.empty());
		return commands.front();
	}
	unsigned int getCommandCount() const;
	CmdResult giveCommand(Command *command);		//give a command
	CmdResult finishCommand();						//command finished
	CmdResult cancelCommand();						//cancel command on back of queue
	CmdResult cancelCurrCommand();					//cancel current command
	void removeCommands();

	//lifecycle
	void create(bool startingUnit = false);
	void born(bool reborn = false);
	void kill();
	void replace();
	void capture();
	void undertake();

	//other
	void resetHighlight();
	const CommandType *computeCommandType(const Vec2i &pos, const Unit *targetUnit= NULL) const;

	string getShortDesc() const;
	int getQueuedOrderCount() const { return (commands.size() > 1 ? commands.size() - 1 : 0); }
	string getLongDesc() const;

	bool computePools();
	bool repair(int amount = 0, fixed multiplier = 1);
	bool decHp(int i);
    bool decCp(int i);
	int update2()										{return ++progress2;}
	TravelState travel(const Vec2i &pos, const MoveSkillType *moveSkill);
	void stop() {setCurrSkill(SkillClass::STOP); }
	void clearPath();

	// SimulationInterface wrappers
	void updateSkillCycle(const SkillCycleTable *skillCycleTable);
	void doUpdateAnimOnDeath(const SkillCycleTable *skillCycleTable);
	void doUpdateAnim(const SkillCycleTable *skillCycleTable);
	void doUnitBorn(const SkillCycleTable *skillCycleTable);
	void doUpdateCommand();

	// World wrappers
	void doUpdate();
	void doKill(Unit *killed);
	void doCapture(Unit *killed);

	// update skill & animation cycles
	void updateSkillCycle(int offset);
	void updateMoveSkillCycle();
	void updateAnimDead();
	void updateAnimCycle(int animOffset, int soundOffset = -1, int attackOffset = -1);
	void resetAnim(int frame) { nextAnimReset = frame; }

	bool update();
	void updateEmanations();
	void face(float rot)		{ targetRotation = rot; }
	void face(const Vec2i &nextPos);

	Unit *tick();
	void applyUpgrade(const UpgradeType *upgradeType);
	void computeTotalUpgrade();
	void incKills();
	void incExp(int addExp);
	bool morph(const MorphCommandType *mct, const UnitType *ut, Vec2i offset = Vec2i(0), bool reprocessCommands = true);
	bool transform(const TransformCommandType *tct, const UnitType *ut, Vec2i pos, CardinalDir facing);
	CmdResult checkCommand(const Command &command) const;
	bool applyCommand(const Command &command);
	void startAttackSystems(const AttackSkillType *ast);
	void startSpellSystems(const CastSpellSkillType *sst);
	Projectile* launchProjectile(ProjectileType *projType, const Vec3f &endPos);
	Splash* createSplash(SplashType *splashType, const Vec3f &pos);

	int getCarriedCount() const { return m_carriedUnits.size(); }
	int getGarrisonedCount() const { return m_garrisonedUnits.size(); }

    void addSpecialization(Specialization *spec);
	void addTrait(Trait *trait);
	void processTraits();

	bool add(Effect *e);
	void remove(Effect *e);
	void effectExpired(Effect *effect);
	bool doRegen(int hpRegeneration);

	void cloak();
	void deCloak();

private:
	float computeHeight(const Vec2i &pos) const;
	void updateTarget(const Unit *target = NULL);
	void clearCommands();
	CmdResult undoCommand(const Command &command);
	void recalculateStats();
	Command *popCommand();
};

inline ostream& operator<<(ostream &stream, const Unit &unit) {
	return stream << "[Unit id:" << unit.getId() << "|" << unit.getType()->getName() << "]";
}


class UnitFactory : public EntityFactory<Unit>, public sigslot::has_slots {
	friend class Glest::Sim::World; // for saved games

private:
	MutUnitSet	m_carriedSet; // set of units not in the world (because they are housed in other units)
	Units		m_deadList;	// list of dead units

public:
	UnitFactory() { }
	~UnitFactory() { }

	Unit* newUnit(const XmlNode *node, Faction *faction, Map *map, const TechTree *tt, bool putInWorld = true);
	Unit* newUnit(const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, CardinalDir face, Unit* master = NULL);

	Unit* getUnit(int id) { return EntityFactory<Unit>::getInstance(id); }
	Unit* getObject(int id) { return EntityFactory<Unit>::getInstance(id); }

	void onUnitDied(Unit *unit);	// book a visit with the grim reaper
	void update();					// send the grim reaper on his rounds
	void deleteUnit(Unit *unit);	// should only be called to undo a creation

	Units::const_iterator begin_dead() const { return m_deadList.begin(); }
	Units::const_iterator end_dead() const { return m_deadList.end(); }

};

}}// end namespace

#endif
