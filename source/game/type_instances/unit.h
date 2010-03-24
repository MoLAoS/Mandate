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

#ifndef _GLEST_GAME_UNIT_H_
#define _GLEST_GAME_UNIT_H_

#include <map>

#include "model.h"
#include "upgrade_type.h"
#include "particle.h"
#include "skill_type.h"
#include "effect.h"
#include "unit_type.h"
#include "stats.h"
#include "unit_reference.h"
#include "math_util.h"
#include "entity.h"
#include "timer.h"
#include "logger.h"

#define UNIT_LOG(x) {}

namespace Glest { namespace Game {

using Shared::Graphics::ParticleSystem;
using Shared::Math::Vec4f;
using Shared::Math::Vec2f;
using Shared::Math::Vec3f;
using Shared::Math::Vec2i;
using Shared::Graphics::Model;
using Shared::Graphics::Entity;
using Shared::Platform::Chrono;

class Map;
class Faction;
class Unit;
class Command;
class SkillType;
class ResourceType;
class CommandType;
class SkillType;
class UpgradeType;
class Level;
class MorphCommandType;
class RepairCommandType;
class Game;
class World;
class UnitState;

//#define UNIT_LOG(x) theLogger.add(x)
#define UNIT_LOG(x) {}

// =====================================================
// 	class UnitObserver
// =====================================================

class UnitObserver {
public:
	enum Event {
		eKill,
		eStateChange
	};

public:
	virtual ~UnitObserver() {}
	virtual void unitEvent(Event event, const Unit *unit)=0;
};

// =====================================================
// 	class UnitPath
// =====================================================
/** Holds the next cells of a Unit movement 
  * @extends std::list<Shared::Math::Vec2i>
  */
class UnitPath : public list<Vec2i> {
private:
	static const int maxBlockCount = 10; /**< number of frames to wait on a blocked path */

private:
	int blockCount;		/**< number of frames this path has been blocked */

public:
	UnitPath() : blockCount(0) {} /**< Construct path object */
	bool isBlocked()	{return blockCount >= maxBlockCount;} /**< is this path blocked @return true if this path has been blocked for at least maxBlockCount frames */
	bool empty()		{return list<Vec2i>::empty();}	/**< is path empty				  */
	int  size()			{return list<Vec2i>::size();}	/**< size of path				 */
	void clear()		{list<Vec2i>::clear(); blockCount = 0;} /**< clear the path		*/
	void incBlockCount(){blockCount++;}		   /**< increment block counter			   */
	void push(Vec2i &pos){push_front(pos);}	  /**< push onto front of path			  */
	Vec2i peek()		{return front();}	 /**< peek at the next position			 */	
	void pop()			{erase(begin());}	/**< pop the next position off the path */

	int getBlockCount() const { return blockCount; }};

class WaypointPath : public list<Vec2i> {
public:
	WaypointPath() {}
	void push(const Vec2i &pos/*, float dist*/)	{ push_front(pos); }
	Vec2i peek() const					{return front();}
	//float waypointToGoal() const		{ return front().second; }
	void pop()							{erase(begin());}
};

// ===============================
// 	class Unit
//
///	A game unit or building
// ===============================

/**
 * Represents a single game unit or building. The Unit class inherits from
 * EnhancementTypeBase as a mechanism to maintain a cache of it's current
 * stat values. These values are only recalculated when an effect is added
 * or removed, an upgrade is applied or the unit levels up. This way, all
 * requests from other parts of the code for the value of a stat can be
 * procured without having to perform lengthy computations. Previously, the
 * TotalUpgrade class provided this functionality.
 */
class Unit : public EnhancementTypeBase, public Entity {
public:
	typedef list<Command*> Commands;
	typedef list<UnitObserver*> Observers;
	typedef list<UnitReference> Pets;
	static const float speedDivider;
	static const int maxDeadCount;
	static const float highlightTime;
	static const int invalidId;

private:
	int id;					/**< unique identifier  */
	int hp;					/**< current hit points */
	int ep;					/**< current energy points */
	int loadCount;			/**< current 'load' (resources carried) */
	int deadCount;			/**< how many frames this unit has been dead */
	float progress;			/**< skill progress, between 0 and 1 */
	float progressSpeed;	/**< cached progress */
	float animProgressSpeed;/**< cached animation progress */
	int nextCommandUpdate;	/**< frame next command update will occur */
	float lastAnimProgress;	/**< animation progress last frame, between 0 and 1 */
	float animProgress;		/**< animation progress, between 0 and 1 */
	float highlight;		/**< alpha for selection circle effects */
	int progress2;			/**< 'secondary' skill progress counter */
	int kills;				/**< number of kills */

	UnitReference targetRef; 

	Field currField;		/**< */
	Field targetField;		/**< */
	const Level *level;		/**< */

	Vec2i pos;				/**< Current position */
	Vec2i lastPos;			/**< The last position before current */
	Vec2i nextPos;			/**< Position unit is moving into next. */
	Vec2i targetPos;		/**< Position of the target, or the cell of the target we care about. */
	Vec3f targetVec;
	Vec2i meetingPos;
	bool faceTarget;				/**< If true and target is set, we continue to face target. */
	bool useNearestOccupiedCell;	/**< If true, targetPos is set to target->getNearestOccupiedCell() */

	float lastRotation;		/**< facing last frame, in degrees */
	float targetRotation;	/**< desired facing, in degrees*/
	float rotation;			/**< current facing, in degrees */

	const UnitType *type;			/**< the UnitType of this unit */
	const ResourceType *loadType;	/**< the type if resource being carried */
	const SkillType *currSkill;		/**< the SkillType currently being executed */

	bool toBeUndertaken;		/**< awaiting a date with the grim reaper */
//	bool alive;
	bool autoRepairEnabled;		/**< is auto repair enabled */
	bool dirty;					/**< need a bath? */

	/** Effects (spells, etc.) currently effecting unit. */
	Effects effects;

	/** All effects created by this unit. */
	Effects effectsCreated;

	/** All stat changes from upgrades and level ups */
	EnhancementTypeBase totalUpgrade;
	Faction *faction;
	ParticleSystem *fire;
	Map *map;

	UnitPath unitPath;
	WaypointPath waypointPath;

	Commands commands;
	Observers observers;
	Pets pets;
	UnitReference master;
	
	const Command *commandCallback; // for script 'command callbacks'
	int hp_below_trigger;		// if non-zero, call the Trigger manager when HP falls below this
	int hp_above_trigger;		// if non-zero, call the Trigger manager when HP rises above this
	bool attacked_trigger;
 	
	float nextUpdateFrames;

public:
	Unit(int id, const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, Unit* master = NULL);
	~Unit();

	//queries
	int getId() const							{return id;}
	Field getCurrField() const					{return currField;}
	Zone getCurrZone() const					{return currField == Field::AIR ? Zone::AIR : Zone::LAND;}
	int getLoadCount() const					{return loadCount;}
	float getLastAnimProgress() const			{return lastAnimProgress;}
	float getProgress() const					{return progress;}
	float getAnimProgress() const				{return animProgress;}
	float getHightlight() const					{return highlight;}
	int getProgress2() const					{return progress2;}
	int getFactionIndex() const					{return faction->getIndex();}
	int getTeam() const							{return faction->getTeam();}
	int getHp() const							{return hp;}
	int getEp() const							{return ep;}
	int getProductionPercent() const;
	float getHpRatio() const					{return clamp(static_cast<float>(hp) / getMaxHp(), 0.f, 1.f);}
	float getEpRatio() const					{return !type->getMaxEp() ? 0.0f : clamp(static_cast<float>(ep)/getMaxEp(), 0.f, 1.f);}
	bool getToBeUndertaken() const				{return toBeUndertaken;}
	Unit *getTarget()							{return targetRef.getUnit();}
	Vec2i getNextPos() const					{return nextPos;}
	Vec2i getTargetPos() const					{return targetPos;}
	Vec3f getTargetVec() const					{return targetVec;}
	Field getTargetField() const				{return targetField;}
	Vec2i getMeetingPos() const					{return meetingPos;}
	Faction *getFaction() const					{return faction;}
	const ResourceType *getLoadType() const		{return loadType;}
	const UnitType *getType() const				{return type;}
	const SkillType *getCurrSkill() const		{return currSkill;}
	const EnhancementTypeBase *getTotalUpgrade() const	{return &totalUpgrade;}
	float getRotation() const					{return rotation;}
	float getVerticalRotation() const			{return 0.0f;}
	ParticleSystem *getFire() const				{return fire;}
	int getKills() const						{return kills;}
	const Level *getLevel() const				{return level;}
	const Level *getNextLevel() const;
	string getFullName() const;
	const UnitPath *getPath() const				{return &unitPath;}
	UnitPath *getPath()							{return &unitPath;}
	WaypointPath *getWaypointPath()				{return &waypointPath;}
	const WaypointPath *getWaypointPath() const {return &waypointPath;}
	int getSpeed(const SkillType *st) const;
	int getSpeed() const						{return getSpeed(currSkill);}
	Unit *getMaster() const						{return master;}
	void setMaster(Unit *master)				{this->master = master;}
	const Pets &getPets() const					{return pets;}
	const Emanations &getGetEmanations() const	{return type->getEmanations();}
	int getPetCount(const UnitType *unitType) const;
	const Commands &getCommands() const			{return commands;}
	const RepairCommandType *getRepairCommandType(const Unit *u) const;
	int getDeadCount() const					{return deadCount;}
	bool isMobile ()							{ return type->isMobile(); }

	void addPet(Unit *u)						{pets.push_back(u);}
	void petDied(Unit *u)						{pets.remove(u);}
	int killPets();

	void setCommandCallback()					{ commandCallback = commands.front(); }
	void clearCommandCallback()					{ commandCallback = NULL; }
	const Command* getCommandCallback() const	{ return commandCallback; }
	void setHPBelowTrigger(int i)				{ hp_below_trigger = i; }
	void setHPAboveTrigger(int i)				{ hp_above_trigger = i; }
	void setAttackedTrigger(bool val)			{ attacked_trigger = val; }
	bool getAttackedTrigger() const				{ return attacked_trigger; }

	/**
	 * Returns the total attack strength (base damage) for this unit using the
	 * supplied skill, taking into account all upgrades & effects.
	 */
	int getAttackStrength(const AttackSkillType *ast) const	{
		return (int)roundf(ast->getAttackStrength() * attackStrengthMult + attackStrength);
	}

	/**
	 * Returns the total attack percentage stolen for this unit using the
	 * supplied skill, taking into account all upgrades & effects.
	 */
	float getAttackPctStolen(const AttackSkillType *ast) const	{
		return ast->getAttackPctStolen() * attackPctStolenMult + attackPctStolen;
	}

	/**
	 * Returns the maximum range (attack range, spell range, etc.) for this unit
	 * using the supplied skill after all modifications due to upgrades &
	 * effects.
	 */
	int getMaxRange(const SkillType *st) const {
		switch(st->getClass()) {
			case SkillClass::ATTACK:
				return (int)roundf(st->getMaxRange() * attackRangeMult + attackRange);
			default:
				return st->getMaxRange();
		}
	}

	int getMaxRange(const AttackSkillTypes *asts) const {
		return (int)roundf(asts->getMaxRange() * attackRangeMult + attackRange);
	}

	//pos
	Vec2i getPos() const				{return pos;}
	Vec2i getLastPos() const			{return lastPos;}
	Vec2i getCenteredPos() const		{return Vec2i((int)type->getHalfSize()) + pos;}
	Vec2f getFloatCenteredPos() const	{return Vec2f(type->getHalfSize()) + Vec2f((float)pos.x, (float)pos.y);}
	Vec2i getNearestOccupiedCell(const Vec2i &from) const;
//	Vec2i getCellPos() const;

	//is
	bool isHighlighted() const			{return highlight > 0.f;}
	bool isDead() const					{return !hp;}
	bool isAlive() const				{return hp;}
	bool isDamaged() const				{return hp < getMaxHp();}
	bool isOperative() const			{return isAlive() && isBuilt();}
	bool isBeingBuilt() const			{return currSkill->getClass() == SkillClass::BE_BUILT;}
	bool isBuilt() const				{return !isBeingBuilt();}
	bool isPutrefacting() const			{return deadCount;}
	bool isAlly(const Unit *unit) const	{return faction->isAlly(unit->getFaction());}
	bool isInteresting(InterestingUnitType iut) const;
	bool isPet(const Unit *u) const;
	bool isAPet() const					{return master;}
	bool isAutoRepairEnabled() const	{return autoRepairEnabled;}
	bool isDirty()						{return dirty;}

	//set
	void setCurrField(Field currField)					{this->currField = currField;}
	void setCurrSkill(const SkillType *currSkill);
	void setCurrSkill(SkillClass sc)					{setCurrSkill(getType()->getFirstStOfClass(sc));}
	void setLoadCount(int loadCount)					{this->loadCount = loadCount;}
	void setLoadType(const ResourceType *loadType)		{this->loadType = loadType;}
	void setProgress2(int progress2)					{this->progress2 = progress2;}
	void setPos(const Vec2i &pos);
	void setNextPos(const Vec2i &nextPos)				{this->nextPos = nextPos; targetRef = NULL;}
	void setTargetPos(const Vec2i &targetPos)			{this->targetPos = targetPos; targetRef = NULL;}
	void setTarget(const Unit *unit, bool faceTarget = true, bool useNearestOccupiedCell = true);
	void setTargetVec(const Vec3f &targetVec)			{this->targetVec = targetVec;}
	void setMeetingPos(const Vec2i &meetingPos)			{this->meetingPos = meetingPos;}
	void setAutoRepairEnabled(bool autoRepairEnabled) {
		this->autoRepairEnabled = autoRepairEnabled;
		notifyObservers(UnitObserver::eStateChange);
	}
	void setDirty(bool dirty)							{this->dirty = dirty;}
	void setNextUpdateFrames(float nextUpdateFrames)	{this->nextUpdateFrames = nextUpdateFrames;}
	
	//render related
	const Model *getCurrentModel() const				{return currSkill->getAnimation();}
	Vec3f getCurrVector() const							{return getCurrVectorFlat() + Vec3f(0.f, type->getHalfHeight(), 0.f);}
	//Vec3f getCurrVectorFlat() const;
	// this is a heavy use function so it's inlined even though it isn't exactly small
	Vec3f getCurrVectorFlat() const {
		Vec3f v(static_cast<float>(pos.x),  computeHeight(pos), static_cast<float>(pos.y));
	
		if (currSkill->getClass() == SkillClass::MOVE) {
			v = Vec3f(static_cast<float>(lastPos.x), computeHeight(lastPos),
					  static_cast<float>(lastPos.y)).lerp(progress, v);
		}

		v.x += type->getHalfSize();
		v.z += type->getHalfSize();
	
		return v;
	}

	//command related
	const CommandType *getFirstAvailableCt(CommandClass commandClass) const;
	bool anyCommand() const								{return !commands.empty();}

	/** return current command, assert that there is always one command */
	Command *getCurrCommand() const {
		assert(!commands.empty());
		return commands.front();
	}
	unsigned int getCommandSize() const;
	CommandResult giveCommand(Command *command);		//give a command
	CommandResult finishCommand();						//command finished
	CommandResult cancelCommand();						//cancel command on back of queue
	CommandResult cancelCurrCommand();					//cancel current command

	//lifecycle
	void create(bool startingUnit = false);
	void born();
	void kill()											{kill(pos, true);}
	void kill(const Vec2i &lastPos, bool removeFromCells);
	void undertake()									{faction->remove(this);}

	//observers
	void addObserver(UnitObserver *unitObserver)		{observers.push_back(unitObserver);}
	void removeObserver(UnitObserver *unitObserver)		{observers.remove(unitObserver);}
	void notifyObservers(UnitObserver::Event event)		{
		for(Observers::iterator it = observers.begin(); it != observers.end(); ++it){
			(*it)->unitEvent(event, this);
		}
	}

	// signals, should prob replace that observer stuff above
	typedef Unit* u_ptr;
	typedef const UnitType* ut_ptr;
	typedef sigslot::signal1<u_ptr>			UnitSignal;
	typedef sigslot::signal2<u_ptr, Vec2i>	UnitPosSignal;
	typedef sigslot::signal2<u_ptr, ut_ptr> MorphSignal;

	//UnitSignal		Created;	 /**< fires when a unit is created		   */
	//UnitSignal		Born;		/**< fires when a unit is 'born'		  */
	//UnitPosSignal	Moving;	   /**< fires just before a unit is moved	 */
	//UnitPosSignal	Moved;	  /**< fires after a unit has moved			*/
	//MorphSignal		Morphed; /**<  */
	UnitSignal		Died;	/**<  */

	//other
	void resetHighlight()								{highlight= 1.f;}
	const CommandType *computeCommandType(const Vec2i &pos, const Unit *targetUnit= NULL) const;
	string getDesc(bool friendly) const;
	bool computeEp();
	bool repair(int amount = 0, float multiplier = 1.0f);
	bool decHp(int i);
	int update2()										{return ++progress2;}
	void preProcessSkill();
	bool update();
	void face(const Vec2i &nextPos);

	Unit *tick();
	void applyUpgrade(const UpgradeType *upgradeType);
	void computeTotalUpgrade();
	void incKills();
	bool morph(const MorphCommandType *mct);
	CommandResult checkCommand(const Command &command) const;
	void applyCommand(const Command &command);

	bool add(Effect *e);
	void remove(Effect *e);
	void effectExpired(Effect *effect);
	bool doRegen(int hpRegeneration, int epRegeneration);

private:
	float computeHeight(const Vec2i &pos) const;
	void updateTarget(const Unit *target = NULL);
	void clearCommands();
	CommandResult undoCommand(const Command &command);
	void recalculateStats();
	Command *popCommand();
};

// =====================================================
// 	class Targets
// =====================================================

/** Utility class for managing multiple targets by distance. */
class Targets : public std::map<Unit *, int> {
public:
	void record(Unit *target, int dist) {
		iterator i = find(target);
		if (i == end() || dist < i->second) {
			(*this)[target] = dist;
		}
	}

	/**
	 * Cheesy input, specify SkillClass::COUNT to tell it not to care about command class.
	 * Specify 0.f for pctHealth and health level wont matter. These should get
	 * optimized out when inlining.
	 */
	Unit *getNearest(SkillClass skillClass = SkillClass::COUNT, float pctHealth = 0.0f) {
		Unit *nearest = NULL;
		int dist = 0x10000;
		for(iterator i = begin(); i != end(); ++i) {
			Unit * u = i->first;
			if(i->second < dist
					&& (skillClass == SkillClass::COUNT || u->getType()->hasSkillClass(skillClass))
					&& (!pctHealth || (float)u->getHp() / (float)u->getMaxHp() < pctHealth)) {
				nearest = i->first;
				dist = i->second;
			}
		}
		return nearest;
	}
};


}}// end namespace

#endif
