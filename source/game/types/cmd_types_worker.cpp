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
#include "unit_updater.h"
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
	const Map* map = theWorld.getMap();
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
bool RepairCommandType::isRepairableUnitType(const UnitType *unitType) const{
	for(int i=0; i<repairableUnits.size(); ++i){
		if(static_cast<const UnitType*>(repairableUnits[i])==unitType){
			return true;
		}
	}
	return false;
}


#if defined(LOG_REPAIR_COMMAND) && LOG_REPAIR_COMMAND
#	define REPAIR_LOG(x) STREAM_LOG(x)
#else
#	define REPAIR_LOG(x)
#endif

void RepairCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const {
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
		Command *newCommand;
		newCommand = new Command(this, CommandFlags(CommandProperties::QUEUE, CommandProperties::AUTO),
			Map::getNearestPos(unit->getPos(), sighted, repairSkillType->getMinRange(), repairSkillType->getMaxRange()));
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


}}