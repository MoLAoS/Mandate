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
//local to keep players from getting stuff they aren't supposed to.
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

void UnitUpdater::init(Game *game){

	this->gui= game->getGui();
	this->gameCamera= game->getGameCamera();
	this->world= game->getWorld();
	this->map= world->getMap();
	this->console= game->getConsole();
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
		}
	}

	updateEmanations(unit);

	//update unit
	if(unit->update()){

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
	if(unit->isDead() && unit->getCurrSkill()->getClass()!=scDie){
		unit->kill();
	}
}

// ==================== progress commands ====================

//VERY IMPORTANT: compute next state depending on the first order of the list
void UnitUpdater::updateUnitCommand(Unit *unit){

	//if unis has command process it
    if(unit->anyCommand()) {
		unit->getCurrCommand()->getCommandType()->update(this, unit);
	}

	//if no commands stop and add stop command or guard command for pets
	if(!unit->anyCommand() && unit->isOperative()){
		if(unit->getMaster() && unit->getType()->hasCommandClass(ccGuard)) {
			unit->setCurrSkill(scStop);
			unit->giveCommand(new Command(unit->getType()->getFirstCtOfClass(ccGuard), CommandFlags(), unit->getMaster()));
		} else {
			unit->setCurrSkill(scStop);
			if(unit->getType()->hasCommandClass(ccStop)){
				unit->giveCommand(new Command(unit->getType()->getFirstCtOfClass(ccStop), CommandFlags()));
			}
		}
	}
}

// ==================== updateStop ====================
/*
bool UnitUpdater::updateAutoActions(Unit *unit){
}
*/
void UnitUpdater::updateStop(Unit *unit){

    Command *command= unit->getCurrCommand();
    const StopCommandType *sct = static_cast<const StopCommandType*>(command->getCommandType());
    Unit *sighted = NULL;

	// if we have another command then stop sitting on your ass
	if(unit->getCommands().size() > 1 && unit->getCommands().front()->getCommandType()->getClass() == ccStop) {
		unit->finishCommand();
		return;
	}

    unit->setCurrSkill(sct->getStopSkillType());

	//we can attack any unit => attack it
	//we can repair any ally => repair it
   	if(unit->getType()->hasSkillClass(scAttack) || unit->getType()->hasSkillClass(scRepair)) {

		for(int i=0; i<unit->getType()->getCommandTypeCount(); ++i){
			const CommandType *ct= unit->getType()->getCommandType(i);

			//look for an attack or repair skill
			const AttackSkillType *ast = NULL;
			const AttackSkillTypes *asts = NULL;
			const RepairCommandType *rct = NULL;
			sighted = NULL;
			switch(ct->getClass()) {
				case ccAttack:
					asts = ((const AttackCommandType*)ct)->getAttackSkillTypes();
					break;
				case ccAttackStopped:
					asts = ((const AttackStoppedCommandType*)ct)->getAttackSkillTypes();
					break;
				case ccRepair:
					rct = (const RepairCommandType*)ct;
					break;
				default:
					break;
			}

			// FIXME: Need a flag in Command class to specify an implicit command
			// so that the attack can cease the moment units are no longer in
			// sight, causing them to return to their original position without
			// having to first go all the way out to the original position the
			// enemy was sighted at.

			//use it to attack
			if(asts) {
				if(attackableOnSight(unit, &sighted, asts, NULL)){
					Command *newCommand = new Command(ct, CommandFlags(cpAuto), sighted->getPos());
					newCommand->setPos2(unit->getPos());
					unit->giveCommand(newCommand);
					return;
				}
			}

			// FIXME: Units that could also attack, will fail to do so while
			// enganged in repair.  Command class needs an "implicit command"
			// flag, as it does for the implicit attack commands, so the unit can
			// switch from repairing to attacking if enemies are sighted while
			// they are repairing from an implicit command.

			//if repairable, put commands in queue, but let an attackable override
			if(rct && unit->isAutoRepairEnabled()) {
				const RepairSkillType *rst = rct->getRepairSkillType();
				if(unit->getEp() >= rst->getEpCost() && repairableOnSight(unit, &sighted, rct, rst->isSelfAllowed())) {
					Command *newCommand;
//					if(unit == sighted) {
//						newCommand = new Command(rct, CommandFlags(cpQueue, cpAuto), unit);
//					} else {
						newCommand = new Command(rct, CommandFlags(cpQueue, cpAuto),
								getNear(unit->getPos(), sighted, rst->getMinRange(), rst->getMaxRange()));
						newCommand->setPos2(unit->getPos());
//					}
					unit->giveCommand(newCommand);
				}
			}
		}
		sighted = NULL;
	}

	//see any unit and cant attack it => run
	if(unit->getCommands().size() == 1 && unit->getType()->hasCommandClass(ccMove)
			&& attackerOnSight(unit, &sighted)) {

		//if there is a friendly military unit that we can heal/repair and is
		//rougly between us, then be brave
		if(unit->getType()->hasCommandClass(ccRepair)) {
			Vec2f myCenter = unit->getFloatCenteredPos();
			Vec2f signtedCenter = sighted->getFloatCenteredPos();
			Vec2f fcenter = (myCenter + signtedCenter) / 2;
			Unit *myHero = NULL;
			int range = (myCenter.dist(signtedCenter) - (unit->getType()->getSize() + sighted->getType()->getSize())) / 2;
			Vec2i center(fcenter.x + 0.5f, fcenter.y + 0.5f);

			//try all of our repair commands
			for (int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
				const CommandType *ct= unit->getType()->getCommandType(i);
				if(ct->getClass() != ccRepair) {
					continue;
				}
				const RepairCommandType *rct = (const RepairCommandType*)ct;
				const RepairSkillType *rst = rct->getRepairSkillType();

				if(repairableOnRange(unit, center, 1, &myHero, rct, rst, range, false, true, false)) {
					return;
				}
			}
		}
		Vec2i escapePos= unit->getPos()*2-sighted->getPos();
		unit->giveCommand(new Command(unit->getType()->getFirstCtOfClass(ccMove), CommandFlags(cpAuto), escapePos));
	}
}

// ==================== updateMove ====================
//#define FOLLOW_DISTANCE 4

void UnitUpdater::updateMove(Unit *unit){
    Command *command= unit->getCurrCommand();
    const MoveCommandType *mct= static_cast<const MoveCommandType*>(command->getCommandType());

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
		if(!command->getUnit()) {
        	unit->finishCommand();
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
    }
    else{
		//compute target pos
		Vec2i pos;
		if(attackableOnSight(unit, &target, act->getAttackSkillTypes(), NULL)){
			pos= target->getCenteredPos();
		} else {
			// if no more targets and on auto command, turn around
			if(command->isAuto() && command->getPos2().x != -1) {
				command->setPos(command->getPos2());
				command->setPos2(Vec2i(-1, -1));
				unit->getPath()->clear();
			}
			pos= targetPos;
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

	if(unit->getCurrSkill()->getClass() != scBuild) {
        //if not building
        const UnitType *ut= command->getUnitType();
		Vec2i unitPos = unit->getPos();
		Vec2i pos = command->getPos();
		int buildingSize = builtUnitType->getSize();

		//are we sitting inside where we need to build?
		bool inside = unitPos.x >= pos.x
				&& unitPos.x <= pos.x + buildingSize - 1
				&& unitPos.y >= pos.y
				&& unitPos.y <= pos.y + buildingSize - 1;

		// pos2 of command is used to designate the intermediate location to
		// travel to in order to begin building.
		if(inside && command->getPos2().x == -1) {
			// need to find a place to go to build.
			Vec2i newpos(unit->getPos());

			//first off, let's try the nearest location not inside the building
			//area, if we can.
			//if(unit->getPos() != command->getPos()) {
			//	newpos = getNear(unit->getPos(), command->getPos(), 1, 1, builtUnitType->getSize());
			//}

			int unitSize = unit->getSize();
			int sideSize = buildingSize + unitSize;
			int possibilities = sideSize * 4;
			int i;

			for(i = 0; i < 50 && !map->isFreeCells(newpos, unit->getSize(), fLand); i++) {
				int selection = random.randRange(0, possibilities - 1);
				switch(selection / sideSize) {
				case 0:
					newpos.x = pos.x - unitSize + selection % sideSize;
					newpos.y = pos.y - unitSize;
					break;
				case 1:
					newpos.x = pos.x + sideSize;
					newpos.y = pos.y - unitSize + selection % sideSize;
					break;
				case 2:
					newpos.x = pos.x + sideSize - selection % sideSize;
					newpos.y = pos.y + sideSize;
					break;
				case 3:
					newpos.x = pos.x - unitSize + selection % sideSize;
					newpos.y = pos.y + sideSize + 1;
					break;
				}
			}
			// can't find a new build location after 50 tries
			if(i == 50) {
				unit->cancelCurrCommand();
			}
			command->setPos2(newpos);
		} else if(!inside && command->getPos2().x != -1) {
			command->setPos2(Vec2i(-1));
		}

		pos = inside ? command->getPos2()
			: getNear(unit->getPos(), command->getPos(), 1, 1, builtUnitType->getSize());

		switch (pathFinder.findPath(unit, pos)){
        case PathFinder::tsOnTheWay:
            unit->setCurrSkill(bct->getMoveSkillType());
            break;

        case PathFinder::tsArrived:
            //if arrived destination
            assert(command->getUnitType()!=NULL);
            if(map->isFreeCells(command->getPos(), ut->getSize(), fLand)){
				if(!verifySubfaction(unit, builtUnitType)) {
					return;
				}
				Unit *builtUnit= new Unit(world->getNextUnitId(), command->getPos(), builtUnitType, unit->getFaction(), world->getMap());
				builtUnit->create();

				if(!builtUnitType->hasSkillClass(scBeBuilt)){
					throw runtime_error("Unit " + builtUnitType->getName() + "has no be_built skill");
				}

				builtUnit->setCurrSkill(scBeBuilt);
				unit->setCurrSkill(bct->getBuildSkillType());
				unit->setTarget(builtUnit);
				map->prepareTerrain(builtUnit);
				command->setUnit(builtUnit);
				unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), false);

				//play start sound
				if(unit->getFactionIndex()==world->getThisFactionIndex()){
					SoundRenderer::getInstance().playFx(
						bct->getStartSound(),
						unit->getCurrVector(),
						gameCamera->getPos());
				}
			}
            else{
                //if there are no free cells
				unit->cancelCurrCommand();
                unit->setCurrSkill(scStop);
				if(unit->getFactionIndex()==world->getThisFactionIndex()){
                     console->addStdMessage("BuildingNoPlace");
				}
            }
            break;

        case PathFinder::tsBlocked:
			if(unit->getPath()->isBlocked()){
				unit->cancelCurrCommand();
			}
            break;
        }
    }
    else{
        //if building
        Unit *builtUnit= map->getCell(unit->getTargetPos())->getUnit(fLand);

        //if u is killed while building then u==NULL;
		if(builtUnit!=NULL && builtUnit!=command->getUnit()){
			unit->setCurrSkill(scStop);
		}
		else if(builtUnit==NULL || builtUnit->isBuilt()){
            unit->finishCommand();
            unit->setCurrSkill(scStop);
        }
        else if(builtUnit->repair()){
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
		if(unit->getLoadCount()==0){
			//if not loaded go for resources
			Resource *r= map->getSurfaceCell(Map::toSurfCoords(command->getPos()))->getResource();
			if(r!=NULL && hct->canHarvest(r->getType())){
				//if can harvest dest. pos
				if(unit->getPos().dist(command->getPos())<harvestDistance &&
					map->isResourceNear(unit->getPos(), r->getType(), targetPos)) {
						//if it finds resources it starts harvesting
						unit->setCurrSkill(hct->getHarvestSkillType());
						unit->setTargetPos(targetPos);
						unit->setLoadCount(0);
						unit->setLoadType(map->getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()))->getResource()->getType());
				}
				else{
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
			if(store!=NULL){
				switch(pathFinder.findPath(unit, store->getCenteredPos())){
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
			//if there is no resource, just stop
			unit->setCurrSkill(hct->getStopLoadedSkillType());
		}
	}
}


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

	if(repairableOnRange(unit, &repaired, rct, rst->getMaxRange(), rst->isSelfAllowed())) {
		unit->setTarget(repaired);
		unit->setCurrSkill(rst);
	} else {
		Vec2i targetPos;
		if(repairableOnSight(unit, &repaired, rct, rst->isSelfAllowed())) {
			//
			targetPos = getNear(unit->getPos(), repaired, 1, rst->getMaxRange());

			//if the spot I would like to get to is occupied, let's just try to
			//get to the cente of the target
			if(!map->isFreeCells(targetPos, unit->getSize(), fLand)) {
				targetPos = repaired->getCenteredPos();
			}

			unit->setTargetPos(targetPos);
		} else {
			// if no more damaged units and on auto command, turn around
			if(command->isAuto() && command->getPos2().x != -1) {
				command->setPos(command->getPos2());
				command->setPos2(Vec2i(-1, -1));
				unit->getPath()->clear();
			}
			targetPos = command->getPos();
		}

		switch(pathFinder.findPath(unit, targetPos)){
		case PathFinder::tsArrived:
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

			if(repaired->repair(rst->getAmount(), rst->getMultiplier())) {
				unit->setCurrSkill(scStop);
				//unit->finishCommand();
			}
        }
    }
}


// ==================== updateProduce ====================

void UnitUpdater::updateProduce(Unit *unit){

    Command *command= unit->getCurrCommand();
    const ProduceCommandType *pct= static_cast<const ProduceCommandType*>(command->getCommandType());
    Unit *produced;

    if(unit->getCurrSkill()->getClass()!=scProduce){
        //if not producing
		if(!verifySubfaction(unit, pct->getProducedUnit())) {
			return;
		}
        unit->setCurrSkill(pct->getProduceSkillType());
		unit->getFaction()->checkAdvanceSubfaction(pct->getProducedUnit(), false);
    }
    else{
		unit->update2();
        if(unit->getProgress2()>pct->getProduced()->getProductionTime()){
			produced= new Unit(world->getNextUnitId(), Vec2i(0), pct->getProducedUnit(), unit->getFaction(), world->getMap());

			//if no longer a valid subfaction, let them have the unit, but make
			//sure it doesn't advance the subfaction
			if(verifySubfaction(unit, pct->getProducedUnit())) {
				unit->getFaction()->checkAdvanceSubfaction(pct->getProducedUnit(), true);
			}

			//place unit creates the unit
			if(!world->placeUnit(unit->getCenteredPos(), 10, produced)){
				unit->cancelCurrCommand();
				delete produced;
			}
			else{
				produced->create();
				produced->born();
				world->getStats()->produce(unit->getFactionIndex());
				const CommandType *ct= produced->computeCommandType(unit->getMeetingPos());
				if(ct!=NULL){
					produced->giveCommand(new Command(ct, CommandFlags(), unit->getMeetingPos()));
				}
				if(pct->getProduceSkillType()->isPet()) {
					unit->addPet(produced);
					produced->setMaster(unit);
				}
				unit->finishCommand();
			}
			unit->setCurrSkill(scStop);
        }
    }
}

// ==================== updateUpgrade ====================

void UnitUpdater::updateUpgrade(Unit *unit){

    Command *command= unit->getCurrCommand();
    const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());

	//if subfaction becomes invalid while updating this command, then cancel it.
	if(!verifySubfaction(unit, uct->getProduced())) {
		return;
	}

	if(unit->getCurrSkill()->getClass()!=scUpgrade){
		//if not producing
		unit->setCurrSkill(uct->getUpgradeSkillType());
		unit->getFaction()->checkAdvanceSubfaction(uct->getProducedUpgrade(), false);
    }
	else{
		//if producing
		unit->update2();
        if(unit->getProgress2()>uct->getProduced()->getProductionTime()){
            unit->finishCommand();
            unit->setCurrSkill(scStop);
			unit->getFaction()->finishUpgrade(uct->getProducedUpgrade());
			unit->getFaction()->checkAdvanceSubfaction(uct->getProducedUpgrade(), true);
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

    if(unit->getCurrSkill()->getClass()!=scMorph){
		//if not morphing, check space
		if(map->isFreeCellsOrHasUnit(unit->getPos(), mct->getMorphUnit()->getSize(), unit->getCurrField(), unit)){
			unit->setCurrSkill(mct->getMorphSkillType());
			unit->getFaction()->checkAdvanceSubfaction(mct->getMorphUnit(), false);
		}
		else{
			if(unit->getFactionIndex()==world->getThisFactionIndex()){
				console->addStdMessage("InvalidPosition");
			}
			unit->cancelCurrCommand();
		}
    }
    else{
		unit->update2();
        if(unit->getProgress2()>mct->getProduced()->getProductionTime()){

			//finish the command
			if(unit->morph(mct)){
				unit->finishCommand();
				gui->onSelectionChanged();
				unit->getFaction()->checkAdvanceSubfaction(mct->getMorphUnit(), true);
			}
			else{
				unit->cancelCurrCommand();
				if(unit->getFactionIndex()==world->getThisFactionIndex()){
					console->addStdMessage("InvalidPosition");
				}
			}
			unit->setCurrSkill(scStop);

        }
    }
}

// ==================== updateCastSpell ====================

void UnitUpdater::updateCastSpell(Unit *unit) {
}

// ==================== updateGuard ====================

void UnitUpdater::updateGuard(Unit *unit) {
	Command *command= unit->getCurrCommand();
    const GuardCommandType *gct = static_cast<const GuardCommandType*>(command->getCommandType());
	Unit *target= command->getUnit();
	Vec2i pos;

	if(target && target->isDead()) {
		//if you suck ass as a body guard then you have to hang out where
		//your client died.
		command->setUnit(NULL);
		command->setPos(target->getPos());
		target = NULL;
	}

	if(target) {
		pos = getNear(unit->getPos(), target, 1, gct->getMaxDistance());
	} else {
		pos = getNear(unit->getPos(), command->getPos(), 1, gct->getMaxDistance());
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
		pos = getNear(unit->getPos(), pos, 1, pct->getMaxDistance());
	}

	if(target2 && target2->isDead()) {
		command->setUnit2(NULL);
		command->setPos2(target2->getCenteredPos());
	}

	// If destination reached or blocked, turn around on next frame.
	if(updateAttackGeneric(unit, command, pct, NULL, pos)) {
		command->swap();
	}
}


void UnitUpdater::updateEmanations(Unit *unit){
	// This is a little hokey, but probably the best way to reduce redundant code
	static EffectTypes singleEmanation;
	for(Emanations::const_iterator i = unit->getGetEmanations().begin();
			i != unit->getGetEmanations().end(); i++) {
		singleEmanation.clear();
		singleEmanation.resize(1);
		singleEmanation[0] = *i;
		applyEffects(unit, singleEmanation, unit->getPos(), unit->getCurrField(), (*i)->getRadius());
	}
}

// ==================== PRIVATE ====================

// ==================== attack ====================

void UnitUpdater::hit(Unit *attacker){
	hit(attacker, static_cast<const AttackSkillType*>(attacker->getCurrSkill()), attacker->getTargetPos(), attacker->getTargetField());
}

void UnitUpdater::hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField){

	//hit attack positions
	if(ast->getSplash()){
		std::map<Unit*, float> hitList;
		std::map<Unit*, float>::iterator i;

		PosCircularIterator pci(map, targetPos, ast->getSplashRadius());
		while(pci.next()){
			Unit *attacked= map->getCell(pci.getPos())->getUnit(targetField);
			if(attacked!=NULL){
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
	}
	else{
		Unit *attacked= map->getCell(targetPos)->getUnit(targetField);
		if(attacked!=NULL){
			damage(attacker, ast, attacked, 0.f);
			if(ast->isHasEffects()) {
				applyEffects(attacker, ast->getEffectTypes(), attacked, 0.f);
			}
		}
	}
}

void UnitUpdater::damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, float distance){

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
		actualDamage= damage;
	}

	//add stolen health to attacker
	if(attacker->getAttackPctStolen(ast) || ast->getAttackPctVar()) {
		float pct = attacker->getAttackPctStolen(ast)
				+ random.randRange(-ast->getAttackPctVar(), ast->getAttackPctVar());
		int stolen = (int) ((float)actualDamage * pct + 0.5f);
		if(stolen && attacker->doRegen(stolen, 0)) {
			// stealing a negative percentage and dying?
			world->doKill(attacker, attacker);
		}
	}

	//complain
	const Vec3f &attackerVec = attacked->getCurrVector();
	if(gui->isVisible(Vec2i(attackerVec.x, attackerVec.y))) {
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

	Vec3f startPos= unit->getCurrVector();
	Vec3f endPos= unit->getTargetVec();

	//make particle system
	const SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(unit->getPos()));
	const SurfaceCell *tsc= map->getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()));
	bool visible= sc->isVisible(world->getThisTeamIndex()) || tsc->isVisible(world->getThisTeamIndex());

	//projectile
	if(pstProj!=NULL){
		psProj= pstProj->create();
		psProj->setPath(startPos, endPos);
		psProj->setObserver(new ParticleDamager(unit, this, gameCamera));
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
/**
 * Find the nearest position from dest to orig that is at least minRange from
 * the target, but no greater than maxRange.  If dest is less than minRange,
 * the nearest location at least minRange is returned.  If orig and dest are the
 * same and minRange is > 0, the return value will contain an undefined position.
 */
Vec2i UnitUpdater::getNear(const Vec2i &orig, Vec2i dest, int minRange, int maxRange, int destSize) {
	assert(minRange >= 0);
	assert(minRange <= maxRange);
	assert(destSize > 0);
	Vec2f fdest;

	if(destSize > 1) {
		// use a circular distance around the parameter
		int offset = destSize - 1;

		if (orig.x > dest.x + offset) {
			fdest.x = dest.x + offset;
		} else if(orig.x > dest.x) {
			fdest.x = orig.x;
		} else {
			fdest.x = dest.x;
		}

		if (orig.y > dest.y + offset) {
			fdest.y = dest.y + offset;
		} else if(orig.y > dest.y) {
			fdest.y = orig.y;
		} else {
			fdest.y = dest.y;
		}
	} else {
		fdest.x = dest.x;
		fdest.y = dest.y;
	}

	Vec2f forig(orig.x, orig.y);
	//Vec2f fdest(dest.x, dest.y);
	float len = forig.dist(fdest);

	if((int)len <= minRange && (int)len <= maxRange) {
		return orig;
	} else {
		// make sure we don't divide by zero, that's how this whole universe
		// thing got started in the first place (fyi, it wasn't a big bang).
		assert(minRange == 0 || orig.x != dest.x || orig.y != dest.y);
		Vec2f result = fdest.lerp((float)(len <= minRange ? minRange : maxRange) / len, forig);
		return Vec2i(result.x, result.y);
	}
}


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

	Vec2f floatCenter = Vec2f(center.x + ((float)centerSize - 1) / 2.f, center.y + ((float)centerSize - 1) / 2.f);
	int targetRange = 0x10000;
	Unit *target = *rangedPtr;
	range += centerSize / 2;

	if(target && target->isAlive() && target->isDamaged() && rct->isRepairableUnitType(target->getType())) {
		float rangeToTarget = floatCenter.dist(target->getFloatCenteredPos()) - (float)(centerSize + target->getSize()) / 2.0f;
		if((int)rangeToTarget <= range) {
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

ParticleDamager::ParticleDamager(Unit *attacker, UnitUpdater *unitUpdater, const GameCamera *gameCamera){
	this->gameCamera= gameCamera;
	this->attackerRef= attacker;
	this->ast= static_cast<const AttackSkillType*>(attacker->getCurrSkill());
	this->targetPos= attacker->getTargetPos();
	this->targetField= attacker->getTargetField();
	this->unitUpdater= unitUpdater;
}

void ParticleDamager::update(ParticleSystem *particleSystem){
	Unit *attacker= attackerRef.getUnit();

	if(attacker!=NULL){

		unitUpdater->hit(attacker, ast, targetPos, targetField);

		//play sound
		StaticSound *projSound= ast->getProjSound();
		if(particleSystem->getVisible() && projSound!=NULL){
			SoundRenderer::getInstance().playFx(projSound, attacker->getCurrVector(), gameCamera->getPos());
		}
	}
	particleSystem->setObserver(NULL);
	delete this;
}

}}//end namespace
