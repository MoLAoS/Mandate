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

namespace Glest { namespace Game {

using Shared::Graphics::ParticleSystem;
using Shared::Graphics::Vec4f;
using Shared::Graphics::Vec2f;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Model;
using Shared::Graphics::Entity;

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
	iutIdleHarvester,
	iutBuiltBuilding,
	iutProducer,
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

	Vec2i pos;
	Vec2i lastPos;
	Vec2i targetPos;		//absolute target pos
	Vec3f targetVec;
	Vec2i meetingPos;

	float lastRotation;		//in degrees
	float targetRotation;
	float rotation;

	const UnitType *type;
	const ResourceType *loadType;
	const SkillType *currSkill;
// 	const SkillType *lastSkill;

	bool toBeUndertaken;
	bool alive;
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

public:
	Unit(int id, const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, Unit* master = NULL);
	Unit(const XmlNode *node, Faction *faction, Map *map, const TechTree *tt, bool putInWorld = true);
	~Unit();

	//queries
	int getId() const							{return id;}
	Field getCurrField() const					{return currField;}
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
	float getHpRatio() const					{return clamp(static_cast<float>(hp)/getMaxHp(), 0.f, 1.f);}
	float getEpRatio() const					{return !type->getMaxEp() ? 0.0f : clamp(static_cast<float>(ep)/getMaxEp(), 0.f, 1.f);}
	bool getToBeUndertaken() const				{return toBeUndertaken;}
	Unit *getTarget()							{return targetRef.getUnit();}
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
	const Effects &getEffects() const			{return effects;}
	const RepairCommandType *getRepairCommandType(const Unit *u) const;
	int getDeadCount() const					{return deadCount;}

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
	Vec2i getCenteredPos() const		{return Vec2i(pos.x + type->getSize() / 2.f, pos.y + type->getSize() / 2.f);}
	Vec2f getFloatCenteredPos() const	{return Vec2f(pos.x + type->getSize() / 2.f, pos.y + type->getSize() / 2.f);}
	Vec2i getCellPos() const;

	//is
	bool isHighlighted() const			{return highlight>0.f;}
	bool isDead() const					{return !alive;}
	bool isAlive() const				{return alive;}

	bool isOperative() const			{return isAlive() && isBuilt();}
	bool isBeingBuilt() const			{return currSkill->getClass()==scBeBuilt;}
	bool isBuilt() const				{return !isBeingBuilt();}
	bool isPutrefacting() const			{return deadCount!=0;}
	bool isAlly(const Unit *unit) const	{return faction->isAlly(unit->getFaction());}
	bool isDamaged() const				{return hp < getMaxHp();}
	bool isInteresting(InterestingUnitType iut) const;
	bool isPet(const Unit *u) const;
	bool isAutoRepairEnabled() const	{return autoRepairEnabled;}
	bool isDirty()						{return dirty;}
	bool isReady(const CommandType *ct)	{return faction->reqsOk(ct) && ep >= ct->getEpCost();}

	//set
	void setCurrField(Field currField)					{this->currField= currField;}
	void setCurrSkill(const SkillType *currSkill);
	void setCurrSkill(SkillClass sc)					{setCurrSkill(getType()->getFirstStOfClass(sc));}
	void setLoadCount(int loadCount)					{this->loadCount= loadCount;}
	void setLoadType(const ResourceType *loadType)		{this->loadType= loadType;}
	void setProgress2(int progress2)					{this->progress2= progress2;}
	void setPos(const Vec2i &pos);
	void setTargetPos(const Vec2i &targetPos);
	void setTarget(const Unit *unit);
	void setTargetVec(const Vec3f &targetVec)			{this->targetVec= targetVec;}
	void setMeetingPos(const Vec2i &meetingPos, bool queue);
	void setAutoRepairEnabled(bool autoRepairEnabled)	{
		this->autoRepairEnabled = autoRepairEnabled;
		notifyObservers(UnitObserver::eStateChange);
	}
	void setDirty(bool dirty)							{this->dirty = dirty;}

	//render related
	const Model *getCurrentModel() const				{return currSkill->getAnimation();}
	Vec3f getCurrVector() const							{return getCurrVectorFlat() + Vec3f(0.f, type->getHeight()/2.f, 0.f);}
	Vec3f getCurrVectorFlat() const;

	//command related

	const CommandType *getFirstAvailableCt(CommandClass commandClass) const;
	bool anyCommand() const								{return !commands.empty();	}

	/** return current command or NULL if none */
	Command *getCurrCommand() const {
		return commands.empty() ? NULL : commands.front();
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
	void undertake()									{faction->removeUnit(this);}
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
	void update(const XmlNode *node, const TechTree *tt, bool creation, bool putInWorld);
	void updateMinor(const XmlNode *node);
	void writeMinorUpdate(XmlNode *node) const;


	/**
	 * Update the unit by one tick.
	 *
	 * @returns if the unit died during this call, the killer is returned, NULL
	 *		  otherwise.
	 */
	Unit *tick();
	void applyUpgrade(const UpgradeType *upgradeType);
	void computeTotalUpgrade();
	void incKills();
	bool morph(const MorphCommandType *mct);
	CommandResult checkCommand(Command *command) const;
	void applyCommand(Command *command);

	/**
	 * Add/apply the supplied effect to this unit.
	 *
	 * @returns true if this effect had an immediate regen/degen that killed the
	 *		  unit.
	 */
	bool add(Effect *e);

	/** Remove the specified effect from this unit, if it is present. */
	void remove(Effect *e);

	/**
	 * Notify this unit that the supplied effect has expired. This effect will
	 * (should) have been one that this unit caused.
	 */
	void effectExpired(Effect *effect);

	/**
	 * Do positive and/or negative Hp and Ep regeneration. This method is
	 * provided to reduce redundant code in a number of other places, mostly in
	 * UnitUpdater.
	 *
	 * @returns true if the unit dies
	 */
	bool doRegen(int hpRegeneration, int epRegeneration);

//	void writeState(UnitState &us);
//	void readState(UnitState &us);

private:
	float computeHeight(const Vec2i &pos) const;
	void updateTarget();
	void clearCommands();
	CommandResult undoCommand(Command *command);

	/**
	 * Recalculate the unit's stats (contained in base class
	 * EnhancementTypeBase) to take into account changes in the effects and/or
	 * totalUpgrade objects.
	 */
	void recalculateStats();
};

// =====================================================
// 	class Targets
//
/// Class for managing multiple targets by distance.
// =====================================================

class Targets : public std::map<Unit *, int> {
public:
	void record(Unit * target, int dist) {
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
