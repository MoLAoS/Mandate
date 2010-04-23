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
#include "command_type.h"

#include <algorithm>
#include <cassert>
#include <climits>

#include "unit_type.h"
#include "sound.h"
#include "util.h"
#include "graphics_interface.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "renderer.h"
#include "world.h"
#include "unit.h"
#include "sound_renderer.h"
#include "route_planner.h"
#include "cartographer.h"
#include "script_manager.h"
#include "game.h"

#include "leak_dumper.h"
#include "logger.h"
#include "sim_interface.h"

using namespace Glest::Sim;

using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
// 	class RepairCommandType
// =====================================================

//find a unit we can repair
/** rangedPtr should point to a pointer that is either NULL or a valid Unit */
bool RepairCommandType::repairableInRange(const Unit *unit, Vec2i centre, int centreSize,
		Unit **rangedPtr, const RepairCommandType *rct, const RepairSkillType *rst,
		int range, bool allowSelf, bool militaryOnly, bool damagedOnly) {
	_PROFILE_COMMAND_UPDATE();
	REPAIR_LOG2( 
		__FUNCTION__ << "(): with unit: " << *unit << " @ " << unit->getPos() << ", "
		<< " centre: " << centre << ", centreSize: " << centreSize;		
	);

	Targets repairables;
	fixedVec2 fixedCentre(centre.x + centreSize / fixed(2), centre.y + centreSize / fixed(2));
	fixed targetRange = fixed::max_int();
	Unit *target = *rangedPtr;
	range += centreSize / 2;

	if (target && target->isAlive() && target->isDamaged() && rct->canRepair(target->getType())) {
		fixed rangeToTarget = fixedCentre.dist(target->getFixedCenteredPos()) - (centreSize + target->getSize()) / fixed(2);
		REPAIR_LOG2( "\tRange check to target : " << (rangeToTarget > range ? "not " : "") << "in range." );

		if (rangeToTarget <= range) { // current target is in range
			return true;
		} else { // current target not in range
			return false;
		}
	}
	// no target unit, or target no longer of interest (dead or fully repaired)
	target = NULL;

	// nearby cells
	Vec2i pos;
	fixed distance;
	const Map* map = theWorld.getMap();
	PosCircularIteratorSimple pci(map->getBounds(), centre, range);
	while (pci.getNext(pos, distance)) {
		// all zones
		for (int z = 0; z < Zone::COUNT; z++) {
			Unit *candidate = map->getCell(pos)->getUnit(enum_cast<Field>(z));
	
			//is it a repairable?
			if (candidate && (allowSelf || candidate != unit)
			&& (!rst->isSelfOnly() || candidate == unit)
			&& candidate->isAlive() && unit->isAlly(candidate)
			&& (!rst->isPetOnly() || unit->isPet(candidate))
			&& (!damagedOnly || candidate->isDamaged())
			&& (!militaryOnly || candidate->getType()->hasCommandClass(CommandClass::ATTACK))
			&& rct->canRepair(candidate->getType())) {
				//record the nearest distance to target (target may be on multiple cells)
				repairables.record(candidate, distance);
			}
		}
	}
	// if no repairables or just one then it's a simple choice.
	if (repairables.empty()) {
		REPAIR_LOG2( "\tSearch found no targets." );
		return false;
	} else if (repairables.size() == 1) {
		*rangedPtr = repairables.begin()->first;
		REPAIR_LOG2( "\tSearch found single possible target. Unit: " 
			<< **rangedPtr << " @ " << (*rangedPtr)->getPos() );
		return true;
	}
	//heal cloesest ally that can attack (and are probably fighting) first.
	//if none, go for units that are less than 20%
	//otherwise, take the nearest repairable unit
	if(!(*rangedPtr = repairables.getNearestSkillClass(SkillClass::ATTACK))
	&& !(*rangedPtr = repairables.getNearestHpRatio(fixed(2) / 10))
	&& !(*rangedPtr = repairables.getNearest())) {
		REPAIR_LOG2( "\tSomething very odd happened..." );
		// this is unreachable, we've already established repaiables is not empty, so
		// getNearest() will always return something for us...
		return false;
	}
	REPAIR_LOG2( "\tSearch found " << repairables.size() << " possible targets. Selected Unit: " 
		<< **rangedPtr << " @ " << (*rangedPtr)->getPos() );
	return true;
}

bool RepairCommandType::repairableInRange(const Unit *unit, Unit **rangedPtr, 
		const RepairCommandType *rct, int range, bool allowSelf, bool militaryOnly, bool damagedOnly) {
	return repairableInRange(unit, unit->getPos(), unit->getType()->getSize(),
			rangedPtr, rct, rct->getRepairSkillType(), range, allowSelf, militaryOnly, damagedOnly);
}

bool RepairCommandType::repairableInSight(const Unit *unit, Unit **rangedPtr, 
							const RepairCommandType *rct, bool allowSelf) {
	return repairableInRange(unit, rangedPtr, rct, unit->getSight(), allowSelf);
}

bool RepairCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ft);

	//repair
	try {
		string skillName= n->getChild("repair-skill")->getAttribute("value")->getRestrictedValue();
		repairSkillType= static_cast<const RepairSkillType*>(unitType->getSkillType(skillName, SkillClass::REPAIR));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	//repaired units
	try {
		const XmlNode *unitsNode= n->getChild("repaired-units");
		for(int i=0; i<unitsNode->getChildCount(); ++i){
			const XmlNode *unitNode= unitsNode->getChild("unit", i);
			repairableUnits.push_back(ft->getUnitType(unitNode->getAttribute("name")->getRestrictedValue()));
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void RepairCommandType::doChecksum(Checksum &checksum) const {
	MoveBaseCommandType::doChecksum(checksum);
	checksum.add(repairSkillType->getName());
	for (int i=0; i < repairableUnits.size(); ++i) {
		checksum.add(repairableUnits[i]->getName());
	}
}

void RepairCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	repairSkillType->getDesc(str, unit);

	str+="\n" + lang.get("CanRepair") + ":\n";
	if(repairSkillType->isSelfOnly()) {
		str += lang.get("SelfOnly");
	} else if(repairSkillType->isPetOnly()) {
		str += lang.get("PetOnly");
	} else {
		for(int i=0; i<repairableUnits.size(); ++i){
			const UnitType *ut = (const UnitType*)repairableUnits[i];
			if(ut->isAvailableInSubfaction(unit->getFaction()->getSubfaction())) {
				str+= ut->getName()+"\n";
			}
		}
	}
}

//get
bool RepairCommandType::canRepair(const UnitType *unitType) const{
	for(int i=0; i<repairableUnits.size(); ++i){
		if(static_cast<const UnitType*>(repairableUnits[i])==unitType){
			return true;
		}
	}
	return false;
}

void RepairCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	const RepairSkillType * const &rst = repairSkillType;
	bool repairThisFrame = unit->getCurrSkill()->getClass() == SkillClass::REPAIR;
	Unit *repaired = command->getUnit();

	// If the unit I was supposed to repair died or is already fixed then finish
	if (repaired && (repaired->isDead() || !repaired->isDamaged())) {
		unit->setCurrSkill(SkillClass::STOP);
		unit->finishCommand();
		return;
	}

	Command *autoCmd;
	if (command->isAuto() && (autoCmd = doAutoCommand(unit))) {
		if (autoCmd->getType()->getClass() == CommandClass::ATTACK) {
			unit->giveCommand(autoCmd);
		}
	}

	if (repairableInRange(unit, &repaired, this, rst->getMaxRange(), rst->isSelfAllowed())) {
		unit->setTarget(repaired, true, true);
		unit->setCurrSkill(rst);
	} else {
		Vec2i targetPos;
		if (repairableInSight(unit, &repaired, this, rst->isSelfAllowed())) {
			if (repaired->isMobile()) {
				if (!theWorld.getMap()->getNearestFreePos(targetPos, unit, repaired, 1, rst->getMaxRange())) {
					unit->setCurrSkill(SkillClass::STOP);
					unit->finishCommand();
					return;
				}
			} else {
				targetPos = repaired->getPos() + Vec2i(repaired->getSize() / 2);
			}
			if (targetPos != unit->getTargetPos()) {
				unit->setTargetPos(targetPos);
				unit->getPath()->clear();
			}
		} else {
			// if no more damaged units and on auto command, turn around
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

		TravelState result;
		if (repaired && !repaired->isMobile()) {
			unit->setTargetPos(targetPos);
			result = theRoutePlanner.findPathToBuildSite(unit, repaired->getType(), repaired->getPos());
		} else {
			result = theRoutePlanner.findPath(unit, targetPos);
		}
		switch (result) {
			case TravelState::ARRIVED:
				if (repaired) {
					unit->setCurrSkill(rst);
				} else {
					unit->setCurrSkill(SkillClass::STOP);
					unit->finishCommand();
				}
				break;

			case TravelState::MOVING:
				unit->setCurrSkill(moveSkillType);
				unit->face(unit->getNextPos());
				break;

			case TravelState::BLOCKED:
				unit->setCurrSkill(SkillClass::STOP);
				if(unit->getPath()->isBlocked()){
					unit->setCurrSkill(SkillClass::STOP);
					unit->finishCommand();
					REPAIR_LOG( "Unit: " << unit->getId() << " path blocked, cancelling." );
				}
				break;

			case TravelState::IMPOSSIBLE:
				unit->setCurrSkill(SkillClass::STOP);
				unit->finishCommand();
				REPAIR_LOG( "Unit: " << unit->getId() << " path impossible, cancelling." );
				break;

			default: throw runtime_error("Error: RoutePlanner::findPath() returned invalid result.");
		}
	}

	if (repaired && !repaired->isDamaged()) {
		unit->setCurrSkill(SkillClass::STOP);
		unit->finishCommand();
	}

	if (repairThisFrame && unit->getCurrSkill()->getClass() == SkillClass::REPAIR) {
		//if repairing
		if (!repaired) {
			unit->setCurrSkill(SkillClass::STOP);
		} else {
			unit->setTarget(repaired, true, true);

			//shiney
			if (rst->getSplashParticleSystemType()) {
				const Tile *sc = theWorld.getMap()->getTile(Map::toTileCoords(repaired->getCenteredPos()));
				bool visible = sc->isVisible(theWorld.getThisTeamIndex());

				SplashParticleSystem *psSplash = rst->getSplashParticleSystemType()->createSplashParticleSystem();
				psSplash->setPos(repaired->getCurrVector());
				psSplash->setVisible(visible);
				theRenderer.manageParticleSystem(psSplash, rsGame);
			}

			bool wasBuilt = repaired->isBuilt();

			assert(repaired->isAlive() && repaired->getHp() > 0);

			if (repaired->repair(rst->getAmount(), rst->getMultiplier())) {
				unit->setCurrSkill(SkillClass::STOP);
				if (!wasBuilt) {
					//building finished
					ScriptManager::onUnitCreated(repaired);
					if (unit->getFactionIndex() == theWorld.getThisFactionIndex()) {
						// try to find finish build sound
						BuildCommandType *bct = (BuildCommandType *)unit->getType()->getFirstCtOfClass(CommandClass::BUILD);
						if (bct) {
							theSoundRenderer.playFx(bct->getBuiltSound(), 
								unit->getCurrVector(), theGame.getGameCamera()->getPos());
						}
					}
				}
			}
		}
	}
}

Command *RepairCommandType::doAutoRepair(Unit *unit) const {
	if (!unit->isAutoRepairEnabled() || !unit->getFaction()->isAvailable(this)) {
		return 0;
	}
	// look for someone to repair
	Unit *sighted = NULL;
	if (unit->getEp() >= repairSkillType->getEpCost()
	&& repairableInSight(unit, &sighted, this, repairSkillType->isSelfAllowed())) {
		REPAIR_LOG( __FUNCTION__ << "(): Unit:" << *unit << " @ " << unit->getPos()
			<< ", found someone (" << *sighted << ") to repair @ " << sighted->getPos() );
		Command *newCommand;
		
		Vec2i pos = Map::getNearestPos(unit->getPos(), sighted, repairSkillType->getMinRange(), repairSkillType->getMaxRange());
		REPAIR_LOG( "\tMap::getNearestPos(): " << pos );

		newCommand = new Command(this, CommandFlags(CommandProperties::QUEUE, CommandProperties::AUTO), pos);
		newCommand->setPos2(unit->getPos());
		return newCommand;
	}
	return 0;
}

// =====================================================
// 	class BuildCommandType
// =====================================================

BuildCommandType::~BuildCommandType(){
	deleteValues(builtSounds.getSounds().begin(), builtSounds.getSounds().end());
	deleteValues(startSounds.getSounds().begin(), startSounds.getSounds().end());
}

bool BuildCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ft);

	//build
	try {
		string skillName= n->getChild("build-skill")->getAttribute("value")->getRestrictedValue();
		buildSkillType= static_cast<const BuildSkillType*>(unitType->getSkillType(skillName, SkillClass::BUILD));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	//buildings built
	try {
		const XmlNode *buildingsNode= n->getChild("buildings");
		for(int i=0; i<buildingsNode->getChildCount(); ++i){
			const XmlNode *buildingNode= buildingsNode->getChild("building", i);
			string name= buildingNode->getAttribute("name")->getRestrictedValue();
			buildings.push_back(ft->getUnitType(name));
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//start sound
	try { 
		const XmlNode *startSoundNode= n->getChild("start-sound");
		if(startSoundNode->getAttribute("enabled")->getBoolValue()){
			startSounds.resize(startSoundNode->getChildCount());
			for(int i=0; i<startSoundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode= startSoundNode->getChild("sound-file", i);
				string path= soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				startSounds[i]= sound;
			}
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//built sound
	try {
		const XmlNode *builtSoundNode= n->getChild("built-sound");
		if(builtSoundNode->getAttribute("enabled")->getBoolValue()){
			builtSounds.resize(builtSoundNode->getChildCount());
			for(int i=0; i<builtSoundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode= builtSoundNode->getChild("sound-file", i);
				string path= soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				builtSounds[i]= sound;
			}
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void BuildCommandType::doChecksum(Checksum &checksum) const {
	MoveBaseCommandType::doChecksum(checksum);
	checksum.add(buildSkillType->getName());
	for (int i=0; i < buildings.size(); ++i) {
		checksum.add(buildings[i]->getName());
	}
}

const string cmdCancelMsg = " Command cancelled.";

void BuildCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	const UnitType *builtUnitType = command->getUnitType();
	Unit *builtUnit = NULL;
	Unit *target = unit->getTarget();

	Map *map = theWorld.getMap();

	BUILD_LOG( 
		__FUNCTION__ << " : Updating unit " << unit->getId() << ", building type = " << builtUnitType->getName()
		<< ", command target = " << command->getPos();
	);

	if (unit->getCurrSkill()->getClass() != SkillClass::BUILD) {
		// if not building
		// this is just a 'search target', the SiteMap will tell the search when/where it has found a goal cell
		Vec2i targetPos = command->getPos() + Vec2i(builtUnitType->getSize() / 2);
		unit->setTargetPos(targetPos);
		switch (theRoutePlanner.findPathToBuildSite(unit, builtUnitType, command->getPos())) {
			case TravelState::MOVING:
				unit->setCurrSkill(this->getMoveSkillType());
				unit->face(unit->getNextPos());
				BUILD_LOG( "Moving." );
				return;

			case TravelState::BLOCKED:
				unit->setCurrSkill(SkillClass::STOP);
				if(unit->getPath()->isBlocked()) {
					theConsole.addStdMessage("Blocked");
					unit->cancelCurrCommand();
					BUILD_LOG( "Blocked." << cmdCancelMsg );
				}
				return;

			case TravelState::ARRIVED:
				break;

			case TravelState::IMPOSSIBLE:
				theConsole.addStdMessage("Unreachable");
				unit->cancelCurrCommand();
				BUILD_LOG( "Route impossible," << cmdCancelMsg );
				return;

			default: throw runtime_error("Error: RoutePlanner::findPath() returned invalid result.");
		}

		// if arrived destination
		assert(command->getUnitType() != NULL);
		const int &buildingSize = builtUnitType->getSize();

		if (map->canOccupy(command->getPos(), Field::LAND, builtUnitType)) {
			// late resource allocation
			if (!command->isReserveResources()) {
				command->setReserveResources(true);
				if (unit->checkCommand(*command) != CommandResult::SUCCESS) {
					if (unit->getFactionIndex() == theWorld.getThisFactionIndex()) {
						theConsole.addStdMessage("BuildingNoRes");
					}
					BUILD_LOG( "in positioin, late resource allocation failed." << cmdCancelMsg );
					unit->finishCommand();
					return;
				}
				unit->getFaction()->applyCosts(command->getUnitType());
			}

			BUILD_LOG( "in position, starting construction." );
			builtUnit = new Unit(theWorld.getNextUnitId(), command->getPos(), builtUnitType, unit->getFaction(), theWorld.getMap());
			builtUnit->create();
			unit->setCurrSkill(this->getBuildSkillType());
			unit->setTarget(builtUnit, true, true);
			map->prepareTerrain(builtUnit);
			command->setUnit(builtUnit);
			unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), false);

			if (!builtUnit->isMobile()) {
				theWorld.getCartographer()->updateMapMetrics(builtUnit->getPos(), builtUnit->getSize());
			}
			
			//play start sound
			if (unit->getFactionIndex() == theWorld.getThisFactionIndex()) {
				SoundRenderer::getInstance().playFx(this->getStartSound(), unit->getCurrVector(), theGame.getGameCamera()->getPos());
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
				unit->setCurrSkill(this->getBuildSkillType());
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
				if (unit->getFactionIndex() == theWorld.getThisFactionIndex()) {
					theConsole.addStdMessage("BuildingNoPlace");
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
			if (unit->getFactionIndex() == theWorld.getThisFactionIndex()) {
				SoundRenderer::getInstance().playFx(
					this->getBuiltSound(),
					unit->getCurrVector(),
					theGame.getGameCamera()->getPos());
			}
		}
	}
}


// =====================================================
// 	class HarvestCommandType
// =====================================================

bool HarvestCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ft);

	string skillName;
	//harvest
	try {
		skillName= n->getChild("harvest-skill")->getAttribute("value")->getRestrictedValue();
		harvestSkillType= static_cast<const HarvestSkillType*>(unitType->getSkillType(skillName, SkillClass::HARVEST));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	//stop loaded
	try { 
		skillName= n->getChild("stop-loaded-skill")->getAttribute("value")->getRestrictedValue();
		stopLoadedSkillType= static_cast<const StopSkillType*>(unitType->getSkillType(skillName, SkillClass::STOP));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//move loaded
	try {
		skillName= n->getChild("move-loaded-skill")->getAttribute("value")->getRestrictedValue();
		moveLoadedSkillType= static_cast<const MoveSkillType*>(unitType->getSkillType(skillName, SkillClass::MOVE));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	//resources can harvest
	try { 
		const XmlNode *resourcesNode= n->getChild("harvested-resources");
		for(int i=0; i<resourcesNode->getChildCount(); ++i){
			const XmlNode *resourceNode= resourcesNode->getChild("resource", i);
			harvestedResources.push_back(tt->getResourceType(resourceNode->getAttribute("name")->getRestrictedValue()));
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	try { maxLoad= n->getChild("max-load")->getAttribute("value")->getIntValue(); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	try { hitsPerUnit= n->getChild("hits-per-unit")->getAttribute("value")->getIntValue(); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void HarvestCommandType::doChecksum(Checksum &checksum) const {
	MoveBaseCommandType::doChecksum(checksum);
	checksum.add(moveLoadedSkillType->getName());
	checksum.add(harvestSkillType->getName());
	checksum.add(stopLoadedSkillType->getName());
	for (int i=0; i < harvestedResources.size(); ++i) {
		checksum.add(harvestedResources[i]->getName());
	}
	checksum.add<int>(maxLoad);
	checksum.add<int>(hitsPerUnit);
}

void HarvestCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	str+= lang.get("HarvestSpeed")+": "+ intToStr(harvestSkillType->getSpeed()/hitsPerUnit);
	EnhancementType::describeModifier(str, (unit->getSpeed(harvestSkillType) - harvestSkillType->getSpeed())/hitsPerUnit);
	str+= "\n";
	str+= lang.get("MaxLoad")+": "+ intToStr(maxLoad)+"\n";
	str+= lang.get("LoadedSpeed")+": "+ intToStr(moveLoadedSkillType->getSpeed())+"\n";
	if(harvestSkillType->getEpCost()!=0){
		str+= lang.get("EpCost")+": "+intToStr(harvestSkillType->getEpCost())+"\n";
	}
	str+=lang.get("Resources")+":\n";
	for(int i=0; i<getHarvestedResourceCount(); ++i){
		str+= getHarvestedResource(i)->getName()+"\n";
	}
}

bool HarvestCommandType::canHarvest(const ResourceType *resourceType) const{
	return find(harvestedResources.begin(), harvestedResources.end(), resourceType) != harvestedResources.end();
}


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

const int maxResSearchRadius= 10;

/// looks for a resource of a type that hct can harvest, searching from command target pos
/// @return pointer to Resource if found (unit's command pos will have been re-set).
/// NULL if no resource was found within UnitUpdater::maxResSearchRadius.
Resource* searchForResource(Unit *unit, const HarvestCommandType *hct) {
	Vec2i pos;
	Map *map = theWorld.getMap();

	PosCircularIteratorOrdered pci(map->getBounds(), unit->getCurrCommand()->getPos(),
			theWorld.getPosIteratorFactory().getInsideOutIterator(1, maxResSearchRadius));

	while (pci.getNext(pos)) {
		Resource *r = map->getTile(Map::toTileCoords(pos))->getResource();
		if (r && hct->canHarvest(r->getType())) {
			unit->getCurrCommand()->setPos(pos);
			return r;
		}
	}
	return 0;
}

void HarvestCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Vec2i targetPos;
	Map *map = theWorld.getMap();

	Tile *tile = map->getTile(Map::toTileCoords(unit->getCurrCommand()->getPos()));
	Resource *res = tile->getResource();
	if (!res) { // reset command pos, but not Unit::targetPos
		if (!(res = searchForResource(unit, this))) {
			unit->finishCommand();
			unit->setCurrSkill(SkillClass::STOP);
			return;
		}
	}

	if (unit->getCurrSkill()->getClass() != SkillClass::HARVEST) { // if not working
		if  (!unit->getLoadCount() || (unit->getLoadType() == res->getType()
		&& unit->getLoadCount() < this->getMaxLoad() / 2)) {
			// if current load is correct resource type and not more than half loaded, go for resources
			if (res && this->canHarvest(res->getType())) {
				switch (theRoutePlanner.findPathToResource(unit, command->getPos(), res->getType())) {
					case TravelState::ARRIVED:
						if (map->isResourceNear(unit->getPos(), res->getType(), targetPos)) {
							// if it finds resources it starts harvesting
							unit->setCurrSkill(this->getHarvestSkillType());
							unit->setTargetPos(targetPos);
							command->setPos(targetPos);
							unit->face(targetPos);
							unit->setLoadType(map->getTile(Map::toTileCoords(targetPos))->getResource()->getType());
						}
						break;
					case TravelState::MOVING:
						unit->setCurrSkill(this->getMoveSkillType());
						unit->face(unit->getNextPos());
						break;
					default:
						unit->setCurrSkill(SkillClass::STOP);
						break;
				}
			} else { // if can't harvest, search for another resource
				unit->setCurrSkill(SkillClass::STOP);
				if (!searchForResource(unit, this)) {
					unit->finishCommand();
					//FIXME don't just stand around at the store!!
					//insert move command here.
				}
			}
		} else { // if load is wrong type, or more than half loaded, return to store
			Unit *store = theWorld.nearestStore(unit->getPos(), unit->getFaction()->getIndex(), unit->getLoadType());
			if (store) {
				switch (theRoutePlanner.findPathToStore(unit, store)) {
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
						resourceAmount = (int)(resourceAmount * theSimInterface->getGameSettings().getResourceMultilpier(unit->getFactionIndex()));
						//resourceAmount *= ultraRes/*/*/*/*/ourceFactor; // Pull from GameSettings
					}
					unit->getFaction()->incResourceAmount(unit->getLoadType(), resourceAmount);
					theSimInterface->getStats()->harvest(unit->getFactionIndex(), resourceAmount);
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
			if (!this->canHarvest(res->getType())) { // wrong resource type, command changed
				unit->setCurrSkill(getStopLoadedSkill(unit));
				unit->getPath()->clear();
				return;
			}
			unit->update2();
			if (unit->getProgress2() >= this->getHitsPerUnit()) {
				unit->setProgress2(0);
				if (unit->getLoadCount() < this->getMaxLoad()) {
					unit->setLoadCount(unit->getLoadCount() + 1);
				}
				// if resource exausted, then delete it and stop (and let the cartographer know)
				if (res->decAmount(1)) {
					Vec2i rPos = res->getPos();
					tile->deleteResource();
					theWorld.getCartographer()->updateMapMetrics(rPos, GameConstants::cellScale);
					unit->setCurrSkill(getStopLoadedSkill(unit));
				}
				if (unit->getLoadCount() == this->getMaxLoad()) {
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

}}
