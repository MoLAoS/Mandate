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

#include "pch.h"
#include "unit_updater.h"

#include "sound.h"
#include "upgrade.h"
#include "unit.h"
#include "particle_type.h"
#include "core_data.h"
#include "config.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "game.h"
#include "route_planner.h"
#include "object.h"
#include "faction.h"
#include "network_manager.h"
#include "cartographer.h"
#include "network_util.h"
#include "leak_dumper.h"
#include "renderer.h"
#include "earthquake_type.h"

#define UNIT_LOG(x) {}
//#define UNIT_LOG(x) { theLogger.add(x); }

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Glest::Game::Search;

namespace Glest { namespace Game {

//FIXME: We check the subfaction in born too.  Should it be removed from there?
//local func to keep players from getting stuff they aren't supposed to.
static bool verifySubfaction(Unit *unit, const ProducibleType *pt) {
	if (pt->isAvailableInSubfaction(unit->getFaction()->getSubfaction())) {
		return true;
	} else {
		unit->finishCommand();
		unit->setCurrSkill(SkillClass::STOP);
		unit->getFaction()->deApplyCosts(pt);
		return false;
	}
}

// =====================================================
// 	class UnitUpdater
// =====================================================

// ===================== PUBLIC ========================
const fixed UnitUpdater::repairerToFriendlySearchRadius = fixed(5) / 4;

void UnitUpdater::init(Game &game) {
	this->gui = game.getGui();
	this->gameCamera = game.getGameCamera();
	this->world = game.getWorld();
	this->map = world->getMap();
	this->console = game.getConsole();
	routePlanner = world->getRoutePlanner();
	gameSettings = game.getGameSettings();
}

// ==================== progress skills ====================

//skill dependent actions
void UnitUpdater::updateUnit(Unit *unit) {
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();
	GameInterface *gni = theNetworkManager.getGameInterface();
	const SkillType * const currSkill = unit->getCurrSkill();

	//play skill sound
	if (currSkill->getSound()) {
		if (theWorld.getFrameCount() == unit->getSoundStartFrame()) {
			if (map->getTile(Map::toTileCoords(unit->getPos()))->isVisible(world->getThisTeamIndex())) {
				soundRenderer.playFx(currSkill->getSound(), unit->getCurrVector(), gameCamera->getPos());
			}
		}
	}

	//start attack particle system
	if (unit->getCurrSkill()->getClass() == SkillClass::ATTACK
	&& unit->getNextAttackFrame() == world->getFrameCount()) {
		startAttackSystems(unit, static_cast<const AttackSkillType*>(unit->getCurrSkill()));
	}

	// update anim cycle
	if (theWorld.getFrameCount() >= unit->getNextAnimReset()) {
		// new anim cycle (or reset)
		gni->doUpdateAnim(unit);
	}

	//update emanations every 8 frames
	if (unit->getEmanations().size() && !((world->getFrameCount() + unit->getId()) % 8)
	&& unit->isOperative()) {
		updateEmanations(unit);
	}

	//update unit
	if (unit->update()) {
		const UnitType *ut = unit->getType();

		if (unit->getCurrSkill()->getClass() == SkillClass::FALL_DOWN) {
			assert(ut->getFirstStOfClass(SkillClass::GET_UP));
			unit->setCurrSkill(SkillClass::GET_UP);
		} else if (unit->getCurrSkill()->getClass() == SkillClass::GET_UP) {
			unit->setCurrSkill(SkillClass::STOP);
		}
		gni->doUpdateUnitCommand(this, unit);

		//move unit in cells
		if (unit->getCurrSkill()->getClass() == SkillClass::MOVE) {
			world->moveUnitCells(unit);

			//play water sound
			if (map->getCell(unit->getPos())->getHeight() < map->getWaterLevel() 
			&& unit->getCurrField() == Field::LAND
			&& map->getTile(Map::toTileCoords(unit->getPos()))->isVisible(world->getThisTeamIndex())
			&& theRenderer.getCuller().isInside(unit->getPos())) {
				soundRenderer.playFx(CoreData::getInstance().getWaterSound());
			}
		}
	}

	//unit death
	if (unit->isDead() && unit->getCurrSkill()->getClass() != SkillClass::DIE) {
		unit->kill();
	}
	map->assertUnitCells(unit);
}

// ==================== progress commands ====================
// gone to Client/Server-Interface

// ==================== auto commands ====================

Command *UnitUpdater::doAutoAttack(Unit *unit) {
	if (unit->getType()->hasCommandClass(CommandClass::ATTACK) 
	|| unit->getType()->hasCommandClass(CommandClass::ATTACK_STOPPED)) {

		for (int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
			const CommandType *ct = unit->getType()->getCommandType(i);

			if (!unit->getFaction()->isAvailable(ct)) {
				continue;
			}
			//look for an attack skill
			const AttackSkillType *ast = NULL;
			const AttackSkillTypes *asts = NULL;
			Unit *sighted = NULL;

			switch (ct->getClass()) {
				case CommandClass::ATTACK:
					asts = ((const AttackCommandType*)ct)->getAttackSkillTypes();
					break;

				case CommandClass::ATTACK_STOPPED:
					asts = ((const AttackStoppedCommandType*)ct)->getAttackSkillTypes();
					break;

				default:
					break;
			}
			//use it to attack
			if (asts) {
				if (attackableOnSight(unit, &sighted, asts, NULL)) {
					Command *newCommand = new Command(ct, CommandFlags(CommandProperties::AUTO), sighted->getPos());
					newCommand->setPos2(unit->getPos());
					return newCommand;
				}
			}
		}
	}

	return NULL;
}


Command *UnitUpdater::doAutoRepair(Unit *unit) {
	if (unit->getType()->hasCommandClass(CommandClass::REPAIR) && unit->isAutoRepairEnabled()) {
		for (int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
			const CommandType *ct = unit->getType()->getCommandType(i);

			if (!unit->getFaction()->isAvailable(ct) || ct->getClass() != CommandClass::REPAIR) {
				continue;
			}
			// look for a repair skill
			const RepairCommandType *rct = (const RepairCommandType*)ct;
			const RepairSkillType *rst = rct->getRepairSkillType();
			Unit *sighted = NULL;

			if (unit->getEp() >= rst->getEpCost() && repairableOnSight(unit, &sighted, rct, rst->isSelfAllowed())) {
				Command *newCommand;
				newCommand = new Command(rct, CommandFlags(CommandProperties::QUEUE, CommandProperties::AUTO),
							Map::getNearestPos(unit->getPos(), sighted, rst->getMinRange(), rst->getMaxRange()));
				newCommand->setPos2(unit->getPos());
				return newCommand;
			}
		}
	}
	return NULL;
}

Command *UnitUpdater::doAutoFlee(Unit *unit) {
	Unit *sighted = NULL;
	if (unit->getType()->hasCommandClass(CommandClass::MOVE) && attackerOnSight(unit, &sighted)) {
		//if there is a friendly military unit that we can heal/repair and is
		//rougly between us, then be brave
		if (unit->getType()->hasCommandClass(CommandClass::REPAIR)) {
			fixedVec2 myCenter = unit->getFixedCenteredPos();
			fixedVec2 signtedCenter = sighted->getFixedCenteredPos();
			fixedVec2 fcenter =  (myCenter + signtedCenter) / 2;
			Unit *myHero = NULL;

			// calculate the real distance to hostile by subtracting half of the size of each.
			fixed actualDistance = myCenter.dist(signtedCenter)
					- (unit->getType()->getHalfSize() + sighted->getType()->getHalfSize());

			// allow friendly combat unit to be within a radius of 65% of the actual distance to
			// hostile, starting from the half way intersection of the repairer and hostile.
			int searchRadius = (actualDistance * repairerToFriendlySearchRadius / 2).round();
			Vec2i center(fcenter.x.round(), fcenter.y.round());

			//try all of our repair commands
			/*
			for (int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
				const CommandType *ct = unit->getType()->getCommandType(i);
				if (ct->getClass() != CommandClass::REPAIR) {
					continue;
				}
				const RepairCommandType *rct = (const RepairCommandType*)ct;
				const RepairSkillType *rst = rct->getRepairSkillType();

				if (repairableOnRange(unit, center, 1, &myHero, rct, rst, searchRadius, false, true, false)) {
					return NULL;
				}
			}
			*/
		}
		Vec2i escapePos = unit->getPos() * 2 - sighted->getPos();
		return new Command(unit->getType()->getFirstCtOfClass(CommandClass::MOVE),
									CommandFlags(CommandProperties::AUTO), escapePos);
	}
	return NULL;
}

// ==================== updateStop ====================
/// 0: start, 1: continue doing nothing, 2: cancel (another command is pending)
void UnitUpdater::updateStop(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const StopCommandType *sct = static_cast<const StopCommandType*>(command->getType());
	Unit *sighted = NULL;
	Command *autoCmd;

	// if we have another command then stop sitting on your ass
	if (unit->getCommands().size() > 1 && unit->getCommands().front()->getType()->getClass() == CommandClass::STOP) {
		unit->finishCommand();
		UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(unit->getId()) + " cancelling stop" );
		return;
	}

	unit->setCurrSkill(sct->getStopSkillType());

	//we can attack any unit => attack it
	if ((autoCmd = doAutoAttack(unit))) {
		unit->giveCommand(autoCmd);
		return;
	}

	//we can repair any ally => repair it
	if ((autoCmd = doAutoRepair(unit))) {
		unit->giveCommand(autoCmd);
		return;
	}

	//see any unit and cant attack it => run
	if ((autoCmd = doAutoFlee(unit))) {
		unit->giveCommand(autoCmd);
	}
}

// ==================== updateMove ====================
/// 0: start, 1: do nothing (blocked), 2: move
void UnitUpdater::updateMove(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const MoveCommandType *mct = static_cast<const MoveCommandType*>(command->getType());
	bool autoCommand = command->isAuto();

	Vec2i pos;
	if (command->getUnit()) {
		pos = command->getUnit()->getCenteredPos();
		if (!command->getUnit()->isAlive()) {
			command->setPos(pos);
			command->setUnit(NULL);
		}
	} else {
		pos = command->getPos();
	}

	switch (routePlanner->findPath(unit, pos)) {
		case TravelState::MOVING:
			unit->setCurrSkill(mct->getMoveSkillType());
			unit->face(unit->getNextPos());
			//UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(unit->getId()) 
			//	+ " updating move "  + "Unit is at " + Vec2iToStr(unit->getPos()) 
			//	+ " now moving into " + Vec2iToStr(unit->getNextPos()) );
			break;
		case TravelState::BLOCKED:
			unit->setCurrSkill(SkillClass::STOP);
			if(unit->getPath()->isBlocked() && !command->getUnit()){
				unit->finishCommand();
			}
			break;	
		default: // TravelState::ARRIVED or TravelState::IMPOSSIBLE
			unit->finishCommand();	
	}

	// if we're doing an auto command, let's make sure we still want to do it
	if (autoCommand) {
		Command *autoCmd;
		//we can attack any unit => attack it
		if ((autoCmd = doAutoAttack(unit))) {
			unit->giveCommand(autoCmd);
			return;
		}

		//we can repair any ally => repair it
		if ((autoCmd = doAutoRepair(unit))) {
			unit->giveCommand(autoCmd);
			return;
		}

		//see any unit and cant attack it => run
		if ((autoCmd = doAutoFlee(unit))) {
			unit->giveCommand(autoCmd);
			return;
		}
	}
}

// ==================== updateGenericAttack ====================

/** @returns true when completed */
bool UnitUpdater::updateAttackGeneric(Unit *unit, Command *command, const AttackCommandType *act, Unit* target, const Vec2i &targetPos) {
	const AttackSkillType *ast = NULL;
	const AttackSkillTypes *asts = act->getAttackSkillTypes();

	if (target && !asts->getZone(target->getCurrZone()))
		unit->finishCommand();

	if(attackableOnRange(unit, &target, act->getAttackSkillTypes(), &ast)) {
		// found a target in range
		assert(ast);
		if (unit->getEp() >= ast->getEpCost()) {
			unit->setCurrSkill(ast);
			unit->setTarget(target);
		} else {
			unit->setCurrSkill(SkillClass::STOP);
		}
	} else {
		//compute target pos
		Vec2i pos;
		if ( attackableOnSight(unit, &target, asts, NULL) ) {
			// found a target, but not in range
			pos = target->getNearestOccupiedCell(unit->getPos());
			if (pos != unit->getTargetPos()) {
				unit->setTargetPos(pos);
				unit->getPath()->clear();
			}
		} else {
			// if no more targets and on auto command, then turn around
			if (command->isAuto() && command->hasPos2()) {
				if (Config::getInstance().getGsAutoReturnEnabled()) {
					command->popPos();
					pos = command->getPos();
					unit->getPath()->clear();
				} else {
					unit->finishCommand();
				}
			} else {
				pos = targetPos;
			}
		}

		//if unit arrives destPos order has ended
		switch(routePlanner->findPath(unit, pos)) {
		case TravelState::MOVING:
			unit->setCurrSkill(act->getMoveSkillType());
			unit->face(unit->getNextPos());
			break;
		case TravelState::BLOCKED:
			unit->setCurrSkill(SkillClass::STOP);
			if (unit->getPath()->isBlocked()) {
				return true;
			}
			break;
		default:
			return true;
		}
	}

	return false;
}

// ==================== updateAttack ====================
/// 0: start, 1: do nothing (blocked), 2: move to target unit, 3: move to target pos, 4: finsih
void UnitUpdater::updateAttack(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const AttackCommandType *act = static_cast<const AttackCommandType*>(command->getType());
	Unit *target = command->getUnit();

	if (updateAttackGeneric(unit, command, act, target, command->getPos())) {
		unit->finishCommand();
	}
}


// ==================== updateAttackStopped ====================
/// 0: start, 1: do nothing, 2 attack
void UnitUpdater::updateAttackStopped(Unit *unit) {

	Command *command = unit->getCurrCommand();
	const AttackStoppedCommandType *asct = static_cast<const AttackStoppedCommandType*>(command->getType());
	Unit *enemy = NULL;
	const AttackSkillType *ast = NULL;

	if (attackableOnRange(unit, &enemy, asct->getAttackSkillTypes(), &ast)) {
		assert(ast);
		unit->setCurrSkill(ast);
		unit->setTarget(enemy, true, true);
	} else {
		unit->setCurrSkill(asct->getStopSkillType());
	}
}


// ==================== updateBuild ====================
/// 0: start, 1: do nothing (blocked move or waiting for site to clear) 2: build, 3: move to site
void UnitUpdater::updateBuild(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const BuildCommandType *bct = static_cast<const BuildCommandType*>(command->getType());
	const UnitType *builtUnitType = command->getUnitType();
	Unit *builtUnit = NULL;
	Unit *target = unit->getTarget();

	assert(command->getUnitType());

	if (unit->getCurrSkill()->getClass() != SkillClass::BUILD) {
		//if not building

		int buildingSize = builtUnitType->getSize();
		Vec2i waypoint;

		// find the nearest place for the builder
		if (map->getNearestAdjacentFreePos(waypoint, unit, command->getPos(), Field::LAND, buildingSize)) {
			if (waypoint != unit->getTargetPos()) {
				unit->setTargetPos(waypoint);
				unit->getPath()->clear();
			}
		} else {
			console->addStdMessage("Blocked");
			unit->cancelCurrCommand();
			return;
		}

		switch (routePlanner->findPath(unit, waypoint)) {
			case TravelState::MOVING:
				unit->setCurrSkill(bct->getMoveSkillType());
				unit->face(unit->getNextPos());
				return;

			case TravelState::BLOCKED:
				unit->setCurrSkill(SkillClass::STOP);
				if(unit->getPath()->isBlocked()) {
					console->addStdMessage("Blocked");
					unit->cancelCurrCommand();
				}
				return;

			case TravelState::ARRIVED:
				if(unit->getPos() != waypoint) {
					console->addStdMessage("Blocked");
					unit->cancelCurrCommand();
					return;
				}
			default:
				; // otherwise, we fall through
		}

		//if arrived destination
		assert(command->getUnitType() != NULL);
		if (map->areFreeCells(command->getPos(), buildingSize, Field::LAND)) {
			if (!verifySubfaction(unit, builtUnitType)) {
				return;
			}

			// late resource allocation
			if (!command->isReserveResources()) {
				command->setReserveResources(true);
				if (unit->checkCommand(*command) != CommandResult::SUCCESS) {
					if (unit->getFactionIndex() == world->getThisFactionIndex()) {
						console->addStdMessage("BuildingNoRes");
					}
					unit->finishCommand();
					return;
				}
				unit->getFaction()->applyCosts(command->getUnitType());
			}

			builtUnit = new Unit(world->getNextUnitId(), command->getPos(), builtUnitType, unit->getFaction(), world->getMap());
			builtUnit->create();

			if (!builtUnitType->hasSkillClass(SkillClass::BE_BUILT)) {
				throw runtime_error("Unit " + builtUnitType->getName() + " has no be_built skill");
			}

			builtUnit->setCurrSkill(SkillClass::BE_BUILT);
			unit->setCurrSkill(bct->getBuildSkillType());
			unit->setTarget(builtUnit, true, true);
			map->prepareTerrain(builtUnit);
			command->setUnit(builtUnit);
			unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), false);

			if (!builtUnit->isMobile()) {
				world->getCartographer()->updateMapMetrics(builtUnit->getPos(), builtUnit->getSize());
			}
			
			//play start sound
			if (unit->getFactionIndex() == world->getThisFactionIndex()) {
				SoundRenderer::getInstance().playFx(bct->getStartSound(), unit->getCurrVector(), gameCamera->getPos());
			}
		} else {
			// there are no free cells
			vector<Unit *>occupants;
			map->getOccupants(occupants, command->getPos(), buildingSize, Zone::LAND);

			// is construction already under way?
			Unit *builtUnit = occupants.size() == 1 ? occupants[0] : NULL;
			if (builtUnit && builtUnit->getType() == builtUnitType && !builtUnit->isBuilt()) {
				// if we pre-reserved the resources, we have to deallocate them now, although
				// this usually shouldn't happen.
				if (command->isReserveResources()) {
					unit->getFaction()->deApplyCosts(command->getUnitType());
					command->setReserveResources(false);
				}
				unit->setTarget(builtUnit, true, true);
				unit->setCurrSkill(bct->getBuildSkillType());
				command->setUnit(builtUnit);

			} else {
				// is it not free because there are units in the way?
				if (occupants.size()) {
					// Can they get the fuck out of the way?
					vector<Unit *>::const_iterator i;
					for (i = occupants.begin();
							i != occupants.end() && (*i)->getType()->hasSkillClass(SkillClass::MOVE); ++i) ;
					if (i == occupants.end()) {
						// they all have a move command, so we'll wait
						return;
					}
					//TODO: Check for idle units and tell them to get the fuck out of the way.
					//TODO: Possibly add a unit notification to let player know builder is waiting
				}

				// blocked by non-moving units, surface objects (trees, rocks, etc.) or build area
				// contains deeply submerged terain
				unit->cancelCurrCommand();
				unit->setCurrSkill(SkillClass::STOP);
				if (unit->getFactionIndex() == world->getThisFactionIndex()) {
					console->addStdMessage("BuildingNoPlace");
				}
			}
		}
	} else {
		//if building
		Unit *builtUnit = command->getUnit();

		if (builtUnit && builtUnit->getType() != builtUnitType) {
			unit->setCurrSkill(SkillClass::STOP);
		} else if (!builtUnit || builtUnit->isBuilt()) {
			unit->finishCommand();
			unit->setCurrSkill(SkillClass::STOP);
		} else if (builtUnit->repair()) {
			//building finished
			unit->finishCommand();
			unit->setCurrSkill(SkillClass::STOP);
			unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), true);
			ScriptManager::onUnitCreated(builtUnit);
			if (unit->getFactionIndex() == world->getThisFactionIndex()) {
				SoundRenderer::getInstance().playFx(
					bct->getBuiltSound(),
					unit->getCurrVector(),
					gameCamera->getPos());
			}
		}
	}
}

// ==================== updateHarvest ====================
/// 0: start, 1: do nothing (blocked), 2: harvest, 3: move to resource, 4: move to store, 5: finish (no more goods)
void UnitUpdater::updateHarvest(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const HarvestCommandType *hct = static_cast<const HarvestCommandType*>(command->getType());
	Vec2i targetPos;

	if (unit->getCurrSkill()->getClass() != SkillClass::HARVEST) {
		//if not working
		if (!unit->getLoadCount()) {
			//if not loaded go for resources
			Resource *r = map->getTile(Map::toTileCoords(command->getPos()))->getResource();
			if (r && hct->canHarvest(r->getType())) {
				switch (routePlanner->findPathToResource(unit, command->getPos(), r->getType())) {
					case TravelState::ARRIVED:
						if (map->isResourceNear(unit->getPos(), r->getType(), targetPos)) {
							//if it finds resources it starts harvesting
							unit->setCurrSkill(hct->getHarvestSkillType());
							unit->setTargetPos(targetPos);
							command->setPos(targetPos);
							unit->face(targetPos);
							unit->setLoadCount(0);
							unit->setLoadType(map->getTile(Map::toTileCoords(targetPos))->getResource()->getType());
						}
						break;
					case TravelState::MOVING:
						unit->setCurrSkill(hct->getMoveSkillType());
						unit->face(unit->getNextPos());
						break;
					default:
						unit->setCurrSkill(SkillClass::STOP);
						break;
				}
			} else {
				//if can't harvest, search for another resource
				unit->setCurrSkill(SkillClass::STOP);
				if (!searchForResource(unit, hct)) {
					unit->finishCommand();
					//FIXME don't just stand around at the store!!
					//insert move command here.
				}
			}
		} else {
			//if loaded, return to store
			Unit *store = world->nearestStore(unit->getPos(), unit->getFaction()->getIndex(), unit->getLoadType());
			if (store) {
				switch (routePlanner->findPathToStore(unit, store)) {
					case TravelState::MOVING:
						unit->setCurrSkill(hct->getMoveLoadedSkillType());
						unit->face(unit->getNextPos());
						break;
					case TravelState::BLOCKED:
						unit->setCurrSkill(hct->getStopLoadedSkillType());
						break;
					default:
						; // otherwise, we fall through
				}
				if (map->isNextTo(unit->getPos(), store)) {
					//update resources
					int resourceAmount = unit->getLoadCount();
					// Just do this for all players ???
					if (unit->getFaction()->getCpuUltraControl()) {
						resourceAmount = (int)(resourceAmount * gameSettings.getResourceMultilpier(unit->getFactionIndex()));
						//resourceAmount *= ultraResourceFactor; // Pull from GameSettings
					}
					unit->getFaction()->incResourceAmount(unit->getLoadType(), resourceAmount);
					world->getStats().harvest(unit->getFactionIndex(), resourceAmount);
					ScriptManager::onResourceHarvested();

					//if next to a store unload resources
					unit->getPath()->clear();
					unit->setCurrSkill(SkillClass::STOP);
					unit->setLoadCount(0);
				}
			} else { // no store found, give up
				unit->finishCommand();
			}
		}
	} else {
		//if working
		Tile *sc = map->getTile(Map::toTileCoords(unit->getTargetPos()));
		Resource *r = sc->getResource();
		if (r != NULL) {
			//if there is a resource, continue working, until loaded
			unit->update2();
			if (unit->getProgress2() >= hct->getHitsPerUnit()) {
				unit->setProgress2(0);
				unit->setLoadCount(unit->getLoadCount() + 1);

				//if resource exausted, then delete it and stop
				if (r->decAmount(1)) {
					// let the cartographer know
					Vec2i rPos = r->getPos();
					sc->deleteResource();
					world->getCartographer()->updateMapMetrics(rPos, 2);
					unit->setCurrSkill(hct->getStopLoadedSkillType());
				}
				if (unit->getLoadCount() == hct->getMaxLoad()) {
					unit->setCurrSkill(hct->getStopLoadedSkillType());
					unit->getPath()->clear();
				}
			}
		} else {
			//if there is no resource
			if (unit->getLoadCount()) {
				unit->setCurrSkill(hct->getStopLoadedSkillType());
			} else {
				unit->finishCommand();
				unit->setCurrSkill(SkillClass::STOP);
			}
		}
	}
}

// ==================== updateRepair ====================
/// 0: start, 1: do nothing (blocked), 2: repair, 3: move to target, 4: move back (auto-return), 5: finish
void UnitUpdater::updateRepair(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const CommandType *ct = command->getType();
	assert(ct->getClass() == CommandClass::REPAIR);

	const RepairCommandType *rct = static_cast<const RepairCommandType*>(ct);
	const RepairSkillType *rst = rct->getRepairSkillType();
	bool repairThisFrame = unit->getCurrSkill()->getClass() == SkillClass::REPAIR;
	Unit *repaired = command->getUnit();

	// If the unit I was supposed to repair died or is already fixed then finish
	if (repaired && (repaired->isDead() || !repaired->isDamaged())) {
		unit->setCurrSkill(SkillClass::STOP);
		unit->finishCommand();
		return;
	}

	if (command->isAuto() && unit->getType()->hasCommandClass(CommandClass::ATTACK)) {
		Command *autoAttackCmd;
		// attacking is 1st priority

		if ((autoAttackCmd = doAutoAttack(unit))) {
			// if doAutoAttack found a target, we need to give it the correct starting address
			if (command->hasPos2()) {
				autoAttackCmd->setPos2(command->getPos2());
			}
			unit->giveCommand(autoAttackCmd);
		}
	}

	if (repairableOnRange(unit, &repaired, rct, rst->getMaxRange(), rst->isSelfAllowed())) {
		unit->setTarget(repaired, true, true);
		unit->setCurrSkill(rst);
	} else {
		Vec2i targetPos;
		if (repairableOnSight(unit, &repaired, rct, rst->isSelfAllowed())) {
			if (!map->getNearestFreePos(targetPos, unit, repaired, 1, rst->getMaxRange())) {
				unit->setCurrSkill(SkillClass::STOP);
				unit->finishCommand();
				return;
			}

			if (targetPos != unit->getTargetPos()) {
				unit->setTargetPos(targetPos);
				unit->getPath()->clear();
			}
		} else {
			// if no more damaged units and on auto command, turn around
			//finishAutoCommand(unit);

			if (command->isAuto() && command->hasPos2()) {
				if (Config::getInstance().getGsAutoReturnEnabled()) {
					command->popPos();
					unit->getPath()->clear();
				} else {
					unit->finishCommand();
				}
			}
			targetPos = command->getPos();
		}

		switch (routePlanner->findPath(unit, targetPos)) {
			case TravelState::ARRIVED:
				if (repaired && unit->getPos() != targetPos) {
					// presume blocked
					unit->setCurrSkill(SkillClass::STOP);
					unit->finishCommand();
					break;
				}
				if (repaired) {
					unit->setCurrSkill(rst);
				} else {
					unit->setCurrSkill(SkillClass::STOP);
					unit->finishCommand();
				}
				break;
			case TravelState::MOVING:
				unit->setCurrSkill(rct->getMoveSkillType());
				unit->face(unit->getNextPos());
				break;

			case TravelState::BLOCKED:
				unit->setCurrSkill(SkillClass::STOP);
				if(unit->getPath()->isBlocked()){
					unit->setCurrSkill(SkillClass::STOP);
					unit->finishCommand();
				}
				break;
			default:
				; // otherwise, we fall through
		}
	}

	if (repaired && !repaired->isDamaged()) {
		unit->setCurrSkill(SkillClass::STOP);
		unit->finishCommand();
	}

	if (repairThisFrame && unit->getCurrSkill()->getClass() == SkillClass::REPAIR) {
		//if repairing
		if (repaired) {
			unit->setTarget(repaired, true, true);
		}

		if (!repaired) {
			unit->setCurrSkill(SkillClass::STOP);
		} else {
			//shiney
			if (rst->getSplashParticleSystemType()) {
				const Tile *sc = map->getTile(Map::toTileCoords(repaired->getCenteredPos()));
				bool visible = sc->isVisible(world->getThisTeamIndex());

				SplashParticleSystem *psSplash = rst->getSplashParticleSystemType()->createSplashParticleSystem();
				psSplash->setPos(repaired->getCurrVector());
				psSplash->setVisible(visible);
				Renderer::getInstance().manageParticleSystem(psSplash, rsGame);
			}

			bool wasBuilt = repaired->isBuilt();

			assert(repaired->isAlive() && repaired->getHp() > 0);

			if (repaired->repair(rst->getAmount(), rst->getMultiplier())) {
				unit->setCurrSkill(SkillClass::STOP);
				if (!wasBuilt) {
					//building finished
					ScriptManager::onUnitCreated(repaired);
					if (unit->getFactionIndex() == world->getThisFactionIndex()) {
						// try to find finish build sound
						BuildCommandType *bct = (BuildCommandType *)unit->getType()->getFirstCtOfClass(CommandClass::BUILD);
						if (bct) {
							SoundRenderer::getInstance().playFx(
								bct->getBuiltSound(),
								unit->getCurrVector(),
								gameCamera->getPos());
						}
					}
				}
			}
		}
	}
}


// ==================== updateProduce ====================
/// 0: start, 1: produce, 2: finsh (ok), 3: cancel (could not place new unit)
void UnitUpdater::updateProduce(Unit *unit) {

	Command *command = unit->getCurrCommand();
	const ProduceCommandType *pct = static_cast<const ProduceCommandType*>(command->getType());
	Unit *produced;

	if (unit->getCurrSkill()->getClass() != SkillClass::PRODUCE) {
		//if not producing
		if (!verifySubfaction(unit, pct->getProducedUnit())) {
			return;
		}

		unit->setCurrSkill(pct->getProduceSkillType());
		unit->getFaction()->checkAdvanceSubfaction(pct->getProducedUnit(), false);
	} else {
		unit->update2();

		if (unit->getProgress2() > pct->getProduced()->getProductionTime()) {
			produced = new Unit(world->getNextUnitId(), Vec2i(0), pct->getProducedUnit(), unit->getFaction(), world->getMap());

			//if no longer a valid subfaction, let them have the unit, but make
			//sure it doesn't advance the subfaction

			if (verifySubfaction(unit, pct->getProducedUnit())) {
				unit->getFaction()->checkAdvanceSubfaction(pct->getProducedUnit(), true);
			}

			if (!world->placeUnit(unit->getCenteredPos(), 10, produced)) {
				unit->cancelCurrCommand();
				delete produced;
			} else {
				produced->create();
				produced->born();
				ScriptManager::onUnitCreated(produced);
				world->getStats().produce(unit->getFactionIndex());
				const CommandType *ct = produced->computeCommandType(unit->getMeetingPos());

				if (ct) {
					produced->giveCommand(new Command(ct, CommandFlags(), unit->getMeetingPos()));
				}

				if (pct->getProduceSkillType()->isPet()) {
					unit->addPet(produced);
					produced->setMaster(unit);
				}

				unit->finishCommand();
			}

			unit->setCurrSkill(SkillClass::STOP);
		}
	}
}

// ==================== updateUpgrade ====================
/// 0: start, 1: upgrade, 2: finish (ok)
void UnitUpdater::updateUpgrade(Unit *unit) {

	Command *command = unit->getCurrCommand();
	const UpgradeCommandType *uct = static_cast<const UpgradeCommandType*>(command->getType());

	//if subfaction becomes invalid while updating this command, then cancel it.

	if (!verifySubfaction(unit, uct->getProduced())) {
		unit->cancelCommand();
		unit->setCurrSkill(SkillClass::STOP);
		return;
	}

	if (unit->getCurrSkill()->getClass() != SkillClass::UPGRADE) {
		//if not producing
		unit->setCurrSkill(uct->getUpgradeSkillType());
		unit->getFaction()->checkAdvanceSubfaction(uct->getProducedUpgrade(), false);
	} else {
		//if producing
		if (unit->getProgress2() < uct->getProduced()->getProductionTime()) {
			unit->update2();
		}

		if (unit->getProgress2() >= uct->getProduced()->getProductionTime()) {
			unit->finishCommand();
			unit->setCurrSkill(SkillClass::STOP);
			unit->getFaction()->finishUpgrade(uct->getProducedUpgrade());
			unit->getFaction()->checkAdvanceSubfaction(uct->getProducedUpgrade(), true);
		}
	}
}


// ==================== updateMorph ====================
/// 0: start, 1: morph, 2: finsh (ok), 3: cancel (could not start because there is not enough space),
/// 4: cancel (could not finish morph because space became blocked)
void UnitUpdater::updateMorph(Unit *unit) {

	Command *command = unit->getCurrCommand();
	const MorphCommandType *mct = static_cast<const MorphCommandType*>(command->getType());

	//if subfaction becomes invalid while updating this command, then cancel it.
	if (!verifySubfaction(unit, mct->getMorphUnit())) {
		return;
	}

	if (unit->getCurrSkill()->getClass() != SkillClass::MORPH) {
		//if not morphing, check space
		// determine morph unit field
		Fields mfs = mct->getMorphUnit()->getFields();
		Field mf;
		if (mfs.get(Field::LAND)) mf = Field::LAND;
		else if (mfs.get(Field::AIR)) mf = Field::AIR;
		if (mfs.get(Field::AMPHIBIOUS)) mf = Field::AMPHIBIOUS;
		else if (mfs.get(Field::ANY_WATER)) mf = Field::ANY_WATER;
		else if (mfs.get(Field::DEEP_WATER)) mf = Field::DEEP_WATER;

		if (map->areFreeCellsOrHasUnit (unit->getPos(), mct->getMorphUnit()->getSize(), mf, unit)) {
			unit->setCurrSkill(mct->getMorphSkillType());
			unit->getFaction()->checkAdvanceSubfaction(mct->getMorphUnit(), false);
		} else {
			if (unit->getFactionIndex() == world->getThisFactionIndex()) {
				Game::getInstance()->getConsole()->addStdMessage("InvalidPosition");
			}
			unit->cancelCurrCommand();
		}
	} else {
		unit->update2();
		if (unit->getProgress2() > mct->getProduced()->getProductionTime()) {
			bool mapUpdate = unit->isMobile() != mct->getMorphUnit()->isMobile();
			//finish the command
			if (unit->morph(mct)) {
				unit->finishCommand();
				if (gui->isSelected(unit)) {
					gui->onSelectionChanged();
				}
				ScriptManager::onUnitCreated(unit);
				unit->getFaction()->checkAdvanceSubfaction(mct->getMorphUnit(), true);
				if (mapUpdate) {
					// obstacle added or removed, update annotated maps
					bool adding = !mct->getMorphUnit()->isMobile();
					world->getCartographer()->updateMapMetrics(unit->getPos(), unit->getSize());
				}
			} else {
				unit->cancelCurrCommand();
				if (unit->getFactionIndex() == world->getThisFactionIndex()) {
					console->addStdMessage("InvalidPosition");
				}
			}
			unit->setCurrSkill(SkillClass::STOP);
		}
	}
}

// ==================== updateCastSpell ====================

void UnitUpdater::updateCastSpell(Unit *unit) {
	//surprise! it never got implemented
}

// ==================== updateGuard ====================
/// 0: start, 1: do nothing, 2: attack 3: move to attack, 4: move to target (unit or pos)
/// no exit state, will gaurd a target if set, if target dies, will gaurd the position the target died at
void UnitUpdater::updateGuard(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const GuardCommandType *gct = static_cast<const GuardCommandType*>(command->getType());
	Unit *target = command->getUnit();
	Vec2i pos;

	if (target && target->isDead()) {
		//if you suck ass as a body guard then you have to hang out where your client died.
		command->setUnit(NULL);
		command->setPos(target->getPos());
		target = NULL;
	}

	if (target) {
		pos = Map::getNearestPos(unit->getPos(), target, 1, gct->getMaxDistance());
	} else {
		pos = Map::getNearestPos(unit->getPos(), command->getPos(), 1, gct->getMaxDistance());
	}

	if (updateAttackGeneric(unit, command, gct, NULL, pos)) {
		unit->setCurrSkill(SkillClass::STOP);
	}
}

// ==================== updatePatrol ====================
/// 0: start, 1: do nothing (blocked), 2: move to waypoint, 3: move to attack, 4: attack
void UnitUpdater::updatePatrol(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const PatrolCommandType *pct = static_cast<const PatrolCommandType*>(command->getType());
	Unit *target = command->getUnit();
	Unit *target2 = command->getUnit2();
	Vec2i pos;

	if (target) {
		pos = target->getCenteredPos();
		if (target->isDead()) {
			command->setUnit(NULL);
			command->setPos(pos);
		}
	} else {
		pos = command->getPos();
	}
	if (target) {
		pos = Map::getNearestPos(unit->getPos(), pos, 1, pct->getMaxDistance());
	}
	if (target2 && target2->isDead()) {
		command->setUnit2(NULL);
		command->setPos2(target2->getCenteredPos());
	}
	// If destination reached or blocked, turn around on next frame.
	if (updateAttackGeneric(unit, command, pct, NULL, pos)) {
		command->swap();
	}
}


void UnitUpdater::updateEmanations(Unit *unit) {
	// This is a little hokey, but probably the best way to reduce redundant code
	static EffectTypes singleEmanation;
	for (Emanations::const_iterator i = unit->getEmanations().begin();
			i != unit->getEmanations().end(); i++) {
		singleEmanation.resize(1);
		singleEmanation[0] = *i;
		applyEffects(unit, singleEmanation, unit->getPos(), Field::LAND, (*i)->getRadius());
		applyEffects(unit, singleEmanation, unit->getPos(), Field::AIR, (*i)->getRadius());
	}
}

// ==================== PRIVATE ====================

// ==================== attack ====================

void UnitUpdater::hit(Unit *attacker) {
	hit(attacker, static_cast<const AttackSkillType*>(attacker->getCurrSkill()), attacker->getTargetPos(), attacker->getTargetField());
}

void UnitUpdater::hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField, Unit *attacked) {
	typedef std::map<Unit*, fixed> DistMap;
	//hit attack positions
	if (ast->getSplash() && ast->getSplashRadius()) {
		Vec2i pos;
		fixed distance;
		DistMap hitSet;
		PosCircularIteratorSimple pci(map->getBounds(), targetPos, ast->getSplashRadius());
		while (pci.getNext(pos, distance)) {
			if ((attacked = map->getCell(pos)->getUnit(targetField))
			&& (hitSet.find(attacked) == hitSet.end() || hitSet[attacked] > distance)) {
				hitSet[attacked] = distance;
			}
		}
		foreach (DistMap, it, hitSet) {
			damage(attacker, ast, it->first, it->second);
			if (ast->isHasEffects()) {
				applyEffects(attacker, ast->getEffectTypes(), it->first, it->second);
			}
		}
	} else {
		if (!attacked) {
			attacked = map->getCell(targetPos)->getUnit(targetField);
		}
		if (attacked) {
			damage(attacker, ast, attacked, 0);
			if (ast->isHasEffects()) {
				applyEffects(attacker, ast->getEffectTypes(), attacked, 0);
			}
		}
	}
}

void UnitUpdater::damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, fixed distance){
	//get vars
	fixed fDamage = attacker->getAttackStrength(ast);
	//int damage = attacker->getAttackStrength(ast);
	int var = ast->getAttackVar();
	int armor = attacked->getArmor();

	fixed damageMultiplier = world->getTechTree()->getDamageMultiplier(ast->getAttackType(),
							 attacked->getType()->getArmorType());
	
	//compute damage
	fDamage += random.randRange(-var, var);
	fDamage /= (distance + 1);
	fDamage -= armor;
	fDamage *= damageMultiplier;
	if (fDamage < 1) {
		fDamage = 1;
	}
	int damage = fDamage.intp();
	int startingHealth = attacked->getHp();
	int actualDamage;
	//damage the unit
	if (attacked->decHp(damage)) {
		world->doKill(attacker, attacked);
		actualDamage = startingHealth;
	} 
	else {
		actualDamage = damage;
	}

	///@todo make effects stuff fixed point

	//add stolen health to attacker
	/*
	if (attacker->getAttackPctStolen(ast) != 0 || ast->getAttackPctVar() != 0) {
		
		fixed pct = attacker->getAttackPctStolen(ast)
				+ random.randRange(-ast->getAttackPctVar(), ast->getAttackPctVar());
		int stolen = (fixed(actualDamage) * pct).intp();
		if (stolen && attacker->doRegen(stolen, 0)) {
			// stealing a negative percentage and dying?
			world->doKill(attacker, attacker);
		}
	}
	*/
	//complain
	/*
	const Vec3f &attackerVec = attacked->getCurrVector();
	if (!gui->isVisible(Vec2i((int)roundf(attackerVec.x), (int)roundf(attackerVec.y)))) {
		attacked->getFaction()->attackNotice(attacked);
	}*/
}

void UnitUpdater::startAttackSystems(Unit *unit, const AttackSkillType *ast) {
	Renderer &renderer = Renderer::getInstance();

	ProjectileParticleSystem *psProj = 0;
	SplashParticleSystem *psSplash;

	ParticleSystemTypeProjectile *pstProj = ast->getProjParticleType();
	ParticleSystemTypeSplash *pstSplash = ast->getSplashParticleType();

	Vec3f startPos = unit->getCurrVector();
	Vec3f endPos = unit->getTargetVec();

	//make particle system
	const Tile *sc = map->getTile(Map::toTileCoords(unit->getPos()));
	const Tile *tsc = map->getTile(Map::toTileCoords(unit->getTargetPos()));
	bool visible = sc->isVisible(world->getThisTeamIndex()) || tsc->isVisible(world->getThisTeamIndex());

	//projectile
	if (pstProj != NULL) {
		psProj = pstProj->createProjectileParticleSystem();

		switch (pstProj->getStart()) {
			case ParticleSystemTypeProjectile::psSelf:
				break;

			case ParticleSystemTypeProjectile::psTarget:
				startPos = unit->getTargetVec();
				break;

			case ParticleSystemTypeProjectile::psSky:
				float skyAltitude = 30.f;
				startPos = endPos;
				startPos.x += random.randRange(-skyAltitude / 8.f, skyAltitude / 8.f);
				startPos.y += skyAltitude;
				startPos.z += random.randRange(-skyAltitude / 8.f, skyAltitude / 8.f);
				break;
		}

		// game network interface calls setPath() on psProj, differently for clients/servers
		theNetworkManager.getGameInterface()->doUpdateProjectile(unit, psProj, startPos, endPos);
		
		if(pstProj->isTracking() && unit->getTarget()) {
			psProj->setTarget(unit->getTarget());
			psProj->setDamager(new ParticleDamager(unit, unit->getTarget(), this, gameCamera));
		} else {
			psProj->setDamager(new ParticleDamager(unit, NULL, this, gameCamera));
		}
		psProj->setVisible(visible);
		renderer.manageParticleSystem(psProj, rsGame);
	} else {
		hit(unit);
	}

	//splash
	if (pstSplash != NULL) {
		psSplash = pstSplash->createSplashParticleSystem();
		psSplash->setPos(endPos);
		psSplash->setVisible(visible);
		renderer.manageParticleSystem(psSplash, rsGame);
		if (pstProj != NULL) {
			psProj->link(psSplash);
		}
	}

	const EarthquakeType *et = ast->getEarthquakeType();
	if (et) {
		et->spawn(*map, unit, unit->getTargetPos(), 1.f);
		if (et->getSound()) {
			// play rather visible or not
			SoundRenderer::getInstance().playFx(et->getSound(), unit->getTargetVec(), gameCamera->getPos());
		}
		// FIXME: hacky mechanism of keeping attackers from walking into their own earthquake :(
		unit->finishCommand();
	}
}

// ==================== effects ====================

// Apply effects to a specific location, with or without splash
void UnitUpdater::applyEffects(Unit *source, const EffectTypes &effectTypes,
				const Vec2i &targetPos, Field targetField, int splashRadius) {
	typedef std::map<Unit*, fixed> DistMap;
	Unit *target;

	if (splashRadius != 0) {
		DistMap hitList;
		DistMap::iterator i;
		Vec2i pos;
		fixed distance;

		PosCircularIteratorSimple pci(map->getBounds(), targetPos, splashRadius);
		while (pci.getNext(pos, distance)) {
			target = map->getCell(pos)->getUnit(targetField);
			if (target) {
				i = hitList.find(target);
				if (i == hitList.end() || i->second > distance) {
					hitList[target] = distance;
				}
			}
		}
		foreach (DistMap, it, hitList) {
			applyEffects(source, effectTypes, it->first, it->second);
		}
	} else {
		target = map->getCell(targetPos)->getUnit(targetField);
		if (target) {
			applyEffects(source, effectTypes, target, 0);
		}
	}
}

//apply effects to a specific target
void UnitUpdater::applyEffects(Unit *source, const EffectTypes &effectTypes, Unit *target, fixed distance) {
	//apply effects
	for (EffectTypes::const_iterator i = effectTypes.begin();
			i != effectTypes.end(); i++) {
		// lots of tests, roughly in order of speed of evaluation.
		if(		// ally/foe test
				(source->isAlly(target)
						? (*i)->isEffectsAlly()
						: (*i)->isEffectsFoe()) &&

				// building/normal unit test
				(target->getType()->isOfClass(UnitClass::BUILDING)
						? (*i)->isEffectsBuildings()
						: (*i)->isEffectsNormalUnits()) &&

				// pet test
				((*i)->isEffectsPetsOnly()
						? source->isPet(target)
						: true) &&

				// random chance test
				((*i)->getChance() != 100.0f
						? random.randRange(0.0f, 100.0f) < (*i)->getChance()
						: true)) {

			
			fixed strength = (*i)->isScaleSplashStrength() ? fixed(1) / (distance + 1) : 1;
			Effect *primaryEffect = new Effect((*i), source, NULL, strength, target, world->getTechTree());

			target->add(primaryEffect);

			for (EffectTypes::const_iterator j = (*i)->getRecourse().begin();
					j != (*i)->getRecourse().end(); j++) {
				source->add(new Effect((*j), NULL, primaryEffect, strength, source, world->getTechTree()));
			}
		}
	}
}

void UnitUpdater::appyEffect(Unit *u, Effect *e) {
	if (u->add(e)) {
		Unit *attacker = e->getSource();
		if (attacker) {
			world->getStats().kill(attacker->getFactionIndex(), u->getFactionIndex());
			attacker->incKills();
		} else if (e->getRoot()) {
			// if killed by a recourse effect, this was suicide
			world->getStats().kill(u->getFactionIndex(), u->getFactionIndex());
		}
	}
}

// ==================== misc ====================

/**
 * Finds the nearest position to dest, presuming that dest is the NW corner of
 * a square destSize cells, that is at least minRange from the target, but no
 * greater than maxRange.
 */
/*
Vec2i UnitUpdater::getNear(const Vec2i &orig, const Vec2i destNW, int destSize, int minRange, int maxRange) {
	if (destSize == 1) {
		return getNear(orig, destNW, minRange, maxRange);
	}

	Vec2f dest;
	int offset = destSize - 1;
	if (orig.x < destNW.x) {
		dest.x = destNW.x;
	} else if (orig.x > destNW.x + offset) {
		dest.x = destNW.x + offset;
	} else {
		dest.x = orig.x;
	}

	if (orig.y < destNW.y) {
		dest.y = destNW.y;
	} else if (orig.y >= destNW.y + offset) {
		dest.y = destNW.y + offset;
	} else {
		dest.y = orig.y;
	}

	return getNear(orig, dest, minRange, maxRange);
}
*/

//looks for a resource of type rt, if rt==NULL looks for any
//resource the unit can harvest
bool UnitUpdater::searchForResource(Unit *unit, const HarvestCommandType *hct) {
	Vec2i pos;

	PosCircularIteratorOrdered pci(map->getBounds(), unit->getCurrCommand()->getPos(),
			world->getPosIteratorFactory().getInsideOutIterator(1, maxResSearchRadius));

	while (pci.getNext(pos)) {
		Resource *r = map->getTile(Map::toTileCoords(pos))->getResource();
		if (r && hct->canHarvest(r->getType())) {
			unit->getCurrCommand()->setPos(pos);
			return true;
		}
	}
	return false;
}

inline bool UnitUpdater::attackerOnSight(const Unit *unit, Unit **rangedPtr) {
	int range = unit->getSight();
	return unitOnRange(unit, range, rangedPtr, NULL, NULL);
}

inline bool UnitUpdater::attackableOnSight(const Unit *unit, Unit **rangedPtr, 
					const AttackSkillTypes *asts, const AttackSkillType **past) {
	int range = unit->getSight();
	return unitOnRange(unit, range, rangedPtr, asts, past);
}

inline bool UnitUpdater::attackableOnRange(const Unit *unit, Unit **rangedPtr, 
					const AttackSkillTypes *asts, const AttackSkillType **past) {
	// can't attack beyond range of vision
	int range = min(unit->getMaxRange(asts), unit->getSight());
	return unitOnRange(unit, range, rangedPtr, asts, past);
}

// =====================================================
// 	class Targets
// =====================================================

/** Utility class for managing multiple targets by distance. */
class Targets : public std::map<Unit*, fixed> {
private:
	Unit *nearest;
	fixed distance;

public:
	Targets() : nearest(0), distance(fixed::max_int()) {}

	void record(Unit *target, fixed dist) {
		if (find(target) == end()) {
			insert(std::make_pair(target, dist));
		}
		if (dist < distance) {
			nearest = target;
			distance = dist;
		}
	}

	Unit* getNearest() {
		return nearest;
	}

	Unit* getNearestSkillClass(SkillClass sc) {
		foreach(Targets, it, *this) {
			if (it->first->getType()->hasSkillClass(sc)) {
				return it->first;
			}
		}
		return false;
	}

	Unit* getNearestHpRatio(fixed hpRatio) {
		foreach(Targets, it, *this) {
			if (it->first->getHpRatioFixed() < hpRatio) {
				return it->first;
			}
		}
		return false;
	}
};

//if the unit has any enemy on range
/** rangedPtr should point to a pointer that is either NULL or a valid Unit */
bool UnitUpdater::unitOnRange(const Unit *unit, int range, Unit **rangedPtr, 
					const AttackSkillTypes *asts, const AttackSkillType **past) {
	///@todo convert to fixed point
	//Vec2f floatCenter = unit->getFloatCenteredPos();
	fixedVec2 fixedCentre = unit->getFixedCenteredPos();
	fixed halfSize = unit->getType()->getHalfSize();
	fixed distance;
	bool needDistance = false;

	if (*rangedPtr && ((*rangedPtr)->isDead() || !asts->getZone((*rangedPtr)->getCurrZone()))) {
		*rangedPtr = NULL;
	}
	if (*rangedPtr) {
		needDistance = true;
	} else {
		Targets enemies;
		Vec2i pos;
		PosCircularIteratorOrdered pci(map->getBounds(), unit->getPos(), 
			world->getPosIteratorFactory().getInsideOutIterator(1, range + halfSize.intp()));

		while (pci.getNext(pos, distance)) {

			//all zones
			foreach_enum (Zone, z) {
				//check zone
				if (!asts || asts->getZone(z)) {
					Unit *possibleEnemy= map->getCell(pos)->getUnit(z);

					//check enemy
					if (possibleEnemy && possibleEnemy->isAlive() && !unit->isAlly(possibleEnemy)) {
						// If enemy and has an attack command we can short circut this loop now
						if (possibleEnemy->getType()->hasCommandClass(CommandClass::ATTACK)) {
							*rangedPtr = possibleEnemy;
							goto unitOnRange_exitLoop;
						}
						// otherwise, we'll record it and figure out who to slap later.
						enemies.record(possibleEnemy, distance);
					}
				}
			}
		}
	
		if (!enemies.size()) {
			return false;
		}
	
		*rangedPtr = enemies.getNearest();
		needDistance = true;
	}
unitOnRange_exitLoop:
	assert(*rangedPtr);
	
	if(needDistance) {
		const fixed &targetHalfSize = (*rangedPtr)->getType()->getHalfSize();
		distance = fixedCentre.dist((*rangedPtr)->getFixedCenteredPos()) - targetHalfSize;
	}

	// check to see if we like this target.
	if(asts && past) {
		return (bool)(*past = asts->getPreferredAttack(unit, *rangedPtr, (distance - halfSize).intp()));
	}
	return true;
}

//find a unit we can repair
/** rangedPtr should point to a pointer that is either NULL or a valid Unit */
bool UnitUpdater::repairableOnRange(
		const Unit *unit,
		Vec2i centre,
		int centreSize,
		Unit **rangedPtr,
		const RepairCommandType *rct,
		const RepairSkillType *rst,
		int range,
		bool allowSelf,
		bool militaryOnly,
		bool damagedOnly) {
	Targets repairables;

	fixedVec2 fixedCentre(centre.x + centreSize / fixed(2), centre.y + centreSize / fixed(2));
	fixed targetRange = fixed::max_int();
	Unit *target = *rangedPtr;
	range += centreSize / 2;

	if (target && target->isAlive() && target->isDamaged() && rct->isRepairableUnitType(target->getType())) {
		fixed rangeToTarget = fixedCentre.dist(target->getFixedCenteredPos()) - (centreSize + target->getSize()) / fixed(2);
		if (rangeToTarget <= range) {
			// current target is good
			return true;
		} else {
			return false;
		}
	}
	target = NULL;

	//nearby cells
	Vec2i pos;
	fixed distance;
	PosCircularIteratorSimple pci(map->getBounds(), centre, range);
	while (pci.getNext(pos, distance)) {
		//all zones
		for (int z = 0; z < Zone::COUNT; z++) {
			Unit *candidate = map->getCell(pos)->getUnit(enum_cast<Field>(z));
	
			//is it a repairable?
			if (candidate && (allowSelf || candidate != unit)
			&& (!rst->isSelfOnly() || candidate == unit)
			&& candidate->isAlive() && unit->isAlly(candidate)
			&& (!rst->isPetOnly() || unit->isPet(candidate))
			&& (!damagedOnly || candidate->isDamaged())
			&& (!militaryOnly || candidate->getType()->hasCommandClass(CommandClass::ATTACK))
			&& rct->isRepairableUnitType(candidate->getType())) {
				//record the nearest distance to target (target may be on multiple cells)
				repairables.record(candidate, distance.intp());
			}
		}
	}
	// if no repairables or just one then it's a simple choice.
	if (repairables.empty()) {
		return false;
	} else if (repairables.size() == 1) {
		*rangedPtr = repairables.begin()->first;
		return true;
	}
	//heal cloesest ally that can attack (and are probably fighting) first.
	//if none, go for units that are less than 20%
	//otherwise, take the nearest repairable unit
	if(!(*rangedPtr = repairables.getNearestSkillClass(SkillClass::ATTACK))
	&& !(*rangedPtr = repairables.getNearestHpRatio(fixed(2) / 10))
	&& !(*rangedPtr = repairables.getNearest())) {
		return false;
	}
	return true;
}

// =====================================================
//	class ParticleDamager
// =====================================================

ParticleDamager::ParticleDamager(Unit *attacker, Unit *target, UnitUpdater *unitUpdater, const GameCamera *gameCamera) {
	this->gameCamera = gameCamera;
	this->attackerRef = attacker;
	this->targetRef = target;
	this->ast = static_cast<const AttackSkillType*>(attacker->getCurrSkill());
	this->targetPos = attacker->getTargetPos();
	this->targetField = attacker->getTargetField();
	this->unitUpdater = unitUpdater;
}

void ParticleDamager::execute(ParticleSystem *particleSystem) {
	Unit *attacker = attackerRef.getUnit();

	if (attacker) {
		Unit *target = targetRef.getUnit();
		if (target) {
			targetPos = target->getCenteredPos();
			// manually feed the attacked unit here to avoid problems with cell maps and such
			unitUpdater->hit(attacker, ast, targetPos, targetField, target);
		} else {
			unitUpdater->hit(attacker, ast, targetPos, targetField, NULL);
		}

		//play sound
		StaticSound *projSound = ast->getProjSound();
		if (particleSystem->getVisible() && projSound) {
			SoundRenderer::getInstance().playFx(
				projSound, Vec3f(float(targetPos.x), 0.f, float(targetPos.y)), gameCamera->getPos());
		}
	}
}

}}//end namespace
