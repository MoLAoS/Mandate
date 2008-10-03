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

namespace Glest{ namespace Game{

//FIXME: We check the subfaction in born too.  Should it be removed from there?
//local func to keep players from getting stuff they aren't supposed to.
bool verifySubfaction(Unit *unit, const ProducibleType *pt) {
	if(pt->isAvailableInSubfaction(unit->getFaction()->getSubfaction())) {
		return true;
	} else {
		unit->finishCommand();
		unit->setCurrSkill(scStop);
		unit->getFaction()->deApplyCosts(pt);
		return false;
	}
}

// =====================================================
// 	class UnitUpdater
// =====================================================

// ===================== PUBLIC ========================
const float UnitUpdater::repairerToFriendlySearchRadius = 1.25f;

void UnitUpdater::init(Game &game) {
	this->gui = game.getGui();
	this->gameCamera = game.getGameCamera();
	this->world = game.getWorld();
	this->map = world->getMap();
	this->console = game.getConsole();
	pathFinder.init(map);
}


// ==================== progress skills ====================

//skill dependent actions
void UnitUpdater::updateUnit(Unit *unit){
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	//play skill sound
	const SkillType *currSkill= unit->getCurrSkill();
	if(currSkill->getSound()!=NULL){
		float soundStartTime= currSkill->getSoundStartTime();
		if(soundStartTime>=unit->getLastAnimProgress() && soundStartTime<unit->getAnimProgress()){
			if(map->getSurfaceCell(Map::toSurfCoords(unit->getPos()))->isVisible(world->getThisTeamIndex())){
				soundRenderer.playFx(currSkill->getSound(), unit->getCurrVector(), gameCamera->getPos());
			}
		}
	}

	//start attack particle system
	if(unit->getCurrSkill()->getClass()==scAttack){
		const AttackSkillType *ast= static_cast<const AttackSkillType*>(unit->getCurrSkill());
		float attackStartTime= ast->getStartTime();
		if(attackStartTime>=unit->getLastAnimProgress() && attackStartTime<unit->getAnimProgress()){
			startAttackParticleSystem(unit);
			const EarthquakeType *et = ast->getEarthquakeType();
			if(et) {
				et->spawn(*map, unit, unit->getTargetPos(), 1.f);
				if(et->getSound()) {
					// play rather visible or not
					soundRenderer.playFx(et->getSound(), unit->getTargetVec(), gameCamera->getPos());
				}
				unit->finishCommand();
			}
		}
	}

	if(unit->isOperative()) {
		updateEmanations(unit);
	}

	//update unit
	if(unit->update()) {
		const UnitType *ut = unit->getType();

		if(unit->getCurrSkill()->getClass() == scFallDown) {
			assert(ut->getFirstStOfClass(scGetUp));
			unit->setCurrSkill(scGetUp);
		} else if(unit->getCurrSkill()->getClass() == scGetUp) {
			unit->setCurrSkill(scStop);
		}

		updateUnitCommand(unit);

		//if unit is out of EP, it stops
		// FIXME: This wont work with releasing controlled units when mana runs out.
		if(unit->computeEp()){
			if(unit->getCurrCommand()) {
				unit->cancelCurrCommand();
			}
			unit->setCurrSkill(scStop);
		}

		//move unit in cells
		if(unit->getCurrSkill()->getClass()==scMove){
			world->moveUnitCells(unit);

			//play water sound
			if(map->getCell(unit->getPos())->getHeight()<map->getWaterLevel() && unit->getCurrField()==fLand){
				soundRenderer.playFx(CoreData::getInstance().getWaterSound());
			}
		}
	}

	//unit death
	if(unit->isDead() && unit->getCurrSkill()->getClass() != scDie){
		unit->kill();
	}
	map->assertUnitCells(unit);
}

// ==================== progress commands ====================

//VERY IMPORTANT: compute next state depending on the first order of the list
void UnitUpdater::updateUnitCommand(Unit *unit) {
	const SkillType *st = unit->getCurrSkill();

	//commands aren't updated for these skills
	switch(st->getClass()) {
		case scWaitForServer:
		case scFallDown:
		case scGetUp:
			return;

		default:
			break;
	}

	//if unis has command process it
	if(unit->anyCommand()) {
		unit->getCurrCommand()->getCommandType()->update(this, unit);
	}

	//if no commands stop and add stop command or guard command for pets
	if(!unit->anyCommand() && unit->isOperative()){
		const UnitType *ut = unit->getType();
		unit->setCurrSkill(scStop);
		if(unit->getMaster() && ut->hasCommandClass(ccGuard)) {
			unit->giveCommand(new Command(ut->getFirstCtOfClass(ccGuard), CommandFlags(cpAuto), unit->getMaster()));
		} else {
			if(ut->hasCommandClass(ccStop)){
				unit->giveCommand(new Command(ut->getFirstCtOfClass(ccStop), CommandFlags()));
			}
		}
	}
}

// ==================== updateStop ====================


Command *UnitUpdater::doAutoAttack(Unit *unit) {
	if(unit->getType()->hasCommandClass(ccAttack) || unit->getType()->hasCommandClass(ccAttackStopped)) {

		for(int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
			const CommandType *ct = unit->getType()->getCommandType(i);

			if(!unit->getFaction()->isAvailable(ct)) {
				continue;
			}

			//look for an attack or repair skill
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
			if(asts) {
				if(attackableOnSight(unit, &sighted, asts, NULL)) {
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
	if(unit->getType()->hasCommandClass(ccRepair) && unit->isAutoRepairEnabled()) {

		for(int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
			const CommandType *ct = unit->getType()->getCommandType(i);

			if(!unit->getFaction()->isAvailable(ct) || ct->getClass() != ccRepair) {
				continue;
			}

			//look a repair skill
			const RepairCommandType *rct = (const RepairCommandType*)ct;
			const RepairSkillType *rst = rct->getRepairSkillType();
			Unit *sighted = NULL;

			if(unit->getEp() >= rst->getEpCost() && repairableOnSight(unit, &sighted, rct, rst->isSelfAllowed())) {
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
	if(unit->getType()->hasCommandClass(ccMove) && attackerOnSight(unit, &sighted)) {
		//if there is a friendly military unit that we can heal/repair and is
		//rougly between us, then be brave
		if(unit->getType()->hasCommandClass(ccRepair)) {
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
				const CommandType *ct= unit->getType()->getCommandType(i);
				if(ct->getClass() != ccRepair) {
					continue;
				}
				const RepairCommandType *rct = (const RepairCommandType*)ct;
				const RepairSkillType *rst = rct->getRepairSkillType();

				if(repairableOnRange(unit, center, 1, &myHero, rct, rst, searchRadius, false, true, false)) {
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

	Command *command= unit->getCurrCommand();
	const StopCommandType *sct = static_cast<const StopCommandType*>(command->getCommandType());
	Unit *sighted = NULL;
	Command *autoCmd;

	// if we have another command then stop sitting on your ass
	if(unit->getCommands().size() > 1 && unit->getCommands().front()->getCommandType()->getClass() == ccStop) {
		unit->finishCommand();
		return;
	}

	unit->setCurrSkill(sct->getStopSkillType());

	//we can attack any unit => attack it
	if((autoCmd = doAutoAttack(unit))) {
		unit->giveCommand(autoCmd);
		return;
	}

	//we can repair any ally => repair it
	if((autoCmd = doAutoRepair(unit))) {
		unit->giveCommand(autoCmd);
		return;
	}

	/*
	if(unit->getType()->hasSkillClass(scRepair) && unit->isAutoRepairEnabled()) {

		for(int i=0; i<unit->getType()->getCommandTypeCount(); ++i){
			const CommandType *ct = unit->getType()->getCommandType(i);
			if(!unit->getFaction()->isAvailable(ct) || ct->getClass() != ccRepair) {
				continue;
			}

			//look a repair skill
			const RepairCommandType *rct = (const RepairCommandType*)ct;;
			const RepairSkillType *rst = rct->getRepairSkillType();
			sighted = NULL;

			if(unit->getEp() >= rst->getEpCost() && repairableOnSight(unit, &sighted, rct, rst->isSelfAllowed())) {
				Command *newCommand;
				newCommand = new Command(rct, CommandFlags(cpQueue, cpAuto),
						Map::getNearestPos(unit->getPos(), sighted, rst->getMinRange(), rst->getMaxRange()));
				newCommand->setPos2(unit->getPos());
				unit->giveCommand(newCommand);
				if(isNetworkServer()) {
					getServerInterface()->unitUpdate(unit);
				}
				return;
			}
		}
	}*/

	//see any unit and cant attack it => run
	if((autoCmd = doAutoFlee(unit))) {
		unit->giveCommand(autoCmd);
	}
/*
	if(unit->getCommands().size() == 1 && unit->getType()->hasCommandClass(ccMove)
			&& attackerOnSight(unit, &sighted)) {

		//if there is a friendly military unit that we can heal/repair and is
		//rougly between us, then be brave
		if(unit->getType()->hasCommandClass(ccRepair)) {
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
				const CommandType *ct= unit->getType()->getCommandType(i);
				if(ct->getClass() != ccRepair) {
					continue;
				}
				const RepairCommandType *rct = (const RepairCommandType*)ct;
				const RepairSkillType *rst = rct->getRepairSkillType();

				if(repairableOnRange(unit, center, 1, &myHero, rct, rst, searchRadius, false, true, false)) {
					return;
				}
			}
		}
		Vec2i escapePos= unit->getPos()*2-sighted->getPos();
		unit->giveCommand(new Command(unit->getType()->getFirstCtOfClass(ccMove), CommandFlags(cpAuto), escapePos));
	}*/
}

// ==================== updateMove ====================

void UnitUpdater::updateMove(Unit *unit){
	Command *command= unit->getCurrCommand();
	const MoveCommandType *mct= static_cast<const MoveCommandType*>(command->getCommandType());
	bool autoCommand = command->isAuto();

	Vec2i pos;
	if(command->getUnit()) {
		pos = command->getUnit()->getCenteredPos();
		if(!command->getUnit()->isAlive()) {
			command->setPos(pos);
			command->setUnit(NULL);
		}
	} else {
		pos = command->getPos();
	}

	switch(pathFinder.findPath(unit, pos)){
	case PathFinder::tsOnTheWay:
		unit->setCurrSkill(mct->getMoveSkillType());
		break;

	case PathFinder::tsBlocked:
		if(unit->getPath()->isBlocked() && !command->getUnit()){
			unit->finishCommand();
		}
		break;

	default:
		unit->finishCommand();
	}

	// if we're doing an auto command, let's make sure we still want to do it
	if(autoCommand) {
		Command *autoCmd;
		//we can attack any unit => attack it
		if((autoCmd = doAutoAttack(unit))) {
			unit->giveCommand(autoCmd);
			return;
		}

		//we can repair any ally => repair it
		if((autoCmd = doAutoRepair(unit))) {
			unit->giveCommand(autoCmd);
			return;
		}

		//see any unit and cant attack it => run
		if((autoCmd = doAutoFlee(unit))) {
			unit->giveCommand(autoCmd);
			return;
		}
	}
}

// ==================== updateGenericAttack ====================

/** Returns true when completed */
bool UnitUpdater::updateAttackGeneric(Unit *unit, Command *command, const AttackCommandType *act, Unit* target, const Vec2i &targetPos){
	const AttackSkillType *ast = NULL;

	//if found
	if(attackableOnRange(unit, &target, act->getAttackSkillTypes(), &ast)){
		assert(ast);
		if(unit->getEp() >= ast->getEpCost()){
			unit->setCurrSkill(ast);
			unit->setTarget(target);
		}
		else{
			unit->setCurrSkill(scStop);
		}
	} else {
		//compute target pos
		Vec2i pos;
		if(attackableOnSight(unit, &target, act->getAttackSkillTypes(), NULL)) {
			pos = target->getCenteredPos();
			if(pos != unit->getTargetPos()) {
				unit->setTargetPos(pos);
				unit->getPath()->clear();
			}
		} else {
			// if no more targets and on auto command, then turn around
			if(command->isAuto() && command->hasPos2()) {
				command->popPos();
				pos = command->getPos();
				unit->getPath()->clear();
			} else {
				pos = targetPos;
			}
		}

		//if unit arrives destPos order has ended
		switch (pathFinder.findPath(unit, pos)){
		case PathFinder::tsOnTheWay:
			unit->setCurrSkill(act->getMoveSkillType());
			break;
		case PathFinder::tsBlocked:
			if(unit->getPath()->isBlocked()){
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

void UnitUpdater::updateAttack(Unit *unit){
	Command *command= unit->getCurrCommand();
	const AttackCommandType *act= static_cast<const AttackCommandType*>(command->getCommandType());
	Unit *target= command->getUnit();

	if(updateAttackGeneric(unit, command, act, target, command->getPos())) {
		unit->finishCommand();
	}
}


// ==================== updateAttackStopped ====================

void UnitUpdater::updateAttackStopped(Unit *unit){

	Command *command= unit->getCurrCommand();
	const AttackStoppedCommandType *asct= static_cast<const AttackStoppedCommandType*>(command->getCommandType());
	Unit *enemy = NULL;
	const AttackSkillType *ast = NULL;

	if(attackableOnRange(unit, &enemy, asct->getAttackSkillTypes(), &ast)){
		assert(ast);
		unit->setCurrSkill(ast);
		unit->setTarget(enemy);
	}
	else{
		unit->setCurrSkill(asct->getStopSkillType());
	}
}


// ==================== updateBuild ====================

void UnitUpdater::updateBuild(Unit *unit){
	Command *command= unit->getCurrCommand();
	const BuildCommandType *bct= static_cast<const BuildCommandType*>(command->getCommandType());
	const UnitType *builtUnitType= command->getUnitType();
	Unit *builtUnit = NULL;
	Unit *target = unit->getTarget();

	assert(command->getUnitType());

	if(unit->getCurrSkill()->getClass() != scBuild) {
		//if not building

		int buildingSize = builtUnitType->getSize();
		Vec2i waypoint;

		// find the nearest place for the builder
		if(map->getNearestAdjacentFreePos(waypoint, unit, command->getPos(), fLand, buildingSize)) {
			if(waypoint != unit->getTargetPos()) {
				unit->setTargetPos(waypoint);
				unit->getPath()->clear();
			}
		} else {
			console->addStdMessage("Blocked");
			unit->cancelCurrCommand();
			return;
		}

		switch (pathFinder.findPath(unit, waypoint)) {
		case PathFinder::tsOnTheWay:
			unit->setCurrSkill(bct->getMoveSkillType());
			return;

		case PathFinder::tsBlocked:
			if(unit->getPath()->isBlocked()) {
				console->addStdMessage("Blocked");
				unit->cancelCurrCommand();
			}
			return;

		case PathFinder::tsArrived:
			if(unit->getPos() != waypoint) {
				console->addStdMessage("Blocked");
				unit->cancelCurrCommand();
				return;
			}
			// otherwise, we fall through
		}

		//if arrived destination
		if(map->isFreeCells(command->getPos(), buildingSize, fLand)) {
			if(!verifySubfaction(unit, builtUnitType)) {
				return;
			}

			// network client has to wait for the server to tell them to begin building.  If the
			// creates the building, we can have an id mismatch.
			if(isNetworkClient()) {
				unit->setCurrSkill(scWaitForServer);
				// FIXME: Might play start sound multiple times or never at all
			} else {
				// late resource allocation
				if(!command->isReserveResources()) {
					command->setReserveResources(true);
					if(unit->checkCommand(command) != crSuccess) {
						if(unit->getFactionIndex() == world->getThisFactionIndex()){
							console->addStdMessage("BuildingNoRes");
						}
						unit->finishCommand();
						return;
					}
					unit->getFaction()->applyCosts(command->getUnitType());
				}

				builtUnit = new Unit(world->getNextUnitId(), command->getPos(), builtUnitType, unit->getFaction(), world->getMap());
				builtUnit->create();

				if(!builtUnitType->hasSkillClass(scBeBuilt)){
					throw runtime_error("Unit " + builtUnitType->getName() + " has no be_built skill");
				}

				builtUnit->setCurrSkill(scBeBuilt);
				unit->setCurrSkill(bct->getBuildSkillType());
				unit->setTarget(builtUnit);
				map->prepareTerrain(builtUnit);
				command->setUnit(builtUnit);
				unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), false);
				if(isNetworkGame()) {
					getServerInterface()->newUnit(builtUnit);
					getServerInterface()->unitUpdate(unit);
					getServerInterface()->updateFactions();
				}
			}

			//play start sound
			if(unit->getFactionIndex()==world->getThisFactionIndex()){
				SoundRenderer::getInstance().playFx(
					bct->getStartSound(),
					unit->getCurrVector(),
					gameCamera->getPos());
			}
		} else {
			// there are no free cells
			vector<Unit *>occupants;
			map->getOccupants(occupants, command->getPos(), buildingSize, fLand);

			// is construction already under way?
			Unit *builtUnit = occupants.size() == 1 ? occupants[0] : NULL;
			if(builtUnit && builtUnit->getType() == builtUnitType && !builtUnit->isBuilt()) {
				// if we pre-reserved the resources, we have to deallocate them now, although
				// this usually shouldn't happen.
				if(command->isReserveResources()) {
					unit->getFaction()->deApplyCosts(command->getUnitType());
					command->setReserveResources(false);
				}
				unit->setTarget(builtUnit);
				unit->setCurrSkill(bct->getBuildSkillType());
				command->setUnit(builtUnit);

			} else {
				// is it not free because there are units in the way?
				if (occupants.size()) {
					// Can they get the fuck out of the way?
					vector<Unit *>::const_iterator i;
					for (i = occupants.begin(); i != occupants.end() && (*i)->getType()->hasSkillClass(scMove); ++i);
					if (i == occupants.end()) {
						// they all have a move command, so we'll wait
						return;
					}
				}

				// blocked by non-moving units, surface objects (trees, rocks, etc.) or build area
				// contains deeply submerged terain
				unit->cancelCurrCommand();
				unit->setCurrSkill(scStop);
				if (unit->getFactionIndex() == world->getThisFactionIndex()) {
					console->addStdMessage("BuildingNoPlace");
				}
			}
		}
	} else {
		//if building
		Unit *builtUnit = command->getUnit();

		if(builtUnit && builtUnit->getType() != builtUnitType) {
			unit->setCurrSkill(scStop);
		} else if(!builtUnit || builtUnit->isBuilt()) {
			unit->finishCommand();
			unit->setCurrSkill(scStop);
		} else if(builtUnit->repair()) {
			//building finished
			unit->finishCommand();
			unit->setCurrSkill(scStop);
			unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), true);
			if(unit->getFactionIndex()==world->getThisFactionIndex()){
				SoundRenderer::getInstance().playFx(
					bct->getBuiltSound(),
					unit->getCurrVector(),
					gameCamera->getPos());
			}
			if(isNetworkServer()) {
				getServerInterface()->unitUpdate(unit);
				getServerInterface()->unitUpdate(builtUnit);
				getServerInterface()->updateFactions();
			}
		}
	}
}


// ==================== updateHarvest ====================

void UnitUpdater::updateHarvest(Unit *unit){
	Command *command= unit->getCurrCommand();
	const HarvestCommandType *hct= static_cast<const HarvestCommandType*>(command->getCommandType());
	Vec2i targetPos;

	if(unit->getCurrSkill()->getClass() != scHarvest) {
		//if not working
		if(!unit->getLoadCount()){
			//if not loaded go for resources
			Resource *r= map->getSurfaceCell(Map::toSurfCoords(command->getPos()))->getResource();
			if(r && hct->canHarvest(r->getType())){
				//if can harvest dest. pos
				if(unit->getPos().dist(command->getPos())<harvestDistance &&
						map->isResourceNear(unit->getPos(), r->getType(), targetPos)) {
					//if it finds resources it starts harvesting
					unit->setCurrSkill(hct->getHarvestSkillType());
					unit->setTargetPos(targetPos);
					unit->setLoadCount(0);
					unit->setLoadType(map->getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()))->getResource()->getType());
				} else {
					//if not continue walking
					switch(pathFinder.findPath(unit, command->getPos())){
					case PathFinder::tsOnTheWay:
						unit->setCurrSkill(hct->getMoveSkillType());
						break;
					default:
						break;
					}
				}
			}
			else{
				//if can't harvest, search for another resource
				unit->setCurrSkill(scStop);
				if(!searchForResource(unit, hct)){
					unit->finishCommand();
				}
			}
		}
		else{
			//if loaded, return to store
			Unit *store= world->nearestStore(unit->getPos(), unit->getFaction()->getIndex(), unit->getLoadType());
			if(store) {
				switch(pathFinder.findPath(unit, store->getCenteredPos())) {
				case PathFinder::tsOnTheWay:
					unit->setCurrSkill(hct->getMoveLoadedSkillType());
					break;
				default:
					break;
				}

				//world->changePosCells(unit,unit->getPos()+unit->getDest());
				if(map->isNextTo(unit->getPos(), store)){

					//update resources
					int resourceAmount= unit->getLoadCount();
					if(unit->getFaction()->getCpuUltraControl()){
						resourceAmount*= ultraResourceFactor;
					}
					unit->getFaction()->incResourceAmount(unit->getLoadType(), resourceAmount);
					world->getStats()->harvest(unit->getFactionIndex(), resourceAmount);

					//if next to a store unload resources
					unit->getPath()->clear();
					unit->setCurrSkill(scStop);
					unit->setLoadCount(0);
					if(isNetworkServer()) {
						// FIXME: wasteful full update here
						getServerInterface()->unitUpdate(unit);
						getServerInterface()->updateFactions();
					}
				}
			}
			else{
				unit->finishCommand();
			}
		}
	}
	else{
		//if working
		SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()));
		Resource *r= sc->getResource();
		if(r!=NULL){
			//if there is a resource, continue working, until loaded
			unit->update2();
			if(unit->getProgress2()>=hct->getHitsPerUnit()){
				unit->setProgress2(0);
				unit->setLoadCount(unit->getLoadCount()+1);

				//if resource exausted, then delete it and stop
				if(r->decAmount(1)){
					sc->deleteResource();
					unit->setCurrSkill(hct->getStopLoadedSkillType());
				}

				if(unit->getLoadCount()==hct->getMaxLoad()){
					unit->setCurrSkill(hct->getStopLoadedSkillType());
					unit->getPath()->clear();
				}
			}
		}
		else{
			//if there is no resource
			if(unit->getLoadCount()) {
				unit->setCurrSkill(hct->getStopLoadedSkillType());
			} else {
				unit->finishCommand();
				unit->setCurrSkill(scStop);
			}
		}
	}
}
/*
inline bool finishAutoCommand(Unit *unit) {
	Command *command= unit->getCurrCommand();
	if(command->isAuto() && command->getPos2().x != -1) {
		command->setPos(command->getPos2());
		command->setPos2(Vec2i(-1, -1));
		unit->getPath()->clear();
		return true;
	} else {
		return false;
	}
}
*/

// ==================== updateRepair ====================

void UnitUpdater::updateRepair(Unit *unit){
	Command *command= unit->getCurrCommand();
	assert(command->getCommandType()->getClass() == ccRepair);

	const RepairCommandType *rct= static_cast<const RepairCommandType*>(command->getCommandType());
	const RepairSkillType *rst = rct->getRepairSkillType();
	bool repairThisFrame = unit->getCurrSkill()->getClass() == scRepair;
	Unit *repaired = command->getUnit();

	// If the unit I was supposed to repair died or is already fixed then finish
	if(repaired && (repaired->isDead() || !repaired->isDamaged())) {
		unit->setCurrSkill(scStop);
		unit->finishCommand();
		return;
	}

	if(command->isAuto() && unit->getType()->hasCommandClass(ccAttack)) {
		Command *autoAttackCmd;
		// attacking is 1st priority

		if((autoAttackCmd = doAutoAttack(unit))) {
			// if doAutoAttack found a target, we need to give it the correct starting address
			if(command->hasPos2()) {
				autoAttackCmd->setPos2(command->getPos2());
			}
			unit->giveCommand(autoAttackCmd);
		}
	}

	if(repairableOnRange(unit, &repaired, rct, rst->getMaxRange(), rst->isSelfAllowed())) {
		unit->setTarget(repaired);
		unit->setCurrSkill(rst);
	} else {
		Vec2i targetPos;
		if(repairableOnSight(unit, &repaired, rct, rst->isSelfAllowed())) {
			if(!map->getNearestFreePos(targetPos, unit, repaired, 1, rst->getMaxRange())) {
				unit->setCurrSkill(scStop);
				unit->finishCommand();
				return;
			}

			if(targetPos != unit->getTargetPos()) {
				unit->setTargetPos(targetPos);
				unit->getPath()->clear();
			}
		} else {
			// if no more damaged units and on auto command, turn around
			//finishAutoCommand(unit);

			if(command->isAuto() && command->hasPos2()) {
				command->popPos();
				unit->getPath()->clear();
			}
			targetPos = command->getPos();
		}

		switch(pathFinder.findPath(unit, targetPos)){
		case PathFinder::tsArrived:
			if(repaired && unit->getPos() != targetPos) {
				// presume blocked
				unit->setCurrSkill(scStop);
				unit->finishCommand();
				break;
			}
			if(repaired) {
				unit->setCurrSkill(rst);
			} else {
				unit->setCurrSkill(scStop);
				unit->finishCommand();
			}
			break;

		case PathFinder::tsOnTheWay:
			unit->setCurrSkill(rct->getMoveSkillType());
			break;

		case PathFinder::tsBlocked:
			if(unit->getPath()->isBlocked()){
				unit->setCurrSkill(scStop);
				unit->finishCommand();
			}
			break;
		}
	}

	if(repaired && !repaired->isDamaged()) {
		unit->setCurrSkill(scStop);
		unit->finishCommand();
	}

	if(repairThisFrame && unit->getCurrSkill()->getClass() == scRepair) {
		//if repairing
		if(repaired){
			unit->setTarget(repaired);
		}

		if(!repaired) {
			unit->setCurrSkill(scStop);
		} else {
			//shiney
			if(rst->getSplashParticleSystemType()){
				const SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(repaired->getCenteredPos()));
				bool visible= sc->isVisible(world->getThisTeamIndex());

				SplashParticleSystem *psSplash = rst->getSplashParticleSystemType()->create();
				psSplash->setPos(repaired->getCurrVector());
				psSplash->setVisible(visible);
				Renderer::getInstance().manageParticleSystem(psSplash, rsGame);
			}

			bool wasBuilt = repaired->isBuilt();

			assert(repaired->isAlive() && repaired->getHp() > 0);

			if(repaired->repair(rst->getAmount(), rst->getMultiplier())) {
				unit->setCurrSkill(scStop);
				if(!wasBuilt) {
					//building finished
					if(unit->getFactionIndex() == world->getThisFactionIndex()) {
						// try to find finish build sound
						BuildCommandType *bct = (BuildCommandType *)unit->getType()->getFirstCtOfClass(ccBuild);
						if(bct) {
							SoundRenderer::getInstance().playFx(
								bct->getBuiltSound(),
								unit->getCurrVector(),
								gameCamera->getPos());
						}
					}
					if(isNetworkServer()) {
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
	const ProduceCommandType *pct = static_cast<const ProduceCommandType*>(command->getCommandType());
	Unit *produced;

	if(unit->getCurrSkill()->getClass() != scProduce) {
		//if not producing
		if(!verifySubfaction(unit, pct->getProducedUnit())) {
			return;
		}

		unit->setCurrSkill(pct->getProduceSkillType());
		unit->getFaction()->checkAdvanceSubfaction(pct->getProducedUnit(), false);
	} else {
		unit->update2();

		if(unit->getProgress2() > pct->getProduced()->getProductionTime()) {
			if(isNetworkClient()) {
				// client predict, presume the server will send us the unit soon.
				unit->finishCommand();
				unit->setCurrSkill(scStop);
				return;
			}

			produced = new Unit(world->getNextUnitId(), Vec2i(0), pct->getProducedUnit(), unit->getFaction(), world->getMap());

			//if no longer a valid subfaction, let them have the unit, but make
			//sure it doesn't advance the subfaction

			if(verifySubfaction(unit, pct->getProducedUnit())) {
				unit->getFaction()->checkAdvanceSubfaction(pct->getProducedUnit(), true);
			}

			//place unit creates the unit
			if(!world->placeUnit(unit->getCenteredPos(), 10, produced)) {
				unit->cancelCurrCommand();
				delete produced;
			} else {
				produced->create();
				produced->born();
				world->getStats()->produce(unit->getFactionIndex());
				const CommandType *ct = produced->computeCommandType(unit->getMeetingPos());

				if(ct) {
					produced->giveCommand(new Command(ct, CommandFlags(), unit->getMeetingPos()));
				}

				if(pct->getProduceSkillType()->isPet()) {
					unit->addPet(produced);
					produced->setMaster(unit);
				}

				unit->finishCommand();

				if(isNetworkServer()) {
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
	const UpgradeCommandType *uct = static_cast<const UpgradeCommandType*>(command->getCommandType());

	//if subfaction becomes invalid while updating this command, then cancel it.

	if(!verifySubfaction(unit, uct->getProduced())) {
		unit->cancelCommand();
		unit->setCurrSkill(scStop);
		return;
	}

	if(unit->getCurrSkill()->getClass() != scUpgrade) {
		//if not producing
		unit->setCurrSkill(uct->getUpgradeSkillType());
		unit->getFaction()->checkAdvanceSubfaction(uct->getProducedUpgrade(), false);
	} else {
		//if producing
		if(unit->getProgress2() < uct->getProduced()->getProductionTime()) {
			unit->update2();
		}

		if(unit->getProgress2() >= uct->getProduced()->getProductionTime()) {
			if(isNetworkClient()) {
				// clients will wait for the server to tell them that the upgrade is finished
				return;
			}
			unit->finishCommand();
			unit->setCurrSkill(scStop);
			unit->getFaction()->finishUpgrade(uct->getProducedUpgrade());
			unit->getFaction()->checkAdvanceSubfaction(uct->getProducedUpgrade(), true);
			if(isNetworkServer()) {
				getServerInterface()->unitUpdate(unit);
				getServerInterface()->updateFactions();
			}
		}
	}
}


// ==================== updateMorph ====================

void UnitUpdater::updateMorph(Unit *unit){

	Command *command= unit->getCurrCommand();
	const MorphCommandType *mct= static_cast<const MorphCommandType*>(command->getCommandType());

	//if subfaction becomes invalid while updating this command, then cancel it.
	if(!verifySubfaction(unit, mct->getMorphUnit())) {
		return;
	}

	if(unit->getCurrSkill()->getClass() != scMorph){
		//if not morphing, check space
		if(map->isFreeCellsOrHasUnit(unit->getPos(), mct->getMorphUnit()->getSize(), unit->getCurrField(), unit)){
			unit->setCurrSkill(mct->getMorphSkillType());
			unit->getFaction()->checkAdvanceSubfaction(mct->getMorphUnit(), false);
		} else {
			if(unit->getFactionIndex() == world->getThisFactionIndex()){
				console->addStdMessage("InvalidPosition");
			}
			unit->cancelCurrCommand();
		}
	} else {
		unit->update2();
		if(unit->getProgress2()>mct->getProduced()->getProductionTime()) {

			//finish the command
			if(unit->morph(mct)){
				unit->finishCommand();
				if(gui->isSelected(unit)) {
					gui->onSelectionChanged();
				}
				unit->getFaction()->checkAdvanceSubfaction(mct->getMorphUnit(), true);
				if(isNetworkServer()) {
					getServerInterface()->unitMorph(unit);
					getServerInterface()->updateFactions();
				}
			} else {
				unit->cancelCurrCommand();
				if(unit->getFactionIndex() == world->getThisFactionIndex()){
					console->addStdMessage("InvalidPosition");
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
	Command *command= unit->getCurrCommand();
	const GuardCommandType *gct = static_cast<const GuardCommandType*>(command->getCommandType());
	Unit *target= command->getUnit();
	Vec2i pos;

	if(target && target->isDead()) {
		//if you suck ass as a body guard then you have to hang out where your client died.
		command->setUnit(NULL);
		command->setPos(target->getPos());
		target = NULL;
	}

	if(target) {
		pos = Map::getNearestPos(unit->getPos(), target, 1, gct->getMaxDistance());
	} else {
		pos = Map::getNearestPos(unit->getPos(), command->getPos(), 1, gct->getMaxDistance());
	}

	if(updateAttackGeneric(unit, command, gct, NULL, pos)) {
		unit->setCurrSkill(scStop);
	}
}

// ==================== updatePatrol ====================

void UnitUpdater::updatePatrol(Unit *unit){
	Command *command= unit->getCurrCommand();
	const PatrolCommandType *pct= static_cast<const PatrolCommandType*>(command->getCommandType());
	Unit *target = command->getUnit();
	Unit *target2 = command->getUnit2();
	Vec2i pos;

	if(target) {
		pos = target->getCenteredPos();
		if(target->isDead()) {
			command->setUnit(NULL);
			command->setPos(pos);
		}
	} else {
		pos = command->getPos();
	}

	if(target) {
		pos = Map::getNearestPos(unit->getPos(), pos, 1, pct->getMaxDistance());
	}

	if(target2 && target2->isDead()) {
		command->setUnit2(NULL);
		command->setPos2(target2->getCenteredPos());
	}

	// If destination reached or blocked, turn around on next frame.
	if(updateAttackGeneric(unit, command, pct, NULL, pos)) {
		command->swap();
		/*
		// can't use minor update here because the command has changed and that wont give the new
		// command
		if(isNetworkGame() && isServer()) {
			getServerInterface()->minorUnitUpdate(unit);
		}*/
	}
}


void UnitUpdater::updateEmanations(Unit *unit){
	// This is a little hokey, but probably the best way to reduce redundant code
	static EffectTypes singleEmanation;
	for(Emanations::const_iterator i = unit->getGetEmanations().begin();
			i != unit->getGetEmanations().end(); i++) {
		singleEmanation.resize(1);
		singleEmanation[0] = *i;
		applyEffects(unit, singleEmanation, unit->getPos(), fLand, (*i)->getRadius());
		applyEffects(unit, singleEmanation, unit->getPos(), fAir, (*i)->getRadius());
	}
}

// ==================== PRIVATE ====================

// ==================== attack ====================

void UnitUpdater::hit(Unit *attacker) {
	hit(attacker, static_cast<const AttackSkillType*>(attacker->getCurrSkill()), attacker->getTargetPos(), attacker->getTargetField());
}

void UnitUpdater::hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField, Unit *attacked){

	//hit attack positions
	if(ast->getSplash()){
		std::map<Unit*, float> hitList;
		std::map<Unit*, float>::iterator i;

		PosCircularIterator pci(map, targetPos, ast->getSplashRadius());
		while(pci.next()){
			attacked = map->getCell(pci.getPos())->getUnit(targetField);
			if(attacked) {
				float distance = pci.getPos().dist(attacker->getTargetPos());
				damage(attacker, ast, attacked, distance);

				// Remember all units we hit with the closest distance. We only
				// want to hit them with effects once.
				if(ast->isHasEffects()) {
					i = hitList.find(attacked);
					if(i == hitList.end() || i->second > distance) {
						hitList[attacked] = distance;
					}
				}
			}
		}

		if (ast->isHasEffects()) {
			for(i = hitList.begin(); i != hitList.end(); i++) {
				applyEffects(attacker, ast->getEffectTypes(), i->first, i->second);
			}
		}
	} else {
		if(!attacked) {
			attacked= map->getCell(targetPos)->getUnit(targetField);
		}

		if(attacked){
			damage(attacker, ast, attacked, 0.f);
			if(ast->isHasEffects()) {
				applyEffects(attacker, ast->getEffectTypes(), attacked, 0.f);
			}
		}
	}
}

void UnitUpdater::damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, float distance){

	if(isNetworkClient()) {
		return;
	}

	//get vars
	float damage= (float)attacker->getAttackStrength(ast);
	int var= ast->getAttackVar();
	int armor= attacked->getArmor();
	float damageMultiplier= world->getTechTree()->getDamageMultiplier(ast->getAttackType(),
			attacked->getType()->getArmorType());
	int startingHealth= attacked->getHp();
	int actualDamage;

	//compute damage
	damage+= random.randRange(-var, var);
	damage/= distance + 1.0f;
	damage-= armor;
	damage*= damageMultiplier;
	if(damage<1){
		damage= 1;
	}

	//damage the unit
	if(attacked->decHp(static_cast<int>(damage))){
		world->doKill(attacker, attacked);
		actualDamage= startingHealth;
	} else {
		actualDamage= (int)roundf(damage);
	}

	//add stolen health to attacker
	if(attacker->getAttackPctStolen(ast) || ast->getAttackPctVar()) {
		float pct = attacker->getAttackPctStolen(ast)
				+ random.randRange(-ast->getAttackPctVar(), ast->getAttackPctVar());
		int stolen = (int)roundf((float)actualDamage * pct);
		if(stolen && attacker->doRegen(stolen, 0)) {
			// stealing a negative percentage and dying?
			world->doKill(attacker, attacker);
		}
	}
	if(isNetworkServer()) {
		getServerInterface()->minorUnitUpdate(attacker);
		getServerInterface()->minorUnitUpdate(attacked);
	}

	//complain
	const Vec3f &attackerVec = attacked->getCurrVector();
	if(gui->isVisible(Vec2i((int)roundf(attackerVec.x), (int)roundf(attackerVec.y)))) {
		attacked->getFaction()->attackNotice(attacked);
	}
}

void UnitUpdater::startAttackParticleSystem(Unit *unit){
	Renderer &renderer= Renderer::getInstance();

	ProjectileParticleSystem *psProj = 0;
	SplashParticleSystem *psSplash;

	const AttackSkillType *ast= static_cast<const AttackSkillType*>(unit->getCurrSkill());
	ParticleSystemTypeProjectile *pstProj= ast->getProjParticleType();
	ParticleSystemTypeSplash *pstSplash= ast->getSplashParticleType();

	Vec3f startPos = unit->getCurrVector();
	Vec3f endPos = unit->getTargetVec();

	//make particle system
	const SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(unit->getPos()));
	const SurfaceCell *tsc= map->getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()));
	bool visible= sc->isVisible(world->getThisTeamIndex()) || tsc->isVisible(world->getThisTeamIndex());

	//projectile
	if(pstProj!=NULL){
		psProj= pstProj->create();

		switch(pstProj->getStart()) {
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
		if(pstProj->isTracking() && unit->getTarget()) {
			psProj->setTarget(unit->getTarget());
			psProj->setObserver(new ParticleDamager(unit, unit->getTarget(), this, gameCamera));
		} else {
			psProj->setObserver(new ParticleDamager(unit, NULL, this, gameCamera));
		}

		psProj->setVisible(visible);
		renderer.manageParticleSystem(psProj, rsGame);
	}
	else{
		hit(unit);
	}

	//splash
	if(pstSplash!=NULL){
		psSplash= pstSplash->create();
		psSplash->setPos(endPos);
		psSplash->setVisible(visible);
		renderer.manageParticleSystem(psSplash, rsGame);
		if(pstProj!=NULL){
			psProj->link(psSplash);
		}
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

		PosCircularIterator pci(map, targetPos, splashRadius);
		while (pci.next()) {
			target = map->getCell(pci.getPos())->getUnit(targetField);
			if (target) {
				float distance = pci.getPos().dist(targetPos);

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
	if(u->add(e)){
		Unit *attacker = e->getSource();
		if(attacker) {
			world->getStats()->kill(attacker->getFactionIndex(), u->getFactionIndex());
			attacker->incKills();
		} else if (e->getRoot()) {
			// if killed by a recourse effect, this was suicide
			world->getStats()->kill(u->getFactionIndex(), u->getFactionIndex());
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
	if(destSize == 1) {
		return getNear(orig, destNW, minRange, maxRange);
	}

	Vec2f dest;
	int offset = destSize - 1;
	if(orig.x < destNW.x) {
		dest.x = destNW.x;
	} else if (orig.x > destNW.x + offset) {
		dest.x = destNW.x + offset;
	} else {
		dest.x = orig.x;
	}

	if(orig.y < destNW.y) {
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
	Vec2i pos= unit->getCurrCommand()->getPos();

	for(int radius= 0; radius<maxResSearchRadius; radius++){
		for(int i=pos.x-radius; i<=pos.x+radius; ++i){
			for(int j=pos.y-radius; j<=pos.y+radius; ++j){
				if(map->isInside(i, j)){
					Resource *r= map->getSurfaceCell(Map::toSurfCoords(Vec2i(i, j)))->getResource();
					if(r!=NULL){
						if(hct->canHarvest(r->getType())){
							unit->getCurrCommand()->setPos(Vec2i(i, j));
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}


bool UnitUpdater::attackerOnSight(const Unit *unit, Unit **rangedPtr){
	int range= unit->getSight();
	return unitOnRange(unit, range, rangedPtr, NULL, NULL);
}

bool UnitUpdater::attackableOnSight(const Unit *unit, Unit **rangedPtr, const AttackSkillTypes *asts, const AttackSkillType **past) {
	int range= unit->getSight();
	return unitOnRange(unit, range, rangedPtr, asts, past);
}

bool UnitUpdater::attackableOnRange(const Unit *unit, Unit **rangedPtr, const AttackSkillTypes *asts, const AttackSkillType **past) {
	int range= unit->getMaxRange(asts);
	return unitOnRange(unit, range, rangedPtr, asts, past);
}

/*
AttackSkillPreference
	aspWhenPossible,
	aspWhenInRange,
	aspWhenClear,
	aspOnLarge,
	aspOnBuilding,
	aspWhenDamaged,
*/

//if the unit has any enemy on range
/** rangedPtr should point to a pointer that is either NULL or a valid Unit */
bool UnitUpdater::unitOnRange(const Unit *unit, int range, Unit **rangedPtr, const AttackSkillTypes *asts, const AttackSkillType **past) {
	Targets enemies;

	if(*rangedPtr!=NULL && (*rangedPtr)->isDead()){
		*rangedPtr= NULL;
	}

	//aux vars
	int size= unit->getType()->getSize();
	Vec2i center= unit->getPos();
	Vec2f floatCenter= unit->getFloatCenteredPos();
	range += size / 2;

	//nearby cells
	for (int i = center.x - range; i < center.x + range + 1; ++i){
		for (int j = center.y - range; j < center.y + range + 1; ++j){

			//cells insede map and in range
			if(map->isInside(i, j) && floor(floatCenter.dist(Vec2f((float)i, (float)j))) <= (range + 1)){

				//all fields
				for(int k=0; k<fCount; k++){
					Field f= static_cast<Field>(k);

					//check field
					if(!asts || asts->getField(f)){
						Unit *possibleEnemy= map->getCell(i, j)->getUnit(f);

						//check enemy
						if(possibleEnemy!=NULL && possibleEnemy->isAlive()){
							if((!unit->isAlly(possibleEnemy) && *rangedPtr==NULL) || *rangedPtr==possibleEnemy){
								enemies.record(possibleEnemy, floatCenter.dist(possibleEnemy->getFloatCenteredPos()));
							}
						}
					}
				}
			}
		}
	}

	if(!enemies.size()) {
		return false;
	}

	// get targets that can attack 1st.
	if(!(*rangedPtr = enemies.getNearest(scAttack))) {
		*rangedPtr = enemies.getNearest();
	}
	assert(*rangedPtr);

	// check to see if we like this target.
	if(asts && past) {
		*past = asts->getPreferredAttack(unit, *rangedPtr, floatCenter.dist((*rangedPtr)->getFloatCenteredPos())
				- (float)(*rangedPtr)->getType()->getSize() / 2.0f);
		return (bool)*past;
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

	if(target && target->isAlive() && target->isDamaged() && rct->isRepairableUnitType(target->getType())) {
		float rangeToTarget = floatCenter.dist(target->getFloatCenteredPos()) - (float)(centerSize + target->getSize()) / 2.0f;
		if(rangeToTarget <= range) {
			// current target is good
			return true;
		} else {
			return false;
		}
	}
	target = NULL;

	//nearby cells
	for (int i = center.x - range; i < center.x + range + 1; ++i){
		for (int j = center.y - range; j < center.y + range + 1; ++j){

			//cells insede map and in range
			if(map->isInside(i, j) && floor(floatCenter.dist(Vec2f((float)i, (float)j))) <= (range + 1)){

				//all fields
				for(int k=0; k<fCount; k++){
					Field f= (Field)k;
					Unit *candidate= map->getCell(i, j)->getUnit(f);

					//is it a repairable?
					if(candidate
							&& (allowSelf || candidate != unit)
							&& (!rst->isSelfOnly() || candidate == unit)
							&& candidate->isAlive()
							&& unit->isAlly(candidate)
							&& (!rst->isPetOnly() || unit->isPet(candidate))
							&& (!damagedOnly || candidate->isDamaged())
							&& (!militaryOnly || candidate->getType()->hasCommandClass(ccAttack))
							&& rct->isRepairableUnitType(candidate->getType())) {

						//record the nearest distance to target (target may be on multiple cells)
						repairables.record(candidate, floatCenter.dist(candidate->getFloatCenteredPos()));
					}
				}
			}
		}
	}

	// if no repairables or just one then it's a simple choice.
	if(repairables.empty()) {
		return false;
	} else if(repairables.size() == 1) {
		*rangedPtr = repairables.begin()->first;
		return true;
	}

	//heal cloesest ally that can attack (and are probably fighting) first.
	//if none, go for units that are less than 20%
	//otherwise, take the nearest repairable unit
	if(!(*rangedPtr = repairables.getNearest(scAttack))
			&& !(*rangedPtr = repairables.getNearest(scCount, 0.2f))
			&& !(*rangedPtr= repairables.getNearest())) {
		return false;
	}
	return true;
}

// =====================================================
//	class ParticleDamager
// =====================================================

ParticleDamager::ParticleDamager(Unit *attacker, Unit *target, UnitUpdater *unitUpdater, const GameCamera *gameCamera){
	this->gameCamera= gameCamera;
	this->attackerRef= attacker;
	this->targetRef = target;
	this->ast= static_cast<const AttackSkillType*>(attacker->getCurrSkill());
	this->targetPos= attacker->getTargetPos();
	this->targetField= attacker->getTargetField();
	this->unitUpdater= unitUpdater;
}

void ParticleDamager::update(ParticleSystem *particleSystem){
	Unit *attacker= attackerRef.getUnit();

	if(attacker) {
		Unit *target = targetRef.getUnit();
		if(target) {
			targetPos = target->getCenteredPos();
			// manually feed the attacked unit here to avoid problems with cell maps and such
			unitUpdater->hit(attacker, ast, targetPos, targetField, target);
		} else {
			unitUpdater->hit(attacker, ast, targetPos, targetField, NULL);
		}

		//play sound
		StaticSound *projSound= ast->getProjSound();
		if(particleSystem->getVisible() && projSound){
			SoundRenderer::getInstance().playFx(projSound, Vec3f(targetPos.x, 0.f, targetPos.y), gameCamera->getPos());
		}
	}
	particleSystem->setObserver(NULL);
	delete this;
}

}}//end namespace
