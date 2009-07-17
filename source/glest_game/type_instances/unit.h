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

namespace Glest { namespace Game {

using Shared::Graphics::ParticleSystem;
using Shared::Graphics::Vec4f;
using Shared::Graphics::Vec2f;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Vec2i;
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

enum CommandResult {
	crSuccess,
	crFailRes,
	crFailReqs,
	crFailPetLimit,
	crFailUndefined,
	crSomeFailed
};

enum InterestingUnitType {
	iutIdleBuilder,
	iutIdleHarvester,
	iutIdleWorker,
	iutIdleRepairer,
	iutIdleRestorer,
	iutBuiltBuilding,
	iutProducer,
	iutIdleProducer,
	iutDamaged,
	iutStore
};

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
//
/// Holds the next cells of a Unit movement
// =====================================================

class UnitPath {
private:
	static const int maxBlockCount;

private:
	int blockCount;
	vector<Vec2i> pathQueue;

public:
	UnitPath() : blockCount(0), pathQueue() {}
	bool isBlocked()				{return blockCount >= maxBlockCount;}
	bool isEmpty()					{return pathQueue.empty();}

	void clear()					{pathQueue.clear(); blockCount = 0;}
	void incBlockCount()			{pathQueue.clear();	blockCount++;}
	void push(const Vec2i &path)	{pathQueue.push_back(path);}
	Vec2i pop()						{
		Vec2i p = pathQueue.front();
		pathQueue.erase(pathQueue.begin());
		return p;
	}
	
	void read(const XmlNode *node);
	void write(XmlNode *node) const;
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
	int id;
	int hp;
	int ep;
	int loadCount;
	int deadCount;
	float progress;			//between 0 and 1
	float lastAnimProgress;	//between 0 and 1
	float animProgress;		//between 0 and 1
	float highlight;
	int progress2;
	int kills;

	UnitReference targetRef;

	Field currField;
	Field targetField;
	const Level *level;

	Vec2i pos;				/** Current position */
	Vec2i lastPos;			/** The last position before current */
	Vec2i nextPos;			/** Position unit is moving into next. */
	Vec2i targetPos;		/** Position of the target, or the cell of the target we care about. */
	Vec3f targetVec;
	Vec2i meetingPos;
	bool faceTarget;		/** If true and target is set, we continue to face target. */
	bool useNearestOccupiedCell;	/** If true, targetPos is set to target->getNearestOccupiedCell() */

	float lastRotation;		//in degrees
	float targetRotation;
	float rotation;

	const UnitType *type;
	const ResourceType *loadType;
	const SkillType *currSkill;

	bool toBeUndertaken;
//	bool alive;
	bool autoRepairEnabled;
	bool dirty;

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

	Commands commands;
	Observers observers;
	Pets pets;
	UnitReference master;
	
	
	int64 lastCommandUpdate;	// microseconds
	int64 lastUpdated;			// microseconds
	int64 lastCommanded;		// milliseconds
	float nextUpdateFrames;

public:
	Unit(int id, const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, Unit* master = NULL);
	Unit(const XmlNode *node, Faction *faction, Map *map, const TechTree *tt, bool putInWorld = true);
	~Unit();

	//queries
	int getId() const							{return id;}
	Field getCurrField() const					{return currField;}
   Zone getCurrZone () const  { return currField == FieldAir ? ZoneAir : ZoneSurface; }
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
	const int64 &getLastUpdated() const			{return lastUpdated;}
	int64 getLastCommanded() const				{return Chrono::getCurMillis() - lastCommanded;}
	int64 getLastCommandUpdate() const			{return Chrono::getCurMicros() - lastCommandUpdate;}
   bool isMobile () { return type->isMobile(); }


	void addPet(Unit *u)						{pets.push_back(u);}
	void petDied(Unit *u)						{pets.remove(u);}
	int killPets();


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
			case scAttack:
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
	bool isBeingBuilt() const			{return currSkill->getClass() == scBeBuilt;}
	bool isBuilt() const				{return !isBeingBuilt();}
	bool isPutrefacting() const			{return deadCount;}
	bool isAlly(const Unit *unit) const	{return faction->isAlly(unit->getFaction());}
	bool isInteresting(InterestingUnitType iut) const;
	bool isPet(const Unit *u) const;
	bool isAPet() const					{return master;}
	bool isAutoRepairEnabled() const	{return autoRepairEnabled;}
	bool isDirty()						{return dirty;}
	bool wasRecentlyCommanded() const	{return getLastCommanded() < 750;}

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
	void setLastUpdated(const int64 &lastUpdated)		{this->lastUpdated = lastUpdated;}
	void setNextUpdateFrames(float nextUpdateFrames)	{this->nextUpdateFrames = nextUpdateFrames;}
	void resetLastCommanded()							{lastCommanded = Chrono::getCurMillis();}
	void resetLastCommandUpdated()						{lastCommandUpdate = Chrono::getCurMicros();}

	//render related
	const Model *getCurrentModel() const				{return currSkill->getAnimation();}
	Vec3f getCurrVector() const							{return getCurrVectorFlat() + Vec3f(0.f, type->getHalfHeight(), 0.f);}
	//Vec3f getCurrVectorFlat() const;
	// this is a heavy use function so it's inlined even though it isn't exactly small
	Vec3f getCurrVectorFlat() const {
		Vec3f v(static_cast<float>(pos.x),  computeHeight(pos), static_cast<float>(pos.y));
	
		if (currSkill->getClass() == scMove) {
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
	void save(XmlNode *node, bool morphed = false) const;

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
	string getDesc() const;
	bool computeEp();
	bool repair(int amount = 0, float multiplier = 1.0f);
	bool decHp(int i);
	int update2()										{return ++progress2;}
	bool update();
	void update(const XmlNode *node, const TechTree *tt, bool creation, bool putInWorld, bool netClient, float nextAdvanceFrames);
	void updateMinor(const XmlNode *node);
	void writeMinorUpdate(XmlNode *node) const;
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

//	void writeState(UnitState &us);
//	void readState(UnitState &us);

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
	 * Cheesy input, specify scCount to tell it not to care about command class.
	 * Specify 0.f for pctHealth and health level wont matter. These should get
	 * optimized out when inlining.
	 */
	Unit *getNearest(SkillClass skillClass = scCount, float pctHealth = 0.0f) {
		Unit *nearest = NULL;
		int dist = 0x10000;
		for(iterator i = begin(); i != end(); i++) {
			Unit * u = i->first;
			if(i->second < dist
					&& (skillClass == scCount || u->getType()->hasSkillClass(skillClass))
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
