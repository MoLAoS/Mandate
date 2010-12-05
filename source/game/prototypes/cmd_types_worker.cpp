// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//                2008-2009 Daniel Santos
//                2009-2010 Nathan Turner
//                2009-2010 James McCulloch
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

namespace Glest { namespace ProtoTypes {

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
	const Map* map = g_world.getMap();
	PosCircularIteratorSimple pci(map->getBounds(), centre, range);
	while (pci.getNext(pos, distance)) {
		// all zones
		for (int z = 0; z < Zone::COUNT; z++) {
			Unit *candidate = map->getCell(pos)->getUnit(enum_cast<Field>(z));
	
			//is it a repairable?
			if (candidate && (allowSelf || candidate != unit)
			&& (!rst->isSelfOnly() || candidate == unit)
			&& candidate->isAlive() && unit->isAlly(candidate)
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
		g_errorLog.addXmlError(dir, e.what ());
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
		g_errorLog.addXmlError(dir, e.what ());
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
				if (!g_world.getMap()->getNearestFreePos(targetPos, unit, repaired, 1, rst->getMaxRange())) {
					unit->setCurrSkill(SkillClass::STOP);
					unit->finishCommand();
					return;
				}
			} else {
				targetPos = repaired->getPos() + Vec2i(repaired->getSize() / 2);
			}
			if (targetPos != unit->getTargetPos()) {
				unit->setTargetPos(targetPos);
				unit->clearPath();
			}
		} else {
			// if no more damaged units and on auto command, turn around
			if (command->isAuto() && command->hasPos2()) {
				if (Config::getInstance().getGsAutoReturnEnabled()) {
					command->popPos();
					unit->clearPath();
				} else {
					unit->finishCommand();
				}
			}
			targetPos = command->getPos();
		}

		TravelState result;
		if (repaired && !repaired->isMobile()) {
			unit->setTargetPos(targetPos);
			result = g_routePlanner.findPathToBuildSite(unit, repaired->getType(), repaired->getPos(), repaired->getModelFacing());
		} else {
			result = g_routePlanner.findPath(unit, targetPos);
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
				unit->setCurrSkill(m_moveSkillType);
				unit->face(unit->getNextPos());
				break;

			case TravelState::BLOCKED:
				unit->setCurrSkill(SkillClass::STOP);
				if(unit->getPath()->isBlocked()){
					unit->setCurrSkill(SkillClass::STOP);
					unit->clearPath();
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
				const Tile *sc = g_world.getMap()->getTile(Map::toTileCoords(repaired->getCenteredPos()));
				bool visible = sc->isVisible(g_world.getThisTeamIndex()) 
					&& g_renderer.getCuller().isInside(repaired->getCenteredPos());

				Splash *psSplash = rst->getSplashParticleSystemType()->createSplashParticleSystem(visible);
				psSplash->setPos(repaired->getCurrVector());
				g_renderer.manageParticleSystem(psSplash, ResourceScope::GAME);
			}

			bool wasBuilt = repaired->isBuilt();

			assert(repaired->isAlive() && repaired->getHp() > 0);

			if (repaired->repair(rst->getAmount(), rst->getMultiplier())) {
				unit->setCurrSkill(SkillClass::STOP);
				if (!wasBuilt) {
					//building finished
					ScriptManager::onUnitCreated(repaired);
					if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
						// try to find finish build sound
						BuildCommandType *bct = (BuildCommandType *)unit->getType()->getFirstCtOfClass(CommandClass::BUILD);
						if (bct) {
							RUNTIME_CHECK(!unit->isCarried());
							g_soundRenderer.playFx(bct->getBuiltSound(), 
								unit->getCurrVector(), g_gameState.getGameCamera()->getPos());
						}
					}
				}
			}
		}
	}
}

Command *RepairCommandType::doAutoRepair(Unit *unit) const {
	if (!unit->isAutoCmdEnabled(AutoCmdFlag::REPAIR) || !unit->getFaction()->isAvailable(this)) {
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
	deleteValues(m_builtSounds.getSounds().begin(), m_builtSounds.getSounds().end());
	deleteValues(m_startSounds.getSounds().begin(), m_startSounds.getSounds().end());
}

bool BuildCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ft);

	//build
	try {
		string skillName = n->getChild("build-skill")->getAttribute("value")->getRestrictedValue();
		m_buildSkillType = static_cast<const BuildSkillType*>(unitType->getSkillType(skillName, SkillClass::BUILD));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	//buildings built
	try {
		const XmlNode *buildingsNode = n->getChild("buildings");
		for(int i=0; i<buildingsNode->getChildCount(); ++i){
			const XmlNode *buildingNode = buildingsNode->getChild("building", i);
			string name = buildingNode->getAttribute("name")->getRestrictedValue();
			m_buildings.push_back(ft->getUnitType(name));
			try {
				m_tipKeys[name] = buildingNode->getRestrictedAttribute("tip");
			} catch (runtime_error &e) {
				m_tipKeys[name] = "";
		}
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}

	//start sound
	try { 
		const XmlNode *startSoundNode = n->getChild("start-sound");
		if(startSoundNode->getAttribute("enabled")->getBoolValue()){
			m_startSounds.resize(startSoundNode->getChildCount());
			for(int i = 0; i < startSoundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode = startSoundNode->getChild("sound-file", i);
				string path = soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound = new StaticSound();
				sound->load(dir + "/" + path);
				m_startSounds[i] = sound;
			}
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}

	//built sound
	try {
		const XmlNode *builtSoundNode= n->getChild("built-sound");
		if(builtSoundNode->getAttribute("enabled")->getBoolValue()){
			m_builtSounds.resize(builtSoundNode->getChildCount());
			for(int i=0; i<builtSoundNode->getChildCount(); ++i){
				const XmlNode *soundFileNode= builtSoundNode->getChild("sound-file", i);
				string path= soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				m_builtSounds[i]= sound;
			}
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void BuildCommandType::doChecksum(Checksum &checksum) const {
	MoveBaseCommandType::doChecksum(checksum);
	checksum.add(m_buildSkillType->getName());
	for (int i=0; i < m_buildings.size(); ++i) {
		checksum.add(m_buildings[i]->getName());
	}
}

const ProducibleType *BuildCommandType::getProduced(int i) const {
	return m_buildings[i];
}


const string cmdCancelMsg = " Command cancelled.";

void BuildCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	const UnitType *builtUnitType = static_cast<const UnitType*>(command->getProdType());

	BUILD_LOG( 
		__FUNCTION__ << " : Updating unit " << unit->getId() << ", building type = " << builtUnitType->getName()
		<< ", command target = " << command->getPos();
	);

	if (unit->getCurrSkill()->getClass() != SkillClass::BUILD) {
		if (!hasArrived(unit, command, builtUnitType)) {
			return; // not there yet, try next update
		}

		// the blocking checks are done before the command is issued and again when the unit arrives

		Map *map = g_world.getMap();
		if (map->canOccupy(command->getPos(), builtUnitType->getField(), builtUnitType, command->getFacing())) {
			acceptBuild(unit, command, builtUnitType);
		} else {
			// there are no free cells
			vector<Unit *> occupants;
			map->getOccupants(occupants, command->getPos(), builtUnitType->getSize(), Zone::LAND);
			
			// is construction already under way?
			Unit *builtUnit = occupants.size() == 1 ? occupants[0] : NULL;
			if (builtUnit && builtUnit->getType() == builtUnitType && !builtUnit->isBuilt()) {
				existingBuild(unit, command, builtUnit);
			} else if (occupants.size() && attemptMoveUnits(occupants)) {
				return; // success, wait for units to move
			} else {
				blockedBuild(unit);
			}
		}
	} else {
		continueBuild(unit, command, builtUnitType);
	}
}

bool BuildCommandType::isBlocked(const UnitType *builtUnitType, const Vec2i &pos, CardinalDir face) const {
	bool blocked = false;
	
	Map *map = g_world.getMap();
	if (!map->canOccupy(pos, builtUnitType->getField(), builtUnitType, face)) {
		// there are no free cells
		vector<Unit *> occupants;
		map->getOccupants(occupants, pos, builtUnitType->getSize(), Zone::LAND);
		
		// if no existing construction and can't move units then is blocked
		Unit *builtUnit = occupants.size() == 1 ? occupants[0] : NULL;
		if (builtUnit && builtUnit->getType() == builtUnitType && !builtUnit->isBuilt()) {
			// existing build
		} else if (occupants.size() && attemptMoveUnits(occupants)) {
			// units can move
		} else {
			blocked = true;
		}
	}

	return blocked;
}

// --- Private ---

bool BuildCommandType::hasArrived(Unit *unit, const Command *command, const UnitType *builtUnitType) const {
	// if not building
	// this is just a 'search target', the SiteMap will tell the search when/where it has found a goal cell
	Vec2i targetPos = command->getPos() + Vec2i(builtUnitType->getSize() / 2); /// @todo replace with getHalfSize()? -hail 13June2010
	unit->setTargetPos(targetPos);
	bool arrived = false;

	switch (g_routePlanner.findPathToBuildSite(unit, builtUnitType, command->getPos(), command->getFacing())) {
		case TravelState::MOVING:
			unit->setCurrSkill(this->getMoveSkillType());
			unit->face(unit->getNextPos());
			BUILD_LOG( "Moving." );
			break;

		case TravelState::BLOCKED:
			unit->setCurrSkill(SkillClass::STOP);
			if(unit->getPath()->isBlocked()) {
				g_console.addStdMessage("Blocked");
				unit->clearPath();
				unit->cancelCurrCommand();
				BUILD_LOG( "Blocked." << cmdCancelMsg );
			}
			break;

		case TravelState::ARRIVED:
			arrived = true;
			break;

		case TravelState::IMPOSSIBLE:
			g_console.addStdMessage("Unreachable");
			unit->cancelCurrCommand();
			BUILD_LOG( "Route impossible," << cmdCancelMsg );
			break;

		default:
			throw runtime_error("Error: RoutePlanner::findPath() returned invalid result.");
	}

	return arrived;
}

void BuildCommandType::existingBuild(Unit *unit, Command *command, Unit *builtUnit) const {
	// if we pre-reserved the resources, we have to deallocate them now
	if (command->isReserveResources()) {
		unit->getFaction()->deApplyCosts(static_cast<const UnitType*>(command->getProdType()));
		command->setReserveResources(false);
	}
	unit->setTarget(builtUnit, true, true);
	unit->setCurrSkill(this->getBuildSkillType());
	command->setUnit(builtUnit);
	BUILD_LOG( "in position, building already under construction." );
}

bool BuildCommandType::attemptMoveUnits(const vector<Unit *> &occupants) const {
	// Can they get the fuck out of the way?
	vector<Unit *>::const_iterator i;
	for (i = occupants.begin();	i != occupants.end(); ++i) {
		// NOTE: this might be useful to move units but not to test if they are already moving. If
		// that's not meant to be the case then it's a bug.
		//if (!(*i)->getType()->hasSkillClass(SkillClass::MOVE)) {
		if ((*i)->getCurrSkill()->getClass() != SkillClass::MOVE) {
			return false;
		}
	}
	BUILD_LOG( "in position, site blocked, waiting for people to get out of the way." );
	// they all have a move command, so we'll wait
	return true;

	/// @TODO: Check for idle units and tell them to get the fuck out of the way.
	/// @TODO: Possibly add a unit notification to let player know builder is waiting
}

void BuildCommandType::blockedBuild(Unit *unit) const {
	// blocked by non-moving units, surface objects (trees, rocks, etc.) or build area
	// contains deeply submerged terain
	unit->cancelCurrCommand();
	unit->setCurrSkill(SkillClass::STOP);
	if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
		g_console.addStdMessage("BuildingNoPlace");
	}
	BUILD_LOG( "site blocked." << cmdCancelMsg );
}

void BuildCommandType::acceptBuild(Unit *unit, Command *command, const UnitType *builtUnitType) const {
	Map *map = g_world.getMap();
	Unit *builtUnit = NULL;
	// late resource allocation
	if (!command->isReserveResources()) {
		command->setReserveResources(true);
		if (unit->checkCommand(*command) != CommandResult::SUCCESS) {
			if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
				g_console.addStdMessage("BuildingNoRes");
			}
			BUILD_LOG( "in positioin, late resource allocation failed." << cmdCancelMsg );
			unit->finishCommand();
			return;
		}
		unit->getFaction()->applyCosts(static_cast<const UnitType*>(command->getProdType()));
	}

	BUILD_LOG( "in position, starting construction." );
	builtUnit = g_simInterface->getUnitFactory().newInstance(
		command->getPos(), builtUnitType, unit->getFaction(), map, command->getFacing());
	builtUnit->create();
	unit->setCurrSkill(m_buildSkillType);
	unit->setTarget(builtUnit, true, true);

	unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), false);

	const Tile *const &refTile = map->getTile(Map::toTileCoords(builtUnit->getCenteredPos())); 
	if (builtUnitType->getField() == Field::LAND 
	|| (builtUnitType->getField() == Field::AMPHIBIOUS && !map->getSubmerged(refTile))) {
		map->prepareTerrain(builtUnit);
	}
	command->setUnit(builtUnit);

	if (!builtUnit->isMobile()) {
		g_world.getCartographer()->updateMapMetrics(builtUnit->getPos(), builtUnit->getSize());
	}
	
	//play start sound
	if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
		RUNTIME_CHECK(!unit->isCarried());
		g_soundRenderer.playFx(getStartSound(), unit->getCurrVector(), g_gameState.getGameCamera()->getPos());
	}
}

void BuildCommandType::continueBuild(Unit *unit, const Command *command, const UnitType *builtUnitType) const {
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
		if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
			RUNTIME_CHECK(!unit->isCarried());
			g_soundRenderer.playFx(getBuiltSound(), unit->getCurrVector(), g_gameState.getGameCamera()->getPos());
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
		skillName = n->getChild("harvest-skill")->getAttribute("value")->getRestrictedValue();
		m_harvestSkillType = static_cast<const HarvestSkillType*>(unitType->getSkillType(skillName, SkillClass::HARVEST));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	//stop loaded
	try { 
		skillName = n->getChild("stop-loaded-skill")->getAttribute("value")->getRestrictedValue();
		m_stopLoadedSkillType = static_cast<const StopSkillType*>(unitType->getSkillType(skillName, SkillClass::STOP));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}

	//move loaded
	try {
		skillName = n->getChild("move-loaded-skill")->getAttribute("value")->getRestrictedValue();
		m_moveLoadedSkillType = static_cast<const MoveSkillType*>(unitType->getSkillType(skillName, SkillClass::MOVE));
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	//resources can harvest
	try { 
		const XmlNode *resourcesNode = n->getChild("harvested-resources");
		for(int i = 0; i < resourcesNode->getChildCount(); ++i){
			const XmlNode *resourceNode = resourcesNode->getChild("resource", i);
			m_harvestedResources.push_back(tt->getResourceType(resourceNode->getAttribute("name")->getRestrictedValue()));
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	try { m_maxLoad = n->getChild("max-load")->getAttribute("value")->getIntValue(); }
	catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	try { m_hitsPerUnit = n->getChild("hits-per-unit")->getAttribute("value")->getIntValue(); }
	catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void HarvestCommandType::doChecksum(Checksum &checksum) const {
	MoveBaseCommandType::doChecksum(checksum);
	checksum.add(m_moveLoadedSkillType->getName());
	checksum.add(m_harvestSkillType->getName());
	checksum.add(m_stopLoadedSkillType->getName());
	for (int i=0; i < m_harvestedResources.size(); ++i) {
		checksum.add(m_harvestedResources[i]->getName());
	}
	checksum.add<int>(m_maxLoad);
	checksum.add<int>(m_hitsPerUnit);
}

void HarvestCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	str+= lang.get("HarvestSpeed")+": "+ intToStr(m_harvestSkillType->getSpeed() / m_hitsPerUnit);
	m_harvestSkillType->descEpCost(str, unit);
	EnhancementType::describeModifier(str, (unit->getSpeed(m_harvestSkillType) - m_harvestSkillType->getSpeed()) / m_hitsPerUnit);
	str+= "\n";
	str += lang.get("MaxLoad") + ": " + intToStr(m_maxLoad) + "\n";
	str += lang.get("LoadedSpeed") + ": " + intToStr(m_moveLoadedSkillType->getSpeed()) + "\n";
	str+=lang.get("Resources")+":\n";
	for(int i=0; i<getHarvestedResourceCount(); ++i){
		str+= getHarvestedResource(i)->getName()+"\n";
	}
}

bool HarvestCommandType::canHarvest(const ResourceType *resourceType) const{
	return find(m_harvestedResources.begin(), m_harvestedResources.end(), resourceType) != m_harvestedResources.end();
}


// hacky helper for !HarvestCommandType::canHarvest(u->getLoadType()) animanition issue
const MoveSkillType* getMoveLoadedSkill(Unit *u) {
	const MoveSkillType *mst = 0;
	for (int i=0; i < u->getType()->getCommandTypeCount<HarvestCommandType>(); ++i) {
		const HarvestCommandType *t = u->getType()->getCommandType<HarvestCommandType>(i);
			if (t->canHarvest(u->getLoadType())) {
				mst = t->getMoveLoadedSkillType();
				break;
			}
		}
	return mst;
}

// hacky helper for !HarvestCommandType::canHarvest(u->getLoadType()) animanition issue
const StopSkillType* getStopLoadedSkill(Unit *u) {
	const StopSkillType *sst = 0;
	for (int i=0; i < u->getType()->getCommandTypeCount<HarvestCommandType>(); ++i) {
		const HarvestCommandType *t = u->getType()->getCommandType<HarvestCommandType>(i);
			if (t->canHarvest(u->getLoadType())) {
				sst = t->getStopLoadedSkillType();
				break;
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
	Map *map = g_world.getMap();

	PosCircularIteratorOrdered pci(map->getBounds(), unit->getCurrCommand()->getPos(),
			g_world.getPosIteratorFactory().getInsideOutIterator(1, maxResSearchRadius));

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
	Map *map = g_world.getMap();

	Tile *tile = map->getTile(Map::toTileCoords(unit->getCurrCommand()->getPos()));
	Resource *res = tile->getResource();

	if (!res) { // reset command pos, but not Unit::targetPos
		HARVEST_LOG( "No resource at command target." );
		if (!(res = searchForResource(unit, this))) {
			if (!unit->getLoadCount()) {
				unit->finishCommand();
				unit->setCurrSkill(SkillClass::STOP);
				HARVEST_LOG( "No resource found nearby, command finished." );
				return;
			}
			HARVEST_LOG( "No resource found nearby, returning to store." );
		}
		HARVEST_LOG( "Found resource nearby, command target reset." );
	}
	if (unit->getCurrSkill()->getClass() != SkillClass::HARVEST) { // if not working
		if  (!unit->getLoadCount() || (res && unit->getLoadType() == res->getType()
		&& unit->getLoadCount() < this->getMaxLoad() / 2)) {
			// if current load is correct resource type and not more than half loaded, go for resources
			if (res && canHarvest(res->getType())) {
				switch (g_routePlanner.findPathToResource(unit, command->getPos(), res->getType())) {
					case TravelState::ARRIVED:
						if (map->isResourceNear(unit->getPos(), unit->getSize(), res->getType(), targetPos)) {
							HARVEST_LOG( "Arrived, my pos: " << unit->getPos() << ", targetPos" << targetPos );
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
						HARVEST_LOG( "Moving, pos: " << unit->getPos() << ", nextPos: " << unit->getNextPos() );
						break;
					default:
						HARVEST_LOG( "Blocked?" );
						unit->setCurrSkill(SkillClass::STOP);
						break;
				}
				return;
			} else { // if can't harvest, search for another resource
				unit->setCurrSkill(SkillClass::STOP);
				if (!searchForResource(unit, this)) {
					if (!unit->getLoadCount()) {
						unit->finishCommand();
						return;
					}
				}
			}
		} 
		// if load is wrong type, or more than half loaded, or no more resource to harvest, return to store
		Unit *store = g_world.nearestStore(unit->getPos(), unit->getFaction()->getIndex(), unit->getLoadType());
		if (store) {
			switch (g_routePlanner.findPathToStore(unit, store)) {
				case TravelState::MOVING:
					unit->setCurrSkill(getMoveLoadedSkill(unit));
					unit->face(unit->getNextPos());
					return;
				case TravelState::BLOCKED:
					unit->setCurrSkill(SkillClass::STOP);
					return;
				case TravelState::IMPOSSIBLE:
					unit->setCurrSkill(SkillClass::STOP);
					unit->cancelCurrCommand();
					return;
				case TravelState::ARRIVED: 
					break;
			}
			// update resources
			int resourceAmount = unit->getLoadCount();
			// Just do this for all players ???
			if (unit->getFaction()->getCpuControl()) {
				const float &mult = g_simInterface->getGameSettings().getResourceMultilpier(unit->getFactionIndex());
				resourceAmount = int(resourceAmount * mult);
			}
			unit->getFaction()->incResourceAmount(unit->getLoadType(), resourceAmount);
			g_simInterface->getStats()->harvest(unit->getFactionIndex(), resourceAmount);
			ScriptManager::onResourceHarvested();

			// if next to a store unload resources
			unit->clearPath();
			unit->setCurrSkill(SkillClass::STOP);
			unit->setLoadCount(0);
			unit->StateChanged(unit);
		} else { // no store found, give up
			unit->finishCommand();
		}
	} else { // if working
		HARVEST_LOG( "Working, targetPos: " << unit->getTargetPos() );
		res = map->getTile(Map::toTileCoords(unit->getTargetPos()))->getResource();
		if (res) { // if there is a resource, continue working, until loaded
			if (!this->canHarvest(res->getType())) { // wrong resource type, command changed
				unit->setCurrSkill(getStopLoadedSkill(unit));
				unit->clearPath();
				HARVEST_LOG( "wrong resource type, command changed." );
				return;
			}
			HARVEST_LOG( "Working..." );
			unit->update2();
			if (unit->getProgress2() >= this->getHitsPerUnit()) {
				unit->setProgress2(0);
				if (unit->getLoadCount() < this->getMaxLoad()) {
					unit->setLoadCount(unit->getLoadCount() + 1);
				}
				HARVEST_LOG( "Harvested 1 unit, load now: " << unit->getLoadCount() );
				// if resource exausted, then delete it and stop (and let the cartographer know)
				if (res->decAmount(1)) {
					Vec2i rPos = res->getPos();
					tile->deleteResource();
					g_world.getCartographer()->updateMapMetrics(rPos, GameConstants::cellScale);
					unit->setCurrSkill(getStopLoadedSkill(unit));
				}
				if (unit->getLoadCount() == this->getMaxLoad()) {
					HARVEST_LOG( "Loaded, setting stop-loaded." );
					unit->setCurrSkill(getStopLoadedSkill(unit));
					unit->clearPath();
				}
			}
		} else { // if there is no resource
			if (unit->getLoadCount()) {
				HARVEST_LOG( "No resource..." );
				unit->setCurrSkill(getStopLoadedSkill(unit));
			} else {
				HARVEST_LOG( "No resource, finsihed." );
				unit->finishCommand();
				unit->setCurrSkill(SkillClass::STOP);
			}
		}
	}
}

}}
