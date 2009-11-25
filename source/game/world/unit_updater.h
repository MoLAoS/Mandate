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

#ifndef _GLEST_GAME_UNITUPDATER_H_
#define _GLEST_GAME_UNITUPDATER_H_

#include "gui.h"
#include "path_finder.h"
#include "particle.h"
#include "random.h"
#include "network_manager.h"

using Shared::Graphics::ParticleObserver;
using Shared::Util::Random;

namespace Glest { namespace Game {

class Unit;
class Map;
class ScriptManager;
class ParticleDamager;
namespace Search { class PathFinder; }

// =====================================================
// class UnitUpdater
//
/// Updates all units in the game, even the player
/// controlled units, performs basic actions only
/// such as responding to an attack
// =====================================================

class UnitUpdater {
private:
	friend class ParticleDamager;
	friend class World;

private:
	static const int maxResSearchRadius = 10;
	static const int harvestDistance = 5;
//	static const int ultraResourceFactor= 3;

	/**
	 * When a unit who can repair, but not attack is faced with a hostile, this is the percentage
	 * of the radius that we search from the center of the intersection point for a friendly that
	 * can attack.  This is used to decide if the repiarer stays put in to backup a friendly who
	 * we presume will be fighting, of if the repairer flees.
	 */
	static const float repairerToFriendlySearchRadius;

private:
	const GameCamera *gameCamera;
	Gui &gui;
	World &world;
	Map *map;
	Console &console;
	ScriptManager *scriptManager;
	Search::PathFinder *pathFinder;
	Random random;
	GameSettings &gameSettings;

public:
	UnitUpdater(Game &game);
	void init(Game &game);

	//update skills
	void updateUnit(Unit *unit);

	//update commands
	void updateUnitCommand(Unit *unit);
	void updateStop(Unit *unit);
	void updateMove(Unit *unit);
	void updateAttack(Unit *unit);
	void updateAttackStopped(Unit *unit);
	void updateBuild(Unit *unit);
	void updateHarvest(Unit *unit);
	void updateRepair(Unit *unit);
	void updateProduce(Unit *unit);
	void updateUpgrade(Unit *unit);
	void updateMorph(Unit *unit);
	void updateCastSpell(Unit *unit);
	void updateGuard(Unit *unit);
	void updatePatrol(Unit *unit);

private:
	//attack
	void hit(Unit *attacker);
	void hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField, Unit *attacked = NULL);
	void damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, float distance);
	void startAttackSystems(Unit *unit, const AttackSkillType* ast);

	//effects
	void applyEffects(Unit *source, const EffectTypes &effectTypes, const Vec2i &targetPos, Field targetField, int splashRadius);
	void applyEffects(Unit *source, const EffectTypes &effectTypes, Unit *dest, float distance);
	void appyEffect(Unit *unit, Effect *effect);
	void updateEmanations(Unit *unit);

	//misc
	Command *doAutoAttack(Unit *unit);
	Command *doAutoRepair(Unit *unit);
	Command *doAutoFlee(Unit *unit);
	bool searchForResource(Unit *unit, const HarvestCommandType *hct);
	bool attackerOnSight(const Unit *unit, Unit **enemyPtr);
	bool attackableOnSight(const Unit *unit, Unit **enemyPtr, const AttackSkillTypes *asts, const AttackSkillType **past);
	bool attackableOnRange(const Unit *unit, Unit **enemyPtr, const AttackSkillTypes *asts, const AttackSkillType **past);
	bool unitOnRange(const Unit *unit, int range, Unit **enemyPtr, const AttackSkillTypes *asts, const AttackSkillType **past);
	bool repairableOnRange(
			const Unit *unit,
			Vec2i center,
			int centerSize,
			Unit **rangedPtr,
			const RepairCommandType *rct,
			const RepairSkillType *rst,
			int range,
			bool allowSelf = false,
			bool militaryOnly = false,
			bool damagedOnly = true);

	bool repairableOnRange(
			const Unit *unit,
			Unit **rangedPtr,
			const RepairCommandType *rct,
			int range,
			bool allowSelf = false,
			bool militaryOnly = false,
			bool damagedOnly = true) {
		return repairableOnRange(unit, unit->getPos(), unit->getType()->getSize(),
								 rangedPtr, rct, rct->getRepairSkillType(), range, allowSelf, militaryOnly, damagedOnly);
	}

	bool repairableOnSight(const Unit *unit, Unit **rangedPtr, const RepairCommandType *rct, bool allowSelf) {
		return repairableOnRange(unit, rangedPtr, rct, unit->getSight(), allowSelf);
	}

	void enemiesAtDistance(const Unit *unit, const Unit *priorityUnit, int distance, vector<Unit*> &enemies);
	bool updateAttackGeneric(Unit *unit, Command *command, const AttackCommandType *act, Unit* target, const Vec2i &targetPos);
	/*
	 Vec2i getNear(const Vec2i &pos, Vec2i target, int minRange, int maxRange, int targetSize = 1) {
	  return map->getNearestPos(pos, target, targetSize, minRange, maxRange);
	 }

	 Vec2i getNear(const Vec2i &pos, const Unit *target, int minRange, int maxRange) {
	  return map->getNearestPos(pos, target, minRange, maxRange);
	 }*/

	bool isLocal()							{return NetworkManager::getInstance().isLocal();}
	bool isNetworkGame()					{return NetworkManager::getInstance().isNetworkGame();}
	bool isNetworkServer()					{return NetworkManager::getInstance().isNetworkServer();}
	bool isNetworkClient()					{return NetworkManager::getInstance().isNetworkClient();}
	ServerInterface *getServerInterface()	{return NetworkManager::getInstance().getServerInterface();}
};

// =====================================================
// class ParticleDamager
// =====================================================

class ParticleDamager: public ParticleObserver {
public:
	UnitReference attackerRef;
	const AttackSkillType* ast;
	UnitUpdater *unitUpdater;
	const GameCamera *gameCamera;
	Vec2i targetPos;
	Field targetField;
	UnitReference targetRef;


public:
	ParticleDamager(Unit *attacker, Unit *target, UnitUpdater *unitUpdater, const GameCamera *gameCamera);
	virtual void update(ParticleSystem *particleSystem);
};

}}//end namespace

#endif
