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

//#include <map>

#include "model.h"
#include "upgrade_type.h"
#include "particle.h"
#include "skill_type.h"
#include "effect.h"
#include "unit_type.h"
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

using namespace ProtoTypes;
using Sim::Map;

class Unit;
class UnitFactory;

WRAPPED_ENUM( AutoCmdFlag,
	REPAIR,
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

// ===============================
// 	class Unit
//
///	A game unit or building
// ===============================

/** Represents a single game unit or building. The Unit class inherits from
  * EnhancementType as a mechanism to maintain a cache of it's current
  * stat values. These values are only recalculated when an effect is added
  * or removed, an upgrade is applied or the unit levels up. */
class Unit : public EnhancementType {
	friend class EntityFactory<Unit>;

public:
	typedef list<Command*> Commands;
	//typedef list<UnitId> Pets;

private:
	// basic stats
	int id;					/**< unique identifier  */
	int hp;					/**< current hit points */
	int ep;					/**< current energy points */
	int loadCount;			/**< current 'load' (resources carried) */
	int deadCount;			/**< how many frames this unit has been dead */
	int progress2;			/**< 'secondary' skill progress counter (progress for Production) */
	int kills;				/**< number of kills */

	// housed unit bits
	UnitIdList	m_carriedUnits;
	UnitIdList	m_unitsToCarry;
	UnitIdList	m_unitsToUnload;
	UnitId		m_carrier;

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

	const UnitType *type;			/**< the UnitType of this unit */
	const ResourceType *loadType;	/**< the type if resource being carried */
	const SkillType *currSkill;		/**< the SkillType currently being executed */

	// some flags
	bool toBeUndertaken;			/**< awaiting a date with the grim reaper */

	bool m_autoCmdEnable[AutoCmdFlag::COUNT];
	//bool autoRepairEnabled;			/**< is auto repair enabled */

	bool carried;					/**< is the unit being carried */
	//bool visible;

	bool	m_cloaked;

	bool	m_cloaking, m_deCloaking;
	float	m_cloakAlpha;

	// this should go somewhere else
	float highlight;				/**< alpha for selection circle effects */

	Effects effects;				/**< Effects (spells, etc.) currently effecting unit. */
	Effects effectsCreated;			/**< All effects created by this unit. */
	EnhancementType totalUpgrade;	/**< All stat changes from upgrades, level ups and effects */

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
	bool attacked_trigger;

public:
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
	Field getCurrField() const					{return type->getField();}
	Zone getCurrZone() const					{return type->getZone();}
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
	Vec2f getVerticalRotation() const			{return Vec2f(0.f);}
	ParticleSystem *getFire() const				{return fire;}
	int getKills() const						{return kills;}
	const Level *getLevel() const				{return level;}
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
	void housedUnitDied(Unit *unit);

	//bool isVisible() const					{return carried;}
	void setCarried(Unit *host)				{carried = (host != 0); m_carrier = (host ? host->getId() : -1);}
	void loadUnitInit(Command *command);
	void unloadUnitInit(Command *command);
	//----

	///@todo move to a helper of ScriptManager, connect signals...
	void setCommandCallback();
	void clearCommandCallback()					{ commandCallback = 0; }
	const int getCommandCallback() const	{ return commandCallback; }
	void setHPBelowTrigger(int i)				{ hp_below_trigger = i; }
	void setHPAboveTrigger(int i)				{ hp_above_trigger = i; }
	void setAttackedTrigger(bool val)			{ attacked_trigger = val; }
	bool getAttackedTrigger() const				{ return attacked_trigger; }

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
	bool isHighlighted() const			{return highlight > 0.f;}
	bool isPutrefacting() const			{return deadCount;}
	bool isAlly(const Unit *unit) const	{return faction->isAlly(unit->getFaction());}
	bool isInteresting(InterestingUnitType iut) const;
	bool isAutoCmdEnabled(AutoCmdFlag f) const	{return m_autoCmdEnable[f];}
	bool isCloaked() const					{return m_cloaked;}
	bool renderCloaked() const			{return m_cloaked || m_cloaking || m_deCloaking;}
	bool isOfClass(UnitClass uc) const { return type->isOfClass(uc); }
	bool isTargetUnitVisible(int teamIndex) const;
	bool isActive() const;
	bool isBuilding() const;
	bool isDead() const					{return !hp;}
	bool isAlive() const				{return hp;}
	bool isDamaged() const				{return hp < getMaxHp();}
	bool isOperative() const			{return isAlive() && isBuilt();}
	bool isBeingBuilt() const			{
		return currSkill->getClass() == SkillClass::BE_BUILT
			|| currSkill->getClass() == SkillClass::BUILD_SELF;
	}
	bool isBuilt() const				{return !isBeingBuilt();}

	// set
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
	void undertake();

	//other
	void resetHighlight();
	const CommandType *computeCommandType(const Vec2i &pos, const Unit *targetUnit= NULL) const;

	string getShortDesc() const;
	int getQueuedOrderCount() const { return (commands.size() > 1 ? commands.size() - 1 : 0); }
	string getLongDesc() const;
	
	bool computeEp();
	bool repair(int amount = 0, fixed multiplier = 1);
	bool decHp(int i);
	bool decEp(int i);
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
	bool morph(const MorphCommandType *mct, const UnitType *ut, Vec2i offset = Vec2i(0), bool reprocessCommands = true);
	bool transform(const TransformCommandType *tct, const UnitType *ut, Vec2i pos, CardinalDir facing);
	CmdResult checkCommand(const Command &command) const;
	bool checkEnergy(const CommandType *ct) const { return ep >= ct->getEnergyCost(); }
	void applyCommand(const Command &command);
	void startAttackSystems(const AttackSkillType *ast);
	void startSpellSystems(const CastSpellSkillType *sst);
	Projectile* launchProjectile(ProjectileType *projType, const Vec3f &endPos);
	Splash* createSplash(SplashType *splashType, const Vec3f &pos);

	int getCarriedCount() const { return m_carriedUnits.size(); }

	bool add(Effect *e);
	void remove(Effect *e);
	void effectExpired(Effect *effect);
	bool doRegen(int hpRegeneration, int epRegeneration);

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
};

}}// end namespace

#endif
