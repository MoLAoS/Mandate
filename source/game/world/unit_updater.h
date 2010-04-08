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
#include "particle.h"
#include "random.h"
#include "network_manager.h"

using Shared::Util::Random;

#ifndef LOG_COMMAND_UPDATE
#	define LOG_COMMAND_UPDATE 0
#endif

#if LOG_COMMAND_UPDATE
#	define COMMAND_UPDATE_LOG(x) STREAM_LOG(x)
#else
#	define COMMAND_UPDATE_LOG(x)
#endif

namespace Glest{ namespace Game{

class Unit;
class Map;
class ScriptManager;
class ParticleDamager;

namespace Search { 
	class RoutePlanner; 
}

// =====================================================
//	class UnitUpdater
//
///	Updates all units in the game, even the player
///	controlled units, performs basic actions only
///	such as responding to an attack
// =====================================================

class UnitUpdater{
private:
	friend class ParticleDamager;
	friend class World;

private:
	static const int maxResSearchRadius= 10;
	static const int harvestDistance= 5;
//	static const int ultraResourceFactor= 3;
	/**
	 * When a unit who can repair, but not attack is faced with a hostile, this is the percentage
	 * of the radius that we search from the center of the intersection point for a friendly that
	 * can attack.  This is used to decide if the repiarer stays put in to backup a friendly who
	 * we presume will be fighting, of if the repairer flees.
	 */
	static const fixed repairerToFriendlySearchRadius;

private:
	const GameCamera *gameCamera;
	Gui *gui;
	Map *map;
	World *world;
	Console *console;
	Search::RoutePlanner *routePlanner;
	Random random;
	GameSettings gameSettings;

public:
	void init(Game &game);

	//update skills
	//REFACTOR: do this in World::update()
	void updateUnit(Unit *unit);

	//update commands
	//void updateStop(Unit *unit);
	//void updateMove(Unit *unit);
	//void updateAttack(Unit *unit);
	//void updateAttackStopped(Unit *unit);
	void updateBuild(Unit *unit);
	void updateHarvest(Unit *unit);
	//void updateRepair(Unit *unit);
	void updateProduce(Unit *unit);
	void updateUpgrade(Unit *unit);
	void updateMorph(Unit *unit);
	void updateCastSpell(Unit *unit);
	//void updateGuard(Unit *unit);
	//void updatePatrol(Unit *unit);

private:
	//attack
	void hit(Unit *attacker);
	void hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField, Unit *attacked = NULL);
	void damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, fixed distance);
	void startAttackSystems(Unit *unit, const AttackSkillType* ast);

	//effects
	void applyEffects(Unit *source, const EffectTypes &effectTypes, const Vec2i &targetPos, Field targetField, int splashRadius);
	void applyEffects(Unit *source, const EffectTypes &effectTypes, Unit *dest, fixed distance);
	void appyEffect(Unit *unit, Effect *effect);
	void updateEmanations(Unit *unit);

	Resource* searchForResource(Unit *unit, const HarvestCommandType *hct);
};

// =====================================================
//	class ParticleDamager
// =====================================================

class ParticleDamager {
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
	void execute(ParticleSystem *particleSystem);
};

}}//end namespace

#endif
