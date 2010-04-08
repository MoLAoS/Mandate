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

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Glest::Game::Search;

namespace Glest { namespace Game {

#if LOG_BUILD_COMMAND
#	define BUILD_LOG(x) STREAM_LOG(x)
#else
#	define BUILD_LOG(x)
#endif

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

	//REFACTOR: these checks can go in Unit::update()

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

		//REFACOTR: Unit::update() will be called by World::update()
		//	move the rest of this to World::update()

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
// AutoAttack => AttackCommandTypeBase::doAutoAttack()
// AutoFlee   => MoveBaseCommandType::doAutoFlee()
// AutoRepair => RepairCommandType::doAutoRepair()

// ==================== updateStop ====================
// gone to StopCommandType::update()

// ==================== updateMove ====================
// gone to MoveCommand::update()

// ==================== updateGenericAttack ====================
// gone to AttackCommandType::updateGeneric

// ==================== updateAttack ====================
// gone to AttackCommandType::update()

// ==================== updateAttackStopped ====================
// gone to AttackStoppedCommandType::update()


// ==================== updateBuild ====================
/// 0: start, 1: do nothing (blocked move or waiting for site to clear) 2: build, 3: move to site
const string cmdCancelMsg = " Command cancelled.";

void UnitUpdater::updateBuild(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const BuildCommandType *bct = static_cast<const BuildCommandType*>(command->getType());
	const UnitType *builtUnitType = command->getUnitType();
	Unit *builtUnit = NULL;
	Unit *target = unit->getTarget();

	assert(command->getUnitType());

	BUILD_LOG( 
		__FUNCTION__ << " : Updating unit " << unit->getId() << ", building type = " << builtUnitType->getName()
		<< ", command target = " << command->getPos();
	);

	if (unit->getCurrSkill()->getClass() != SkillClass::BUILD) {
		// if not building
		// this is just a 'search target', the SiteMap will tell the search when/where it has found a goal cell
		Vec2i targetPos = command->getPos() + Vec2i(builtUnitType->getSize() / 2);
		unit->setTargetPos(targetPos);
		switch (routePlanner->findPathToBuildSite(unit, builtUnitType, command->getPos())) {
			case TravelState::MOVING:
				unit->setCurrSkill(bct->getMoveSkillType());
				unit->face(unit->getNextPos());
				BUILD_LOG( "Moving." );
				return;

			case TravelState::BLOCKED:
				unit->setCurrSkill(SkillClass::STOP);
				if(unit->getPath()->isBlocked()) {
					console->addStdMessage("Blocked");
					unit->cancelCurrCommand();
					BUILD_LOG( "Blocked." << cmdCancelMsg );
				}
				return;

			case TravelState::ARRIVED:
				break;

			case TravelState::IMPOSSIBLE:
				console->addStdMessage("Unreachable");
				unit->cancelCurrCommand();
				BUILD_LOG( "Route impossible," << cmdCancelMsg );
				return;

			default: throw runtime_error("Error: RoutePlanner::findPath() returned invalid result.");
		}

		// if arrived destination
		assert(command->getUnitType() != NULL);
		const int &buildingSize = builtUnitType->getSize();

		if (map->canOccupy(command->getPos(), Field::LAND, builtUnitType)) {
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
					BUILD_LOG( "in positioin, late resource allocation failed." << cmdCancelMsg );
					unit->finishCommand();
					return;
				}
				unit->getFaction()->applyCosts(command->getUnitType());
			}

			BUILD_LOG( "in position, starting construction." );
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
				// if we pre-reserved the resources, we have to deallocate them now
				if (command->isReserveResources()) {
					unit->getFaction()->deApplyCosts(command->getUnitType());
					command->setReserveResources(false);
				}
				unit->setTarget(builtUnit, true, true);
				unit->setCurrSkill(bct->getBuildSkillType());
				command->setUnit(builtUnit);
				BUILD_LOG( "in position, building already under construction." );

			} else {
				// is it not free because there are units in the way?
				if (occupants.size()) {
					// Can they get the fuck out of the way?
					vector<Unit *>::const_iterator i;
					for (i = occupants.begin();
							i != occupants.end() && (*i)->getType()->hasSkillClass(SkillClass::MOVE); ++i) ;
					if (i == occupants.end()) {
						BUILD_LOG( "in position, site blocked, waiting for people to get out of the way." );
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
				BUILD_LOG( "in position, site blocked." << cmdCancelMsg );
			}
		}
	} else {
		//if building
		BUILD_LOG( "building." );
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
//REFACTOR: These helper functions can probably be completely removed once UnitType::commandTypesByClass
// and the interface to it is implemented, these checks can be done in UnitUpdater::updateHarvest() 
// [which by then might be HarvestCommandType::update() instead ;)]

// hacky helper for !HarvestCommandType::canHarvest(u->getLoadType()) animanition issue
const MoveSkillType* getMoveLoadedSkill(Unit *u) {
	const MoveSkillType *mst = 0;

	//REFACTOR: this, and _lots_of_existing_similar_code_ is to be replaced
	// once UnitType::commandTypesByclass is implemented, to look and be less shit.
	for (int i=0; i < u->getType()->getCommandTypeCount(); ++i) {
		if (u->getType()->getCommandType(i)->getClass() == CommandClass::HARVEST) {
			const HarvestCommandType *t = 
				static_cast<const HarvestCommandType*>(u->getType()->getCommandType(i));
			if (t->canHarvest(u->getLoadType())) {
				mst = t->getMoveLoadedSkillType();
				break;
			}
		}
	}
	return mst;
}

// hacky helper for !HarvestCommandType::canHarvest(u->getLoadType()) animanition issue
const StopSkillType* getStopLoadedSkill(Unit *u) {
	const StopSkillType *sst = 0;

	//REFACTOR: this, and lots of existing similar code is to be replaced
	// once UnitType::commandTypesByclass is implemented, to look and be less shit.
	for (int i=0; i < u->getType()->getCommandTypeCount(); ++i) {
		if (u->getType()->getCommandType(i)->getClass() == CommandClass::HARVEST) {
			const HarvestCommandType *t = 
				static_cast<const HarvestCommandType*>(u->getType()->getCommandType(i));
			if (t->canHarvest(u->getLoadType())) {
				sst = t->getStopLoadedSkillType();
				break;
			}
		}
	}
	return sst;
}

/// looks for a resource of a type that hct can harvest, searching from command target pos
/// @return pointer to Resource if found (unit's command pos will have been re-set).
/// NULL if no resource was found within UnitUpdater::maxResSearchRadius.
Resource* UnitUpdater::searchForResource(Unit *unit, const HarvestCommandType *hct) {
	Vec2i pos;

	PosCircularIteratorOrdered pci(map->getBounds(), unit->getCurrCommand()->getPos(),
			world->getPosIteratorFactory().getInsideOutIterator(1, maxResSearchRadius));

	while (pci.getNext(pos)) {
		Resource *r = map->getTile(Map::toTileCoords(pos))->getResource();
		if (r && hct->canHarvest(r->getType())) {
			unit->getCurrCommand()->setPos(pos);
			return r;
		}
	}
	return 0;
}

void UnitUpdater::updateHarvest(Unit *unit) {
	Command *command = unit->getCurrCommand();
	const HarvestCommandType *hct = static_cast<const HarvestCommandType*>(command->getType());
	Vec2i targetPos;

	Tile *tile = map->getTile(Map::toTileCoords(unit->getCurrCommand()->getPos()));
	Resource *res = tile->getResource();
	if (!res) { // reset command pos, but not Unit::targetPos
		if (!(res = searchForResource(unit, hct))) {
			unit->finishCommand();
			unit->setCurrSkill(SkillClass::STOP);
			return;
		}
	}

	if (unit->getCurrSkill()->getClass() != SkillClass::HARVEST) { // if not working
		if  (!unit->getLoadCount() || (unit->getLoadType() == res->getType()
		&& unit->getLoadCount() < hct->getMaxLoad() / 2)) {
			// if current load is correct resource type and not more than half loaded, go for resources
			if (res && hct->canHarvest(res->getType())) {
				switch (routePlanner->findPathToResource(unit, command->getPos(), res->getType())) {
					case TravelState::ARRIVED:
						if (map->isResourceNear(unit->getPos(), res->getType(), targetPos)) {
							// if it finds resources it starts harvesting
							unit->setCurrSkill(hct->getHarvestSkillType());
							unit->setTargetPos(targetPos);
							command->setPos(targetPos);
							unit->face(targetPos);
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
			} else { // if can't harvest, search for another resource
				unit->setCurrSkill(SkillClass::STOP);
				if (!searchForResource(unit, hct)) {
					unit->finishCommand();
					//FIXME don't just stand around at the store!!
					//insert move command here.
				}
			}
		} else { // if load is wrong type, or more than half loaded, return to store
			Unit *store = world->nearestStore(unit->getPos(), unit->getFaction()->getIndex(), unit->getLoadType());
			if (store) {
				switch (routePlanner->findPathToStore(unit, store)) {
					case TravelState::MOVING:
						unit->setCurrSkill(getMoveLoadedSkill(unit));
						unit->face(unit->getNextPos());
						break;
					case TravelState::BLOCKED:
						unit->setCurrSkill(getStopLoadedSkill(unit));
						break;
					default:
						; // otherwise, we fall through
				}
				if (map->isNextTo(unit->getPos(), store)) {
					// update resources
					int resourceAmount = unit->getLoadCount();
					// Just do this for all players ???
					if (unit->getFaction()->getCpuUltraControl()) {
						resourceAmount = (int)(resourceAmount * gameSettings.getResourceMultilpier(unit->getFactionIndex()));
						//resourceAmount *= ultraRes/*/*/*/*/ourceFactor; // Pull from GameSettings
					}
					unit->getFaction()->incResourceAmount(unit->getLoadType(), resourceAmount);
					world->getStats().harvest(unit->getFactionIndex(), resourceAmount);
					ScriptManager::onResourceHarvested();

					// if next to a store unload resources
					unit->getPath()->clear();
					unit->setCurrSkill(SkillClass::STOP);
					unit->setLoadCount(0);
				}
			} else { // no store found, give up
				unit->finishCommand();
			}
		}
	} else { // if working
		res = map->getTile(Map::toTileCoords(unit->getTargetPos()))->getResource();
		if (res) { // if there is a resource, continue working, until loaded
			if (!hct->canHarvest(res->getType())) { // wrong resource type, command changed
				unit->setCurrSkill(getStopLoadedSkill(unit));
				unit->getPath()->clear();
				return;
			}
			unit->update2();
			if (unit->getProgress2() >= hct->getHitsPerUnit()) {
				unit->setProgress2(0);
				if (unit->getLoadCount() < hct->getMaxLoad()) {
					unit->setLoadCount(unit->getLoadCount() + 1);
				}
				// if resource exausted, then delete it and stop (and let the cartographer know)
				if (res->decAmount(1)) {
					Vec2i rPos = res->getPos();
					tile->deleteResource();
					world->getCartographer()->updateMapMetrics(rPos, GameConstants::cellScale);
					unit->setCurrSkill(getStopLoadedSkill(unit));
				}
				if (unit->getLoadCount() == hct->getMaxLoad()) {
					unit->setCurrSkill(getStopLoadedSkill(unit));
					unit->getPath()->clear();
				}
			}
		} else { // if there is no resource
			if (unit->getLoadCount()) {
				unit->setCurrSkill(getStopLoadedSkill(unit));
			} else {
				unit->finishCommand();
				unit->setCurrSkill(SkillClass::STOP);
			}
		}
	}
}

// ==================== updateRepair ====================
// gone to RepairCommandType::update()

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
// gone to GuardCommandType::update()

// ==================== updatePatrol ====================
// gone to PatrolCommandType::update()


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
