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
#include "effect.h"
#include "unit_type.h"
#include "stats.h"
#include "unit_reference.h"
#include "math_util.h"
#include "entity.h"
#include "timer.h"
#include "logger.h"
#include "factory.h"

#include "game_particle.h"

#include "prototypes_enums.h"
#include "simulation_enums.h"
#include "entities_enums.h"

#define LOG_COMMAND_ISSUE 1

#ifndef LOG_UNIT_LIFECYCLE
#	define LOG_UNIT_LIFECYCLE 0
#endif
#ifndef LOG_COMMAND_ISSUE
#	define LOG_COMMAND_ISSUE 0
#endif

#if LOG_UNIT_LIFECYCLE
#	define UNIT_LOG(x) STREAM_LOG(x)
#else
#	define UNIT_LOG(x)
#endif

#if LOG_COMMAND_ISSUE
#	define COMMAND_LOG(x) STREAM_LOG(x)
#else
#	define COMMAND_LOG(x)
#endif

using namespace Shared::Math;
using namespace Shared::Graphics;

using Shared::Platform::Chrono;
using Shared::Util::SingleTypeFactory;
using namespace Glest::ProtoTypes;
using Glest::Sim::Map;

namespace Glest { namespace Entities {

// =====================================================
// 	class UnitObserver
// =====================================================
///@todo deprecate, use signals
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

class Vec2iList : public list<Vec2i> {
public:
	void read(const XmlNode *node);
	void write(XmlNode *node) const;
};

// =====================================================
// 	class UnitPath
// =====================================================
/** Holds the next cells of a Unit movement 
  * @extends std::list<Shared::Math::Vec2i>
  */
class UnitPath : public Vec2iList {
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

class UnitFactory;

// ===============================
// 	class Unit
//
///	A game unit or building
// ===============================

/**
 * Represents a single game unit or building. The Unit class inherits from
 * EnhancementType as a mechanism to maintain a cache of it's current
 * stat values. These values are only recalculated when an effect is added
 * or removed, an upgrade is applied or the unit levels up. This way, all
 * requests from other parts of the code for the value of a stat can be
 * procured without having to perform lengthy computations. Previously, the
 * TotalUpgrade class provided this functionality.
 */
class Unit : public EnhancementType {
	friend class UnitFactory;
public:
	typedef list<Command*> Commands;
	typedef list<UnitObserver*> Observers;
	typedef list<UnitId> Pets;

private:
	int id;					/**< unique identifier  */
	int hp;					/**< current hit points */
	int ep;					/**< current energy points */
	int loadCount;			/**< current 'load' (resources carried) */
	int deadCount;			/**< how many frames this unit has been dead */

	int lastAnimReset;		/**< the frame the current animation cycle was started */
	int nextAnimReset;		/**< the frame the next animation cycle will begin */

	int lastCommandUpdate;	/**< the frame this unit last updated its command */
	int nextCommandUpdate;	/**< the frame next command update will occur */

	int attackStartFrame;	/**< the frame the unit will start an attack system */
	int soundStartFrame;	/**< the frame the sound for the current skill should be started */

	int progress2;			/**< 'secondary' skill progress counter (progress for Produce/Morph) */
	int kills;				/**< number of kills */

	float highlight;		/**< alpha for selection circle effects */

	// target
	UnitId targetRef; 
	Field targetField;		/**< Field target travels in @todo replace with Zone ? */
	bool faceTarget;				/**< If true and target is set, we continue to face target. */
	bool useNearestOccupiedCell;	/**< If true, targetPos is set to target->getNearestOccupiedCell() */

	const Level *level;		/**< current Level */

	Vec2i pos;				/**< Current position */
	Vec2i lastPos;			/**< The last position before current */
	Vec2i nextPos;			/**< Position unit is moving into next. Note: this will almost always == pos */

	Vec2i targetPos;		/**< Position of the target, or the cell of the target we care about. */
	Vec3f targetVec;		/**< 3D position of target (centre) */

	Vec2i meetingPos;		/**< Cell position of metting point */

	float lastRotation;		/**< facing last frame, in degrees */
	float targetRotation;	/**< desired facing, in degrees*/
	float rotation;			/**< current facing, in degrees */

	const UnitType *type;			/**< the UnitType of this unit */
	const ResourceType *loadType;	/**< the type if resource being carried */
	const SkillType *currSkill;		/**< the SkillType currently being executed */

	bool toBeUndertaken;		/**< awaiting a date with the grim reaper */
	bool autoRepairEnabled;		/**< is auto repair enabled */

	/** Effects (spells, etc.) currently effecting unit. */
	Effects effects;

	/** All effects created by this unit. */
	Effects effectsCreated;

	/** All stat changes from upgrades and level ups */
	EnhancementType totalUpgrade;
	/** All stat changes from upgrades, level ups & effects */
	//EnhancementType totalEnhancement;
	Faction *faction;
	ParticleSystem *fire;
	Map *map;

	UnitPath unitPath;
	WaypointPath waypointPath;

	Commands commands;
	Observers observers;

	UnitParticleSystems eyeCandy;

	int commandCallback;		// for script 'command callbacks'
	int hp_below_trigger;		// if non-zero, call the Trigger manager when HP falls below this
	int hp_above_trigger;		// if non-zero, call the Trigger manager when HP rises above this
	bool attacked_trigger;

public:
	// signals, should prob replace the UnitObserver stuff
	//
	typedef Unit* u_ptr;
	typedef sigslot::signal<u_ptr>		UnitSignal;

	UnitSignal		Created;	  /**< fires when a unit is created		   */
	UnitSignal		Born;		 /**< fires when a unit is 'born'		  */
	UnitSignal		Moving;		/**< fires just before a unit is moved	 */
	UnitSignal		Moved;	   /**< fires after a unit has moved		*/
	UnitSignal		Morphing; /**<  */
	UnitSignal		Morphed; /**<  */
	UnitSignal		Died;	/**<  */

private:
	Unit(int id, const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, Unit* master = NULL);
	Unit(const XmlNode *node, Faction *faction, Map *map, const TechTree *tt, bool putInWorld = true);
	~Unit();

public:
	void save(XmlNode *node) const;

	//queries
	int getId() const							{return id;}
	Field getCurrField() const					{return type->getField();}
	Zone getCurrZone() const					{return type->getZone();}
	int getLoadCount() const					{return loadCount;}
	int getSize() const							{return type->getSize();}
	//float getLastAnimProgress() const			{return lastAnimProgress;}
	float getProgress() const;
	float getAnimProgress() const;
	float getHightlight() const					{return highlight;}
	int getProgress2() const					{return progress2;}

	int getNextCommandUpdate() const			{ return nextCommandUpdate; }
	int getLastCommandUpdate() const			{ return lastCommandUpdate; }
	int getNextAnimReset() const				{ return nextAnimReset; }
	int getNextAttackFrame() const				{ return attackStartFrame; }
	int getSoundStartFrame() const				{ return soundStartFrame; }

	int getFactionIndex() const					{return faction->getIndex();}
	int getTeam() const							{return faction->getTeam();}
	int getHp() const							{return hp;}
	int getEp() const							{return ep;}
	int getProductionPercent() const;
	float getHpRatio() const					{return clamp(float(hp) / getMaxHp(), 0.f, 1.f);}
	fixed getHpRatioFixed() const				{ return fixed(hp) / getMaxHp(); }
	float getEpRatio() const					{return !type->getMaxEp() ? 0.0f : clamp(float(ep)/getMaxEp(), 0.f, 1.f);}
	bool getToBeUndertaken() const				{return toBeUndertaken;}
	UnitId getTarget() const					{return targetRef;}
	Vec2i getNextPos() const					{return nextPos;}
	Vec2i getTargetPos() const					{return targetPos;}
	Vec3f getTargetVec() const					{return targetVec;}
	Field getTargetField() const				{return targetField;}
	Vec2i getMeetingPos() const					{return meetingPos;}
	Faction *getFaction() const					{return faction;}
	const ResourceType *getLoadType() const		{return loadType;}
	const UnitType *getType() const				{return type;}
	const SkillType *getCurrSkill() const		{return currSkill;}
	const EnhancementType *getTotalUpgrade() const	{return &totalUpgrade;}
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
	const Emanations &getEmanations() const		{return type->getEmanations();}
	const Commands &getCommands() const			{return commands;}
	const RepairCommandType *getRepairCommandType(const Unit *u) const;
	int getDeadCount() const					{return deadCount;}
	bool isMobile ()							{ return type->isMobile(); }

	///@todo move to a helper of ScriptManager, connect signals...
	void setCommandCallback();
	void clearCommandCallback()					{ commandCallback = 0; }
	const int getCommandCallback() const	{ return commandCallback; }
	void setHPBelowTrigger(int i)				{ hp_below_trigger = i; }
	void setHPAboveTrigger(int i)				{ hp_above_trigger = i; }
	void setAttackedTrigger(bool val)			{ attacked_trigger = val; }
	bool getAttackedTrigger() const				{ return attacked_trigger; }

	void resetAnim(int frame) { nextAnimReset = frame; }

	/**
	 * Returns the total attack strength (base damage) for this unit using the
	 * supplied skill, taking into account all upgrades & effects.
	 */
	int getAttackStrength(const AttackSkillType *ast) const	{
		return (ast->getAttackStrength() * attackStrengthMult + attackStrength).intp();
	}

	/**
	 * Returns the total attack percentage stolen for this unit using the
	 * supplied skill, taking into account all upgrades & effects.
	 * @todo fix for fixed ;)
	 */
	//fixed getAttackPctStolen(const AttackSkillType *ast) const	{
	//	return (ast->getAttackPctStolen() * attackPctStolenMult + attackPctStolen);
	//}

	/**
	 * Returns the maximum range (attack range, spell range, etc.) for this unit
	 * using the supplied skill after all modifications due to upgrades &
	 * effects.
	 */
	int getMaxRange(const SkillType *st) const {
		switch(st->getClass()) {
			case SkillClass::ATTACK:
				return (st->getMaxRange() * attackRangeMult + attackRange).intp();
			default:
				return st->getMaxRange();
		}
	}

	int getMaxRange(const AttackSkillTypes *asts) const {
		return (asts->getMaxRange() * attackRangeMult + attackRange).intp();
	}

	//pos
	Vec2i getPos() const				{return pos;}
	Vec2i getLastPos() const			{return lastPos;}
	Vec2i getCenteredPos() const		{return Vec2i(type->getHalfSize().intp()) + pos;}
	//Vec2f getFloatCenteredPos() const	{return Vec2f(type->getHalfSize()) + Vec2f((float)pos.x, (float)pos.y);}
	fixedVec2 getFixedCenteredPos() const	{ return fixedVec2(pos.x + type->getHalfSize(), pos.y + type->getHalfSize()); }
	Vec2i getNearestOccupiedCell(const Vec2i &from) const;

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
	bool isAutoRepairEnabled() const	{return autoRepairEnabled;}

	bool isOfClass(UnitClass uc) const { return type->isOfClass(uc); }

	//set
	void setCurrSkill(const SkillType *currSkill);
	void setCurrSkill(SkillClass sc)					{setCurrSkill(getType()->getFirstStOfClass(sc));}
	void setLoadCount(int loadCount)					{this->loadCount = loadCount;}
	void setLoadType(const ResourceType *loadType)		{this->loadType = loadType;}
	void setProgress2(int progress2)					{this->progress2 = progress2;}
	void setPos(const Vec2i &pos);
	void setNextPos(const Vec2i &nextPos)				{this->nextPos = nextPos; targetRef = -1;}
	void setTargetPos(const Vec2i &targetPos)			{this->targetPos = targetPos; targetRef = -1;}
	void setTarget(const Unit *unit, bool faceTarget = true, bool useNearestOccupiedCell = true);
	void setTargetVec(const Vec3f &targetVec)			{this->targetVec = targetVec;}
	void setMeetingPos(const Vec2i &meetingPos)			{this->meetingPos = meetingPos;}
	void setAutoRepairEnabled(bool autoRepairEnabled) {
		this->autoRepairEnabled = autoRepairEnabled;
		notifyObservers(UnitObserver::eStateChange);
	}

	//render related
	const Model *getCurrentModel() const				{return currSkill->getAnimation();}
	Vec3f getCurrVector() const							{
		return getCurrVectorFlat() + Vec3f(0.f, type->getHalfHeight().toFloat(), 0.f);
	}
	//Vec3f getCurrVectorFlat() const;
	// this is a heavy use function so it's inlined even though it isn't exactly small
	Vec3f getCurrVectorFlat() const {
		Vec3f v(float(pos.x),  computeHeight(pos), float(pos.y));
			if (currSkill->getClass() == SkillClass::MOVE) {
			v = Vec3f(float(lastPos.x), computeHeight(lastPos), float(lastPos.y)).lerp(getProgress(), v);
		}
		const float &halfSize = type->getHalfSize().toFloat();
		v.x += halfSize;
		v.z += halfSize;
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

	//other
	void resetHighlight()								{highlight= 1.f;}
	const CommandType *computeCommandType(const Vec2i &pos, const Unit *targetUnit= NULL) const;
	string getDesc(bool friendly) const;
	bool computeEp();
	bool repair(int amount = 0, fixed multiplier = 1);
	bool decHp(int i);
	int update2()										{return ++progress2;}

	void updateSkillCycle(int offset);
	void updateMoveSkillCycle();
	//void updateAnimationCycle();
	void updateAnimDead();
	void updateAnimCycle(int animOffset, int soundOffset = -1, int attackOffset = -1);

	bool update();
	void updateEmanations();
	void face(const Vec2i &nextPos);

	Unit *tick();
	void applyUpgrade(const UpgradeType *upgradeType);
	void computeTotalUpgrade();
	void incKills();
	bool morph(const MorphCommandType *mct);
	CommandResult checkCommand(const Command &command) const;
	void applyCommand(const Command &command);
	void startAttackSystems(const AttackSkillType *ast);

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

inline ostream& operator<<(ostream &stream, const Unit &unit) {
	return stream << "[id:" << unit.getId() << "|" << unit.getType()->getName() << "]";
}


class UnitFactory : public sigslot::has_slots {
private:
	int		idCounter;
	UnitMap unitMap;
	//Units	unitList;
	Units deadList;

public:
	UnitFactory();
	~UnitFactory();
	Unit* newInstance(const XmlNode *node, Faction *faction, Map *map, const TechTree *tt, bool putInWorld = true);
	Unit* newInstance(const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, Unit* master = NULL);
	Unit* getUnit(int id);
	void onUnitDied(Unit *unit);
	void update();
	void deleteUnit(Unit *unit);
};

}}// end namespace

#endif
