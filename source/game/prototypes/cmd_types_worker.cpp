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
	REPAIR_LOG2( unit,
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
		REPAIR_LOG2( unit, "\tRange check to target : " << (rangeToTarget > range ? "not " : "") << "in range." );

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
			if (candidate == 0) {
				continue;
			}
			const Command *cmd = candidate->anyCommand() ? candidate->getCurrCommand() : 0;
			CmdClass cc = cmd ? (CmdClass::Enum)cmd->getType()->getClass() : CmdClass::INVALID;
			const BuildSelfCommandType *bsct = cc == CmdClass::BUILD_SELF
				? static_cast<const BuildSelfCommandType*>(cmd->getType()) : 0;

			//is it a repairable?
			if ((allowSelf || candidate != unit)
			&& (!rst->isSelfOnly() || candidate == unit)
			&& candidate->isAlive()
			&& unit->isAlly(candidate)
			&& (!bsct || bsct->allowRepair())
			&& (!damagedOnly || candidate->isDamaged())
			&& (!militaryOnly || candidate->getType()->hasCommandClass(CmdClass::ATTACK))
			&& rct->canRepair(candidate->getType())) {
				//record the nearest distance to target (target may be on multiple cells)
				repairables.record(candidate, distance);
			}
		}
	}
	// if no repairables or just one then it's a simple choice.
	if (repairables.empty()) {
		REPAIR_LOG2( unit, "\tSearch found no targets." );
		return false;
	} else if (repairables.size() == 1) {
		*rangedPtr = repairables.begin()->first;
		REPAIR_LOG2( unit, "\tSearch found single possible target. Unit: "
			<< **rangedPtr << " @ " << (*rangedPtr)->getPos() );
		return true;
	}
	//heal cloesest ally that can attack (and are probably fighting) first.
	//if none, go for units that are less than 20%
	//otherwise, take the nearest repairable unit
	if(!(*rangedPtr = repairables.getNearestSkillClass(SkillClass::ATTACK))
	&& !(*rangedPtr = repairables.getNearestHpRatio(fixed(2) / 10))
	&& !(*rangedPtr = repairables.getNearest())) {
		REPAIR_LOG2( unit, "\tSomething very odd happened..." );
		// this is unreachable, we've already established repaiables is not empty, so
		// getNearest() will always return something for us...
		return false;
	}
	REPAIR_LOG2( unit, "\tSearch found " << repairables.size() << " possible targets. Selected Unit: "
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
	return repairableInRange(unit, rangedPtr, rct, unit->getUnitStats()->getSight().getValue(), allowSelf);
}

bool RepairCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const CreatableType *ct) {
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ct);
    const FactionType *ft = ct->getFactionType();
	//repair
	try {
		string skillName= n->getChild("repair-skill")->getAttribute("value")->getRestrictedValue();
		repairSkillType= static_cast<const RepairSkillType*>(creatableType->getSkillType(skillName, SkillClass::REPAIR));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
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
		g_logger.logXmlError(dir, e.what ());
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

void RepairCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	repairSkillType->getDesc(msg, unit);
	callback->addElement(msg);
}

void RepairCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	Lang &lang = g_lang;
	const string factionName = unit->getFaction()->getType()->getName();
	callback->addElement(g_lang.get("CanRepair") + ":");
	if (repairSkillType->isSelfOnly()) {
		callback->addElement(lang.get("SelfOnly"));
	} else if (repairSkillType->isPetOnly()) {
		callback->addElement(lang.get("PetOnly"));
	} else {
		foreach_const (vector<const UnitType*>, it, repairableUnits) {
			string name;
			if (!lang.lookUp((*it)->getName(), factionName, name)) {
				name = formatString((*it)->getName());
			}
			callback->addItem(*it, name);
		}
	}
}

CmdResult RepairCommandType::check(const Unit *unit, const Command &command) const {
	// if no commanded unit then it is auto repair and already checked
	if (!command.getCommandedUnit() || command.getUnit() && canRepair(command.getUnit()->getType())) {
		return CmdResult::SUCCESS;
	} else {
		return CmdResult::FAIL_UNDEFINED;
	}
}

//get
bool RepairCommandType::canRepair(const UnitType *unitType) const{
	for(int i=0; i < repairableUnits.size(); ++i) {
		if (static_cast<const UnitType*>(repairableUnits[i]) == unitType) {
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

	// If the unit I was supposed to repair died or is already fixed or not ally then finish
	if (repaired && (repaired->isDead() || !repaired->isDamaged()
			|| !unit->isAlly(repaired))) {
		unit->setCurrSkill(SkillClass::STOP);
		unit->finishCommand();
		return;
	}

	Command *autoCmd;
	if (command->isAuto() && (autoCmd = doAutoCommand(unit))) {
		if (autoCmd->getType()->getClass() == CmdClass::ATTACK) {
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
					REPAIR_LOG( unit, "Unit: " << unit->getId() << " path blocked, cancelling." );
				}
				break;

			case TravelState::IMPOSSIBLE:
				unit->setCurrSkill(SkillClass::STOP);
				unit->finishCommand();
				REPAIR_LOG( unit, "Unit: " << unit->getId() << " path impossible, cancelling." );
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
			if (rst->getSplashParticleType()) {
				const Tile *sc = g_world.getMap()->getTile(Map::toTileCoords(repaired->getCenteredPos()));
				bool visible = sc->isVisible(g_world.getThisTeamIndex())
					&& g_renderer.getCuller().isInside(repaired->getCenteredPos());

				Splash *psSplash = rst->getSplashParticleType()->createSplashParticleSystem(visible);
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
						BuildCommandType *bct = (BuildCommandType *)unit->getType()->getFirstCtOfClass(CmdClass::BUILD);
						if (bct) {
							RUNTIME_CHECK(!unit->isCarried() && !unit->isGarrisoned());
							g_soundRenderer.playFx(bct->getBuiltSound(),
								unit->getCurrVector(), g_gameState.getGameCamera()->getPos());
						}
					}
				}
			}
		}
	}
}

void RepairCommandType::tick(const Unit *unit, Command &command) const {
	replaceDeadReferences(command);
}

Command *RepairCommandType::doAutoRepair(Unit *unit) const {
	if (!unit->isAutoCmdEnabled(AutoCmdFlag::REPAIR) || !unit->getFaction()->isAvailable(this)) {
		return 0;
	}
	// look for someone to repair
	Unit *sighted = NULL;
	if (unit->getEp() >= repairSkillType->getEpCost()
	&& repairableInSight(unit, &sighted, this, repairSkillType->isSelfAllowed())) {
		REPAIR_LOG( unit, __FUNCTION__ << "(): Unit:" << *unit << " @ " << unit->getPos()
			<< ", found someone (" << *sighted << ") to repair @ " << sighted->getPos() );
		Command *newCommand;

		Vec2i pos = Map::getNearestPos(unit->getPos(), sighted, repairSkillType->getMinRange(), repairSkillType->getMaxRange());
		REPAIR_LOG( unit, "\tMap::getNearestPos(): " << pos );

		newCommand = g_world.newCommand(this, CmdFlags(CmdProps::QUEUE, CmdProps::AUTO), pos);
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

bool BuildCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const CreatableType *ct) {
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ct);
    const FactionType *ft = ct->getFactionType();
	//build
	try {
		string skillName = n->getChild("build-skill")->getAttribute("value")->getRestrictedValue();
		m_buildSkillType = static_cast<const BuildSkillType*>(creatableType->getSkillType(skillName, SkillClass::BUILD));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
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
		g_logger.logXmlError(dir, e.what());
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
		g_logger.logXmlError(dir, e.what());
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
		g_logger.logXmlError(dir, e.what());
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

void BuildCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	if (!pt) {
		Lang &lang = g_lang;
		const string factionName = unit->getFaction()->getType()->getName();
		// buildings built
		callback->addElement(g_lang.get("Buildings") + ":");
		foreach_const (vector<const UnitType*>, it, m_buildings) {
			callback->addItem(*it, lang.getTranslatedFactionName(factionName, (*it)->getName()));
		}
	}
}

void BuildCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	m_buildSkillType->getDesc(msg, unit);
	callback->addElement(msg);
}

ProdTypePtr BuildCommandType::getProduced(int i) const {
	return m_buildings[i];
}

const string cmdCancelMsg = " Command cancelled.";

void BuildCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	const UnitType *builtUnitType = static_cast<const UnitType*>(command->getProdType());

	BUILD_LOG( unit,
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
		    map->clearFoundation(builtUnitType->foundation, command->getPos(), builtUnitType->getSize(), builtUnitType->getField());
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

CmdResult BuildCommandType::check(const Unit *unit, const Command &command) const {
	const UnitType *builtUnit = static_cast<const UnitType*>(command.getProdType());
	if (isBlocked(builtUnit, command.getPos(), command.getFacing())) {
		return CmdResult::FAIL_BLOCKED;
	}
	return CmdResult::SUCCESS;
}

void BuildCommandType::undo(Unit *unit, const Command &command) const {
	// return building cost if we reserved resources and are not already building it or dead
	if (command.isReserveResources()
			&& unit->getCurrSkill()->getClass() != SkillClass::BUILD
			&& unit->getCurrSkill()->getClass() != SkillClass::DIE) {
		unit->getFaction()->deApplyCosts(command.getProdType());
	}
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
			BUILD_LOG( unit, "Moving." );
			break;

		case TravelState::BLOCKED:
			unit->setCurrSkill(SkillClass::STOP);
			if(unit->getPath()->isBlocked()) {
				g_console.addStdMessage("Blocked");
				unit->clearPath();
				unit->cancelCurrCommand();
				BUILD_LOG( unit, "Blocked." << cmdCancelMsg );
			}
			break;

		case TravelState::ARRIVED:
			arrived = true;
			break;

		case TravelState::IMPOSSIBLE:
			g_console.addStdMessage("Unreachable");
			unit->cancelCurrCommand();
			BUILD_LOG( unit, "Route impossible," << cmdCancelMsg );
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
	BUILD_LOG( unit, "in position, building already under construction." );
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
//	BUILD_LOG( unit, "in position, site blocked, waiting for people to get out of the way." );
	// they all have a move command, so we'll wait
	return true;

	///@todo Check for idle units and tell them to get the fuck out of the way.
	///@todo Possibly add a unit notification to let player know builder is waiting
}

void BuildCommandType::blockedBuild(Unit *unit) const {
	// blocked by non-moving units, surface objects (trees, rocks, etc.) or build area
	// contains deeply submerged terain
	unit->cancelCurrCommand();
	unit->setCurrSkill(SkillClass::STOP);
	if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
		g_console.addStdMessage("BuildingNoPlace");
	}
	BUILD_LOG( unit, "site blocked." << cmdCancelMsg );
}

void BuildCommandType::acceptBuild(Unit *unit, Command *command, const UnitType *builtUnitType) const {
	Map *map = g_world.getMap();
	Unit *builtUnit = NULL;
	// late resource allocation
	if (!command->isReserveResources()) {
		command->setReserveResources(true);
		if (unit->checkCommand(*command) != CmdResult::SUCCESS) {
			if (unit->getFactionIndex() == g_world.getThisFactionIndex()) {
				g_console.addStdMessage("BuildingNoRes");
			}
			BUILD_LOG( unit, "in positioin, late resource allocation failed." << cmdCancelMsg );
			unit->finishCommand();
			return;
		}
		unit->getFaction()->applyCosts(static_cast<const UnitType*>(command->getProdType()));
	}

	BUILD_LOG( unit, "in position, starting construction." );
	builtUnit = g_world.newUnit(command->getPos(), builtUnitType, unit->getFaction(), map, command->getFacing());
	builtUnit->create();
	unit->setCurrSkill(m_buildSkillType);
	unit->setTarget(builtUnit, true, true);

	unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), false);

	Vec2i tilePos = Map::toTileCoords(builtUnit->getCenteredPos());
	if (builtUnitType->getField() == Field::LAND
	|| (builtUnitType->getField() == Field::AMPHIBIOUS && !map->isTileSubmerged(tilePos))) {
		map->prepareTerrain(builtUnit);
	}
	command->setUnit(builtUnit);

	if (!builtUnit->isMobile()) {
		g_world.getCartographer()->updateMapMetrics(builtUnit->getPos(), builtUnit->getSize());
	}

	//play start sound
	if (unit->getFaction()->isThisFaction()) {
		RUNTIME_CHECK(!unit->isCarried() && !unit->isGarrisoned());
		g_soundRenderer.playFx(getStartSound(), unit->getCurrVector(), g_gameState.getGameCamera()->getPos());
	}
}

void BuildCommandType::continueBuild(Unit *unit, const Command *command, const UnitType *builtUnitType) const {
	BUILD_LOG( unit, "building." );
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
			RUNTIME_CHECK(!unit->isCarried() && !unit->isGarrisoned());
			g_soundRenderer.playFx(getBuiltSound(), unit->getCurrVector(), g_gameState.getGameCamera()->getPos());
		}
	}
}


// =====================================================
// 	class HarvestCommandType
// =====================================================

bool HarvestCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const CreatableType *ct) {
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ct);
    const FactionType *ft = ct->getFactionType();
	string skillName;
	try {
		string storage = n->getChild("storage")->getAttribute("type")->getRestrictedValue();
		if (storage == "local") {
            m_location = true;
		} else if (storage == "faction") {
            m_location = false;
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	//harvest
	try {
		skillName = n->getChild("harvest-skill")->getAttribute("value")->getRestrictedValue();
		m_harvestSkillType = static_cast<const HarvestSkillType*>(creatableType->getSkillType(skillName, SkillClass::HARVEST));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	//stop loaded
	try {
		skillName = n->getChild("stop-loaded-skill")->getAttribute("value")->getRestrictedValue();
		m_stopLoadedSkillType = static_cast<const StopSkillType*>(creatableType->getSkillType(skillName, SkillClass::STOP));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	//move loaded
	try {
		skillName = n->getChild("move-loaded-skill")->getAttribute("value")->getRestrictedValue();
		m_moveLoadedSkillType = static_cast<const MoveSkillType*>(creatableType->getSkillType(skillName, SkillClass::MOVE));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
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
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try { m_maxLoad = n->getChild("max-load")->getAttribute("value")->getIntValue(); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try { m_hitsPerUnit = n->getChild("hits-per-unit")->getAttribute("value")->getIntValue(); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
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
	str += lang.get("HarvestSpeed") + ": " + intToStr(m_harvestSkillType->getBaseSpeed() / m_hitsPerUnit);
	EnhancementType::describeModifier(str, (unit->getSpeed(m_harvestSkillType) - m_harvestSkillType->getBaseSpeed()) / m_hitsPerUnit);
	str+= "\n";
	m_harvestSkillType->descEpCost(str, unit);
	str += lang.get("MaxLoad") + ": " + intToStr(m_maxLoad) + "\n";
	str += lang.get("LoadedSpeed") + ": " + intToStr(m_moveLoadedSkillType->getBaseSpeed()) + "\n";
}

void HarvestCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	getDesc(msg, unit);
	callback->addElement(msg);
}

void HarvestCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	Lang &lang = g_lang;
	callback->addElement(g_lang.get("Harvested") + ":");
	foreach_const (vector<const ResourceType*>, it, m_harvestedResources) {
		callback->addItem(*it, lang.getTranslatedTechName((*it)->getName()));
	}
}

bool HarvestCommandType::canHarvest(const ResourceType *resourceType) const{
	return find(m_harvestedResources.begin(), m_harvestedResources.end(), resourceType) != m_harvestedResources.end();
}

// hacky helper for !HarvestCommandType::canHarvest(u->getLoadType()) animation issue
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

// hacky helper for !HarvestCommandType::canHarvest(u->getLoadType()) animation issue
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
/// NULL if no resource was found within maxResSearchRadius.
MapResource* searchForResource(Unit *unit, const HarvestCommandType *hct) {
	Vec2i pos;
	Map *map = g_world.getMap();

	PosCircularIteratorOrdered pci(map->getBounds(), unit->getCurrCommand()->getPos(),
			g_world.getPosIteratorFactory().getInsideOutIterator(1, maxResSearchRadius));

	while (pci.getNext(pos)) {
		MapResource *r = map->getTile(Map::toTileCoords(pos))->getResource();
		if (r && hct->canHarvest(r->getType())) {
			HARVEST_LOG2( unit, "Found new target at @ " << pos );
			unit->getCurrCommand()->setPos(pos);
			return r;
		}
	}
	HARVEST_LOG2( unit, "Failed to find new target" );
	return 0;
}

void HarvestCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Vec2i targetPos;
	Map *map = g_world.getMap();

	// 1. find resource at command's pos
	Tile *tile = map->getTile(Map::toTileCoords(command->getPos()));
	MapResource *res = tile->getResource();
	if (!res) { // reset command pos, but not Unit::targetPos
		HARVEST_LOG( unit, "No resource at command target " << command->getPos() << "." );
		if (!(res = searchForResource(unit, this))) {
			if (!unit->getLoadCount()) {
				HARVEST_LOG( unit, "No resource found nearby, finishing command." );
				unit->finishCommand();
				unit->setCurrSkill(SkillClass::STOP);
				return;
			}
			HARVEST_LOG( unit, "No resource found nearby, returning to store." );
		} else {
			HARVEST_LOG2( unit, "Found resource nearby: " << *res << ", command pos reset." );
		}
	} else {
		HARVEST_LOG2( unit, "Resource at command target: " << *res );
	}

	if (unit->getCurrSkill()->getClass() != SkillClass::HARVEST) { // if not working
		if  (!unit->getLoadCount() || (res && unit->getLoadType() == res->getType()
		&& unit->getLoadCount() < this->getMaxLoad() / 2)) {
			// if current load is correct resource type and not more than half loaded, go for resources
			if (res && canHarvest(res->getType())) {
				switch (g_routePlanner.findPathToResource(unit, command->getPos(), res->getType())) {
					case TravelState::ARRIVED:
						if (map->isResourceNear(unit->getPos(), unit->getSize(), res->getType(), targetPos)) {
							HARVEST_LOG( unit, "Arrived, my pos: " << unit->getPos() << ", targetPos" << targetPos );
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
						HARVEST_LOG( unit, "Moving, pos: " << unit->getPos() << ", nextPos: " << unit->getNextPos() );
						break;
					default:
						HARVEST_LOG( unit, "Blocked?" );
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
				const float &mult = g_simInterface.getGameSettings().getResourceMultilpier(unit->getFactionIndex());
				resourceAmount = int(resourceAmount * mult);
			}
			if (m_location == false) {
                unit->getFaction()->incResourceAmount(unit->getLoadType(), resourceAmount);
			} else if (m_location == true) {
                store->incResourceAmount(unit->getLoadType(), resourceAmount);
			}
			g_simInterface.getStats()->harvest(unit->getFactionIndex(), resourceAmount);
			ScriptManager::onResourceHarvested(unit);

			// if next to a store unload resources
			unit->clearPath();
			unit->setCurrSkill(SkillClass::STOP);
			unit->setLoadCount(0);
			unit->StateChanged(unit);
		} else { // no store found, give up
			unit->finishCommand();
		}
	} else { // if working
		// Unit::targetPos is the resource we were harvesting, (Command::pos may have been reset).
		res = map->getTile(Map::toTileCoords(unit->getTargetPos()))->getResource();
		if (res) { // if there is a resource, continue working, until loaded
			HARVEST_LOG2( unit, "Resource at unit target-pos: " << *res );
			if (!this->canHarvest(res->getType())) { // wrong resource type, command changed
				unit->setCurrSkill(getStopLoadedSkill(unit));
				unit->clearPath();
				HARVEST_LOG( unit, "wrong resource type, command changed." );
				return;
			}
			HARVEST_LOG( unit, "Working, targetPos: " << unit->getTargetPos() );
			unit->update2();
			if (unit->getProgress2() >= this->getHitsPerUnit()) {
				unit->setProgress2(0);
				if (unit->getLoadCount() < this->getMaxLoad()) {
					unit->setLoadCount(unit->getLoadCount() + 1);
				}
				HARVEST_LOG( unit, "Harvested 1 unit, load now: " << unit->getLoadCount() );
				assert(res->getAmount() > 0);
				// if resource exausted, then delete it and stop (and let the cartographer know)
				if (res->decrement()) {
					Vec2i rPos = res->getPos();
					map->getTile(Map::toTileCoords(rPos))->deleteResource();
					g_world.getCartographer()->updateMapMetrics(rPos, GameConstants::cellScale);
					unit->setCurrSkill(getStopLoadedSkill(unit));
				}
				if (unit->getLoadCount() == this->getMaxLoad()) {
					HARVEST_LOG( unit, "Loaded, setting stop-loaded." );
					unit->setCurrSkill(getStopLoadedSkill(unit));
					unit->clearPath();
				}
			}
		} else { // if there is no resource
			if (unit->getLoadCount()) {
				HARVEST_LOG( unit, "No resource..." );
				unit->setCurrSkill(getStopLoadedSkill(unit));
			} else {
				HARVEST_LOG( unit, "No resource, finsihed." );
				unit->finishCommand();
				unit->setCurrSkill(SkillClass::STOP);
			}
		}
	}
}

// =====================================================
// 	class TransportCommandType
// =====================================================

bool TransportCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const CreatableType *ct) {
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ct);
    const FactionType *ft = ct->getFactionType();
	string skillName;
	try {
		string storage = n->getChild("storage")->getAttribute("value")->getRestrictedValue();
		if (storage == "local") {
            m_location = true;
		} else if (storage == "faction") {
            m_location = false;
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		skillName = n->getChild("transport-skill")->getAttribute("value")->getRestrictedValue();
		m_transportSkillType = static_cast<const TransportSkillType*>(creatableType->getSkillType(skillName, SkillClass::TRANSPORT));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	//stop loaded
	try {
		skillName = n->getChild("stop-loaded-skill")->getAttribute("value")->getRestrictedValue();
		m_stopLoadedSkillType = static_cast<const StopSkillType*>(creatableType->getSkillType(skillName, SkillClass::STOP));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	//move loaded
	try {
		skillName = n->getChild("move-loaded-skill")->getAttribute("value")->getRestrictedValue();
		m_moveLoadedSkillType = static_cast<const MoveSkillType*>(creatableType->getSkillType(skillName, SkillClass::MOVE));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	//resources can harvest
	try {
		const XmlNode *resourcesNode = n->getChild("transported-resources");
		for(int i = 0; i < resourcesNode->getChildCount(); ++i){
			const XmlNode *resourceNode = resourcesNode->getChild("resource", i);
			m_transportedResources.push_back(tt->getResourceType(resourceNode->getAttribute("name")->getRestrictedValue()));
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void TransportCommandType::doChecksum(Checksum &checksum) const {
	MoveBaseCommandType::doChecksum(checksum);
	checksum.add(m_moveLoadedSkillType->getName());
	checksum.add(m_transportSkillType->getName());
	checksum.add(m_stopLoadedSkillType->getName());
	for (int i=0; i < m_transportedResources.size(); ++i) {
		checksum.add(m_transportedResources[i]->getName());
	}
}

void TransportCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();
	m_transportSkillType->descEpCost(str, unit);
	str += lang.get("LoadedSpeed") + ": " + intToStr(m_moveLoadedSkillType->getBaseSpeed()) + "\n";
}

void TransportCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	getDesc(msg, unit);
	callback->addElement(msg);
}

void TransportCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	Lang &lang = g_lang;
	callback->addElement(g_lang.get("Transported") + ":");
	foreach_const (vector<const ResourceType*>, it, m_transportedResources) {
		callback->addItem(*it, lang.getTranslatedTechName((*it)->getName()));
	}
}

void TransportCommandType::goToStore(Unit *unit, Unit *store, Unit *producer) const {
    for (int i = 0; i < producer->getType()->getProcessProductionSystem().getProcesses().size(); ++i) {
    for (int j = 0; j < producer->getType()->getProcessProductionSystem().getProcesses()[i].costs.size(); ++j) {
        const ResourceType *transportType = producer->getType()->getProcessProductionSystem().getProcesses()[i].costs[j].getType();
        for (int k = 0; k < store->getType()->getResourceProductionSystem().getStoredResourceCount(); ++k) {
        const ResourceType *storeType = store->getType()->getResourceProductionSystem().getStoredResource(k, store->getFaction()).getType();
            if (transportType == storeType) {
            int resourceAmount = producer->getType()->getProcessProductionSystem().getProcesses()[i].costs[j].getAmount()*4;
                if (unit->getFaction()->getCpuControl()) {
                const float &mult = g_simInterface.getGameSettings().getResourceMultilpier(unit->getFactionIndex());
                resourceAmount = int(resourceAmount * mult);
                }
                if (store->getSResource(storeType)->getAmount() >= resourceAmount) {
                store->incResourceAmount(storeType, -resourceAmount);
                unit->incResourceAmount(storeType, resourceAmount);
                } else {
                resourceAmount = store->getSResource(storeType)->getAmount();
                store->incResourceAmount(storeType, -resourceAmount);
                unit->incResourceAmount(storeType, resourceAmount);
                }
            }
        }
    }
    }
    for (int i = 0; i < unit->getType()->getResourceProductionSystem().getStoredResourceCount(); ++i) {
        const ResourceType *transportType = unit->getType()->getResourceProductionSystem().getStoredResource(i, store->getFaction()).getType();
        for (int k = 0; k < store->getType()->getResourceProductionSystem().getStoredResourceCount(); ++k) {
        const ResourceType *storeType = store->getType()->getResourceProductionSystem().getStoredResource(k, store->getFaction()).getType();
            if (transportType == storeType) {
            int resourceAmount = unit->getSResource(storeType)->getAmount();
                if (unit->getFaction()->getCpuControl()) {
                const float &mult = g_simInterface.getGameSettings().getResourceMultilpier(unit->getFactionIndex());
                resourceAmount = int(resourceAmount * mult);
                }
                if (unit->getSResource(storeType)->getAmount() >= resourceAmount) {
                unit->incResourceAmount(storeType, -resourceAmount);
                store->incResourceAmount(storeType, resourceAmount);
                } else {
                    resourceAmount = unit->getSResource(storeType)->getAmount();
                    unit->incResourceAmount(storeType, -resourceAmount);
                    if (m_location == true) {
                        store->incResourceAmount(storeType, resourceAmount);
                    } else if (m_location == false) {
                        unit->getFaction()->incResourceAmount(storeType, resourceAmount);
                    }
                }
            }
        }
    }
}

void TransportCommandType::goToProducer(Unit *unit, Unit *store, Unit *producer) const {
    for (int i = 0; i < producer->getType()->getProcessProductionSystem().getProcesses().size(); ++i) {
    for (int j = 0; j < producer->getType()->getProcessProductionSystem().getProcesses()[i].costs.size(); ++j) {
        const ResourceType *transportType = producer->getType()->getProcessProductionSystem().getProcesses()[i].costs[j].getType();
        for (int k = 0; k < store->getType()->getResourceProductionSystem().getStoredResourceCount(); ++k) {
        const ResourceType *storeType = store->getType()->getResourceProductionSystem().getStoredResource(k, store->getFaction()).getType();
            if (transportType == storeType && storeType->getName() != "wealth") {
            int resourceAmount = producer->getType()->getProcessProductionSystem().getProcesses()[i].costs[j].getAmount()*5;
                if (unit->getFaction()->getCpuControl()) {
                const float &mult = g_simInterface.getGameSettings().getResourceMultilpier(unit->getFactionIndex());
                resourceAmount = int(resourceAmount * mult);
                }
                if (store->getSResource(storeType)->getAmount() >= resourceAmount) {
                unit->incResourceAmount(storeType, -resourceAmount);
                producer->incResourceAmount(storeType, resourceAmount);
                } else {
                resourceAmount = unit->getSResource(storeType)->getAmount();
                unit->incResourceAmount(storeType, -resourceAmount);
                producer->incResourceAmount(storeType, resourceAmount);
                }
            }
        }
    }
    }
    for (int i = 0; i < producer->getType()->getProcessProductionSystem().getProcesses().size(); ++i) {
    for (int j = 0; j < producer->getType()->getProcessProductionSystem().getProcesses()[i].products.size(); ++j) {
        const ResourceType *transportType = producer->getType()->getProcessProductionSystem().getProcesses()[i].products[j].getType();
        for (int k = 0; k < store->getType()->getResourceProductionSystem().getStoredResourceCount(); ++k) {
        const ResourceType *storeType = store->getType()->getResourceProductionSystem().getStoredResource(k, store->getFaction()).getType();
        if (transportType == storeType) {
        for (int l = 0; l < producer->getType()->getResourceProductionSystem().getStoredResourceCount(); ++l) {
        const ResourceType *produceType = producer->getType()->getResourceProductionSystem().getStoredResource(l, producer->getFaction()).getType();
            if (storeType == produceType && storeType->getName() != "wealth") {
            int resourceAmount = producer->getSResource(produceType)->getAmount();
                if (unit->getFaction()->getCpuControl()) {
                const float &mult = g_simInterface.getGameSettings().getResourceMultilpier(unit->getFactionIndex());
                resourceAmount = int(resourceAmount * mult);
                }
            unit->incResourceAmount(produceType, resourceAmount);
            producer->incResourceAmount(produceType, -resourceAmount);
            }
        }
        }
        }
    }
    }
    for (int i = 0; i < producer->getType()->getResourceProductionSystem().getCreatedResources().size(); ++i) {
    const ResourceType *transportType = producer->getType()->getResourceProductionSystem().getCreatedResources()[i].getType();
    for (int k = 0; k < store->getType()->getResourceProductionSystem().getStoredResourceCount(); ++k) {
    const ResourceType *storeType = store->getType()->getResourceProductionSystem().getStoredResource(k, store->getFaction()).getType();
        if (transportType == storeType) {
        int resourceAmount = producer->getSResource(storeType)->getAmount();
            if (unit->getFaction()->getCpuControl()) {
            const float &mult = g_simInterface.getGameSettings().getResourceMultilpier(unit->getFactionIndex());
            resourceAmount = int(resourceAmount * mult);
            }
        unit->incResourceAmount(storeType, resourceAmount);
        producer->incResourceAmount(storeType, -resourceAmount);
        }
    }
    }
}

void TransportCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Vec2i targetPos;
	Map *map = g_world.getMap();
	if (unit->productionRoute.getStoreId() != -1 && unit->productionRoute.getProducerId() != -1) {
	    Unit *store = g_world.getUnit(unit->productionRoute.getStoreId());
	    Unit *producer = g_world.getUnit(unit->productionRoute.getProducerId());
        if (unit->productionRoute.getDestination() == store->getPos()) {
            if (unit->travel(unit->productionRoute.getDestination(), m_moveLoadedSkillType) == TravelState::ARRIVED) {
                goToStore(unit, store, producer);
                unit->productionRoute.setDestination(producer->getPos());
                if (unit->getCurrentFocus() == "transport" && unit->getGoalStructure() == NULL) {
                    unit->productionRoute.setProducerId(-1);
                    unit->finishCommand();
                }
            }
        }
        if (unit->productionRoute.getDestination() == producer->getPos()) {
            if (unit->travel(unit->productionRoute.getDestination(), m_moveLoadedSkillType) == TravelState::ARRIVED) {
                goToProducer(unit, store, producer);
                unit->productionRoute.setDestination(store->getPos());
                if (unit->getCurrentFocus() == "transport") {
                    unit->setGoalStructure(NULL);
                }
            }
        }
	} else {
	    unit->finishCommand();
	}
}

// =====================================================
// 	class SetStoreCommandType
// =====================================================

bool SetStoreCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const CreatableType *ct) {
	bool loadOk = StopBaseCommandType::load(n, dir, tt, ct);

	string skillName;
	// set structure
	try {
		skillName = n->getChild("set-structure-skill")->getAttribute("value")->getRestrictedValue();
		m_setStructureSkillType = static_cast<const SetStructureSkillType*>(creatableType->getSkillType(skillName, SkillClass::SET_STRUCTURE));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	return loadOk;
}

void SetStoreCommandType::doChecksum(Checksum &checksum) const {
	StopBaseCommandType::doChecksum(checksum);
	checksum.add(m_setStructureSkillType->getName());
}

void SetStoreCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();
	m_setStructureSkillType->descEpCost(str, unit);
}

void SetStoreCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	getDesc(msg, unit);
	callback->addElement(msg);
}

void SetStoreCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	Lang &lang = g_lang;
}

void SetStoreCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Vec2i targetPos;
	Map *map = g_world.getMap();
	Unit *target = command->getUnit();
	if (target) {
        if (target != unit) {
        unit->productionRoute.setStoreId(target->getId());
        unit->productionRoute.setDestination(target->getPos());
        }
	}
    unit->finishCommand();
}

// =====================================================
// 	class SetProducerCommandType
// =====================================================

bool SetProducerCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const CreatableType *ct) {
	bool loadOk = StopBaseCommandType::load(n, dir, tt, ct);

	string skillName;
	// set structure
	try {
		skillName = n->getChild("set-structure-skill")->getAttribute("value")->getRestrictedValue();
		m_setStructureSkillType = static_cast<const SetStructureSkillType*>(creatableType->getSkillType(skillName, SkillClass::SET_STRUCTURE));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	return loadOk;
}

void SetProducerCommandType::doChecksum(Checksum &checksum) const {
	StopBaseCommandType::doChecksum(checksum);
	checksum.add(m_setStructureSkillType->getName());
}

void SetProducerCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();
	m_setStructureSkillType->descEpCost(str, unit);
}

void SetProducerCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	getDesc(msg, unit);
	callback->addElement(msg);
}

void SetProducerCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	Lang &lang = g_lang;
}

void SetProducerCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Vec2i targetPos;
	Map *map = g_world.getMap();
	Unit *target = command->getUnit();
	if (target) {
        if (target != unit) {
        unit->productionRoute.setProducerId(target->getId());
        //unit->productionRoute.setDestination(target->getPos());
        }
	}
    unit->finishCommand();
}

// =====================================================
// 	class TradeCommandType
// =====================================================

bool TradeCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const CreatableType *ct) {
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ct);
    const FactionType *ft = ct->getFactionType();
	string skillName;
	try {
		string storage = n->getChild("storage")->getAttribute("value")->getRestrictedValue();
		if (storage == "local") {
            m_location = true;
		} else if (storage == "faction") {
            m_location = false;
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		skillName = n->getChild("transport-skill")->getAttribute("value")->getRestrictedValue();
		m_transportSkillType = static_cast<const TransportSkillType*>(creatableType->getSkillType(skillName, SkillClass::TRANSPORT));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	//stop loaded
	try {
		skillName = n->getChild("stop-loaded-skill")->getAttribute("value")->getRestrictedValue();
		m_stopLoadedSkillType = static_cast<const StopSkillType*>(creatableType->getSkillType(skillName, SkillClass::STOP));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	//move loaded
	try {
		skillName = n->getChild("move-loaded-skill")->getAttribute("value")->getRestrictedValue();
		m_moveLoadedSkillType = static_cast<const MoveSkillType*>(creatableType->getSkillType(skillName, SkillClass::MOVE));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void TradeCommandType::doChecksum(Checksum &checksum) const {
	MoveBaseCommandType::doChecksum(checksum);
	checksum.add(m_moveLoadedSkillType->getName());
	checksum.add(m_transportSkillType->getName());
	checksum.add(m_stopLoadedSkillType->getName());
}

void TradeCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();
	m_transportSkillType->descEpCost(str, unit);
	str += lang.get("LoadedSpeed") + ": " + intToStr(m_moveLoadedSkillType->getBaseSpeed()) + "\n";
}

void TradeCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	getDesc(msg, unit);
	callback->addElement(msg);
}

void TradeCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	Lang &lang = g_lang;
	callback->addElement(g_lang.get("Trade"));
}

void TradeCommandType::goToGuild(Unit *unit, Unit *home, Unit *guild) const {
    for (int i = 0; i < home->getType()->getResourceStores().size(); ++i) {
        const ResourceType *requiredType = home->getType()->getResourceStores()[i].getType();
        for (int k = 0; k < guild->getType()->getResourceProductionSystem().getStoredResourceCount(); ++k) {
            const ResourceType *guildType = guild->getType()->getResourceProductionSystem().getStoredResource(k, guild->getFaction()).getType();
            if (requiredType == guildType && requiredType->getName() != "wealth") {
                bool status = false;
                for (int j = 0; j < guild->getType()->getResourceStores().size(); ++j) {
                    if (guild->getType()->getResourceStores()[j].getType() == guildType) {
                        if (guild->getType()->getResourceStores()[j].getStatus() == "stockpile") {
                            status = true;
                            break;
                        }
                    }
                }
                if (!status) {
                    int minWealth = 0;
                    int tradeValue = 0;
                    int freeWealth = 0;
                    int available = 0;
                    int required = home->getType()->getResourceStores()[i].getAmount();
                    int possessed = home->getSResource(requiredType)->getAmount();
                    int requiredAmount = required - possessed;
                    int resourceAmount = guild->getSResource(requiredType)->getAmount();
                    const ResourceType *rt = NULL;
                    for (int j = 0; j < home->getType()->getResourceStores().size(); ++j) {
                        if (home->getType()->getResourceStores()[j].getType()->getName() == "wealth") {
                            minWealth = home->getType()->getResourceStores()[j].getAmount();
                            rt = home->getType()->getResourceStores()[j].getType();
                        }
                    }
                    for (int j = 0; j < unit->getFaction()->getType()->getResourceTrades().size(); ++j) {
                        if (unit->getFaction()->getType()->getResourceTrades()[j].getType() == requiredType) {
                            tradeValue = unit->getFaction()->getType()->getResourceTrades()[j].getAmount();
                        }
                    }
                    if (home->getType()->hasTag("fort")) {
                        freeWealth = home->getFaction()->getSResource(rt)->getAmount() - minWealth;
                    } else {
                        freeWealth = home->getSResource(rt)->getAmount() - minWealth;
                    }
                    if (!guild->getType()->hasTag("shop")) {
                        available = resourceAmount;
                        for (int q = 0; q < guild->getType()->getResourceStores().size(); ++q) {
                            if (guild->getType()->getResourceStores()[q].getType() == requiredType) {
                                int free = resourceAmount - guild->getType()->getResourceStores()[q].getAmount();
                                if (free >= 10) {
                                    available = free;
                                }
                            }
                        }
                    } else {
                        available = resourceAmount;
                    }
                    if (requiredAmount > 10 && available > 10) {
                        if (requiredAmount < available) {
                            available = requiredAmount;
                        }
                        if (available > 100) {
                            available = 100;
                        }
                        int possible = freeWealth / tradeValue;
                        if (possible < available) {
                            available = possible;
                        }
                        int cost = available * tradeValue;
                        if(freeWealth >= cost) {
                            if (home->getType()->hasTag("fort")) {
                                home->getFaction()->incResourceAmount(rt, -cost);
                            } else {
                                int untaxedGold = home->getSResource(rt)->getAmount() - home->taxedGold;
                                if (cost > untaxedGold) {
                                    home->taxedGold = home->taxedGold - (cost - untaxedGold);
                                }
                                home->incResourceAmount(rt, -cost);
                            }
                            guild->incResourceAmount(rt, cost);
                            guild->incResourceAmount(requiredType, -available);
                            unit->incResourceAmount(requiredType, available);
                        }
                    }
                }
            }
        }
    }
}

void TradeCommandType::goToOwner(Unit *unit, Unit *home, Unit *guild) const {
    for (int i = 0; i < unit->getType()->getResourceProductionSystem().getStoredResourceCount(); ++i) {
        const ResourceType *rt = unit->getType()->getResourceProductionSystem().getStoredResource(i, unit->getFaction()).getType();
        int carried = unit->getSResource(rt)->getAmount();
        if (carried > 0) {
            unit->incResourceAmount(rt, -carried);
            if (!home->getType()->hasTag("fort")) {
                home->incResourceAmount(rt, carried);
            } else if (home->getType()->hasTag("fort")) {
                home->getFaction()->incResourceAmount(rt, carried);
            }
        }
    }
}

void TradeCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	Map *map = g_world.getMap();
	if (unit->productionRoute.getStoreId() != -1 && unit->productionRoute.getProducerId() != -1) {
	    Unit *store = g_world.getUnit(unit->productionRoute.getStoreId());
	    Unit *producer = g_world.getUnit(unit->productionRoute.getProducerId());
        if (unit->productionRoute.getDestination() == store->getPos()) {
            if (unit->travel(unit->productionRoute.getDestination(), m_moveLoadedSkillType) == TravelState::ARRIVED) {
                goToOwner(unit, store, producer);
                unit->productionRoute.setDestination(producer->getPos());
                if (unit->getCurrentFocus() == "trade") {
                    unit->productionRoute.setProducerId(-1);
                    unit->finishCommand();
                }
            }
        }
        Vec2i uPos = unit->getPos();
        Vec2i bPos = producer->getCenteredPos();
        int newDistance = sqrt(pow(float(abs(uPos.x - bPos.x)), 2) + pow(float(abs(uPos.y - bPos.y)), 2));
        if (unit->productionRoute.getDestination() == producer->getPos()) {
            if (unit->travel(unit->productionRoute.getDestination(), m_moveLoadedSkillType) == TravelState::ARRIVED
                || newDistance < 3) {
                goToGuild(unit, store, producer);
                unit->productionRoute.setDestination(store->getPos());
            }
        }
	} else {
	    unit->finishCommand();
	}
}

}}
