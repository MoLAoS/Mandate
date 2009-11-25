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

#include <algorithm>
#include <cassert>
#include <map>

#include "sound.h"
#include "upgrade.h"
#include "unit.h"
#include "particle_type.h"
#include "core_data.h"
#include "config.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "game.h"
#include "path_finder.h"
#include "object.h"
#include "faction.h"
#include "network_manager.h"

#include "leak_dumper.h"

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
		unit->setCurrSkill(scStop);
		unit->getFaction()->deApplyCosts(pt);
		return false;
	}
}

// =====================================================
//  class UnitUpdater
// =====================================================

// ===================== PUBLIC ========================
const float UnitUpdater::repairerToFriendlySearchRadius = 1.25f;

UnitUpdater::UnitUpdater(Game &game)
		: gameCamera(game.getGameCamera())
		, gui(*(game.getGui()))
		, world(*(game.getWorld()))
		, map(NULL)
		, console(*(game.getConsole()))
		, scriptManager(NULL)
		, pathFinder(NULL)
		, random() // note, should be initialized identically on all nodes of a network game!
		, gameSettings(game.getGameSettings()) {
}

void UnitUpdater::init(Game &game) {
	map = world.getMap();
	pathFinder = Search::PathFinder::getInstance();
	pathFinder->init(map);
}

// ==================== progress skills ====================

//skill dependent actions
void UnitUpdater::updateUnit(Unit *unit) {
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();

	//play skill sound
	const SkillType *currSkill = unit->getCurrSkill();
	if (currSkill->getSound() != NULL) {
		float soundStartTime = currSkill->getSoundStartTime();
		if (soundStartTime >= unit->getLastAnimProgress() && soundStartTime < unit->getAnimProgress()) {
			if (map->getTile(Map::toTileCoords(unit->getPos()))->isVisible(world.getThisTeamIndex())) {
				soundRenderer.playFx(currSkill->getSound(), unit->getCurrVector(), gameCamera->getPos());
			}
		}
	}

	//start attack particle system
	if (unit->getCurrSkill()->getClass() == scAttack) {
		const AttackSkillType *ast = static_cast<const AttackSkillType*>(unit->getCurrSkill());
		float attackStartTime = ast->getStartTime();
		if (attackStartTime >= unit->getLastAnimProgress() && attackStartTime < unit->getAnimProgress()) {
			startAttackSystems(unit, ast);
		}
	}

	//update emanations every 8 frames
	if (unit->getGetEmanations().size() && !((world.getFrameCount() + unit->getId()) % 8)
			&& unit->isOperative()) {
		updateEmanations(unit);
	}

	//update unit
	if (unit->update()) {
		// make sure attack systems are started even on laggy computers
		/* this is causing double attacks for some reason
		if (unit->getCurrSkill()->getClass() == scAttack) {
			const AttackSkillType *ast = static_cast<const AttackSkillType*>(unit->getCurrSkill());
			if (ast->getStartTime() < unit->getLastAnimProgress()) {
				startAttackSystems(unit, ast);
			}
		}*/

		const UnitType *ut = unit->getType();

		if (unit->getCurrSkill()->getClass() == scFallDown) {
			assert(ut->getFirstStOfClass(scGetUp));
			unit->setCurrSkill(scGetUp);
		} else if (unit->getCurrSkill()->getClass() == scGetUp) {
			unit->setCurrSkill(scStop);
		}

		updateUnitCommand(unit);

		//if unit is out of EP, it stops
		if (unit->computeEp()) {
			if (unit->getCurrCommand()) {
				unit->cancelCurrCommand();
			}
			unit->setCurrSkill(scStop);
		}

		//move unit in cells
		if (unit->getCurrSkill()->getClass() == scMove) {
			world.moveUnitCells(unit);

			//play water sound
			if (map->getCell(unit->getPos())->getHeight() < map->getWaterLevel() && unit->getCurrField() == FieldWalkable) {
				soundRenderer.playFx(CoreData::getInstance().getWaterSound());
			}
		}
	}

	//unit death
	if (unit->isDead() && unit->getCurrSkill()->getClass() != scDie) {
		unit->kill();
	}
	map->assertUnitCells(unit);
}

// ==================== progress commands ====================

//VERY IMPORTANT: compute next state depending on the first order of the list
void UnitUpdater::updateUnitCommand(Unit *unit) {
	const SkillType *st = unit->getCurrSkill();

	//commands aren't updated for these skills
	switch (st->getClass()) {
		case scWaitForServer:
		case scFallDown:
		case scGetUp:
			return;

		default:
			break;
	}
	// check if a command being 'watched' has finished
	if ( unit->getCommandCallback() && unit->getCommandCallback() != unit->getCurrCommand() ) {
		// Trigger Time...
		ScriptManager::commandCallback(unit);
	}
	//if unit has command process it
	if (unit->anyCommand()) {
		unit->getCurrCommand()->getType()->update(this, unit);
	}

	//if no commands stop and add stop command or guard command for pets
	if (!unit->anyCommand() && unit->isOperative()) {
		const UnitType *ut = unit->getType();
		unit->setCurrSkill(scStop);
		if (unit->getMaster() && ut->hasCommandClass(ccGuard)) {
			unit->giveCommand(new Command(ut->getFirstCtOfClass(ccGuard), CommandFlags(cpAuto), unit->getMaster()));
		} else {
			if (ut->hasCommandClass(ccStop)) {
				unit->giveCommand(new Command(ut->getFirstCtOfClass(ccStop), CommandFlags()));
			}
		}
	}
}

// ==================== updateStop ====================


Command *UnitUpdater::doAutoAttack(Unit *unit) {
	if (unit->getType()->hasCommandClass(ccAttack) || unit->getType()->hasCommandClass(ccAttackStopped)) {

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
				case ccAttack:
					asts = ((const AttackCommandType*)ct)->getAttackSkillTypes();
					break;

				case ccAttackStopped:
					asts = ((const AttackStoppedCommandType*)ct)->getAttackSkillTypes();
					break;

				default:
					break;
			}

			//use it to attack
			if (asts) {
				if (attackableOnSight(unit, &sighted, asts, NULL)) {
					Command *newCommand = new Command(ct, CommandFlags(cpAuto), sighted->getPos());
					newCommand->setPos2(unit->getPos());
					return newCommand;
				}
			}
		}
	}

	return NULL;
}


Command *UnitUpdater::doAutoRepair(Unit *unit) {
	if (unit->getType()->hasCommandClass(ccRepair) && unit->isAutoRepairEnabled()) {

		for (int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
			const CommandType *ct = unit->getType()->getCommandType(i);

			if (!unit->getFaction()->isAvailable(ct) || ct->getClass() != ccRepair) {
				continue;
			}

			//look a repair skill
			const RepairCommandType *rct = (const RepairCommandType*)ct;
			const RepairSkillType *rst = rct->getRepairSkillType();
			Unit *sighted = NULL;

			if (unit->getEp() >= rst->getEpCost() && repairableOnSight(unit, &sighted, rct, rst->isSelfAllowed())) {
				Command *newCommand;
				newCommand = new Command(rct, CommandFlags(cpQueue, cpAuto),
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
	if (unit->getType()->hasCommandClass(ccMove) && attackerOnSight(unit, &sighted)) {
		//if there is a friendly military unit that we can heal/repair and is
		//rougly between us, then be brave
		if (unit->getType()->hasCommandClass(ccRepair)) {
			Vec2f myCenter = unit->getFloatCenteredPos();
			Vec2f signtedCenter = sighted->getFloatCenteredPos();
			Vec2f fcenter = (myCenter + signtedCenter) / 2.f;
			Unit *myHero = NULL;

			// calculate the real distance to hostile by subtracting half of the size of each.
			float actualDistance = myCenter.dist(signtedCenter)
					- (unit->getType()->getSize() + sighted->getType()->getSize()) / 2.f;

			// allow friendly combat unit to be within a radius of 65% of the actual distance to
			// hostile, starting from the half way intersection of the repairer and hostile.
			int searchRadius = (int)roundf(actualDistance * repairerToFriendlySearchRadius / 2.f);
			Vec2i center((int)roundf(fcenter.x), (int)roundf(fcenter.y));

			//try all of our repair commands
			for (int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
				const CommandType *ct = unit->getType()->getCommandType(i);
				if (ct->getClass() != ccRepair) {
					continue;
				}
				const RepairCommandType *rct = (const RepairCommandType*)ct;
				const RepairSkillType *rst = rct->getRepairSkillType();

				if (repairableOnRange(unit, center, 1, &myHero, rct, rst, searchRadius, false, true, false)) {
					return NULL;
				}
			}
		}
		Vec2i escapePos = unit->getPos() * 2 - sighted->getPos();
		return new Command(unit->getType()->getFirstCtOfClass(ccMove), CommandFlags(cpAuto), escapePos);
	}
	return NULL;
}

void UnitUpdater::updateStop(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const StopCommandType *sct = static_cast<const StopCommandType*>(command->getType());
	Unit *sighted = NULL;
	Command *autoCmd;

	// if we have another command then stop sitting on your ass
	if (unit->getCommands().size() > 1 && unit->getCommands().front()->getType()->getClass() == ccStop) {
		unit->finishCommand();
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

	switch (pathFinder->findPath(unit, pos)) {
		case Search::tsOnTheWay:
			unit->setCurrSkill(mct->getMoveSkillType());
			unit->face(unit->getNextPos());
			break;

		case Search::tsBlocked:
			if (unit->getPath()->isBlocked() && !command->getUnit()) {
				unit->finishCommand();
			}
			break;

		default:
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

	//if found
	if (attackableOnRange(unit, &target, act->getAttackSkillTypes(), &ast)) {
		assert(ast);
		if (unit->getEp() >= ast->getEpCost()) {
			unit->setCurrSkill(ast);
			unit->setTarget(target, true, true);
		} else {
			unit->setCurrSkill(scStop);
		}
	} else {
		//compute target pos
		Vec2i pos;
		if (attackableOnSight(unit, &target, asts, NULL)) {
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
		switch (pathFinder->findPath(unit, pos)) {
			case Search::tsOnTheWay:
				unit->setCurrSkill(act->getMoveSkillType());
				unit->face(unit->getNextPos());
				break;
			case Search::tsBlocked:
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

void UnitUpdater::updateAttack(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const AttackCommandType *act = static_cast<const AttackCommandType*>(command->getType());
	Unit *target = command->getUnit();

	if (updateAttackGeneric(unit, command, act, target, command->getPos())) {
		unit->finishCommand();
	}
}


// ==================== updateAttackStopped ====================

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

void UnitUpdater::updateBuild(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const BuildCommandType *bct = static_cast<const BuildCommandType*>(command->getType());
	const UnitType *builtUnitType = command->getUnitType();
	Unit *builtUnit = NULL;
	Unit *target = unit->getTarget();

	assert(command->getUnitType());

	if (unit->getCurrSkill()->getClass() != scBuild) {
		//if not building

		int buildingSize = builtUnitType->getSize();
		Vec2i waypoint;

		// find the nearest place for the builder
		if (map->getNearestAdjacentFreePos(waypoint, unit, command->getPos(), FieldWalkable, buildingSize)) {
			if (waypoint != unit->getTargetPos()) {
				unit->setTargetPos(waypoint);
				unit->getPath()->clear();
			}
		} else {
			console.addStdMessage("Blocked");
			unit->cancelCurrCommand();
			return;
		}

		switch (pathFinder->findPath(unit, waypoint)) {
			case Search::tsOnTheWay:
				unit->setCurrSkill(bct->getMoveSkillType());
				unit->face(unit->getNextPos());
				return;

			case Search::tsBlocked:
				if (unit->getPath()->isBlocked()) {
					console.addStdMessage("Blocked");
					unit->cancelCurrCommand();
				}
				return;

			case Search::tsArrived:
				if (unit->getPos() != waypoint) {
					console.addStdMessage("Blocked");
					unit->cancelCurrCommand();
					return;
				}
				// otherwise, we fall through
		}

		//if arrived destination
		assert(command->getUnitType() != NULL);
		if (map->areFreeCells(command->getPos(), buildingSize, FieldWalkable)) {
			if (!verifySubfaction(unit, builtUnitType)) {
				return;
			}

			// network client has to wait for the server to tell them to begin building.  If the
			// creates the building, we can have an id mismatch.
			if (isNetworkClient()) {
				unit->setCurrSkill(scWaitForServer);
				// FIXME: Might play start sound multiple times or never at all
			} else {
				// late resource allocation
				if (!command->isReserveResources()) {
					command->setReserveResources(true);
					if (unit->checkCommand(*command) != crSuccess) {
						if (unit->getFactionIndex() == world.getThisFactionIndex()) {
							console.addStdMessage("BuildingNoRes");
						}
						unit->finishCommand();
						return;
					}
					unit->getFaction()->applyCosts(command->getUnitType());
				}

				builtUnit = new Unit(world.getNextUnitId(), command->getPos(), builtUnitType, unit->getFaction(), world.getMap());
				builtUnit->create();

				if (!builtUnitType->hasSkillClass(scBeBuilt)) {
					throw runtime_error("Unit " + builtUnitType->getName() + " has no be_built skill");
				}

				builtUnit->setCurrSkill(scBeBuilt);
				unit->setCurrSkill(bct->getBuildSkillType());
				unit->setTarget(builtUnit, true, true);
				map->prepareTerrain(builtUnit);
				command->setUnit(builtUnit);
				unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), false);
				if (isNetworkGame()) {
					getServerInterface()->newUnit(builtUnit);
					getServerInterface()->unitUpdate(unit);
					getServerInterface()->updateFactions();
				}
			}
			if (!builtUnit->isMobile()) {
				pathFinder->updateMapMetrics(builtUnit->getPos(), builtUnit->getSize());
			}
			//play start sound
			if (unit->getFactionIndex() == world.getThisFactionIndex()) {
				SoundRenderer::getInstance().playFx(bct->getStartSound(), unit->getCurrVector(), gameCamera->getPos());
			}
		} else {
			// there are no free cells
			vector<Unit *>occupants;
			map->getOccupants(occupants, command->getPos(), buildingSize, ZoneSurface);

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
							i != occupants.end() && (*i)->getType()->hasSkillClass(scMove); ++i) ;
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
				unit->setCurrSkill(scStop);
				if (unit->getFactionIndex() == world.getThisFactionIndex()) {
					console.addStdMessage("BuildingNoPlace");
				}
			}
		}
	} else {
		//if building
		Unit *builtUnit = command->getUnit();

		if (builtUnit && builtUnit->getType() != builtUnitType) {
			unit->setCurrSkill(scStop);
		} else if (!builtUnit || builtUnit->isBuilt()) {
			unit->finishCommand();
			unit->setCurrSkill(scStop);
		} else if (builtUnit->repair()) {
			//building finished
			unit->finishCommand();
			unit->setCurrSkill(scStop);
			unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), true);
			scriptManager->onUnitCreated(builtUnit);
			if (unit->getFactionIndex() == world.getThisFactionIndex()) {
				SoundRenderer::getInstance().playFx(
					bct->getBuiltSound(),
					unit->getCurrVector(),
					gameCamera->getPos());
			}
			if (isNetworkServer()) {
				getServerInterface()->unitUpdate(unit);
				getServerInterface()->unitUpdate(builtUnit);
				getServerInterface()->updateFactions();
			}
		}
	}
}


// ==================== updateHarvest ====================

void UnitUpdater::updateHarvest(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const HarvestCommandType *hct = static_cast<const HarvestCommandType*>(command->getType());
	Vec2i targetPos;

	if (unit->getCurrSkill()->getClass() != scHarvest) {
		//if not working
		if (!unit->getLoadCount()) {
			//if not loaded go for resources
			Resource *r = map->getTile(Map::toTileCoords(command->getPos()))->getResource();
			if (r && hct->canHarvest(r->getType())) {
				//if can harvest dest. pos
				if (unit->getPos().dist(command->getPos()) < harvestDistance
						&&  map->isResourceNear(unit->getPos(), r->getType(), targetPos)) {
					//if it finds resources it starts harvesting
					unit->setCurrSkill(hct->getHarvestSkillType());
					unit->setTargetPos(targetPos);
					unit->face(targetPos);
					unit->setLoadCount(0);
					unit->setLoadType(map->getTile(Map::toTileCoords(targetPos))->getResource()->getType());
				} else { //if not continue walking
					switch (pathFinder->findPathToResource(unit, command->getPos(), r->getType())) {
						case Search::tsOnTheWay:
							unit->setCurrSkill(hct->getMoveSkillType());
							unit->face(unit->getNextPos());
							break;
						case Search::tsArrived:
							for (int i = 0; i < 8; ++i) { // reset target
								Vec2i cPos = unit->getPos() + Search::OffsetsSize1Dist1[i];
								Resource *res = map->getTile(Map::toTileCoords(cPos))->getResource();
								if (res && hct->canHarvest(res->getType())) {
									command->setPos(cPos);
									break;
								}
							}
							//command->setPos (
						default:
							break;
					}
				}
			} else {
				//if can't harvest, search for another resource
				unit->setCurrSkill(scStop);
				if (!searchForResource(unit, hct)) {
					unit->finishCommand();
					//FIXME don't just stand around at the store!!
					//insert move command here.
				}
			}
		} else {
			//if loaded, return to store
			Unit *store = world.nearestStore(unit->getPos(), unit->getFaction()->getIndex(), unit->getLoadType());
			if (store) {
				switch (pathFinder->findPathToStore(unit, store->getNearestOccupiedCell(unit->getPos()), store)) {
					case Search::tsOnTheWay:
						unit->setCurrSkill(hct->getMoveLoadedSkillType());
						unit->face(unit->getNextPos());
						break;
					default:
						break;
				}

				//world.changePosCells(unit,unit->getPos()+unit->getDest());
				if (map->isNextTo(unit->getPos(), store)) {

					//update resources
					int resourceAmount = unit->getLoadCount();
					//
					// Just do this all players ???
					if (unit->getFaction()->getCpuUltraControl()) {
						resourceAmount = (int)(resourceAmount * gameSettings.getResourceMultilpier(unit->getFactionIndex()));
						//resourceAmount *= ultraResourceFactor; // Pull from GameSettings
					}
					unit->getFaction()->incResourceAmount(unit->getLoadType(), resourceAmount);
					world.getStats().harvest(unit->getFactionIndex(), resourceAmount);
					scriptManager->onResourceHarvested();

					//if next to a store unload resources
					unit->getPath()->clear();
					unit->setCurrSkill(scStop);
					unit->setLoadCount(0);
					if (isNetworkServer()) {
						// FIXME: wasteful full update here
						getServerInterface()->unitUpdate(unit);
						getServerInterface()->updateFactions();
					}
				}
			} else {
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
					// let the pathfinder know
					Vec2i rPos = r->getPos();
					sc->deleteResource();
					pathFinder->updateMapMetrics(rPos, 2);
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
				unit->setCurrSkill(scStop);
			}
		}
	}
}

// ==================== updateRepair ====================

void UnitUpdater::updateRepair(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const CommandType *ct = command->getType();
	assert(ct->getClass() == ccRepair);

	const RepairCommandType *rct = static_cast<const RepairCommandType*>(ct);
	const RepairSkillType *rst = rct->getRepairSkillType();
	bool repairThisFrame = unit->getCurrSkill()->getClass() == scRepair;
	Unit *repaired = command->getUnit();

	// If the unit I was supposed to repair died or is already fixed then finish
	if (repaired && (repaired->isDead() || !repaired->isDamaged())) {
		unit->setCurrSkill(scStop);
		unit->finishCommand();
		return;
	}

	if (command->isAuto() && unit->getType()->hasCommandClass(ccAttack)) {
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
				unit->setCurrSkill(scStop);
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

		switch (pathFinder->findPath(unit, targetPos)) {
			case Search::tsArrived:
				if (repaired && unit->getPos() != targetPos) {
					// presume blocked
					unit->setCurrSkill(scStop);
					unit->finishCommand();
					break;
				}
				if (repaired) {
					unit->setCurrSkill(rst);
				} else {
					unit->setCurrSkill(scStop);
					unit->finishCommand();
				}
				break;

			case Search::tsOnTheWay:
				unit->setCurrSkill(rct->getMoveSkillType());
				unit->face(unit->getNextPos());
				break;

			case Search::tsBlocked:
				if (unit->getPath()->isBlocked()) {
					unit->setCurrSkill(scStop);
					unit->finishCommand();
				}
				break;
		}
	}

	if (repaired && !repaired->isDamaged()) {
		unit->setCurrSkill(scStop);
		unit->finishCommand();
	}

	if (repairThisFrame && unit->getCurrSkill()->getClass() == scRepair) {
		//if repairing
		if (repaired) {
			unit->setTarget(repaired, true, true);
		}

		if (!repaired) {
			unit->setCurrSkill(scStop);
		} else {
			//shiney
			if (rst->getSplashParticleSystemType()) {
				const Tile *sc = map->getTile(Map::toTileCoords(repaired->getCenteredPos()));
				bool visible = sc->isVisible(world.getThisTeamIndex());

				SplashParticleSystem *psSplash = rst->getSplashParticleSystemType()->createSplashParticleSystem();
				psSplash->setPos(repaired->getCurrVector());
				psSplash->setVisible(visible);
				Renderer::getInstance().manageParticleSystem(psSplash, rsGame);
			}

			bool wasBuilt = repaired->isBuilt();

			assert(repaired->isAlive() && repaired->getHp() > 0);

			if (repaired->repair(rst->getAmount(), rst->getMultiplier())) {
				unit->setCurrSkill(scStop);
				if (!wasBuilt) {
					//building finished
					scriptManager->onUnitCreated(repaired);
					if (unit->getFactionIndex() == world.getThisFactionIndex()) {
						// try to find finish build sound
						BuildCommandType *bct = (BuildCommandType *)unit->getType()->getFirstCtOfClass(ccBuild);
						if (bct) {
							SoundRenderer::getInstance().playFx(
								bct->getBuiltSound(),
								unit->getCurrVector(),
								gameCamera->getPos());
						}
					}
					if (isNetworkServer()) {
						getServerInterface()->unitUpdate(unit);
						getServerInterface()->unitUpdate(repaired);
						getServerInterface()->updateFactions();
					}
				}
			}
		}
	}
}


// ==================== updateProduce ====================

void UnitUpdater::updateProduce(Unit *unit) {

	Command *command = unit->getCurrCommand();
	const ProduceCommandType *pct = static_cast<const ProduceCommandType*>(command->getType());
	Unit *produced;

	if (unit->getCurrSkill()->getClass() != scProduce) {
		//if not producing
		if (!verifySubfaction(unit, pct->getProducedUnit())) {
			return;
		}

		unit->setCurrSkill(pct->getProduceSkillType());
		unit->getFaction()->checkAdvanceSubfaction(pct->getProducedUnit(), false);
	} else {
		unit->update2();

		if (unit->getProgress2() > pct->getProduced()->getProductionTime()) {
			if (isNetworkClient()) {
				// client predict, presume the server will send us the unit soon.
				unit->finishCommand();
				unit->setCurrSkill(scStop);
				return;
			}

			produced = new Unit(world.getNextUnitId(), Vec2i(0), pct->getProducedUnit(), unit->getFaction(), world.getMap());

			//if no longer a valid subfaction, let them have the unit, but make
			//sure it doesn't advance the subfaction

			if (verifySubfaction(unit, pct->getProducedUnit())) {
				unit->getFaction()->checkAdvanceSubfaction(pct->getProducedUnit(), true);
			}

			//place unit creates the unit
			if (!world.placeUnit(unit->getCenteredPos(), 10, produced)) {
				unit->cancelCurrCommand();
				delete produced;
			} else {
				produced->create();
				produced->born();
				scriptManager->onUnitCreated(produced);
				world.getStats().produce(unit->getFactionIndex());
				const CommandType *ct = produced->computeCommandType(unit->getMeetingPos());

				if (ct) {
					produced->giveCommand(new Command(ct, CommandFlags(), unit->getMeetingPos()));
				}

				if (pct->getProduceSkillType()->isPet()) {
					unit->addPet(produced);
					produced->setMaster(unit);
				}

				unit->finishCommand();

				if (isNetworkServer()) {
					getServerInterface()->newUnit(produced);
				}
			}

			unit->setCurrSkill(scStop);
		}
	}
}

// ==================== updateUpgrade ====================

void UnitUpdater::updateUpgrade(Unit *unit) {

	Command *command = unit->getCurrCommand();
	const UpgradeCommandType *uct = static_cast<const UpgradeCommandType*>(command->getType());

	//if subfaction becomes invalid while updating this command, then cancel it.

	if (!verifySubfaction(unit, uct->getProduced())) {
		unit->cancelCommand();
		unit->setCurrSkill(scStop);
		return;
	}

	if (unit->getCurrSkill()->getClass() != scUpgrade) {
		//if not producing
		unit->setCurrSkill(uct->getUpgradeSkillType());
		unit->getFaction()->checkAdvanceSubfaction(uct->getProducedUpgrade(), false);
	} else {
		//if producing
		if (unit->getProgress2() < uct->getProduced()->getProductionTime()) {
			unit->update2();
		}

		if (unit->getProgress2() >= uct->getProduced()->getProductionTime()) {
			if (isNetworkClient()) {
				// clients will wait for the server to tell them that the upgrade is finished
				return;
			}
			unit->finishCommand();
			unit->setCurrSkill(scStop);
			unit->getFaction()->finishUpgrade(uct->getProducedUpgrade());
			unit->getFaction()->checkAdvanceSubfaction(uct->getProducedUpgrade(), true);
			if (isNetworkServer()) {
				getServerInterface()->unitUpdate(unit);
				getServerInterface()->updateFactions();
			}
		}
	}
}


// ==================== updateMorph ====================

void UnitUpdater::updateMorph(Unit *unit) {

	Command *command = unit->getCurrCommand();
	const MorphCommandType *mct = static_cast<const MorphCommandType*>(command->getType());

	//if subfaction becomes invalid while updating this command, then cancel it.
	if (!verifySubfaction(unit, mct->getMorphUnit())) {
		return;
	}

	if (unit->getCurrSkill()->getClass() != scMorph) {
		//if not morphing, check space
		bool gotSpace = false;
		// redo field
		Fields mfs = mct->getMorphUnit()->getFields();

		Field mf;
		if (mfs.get(FieldWalkable)){
			mf = FieldWalkable;
		} else if (mfs.get(FieldAir)) {
			mf = FieldAir;
		}
		if (mfs.get(FieldAmphibious)) {
			mf = FieldAmphibious;
		} else if (mfs.get(FieldAnyWater)) {
			mf = FieldAnyWater;
		} else if (mfs.get(FieldDeepWater)) {
			mf = FieldDeepWater;
		}

		if (map->areFreeCellsOrHasUnit(unit->getPos(), mct->getMorphUnit()->getSize(), mf, unit)) {
			unit->setCurrSkill(mct->getMorphSkillType());
			unit->getFaction()->checkAdvanceSubfaction(mct->getMorphUnit(), false);
		} else {
			if (unit->getFactionIndex() == world.getThisFactionIndex())
				Game::getInstance()->getConsole()->addStdMessage("InvalidPosition");
			unit->cancelCurrCommand();
		}
	} else {
		unit->update2();
		if (unit->getProgress2() > mct->getProduced()->getProductionTime()) {
			bool mapUpdate = unit->isMobile() != mct->getMorphUnit()->isMobile();
			//finish the command
			if (unit->morph(mct)) {
				unit->finishCommand();
				if (gui.isSelected(unit)) {
					gui.onSelectionChanged();
				}
				scriptManager->onUnitCreated(unit);
				unit->getFaction()->checkAdvanceSubfaction(mct->getMorphUnit(), true);
				if (mapUpdate) {
					pathFinder->updateMapMetrics(unit->getPos(), unit->getSize());
				}
				if (isNetworkServer()) {
					getServerInterface()->unitMorph(unit);
					getServerInterface()->updateFactions();
				}
			} else {
				unit->cancelCurrCommand();
				if (unit->getFactionIndex() == world.getThisFactionIndex()) {
					console.addStdMessage("InvalidPosition");
				}
			}
			unit->setCurrSkill(scStop);
		}
	}
}

// ==================== updateCastSpell ====================

void UnitUpdater::updateCastSpell(Unit *unit) {
	//surprise! it never got implemented
}

// ==================== updateGuard ====================

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
		unit->setCurrSkill(scStop);
	}
}

// ==================== updatePatrol ====================

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
		/*
		// can't use minor update here because the command has changed and that wont give the new
		// command
		if (isNetworkGame() && isServer()) {
			getServerInterface()->minorUnitUpdate(unit);
		}*/
	}
}


void UnitUpdater::updateEmanations(Unit *unit) {
	// This is a little hokey, but probably the best way to reduce redundant code
	static EffectTypes singleEmanation;
	for (Emanations::const_iterator i = unit->getGetEmanations().begin();
			i != unit->getGetEmanations().end(); i++) {
		singleEmanation.resize(1);
		singleEmanation[0] = *i;
		applyEffects(unit, singleEmanation, unit->getPos(), FieldWalkable, (*i)->getRadius());
		applyEffects(unit, singleEmanation, unit->getPos(), FieldAir, (*i)->getRadius());
	}
}

// ==================== PRIVATE ====================

// ==================== attack ====================

void UnitUpdater::hit(Unit *attacker) {
	hit(attacker, static_cast<const AttackSkillType*>(attacker->getCurrSkill()), attacker->getTargetPos(), attacker->getTargetField());
}

void UnitUpdater::hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField, Unit *attacked) {

	//hit attack positions
	if (ast->getSplash() && ast->getSplashRadius()) {
		std::map<Unit*, float> hitList;
		std::map<Unit*, float>::iterator i;
		Vec2i pos;
		float distance;

		PosCircularIteratorSimple pci(*map, targetPos, ast->getSplashRadius());
		while (pci.getNext(pos, distance)) {
			attacked = map->getCell(pos)->getUnit(targetField);
			if (attacked && (distance == 0  || ast->getSplashDamageAll() || !attacker->isAlly(attacked))) {
				//float distance = pci.getPos().dist(attacker->getTargetPos());
				damage(attacker, ast, attacked, distance);

				// Remember all units we hit with the closest distance. We only
				// want to hit them with effects once.
				if (ast->isHasEffects()) {
					i = hitList.find(attacked);
					if (i == hitList.end() || i->second > distance) {
						hitList[attacked] = distance;
					}
				}
			}
		}

		if (ast->isHasEffects()) {
			for (i = hitList.begin(); i != hitList.end(); i++) {
				applyEffects(attacker, ast->getEffectTypes(), i->first, i->second);
			}
		}
	} else {
		if (!attacked) {
			attacked = map->getCell(targetPos)->getUnit(targetField);
		}

		if (attacked) {
			damage(attacker, ast, attacked, 0.f);
			if (ast->isHasEffects()) {
				applyEffects(attacker, ast->getEffectTypes(), attacked, 0.f);
			}
		}
	}
}

void UnitUpdater::damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, float distance) {

	if (isNetworkClient()) {
		return;
	}

	//get vars
	float damage = (float)attacker->getAttackStrength(ast);
	int var = ast->getAttackVar();
	int armor = attacked->getArmor();
	float damageMultiplier = world.getTechTree()->getDamageMultiplier(ast->getAttackType(),
							 attacked->getType()->getArmorType());
	int startingHealth = attacked->getHp();
	int actualDamage;

	//compute damage
	damage += random.randRange(-var, var);
	damage /= distance + 1.0f;
	damage -= armor;
	damage *= damageMultiplier;
	if (damage < 1) {
		damage = 1;
	}

	//damage the unit
	if (attacked->decHp(static_cast<int>(damage))) {
		world.doKill(attacker, attacked);
		actualDamage = startingHealth;
	} else
		actualDamage = (int)roundf(damage);

	//add stolen health to attacker
	if (attacker->getAttackPctStolen(ast) || ast->getAttackPctVar()) {
		float pct = attacker->getAttackPctStolen(ast)
					+ random.randRange(-ast->getAttackPctVar(), ast->getAttackPctVar());
		int stolen = (int)roundf((float)actualDamage * pct);
		if (stolen && attacker->doRegen(stolen, 0)) {
			// stealing a negative percentage and dying?
			world.doKill(attacker, attacker);
		}
	}
	if (isNetworkServer()) {
		getServerInterface()->minorUnitUpdate(attacker);
		getServerInterface()->minorUnitUpdate(attacked);
	}

	//complain
	const Vec3f &attackerVec = attacked->getCurrVector();
	if (!gui.isVisible(Vec2i((int)roundf(attackerVec.x), (int)roundf(attackerVec.y)))) {
		attacked->getFaction()->attackNotice(attacked);
	}
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
	bool visible = sc->isVisible(world.getThisTeamIndex()) || tsc->isVisible(world.getThisTeamIndex());

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

		psProj->setPath(startPos, endPos);
		if (pstProj->isTracking() && unit->getTarget()) {
			psProj->setTarget(unit->getTarget());
			psProj->setObserver(new ParticleDamager(unit, unit->getTarget(), this, gameCamera));
		} else {
			psProj->setObserver(new ParticleDamager(unit, NULL, this, gameCamera));
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

	Unit *target;

	if (splashRadius != 0) {
		std::map<Unit*, float> hitList;
		std::map<Unit*, float>::iterator i;
		Vec2i pos;
		float distance;

		PosCircularIteratorSimple pci(*map, targetPos, splashRadius);
		while (pci.getNext(pos, distance)) {
			target = map->getCell(pos)->getUnit(targetField);
			if (target) {
				//float distance = pci.getPos().dist(targetPos);

				// Remember all units we hit with the closest distance. We only
				// want to hit them once.
				i = hitList.find(target);
				if (i == hitList.end() || (*i).second > distance) {
					hitList[target] = distance;
				}
			}
		}

		for (i = hitList.begin(); i != hitList.end(); i++) {
			applyEffects(source, effectTypes, (*i).first, (*i).second);
		}
	} else {
		target = map->getCell(targetPos)->getUnit(targetField);
		if (target) {
			applyEffects(source, effectTypes, target, 0.f);
		}
	}
}

//apply effects to a specific target
void UnitUpdater::applyEffects(Unit *source, const EffectTypes &effectTypes, Unit *target, float distance) {
	//apply effects
	for (EffectTypes::const_iterator i = effectTypes.begin();
			i != effectTypes.end(); i++) {
		// lots of tests, roughly in order of speed of evaluation.
		if(		// ally/foe test
				(source->isAlly(target)
						? (*i)->isEffectsAlly()
						: (*i)->isEffectsFoe()) &&

				// building/normal unit test
				(target->getType()->isOfClass(ucBuilding)
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

			float strength = (*i)->isScaleSplashStrength() ? 1.0f / (float)(distance + 1) : 1.0f;
			Effect *primaryEffect = new Effect((*i), source, NULL, strength, target, world.getTechTree());

			target->add(primaryEffect);

			for (EffectTypes::const_iterator j = (*i)->getRecourse().begin();
					j != (*i)->getRecourse().end(); j++) {
				source->add(new Effect((*j), NULL, primaryEffect, strength, source, world.getTechTree()));
			}
		}
	}
}

void UnitUpdater::appyEffect(Unit *u, Effect *e) {
	if (u->add(e)) {
		Unit *attacker = e->getSource();
		if (attacker) {
			world.getStats().kill(attacker->getFactionIndex(), u->getFactionIndex());
			attacker->incKills();
		} else if (e->getRoot()) {
			// if killed by a recourse effect, this was suicide
			world.getStats().kill(u->getFactionIndex(), u->getFactionIndex());
		}
	}
}

// ==================== misc ====================

/**
 * Finds the nearest position to dest, presuming that dest is the NW corner of
 * a quare destSize cells, that is at least minRange from the target, but no
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

	PosCircularIteratorOrdered pci(*map, unit->getCurrCommand()->getPos(),
								   world.getPosIteratorFactory().getInsideOutIterator(1, maxResSearchRadius));

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

//if the unit has any enemy on range
/** rangedPtr should point to a pointer that is either NULL or a valid Unit */
bool UnitUpdater::unitOnRange(const Unit *unit, int range, Unit **rangedPtr,
							  const AttackSkillTypes *asts, const AttackSkillType **past) {
	Vec2f floatCenter = unit->getFloatCenteredPos();
	float halfSize = (float)unit->getType()->getSize() / 2.f;
	float distance;
	bool needDistance = false;

	if (*rangedPtr && ((*rangedPtr)->isDead()
					   ||  !asts->getZone((*rangedPtr)->getCurrZone())))
		*rangedPtr = NULL;

	if (*rangedPtr) {
		needDistance = true;
	} else {
		Targets enemies;
		Vec2i pos;
		PosCircularIteratorOrdered pci(*map, unit->getPos(), world.getPosIteratorFactory()
									   .getInsideOutIterator(1, (int)roundf(range + halfSize)));

		while (pci.getNext(pos, distance)) {

			//all fields
			for (int k = 0; k < ZoneCount; k++) {
				Zone f = static_cast<Zone>(k);

				//check field
				if (!asts || asts->getZone(f)) {
					Unit *possibleEnemy = map->getCell(pos)->getUnit(f);

					//check enemy
					if (possibleEnemy && possibleEnemy->isAlive() && !unit->isAlly(possibleEnemy)) {
						// If enemy and has an attack command we can short circut this loop now
						if (possibleEnemy->getType()->hasCommandClass(ccAttack)) {
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

	if (needDistance) {
		float targetHalfSize = (float)(*rangedPtr)->getType()->getSize() / 2.f;
		distance = floatCenter.dist((*rangedPtr)->getFloatCenteredPos()) - targetHalfSize;
	}

	// check to see if we like this target.
	if (asts && past) {
		return (bool)(*past = asts->getPreferredAttack(unit, *rangedPtr, distance - halfSize));
	}
	return true;
}

//find a unit we can repair
/** rangedPtr should point to a pointer that is either NULL or a valid Unit */
bool UnitUpdater::repairableOnRange(
	const Unit *unit,
	Vec2i center,
	int centerSize,
	Unit **rangedPtr,
	const RepairCommandType *rct,
	const RepairSkillType *rst,
	int range,
	bool allowSelf,
	bool militaryOnly,
	bool damagedOnly) {

	Targets repairables;

	Vec2f floatCenter(center.x + centerSize / 2.f, center.y + centerSize / 2.f);
	int targetRange = 0x10000;
	Unit *target = *rangedPtr;
	range += centerSize / 2;

	if (target && target->isAlive() && target->isDamaged() && rct->isRepairableUnitType(target->getType())) {
		float rangeToTarget = floatCenter.dist(target->getFloatCenteredPos()) - (float)(centerSize + target->getSize()) / 2.0f;
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
	float distance;
	PosCircularIteratorSimple pci(*map, center, range);
	while (pci.getNext(pos, distance)) {
		//all fields
		for (int f = 0; f < FieldCount; f++) {
			Unit *candidate = map->getCell(pos)->getUnit((Field)f);

			//is it a repairable?
			if (candidate
					&& (allowSelf || candidate != unit)
					&& (!rst->isSelfOnly() || candidate == unit)
					&& candidate->isAlive()
					&& unit->isAlly(candidate)
					&& (!rst->isPetOnly() || unit->isPet(candidate))
					&& (!damagedOnly || candidate->isDamaged())
					&& (!militaryOnly || candidate->getType()->hasCommandClass(ccAttack))
					&& rct->isRepairableUnitType(candidate->getType())) {

				//record the nearest distance to target (target may be on multiple cells)
				repairables.record(candidate, distance);
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
	if (!(*rangedPtr = repairables.getNearest(scAttack))
			&& !(*rangedPtr = repairables.getNearest(scCount, 0.2f))
			&& !(*rangedPtr = repairables.getNearest())) {
		return false;
	}
	return true;
}

// =====================================================
// class ParticleDamager
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

void ParticleDamager::update(ParticleSystem *particleSystem) {
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
			SoundRenderer::getInstance().playFx(projSound, Vec3f(targetPos.x, 0.f, targetPos.y), gameCamera->getPos());
		}
	}
	particleSystem->setObserver(NULL);
	delete this;
}

}
}//end namespace
