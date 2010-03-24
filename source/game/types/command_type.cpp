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

#include "upgrade_type.h"
#include "unit_type.h"
#include "sound.h"
#include "util.h"
#include "leak_dumper.h"
#include "graphics_interface.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "unit_updater.h"
#include "renderer.h"

#include "leak_dumper.h"
#include "logger.h"

using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
// 	class AttackSkillTypes & enum AttackSkillPreferenceFlags
// =====================================================

void AttackSkillTypes::init() {
	maxRange = 0;

	assert(types.size() == associatedPrefs.size());
	for(int i = 0; i < types.size(); ++i) {
		if(types[i]->getMaxRange() > maxRange) {
			maxRange = types[i]->getMaxRange();
		}
		zones.flags |= types[i]->getZones().flags;
		allPrefs.flags |= associatedPrefs[i].flags;
	}
}

void AttackSkillTypes::getDesc(string &str, const Unit *unit) const {
	if(types.size() == 1) {
		types[0]->getDesc(str, unit);
	} else {
		str += Lang::getInstance().get("Attacks") + ": ";
		bool printedFirst = false;

		for(int i = 0; i < types.size(); ++i) {
			if(printedFirst) {
				str += ", ";
			}
			str += types[i]->getName();
			printedFirst = true;
		}
		str += "\n";
	}
}

const AttackSkillType *AttackSkillTypes::getPreferredAttack(
		const Unit *unit, const Unit *target, int rangeToTarget) const {
	const AttackSkillType *ast = NULL;

	if(types.size() == 1) {
		ast = types[0];
		return unit->getMaxRange(ast) >= rangeToTarget ? ast : NULL;
	}

	//a skill for use when damaged gets 1st priority.
	if(hasPreference(AttackSkillPreference::WHEN_DAMAGED) && unit->isDamaged()) {
		return getSkillForPref(AttackSkillPreference::WHEN_DAMAGED, rangeToTarget);
	}

	//If a skill in this collection is specified as use whenever possible and
	//the target resides in a field that skill can attack, we will only use that
	//skill if in range and return NULL otherwise.
	if(hasPreference(AttackSkillPreference::WHENEVER_POSSIBLE)) {
		ast = getSkillForPref(AttackSkillPreference::WHENEVER_POSSIBLE, 0);
		assert(ast);
		if(ast->getZone(target->getCurrZone())) {
			return unit->getMaxRange(ast) >= rangeToTarget ? ast : NULL;
		}
		ast = NULL;
	}

	if(hasPreference(AttackSkillPreference::ON_BUILDING) && unit->getType()->isOfClass(UnitClass::BUILDING)) {
		ast = getSkillForPref(AttackSkillPreference::ON_BUILDING, rangeToTarget);
	}

	if(!ast && hasPreference(AttackSkillPreference::ON_LARGE) && unit->getType()->getSize() > 1) {
		ast = getSkillForPref(AttackSkillPreference::ON_LARGE, rangeToTarget);
	}

	//still haven't found an attack skill then use the 1st that's in range
	if(!ast) {
		for(int i = 0; i < types.size(); ++i) {
			if(unit->getMaxRange(types[i]) >= rangeToTarget && types[i]->getZone(target->getCurrZone())) {
				ast = types[i];
				break;
			}
		}
	}

	return ast;
}

// =====================================================
// 	class CommandType
// =====================================================

int CommandType::nextId = 0;

CommandType::CommandType(const char* name, CommandClass cc, Clicks clicks, bool queuable)
		: RequirableType(getNextId(), name, NULL)
		, cc(cc)
		, clicks(clicks)
		, queuable(queuable)
		, unitType(NULL)
		, unitTypeIndex(-1) {
}

void CommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	switch(cc) {
		case CommandClass::STOP:
			unitUpdater->updateStop(unit);
			break;

		case CommandClass::MOVE:
			unitUpdater->updateMove(unit);
			break;

		case CommandClass::ATTACK:
			unitUpdater->updateAttack(unit);
			break;

		case CommandClass::ATTACK_STOPPED:
			unitUpdater->updateAttackStopped(unit);
			break;

		case CommandClass::BUILD:
			unitUpdater->updateBuild(unit);
			break;

		case CommandClass::HARVEST:
			unitUpdater->updateHarvest(unit);
			break;

		case CommandClass::REPAIR:
			unitUpdater->updateRepair(unit);
			break;

		case CommandClass::PRODUCE:
			unitUpdater->updateProduce(unit);
			break;

		case CommandClass::UPGRADE:
			unitUpdater->updateUpgrade(unit);
			break;

		case CommandClass::MORPH:
			unitUpdater->updateMorph(unit);
			break;

		case CommandClass::CAST_SPELL:
			unitUpdater->updateCastSpell(unit);
			break;

		case CommandClass::GUARD:
			unitUpdater->updateGuard(unit);
			break;

		case CommandClass::PATROL:
			unitUpdater->updatePatrol(unit);
			break;

		case CommandClass::SET_MEETING_POINT:
		case CommandClass::COUNT:
		case CommandClass::NULL_COMMAND:
			assert(0);  //FIXME: really assertion fail here? which can be disabled by setting NDEBUG
			break;
		default:
			throw runtime_error("unhandled CommandClass");
	}
}

void CommandType::setUnitTypeAndIndex(const UnitType *unitType, int unitTypeIndex) {
	if(unitType->getId() > UCHAR_MAX) {
		stringstream str;
		str <<  "A maximum of " << UCHAR_MAX << " unit types are currently allowed per faction.  "
			"This limit is only imposed for network data compactness and can be easily changed "
			"if you *really* need that many different unit types. Do you really?";
		throw runtime_error(str.str());
	}

	if(unitType->getId() > UCHAR_MAX) {
		stringstream str;
		str <<  "A maximum of " << UCHAR_MAX << " commands are currently allowed per unit.  "
			"This limit is only imposed for network data compactness and can be easily changed "
			"if you *really* need that many commands. Do you really?";
		throw runtime_error(str.str());
	}

	this->unitType = unitType;
	this->unitTypeIndex = unitTypeIndex;
}

bool CommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	name = n->getChildRestrictedValue("name");

	bool ok = DisplayableType::load(n, dir);
	return RequirableType::load(n, dir, tt, ft) && ok;
}

void CommandType::doChecksum(Checksum &checksum) const {
	RequirableType::doChecksum(checksum);
	checksum.add<CommandClass>(cc);
}

// =====================================================
// 	class MoveBaseCommandType
// =====================================================

bool MoveBaseCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//move
	try { 
		string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
		const SkillType *st = unitType->getSkillType(skillName, SkillClass::MOVE);
		moveSkillType= static_cast<const MoveSkillType*>(st);
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError ( dir, e.what() );
		loadOk = false;
	}
	return loadOk;
}

// =====================================================
// 	class StopBaseCommandType
// =====================================================

bool StopBaseCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//stop
	try {
		string skillName= n->getChild("stop-skill")->getAttribute("value")->getRestrictedValue();
		stopSkillType= static_cast<const StopSkillType*>(unitType->getSkillType(skillName, SkillClass::STOP));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError ( dir, e.what() );
		return false;
	}
	return loadOk;
}
// ===============================
// 	class AttackCommandTypeBase
// ===============================

bool AttackCommandTypeBase::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType *unitType) {
	const AttackSkillType *ast;
	string skillName;
	const XmlNode *attackSkillNode = n->getChild("attack-skill", 0, false);
	bool loadOk = true;
	//single attack skill
	if(attackSkillNode) {
		try {
			skillName = attackSkillNode->getAttribute("value")->getRestrictedValue();
			ast = static_cast<const AttackSkillType*>(unitType->getSkillType(skillName, SkillClass::ATTACK));
			attackSkillTypes.push_back(ast, AttackSkillPreferences());
		} catch (runtime_error e) {
			Logger::getErrorLog().addXmlError(dir, e.what ());
			loadOk = false;
		}
	} else { //multiple attack skills
		try {
			const XmlNode *flagsNode;
			const XmlNode *attackSkillsNode;

			attackSkillsNode = n->getChild("attack-skills", 0, false);
			if(!attackSkillsNode)
				throw runtime_error("Must specify either a single <attack-skill> node or an <attack-skills> node with nested <attack-skill>s.");
			int count = attackSkillsNode->getChildCount();

			for(int i = 0; i < count; ++i) {
				try {
					AttackSkillPreferences prefs;
					attackSkillNode = attackSkillsNode->getChild("attack-skill", i);
					skillName = attackSkillNode->getAttribute("value")->getRestrictedValue();
					ast = static_cast<const AttackSkillType*>(unitType->getSkillType(skillName, SkillClass::ATTACK));
					flagsNode = attackSkillNode->getChild("flags", 0, false);
					if(flagsNode) {
						prefs.load(flagsNode, dir, tt, ft);
					}
					attackSkillTypes.push_back(ast, prefs);
				}
				catch (runtime_error e) {
					Logger::getErrorLog().addXmlError(dir, e.what ());
					loadOk = false;
				}
			}
		} catch (runtime_error e) {
			Logger::getErrorLog().addXmlError(dir, e.what ());
			loadOk = false;
		}
	}
	if ( loadOk ) attackSkillTypes.init();
	return loadOk;
}

/** Returns an attack skill for the given field if one exists. */ /*
const AttackSkillType * AttackCommandTypeBase::getAttackSkillType(Field field) const {
	for(AttackSkillTypes::const_iterator i = attackSkillTypes.begin(); i != attackSkillTypes.end(); i++) {
		if(i->first->getField(field)) {
			return i->first;
		}
	}
	return NULL;
}
 */

// =====================================================
// 	class AttackCommandType
// =====================================================

bool AttackCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool ok = MoveBaseCommandType::load(n, dir, tt, ft);
	return AttackCommandTypeBase::load(n, dir, tt, ft, unitType) && ok;
}

// =====================================================
// 	class AttackStoppedCommandType
// =====================================================

bool AttackStoppedCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool ok = StopBaseCommandType::load(n, dir, tt, ft);
	return AttackCommandTypeBase::load(n, dir, tt, ft, unitType) && ok;
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
	checksum.addString(buildSkillType->getName());
	for (int i=0; i < buildings.size(); ++i) {
		checksum.addString(buildings[i]->getName());
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
	checksum.addString(moveLoadedSkillType->getName());
	checksum.addString(harvestSkillType->getName());
	checksum.addString(stopLoadedSkillType->getName());
	for (int i=0; i < harvestedResources.size(); ++i) {
		checksum.addString(harvestedResources[i]->getName());
	}
	checksum.add<int>(maxLoad);
	checksum.add<int>(hitsPerUnit);
}

void HarvestCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	str+= lang.get("HarvestSpeed")+": "+ intToStr(harvestSkillType->getSpeed()/hitsPerUnit);
	EnhancementTypeBase::describeModifier(str, (unit->getSpeed(harvestSkillType) - harvestSkillType->getSpeed())/hitsPerUnit);
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

// =====================================================
// 	class RepairCommandType
// =====================================================

RepairCommandType::~RepairCommandType(){
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
	checksum.addString(repairSkillType->getName());
	for (int i=0; i < repairableUnits.size(); ++i) {
		checksum.addString(repairableUnits[i]->getName());
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

// =====================================================
// 	class ProduceCommandType
// =====================================================

//varios
bool ProduceCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//produce
	try { 
		string skillName= n->getChild("produce-skill")->getAttribute("value")->getRestrictedValue();
		produceSkillType= static_cast<const ProduceSkillType*>(unitType->getSkillType(skillName, SkillClass::PRODUCE));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	try { 
		string producedUnitName= n->getChild("produced-unit")->getAttribute("name")->getRestrictedValue();
		producedUnit= ft->getUnitType(producedUnitName);
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void ProduceCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.addString(produceSkillType->getName());
	checksum.addString(producedUnit->getName());
}

void ProduceCommandType::getDesc(string &str, const Unit *unit) const {
	produceSkillType->getDesc(str, unit);
	str+= "\n" + getProducedUnit()->getReqDesc();
}

string ProduceCommandType::getReqDesc() const{
	return RequirableType::getReqDesc()+"\n"+getProducedUnit()->getReqDesc();
}

const ProducibleType *ProduceCommandType::getProduced() const{
	return producedUnit;
}

// =====================================================
// 	class UpgradeCommandType
// =====================================================

bool UpgradeCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){

	bool loadOk = CommandType::load(n, dir, tt, ft);

	//upgrade
	try {
		string skillName= n->getChild("upgrade-skill")->getAttribute("value")->getRestrictedValue();
		upgradeSkillType= static_cast<const UpgradeSkillType*>(unitType->getSkillType(skillName, SkillClass::UPGRADE));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	try {
		string producedUpgradeName= n->getChild("produced-upgrade")->getAttribute("name")->getRestrictedValue();
		producedUpgrade= ft->getUpgradeType(producedUpgradeName);
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void UpgradeCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.addString(upgradeSkillType->getName());
	checksum.addString(producedUpgrade->getName());
}

string UpgradeCommandType::getReqDesc() const{
	return RequirableType::getReqDesc()+"\n"+getProducedUpgrade()->getReqDesc();
}

const ProducibleType *UpgradeCommandType::getProduced() const{
	return producedUpgrade;
}

// =====================================================
// 	class MorphCommandType
// =====================================================

bool MorphCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);
	//morph skill
	try {
		string skillName= n->getChild("morph-skill")->getAttribute("value")->getRestrictedValue();
		morphSkillType= static_cast<const MorphSkillType*>(unitType->getSkillType(skillName, SkillClass::MORPH));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	//morph unit
	try {
		string morphUnitName= n->getChild("morph-unit")->getAttribute("name")->getRestrictedValue();
		morphUnit= ft->getUnitType(morphUnitName);
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	//discount
	try { discount= n->getChild("discount")->getAttribute("value")->getIntValue(); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void MorphCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.addString(morphSkillType->getName());
	checksum.addString(morphUnit->getName());
	checksum.add<int>(discount);
}

void MorphCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	morphSkillType->getDesc(str, unit);

	//discount
	if(discount!=0){
		str+= lang.get("Discount")+": "+intToStr(discount)+"%\n";
	}

	str+= "\n"+getProduced()->getReqDesc();
}

string MorphCommandType::getReqDesc() const{
	return RequirableType::getReqDesc() + "\n" + getProduced()->getReqDesc();
}

const ProducibleType *MorphCommandType::getProduced() const{
	return morphUnit;
}

// =====================================================
// 	class CastSpellCommandType
// =====================================================

bool CastSpellCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ft);

	//cast spell
	try {
		string skillName= n->getChild("cast-spell-skill")->getAttribute("value")->getRestrictedValue();
		castSpellSkillType= static_cast<const CastSpellSkillType*>(unitType->getSkillType(skillName, SkillClass::CAST_SPELL));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void CastSpellCommandType::getDesc(string &str, const Unit *unit) const{
	castSpellSkillType->getDesc(str, unit);
	castSpellSkillType->descSpeed(str, unit, "Speed");

	//movement speed
	MoveBaseCommandType::getDesc(str, unit);
}

// =====================================================
// 	class GuardCommandType
// =====================================================

bool GuardCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = AttackCommandType::load(n, dir, tt, ft);

	//distance
	try { maxDistance = n->getChild("max-distance")->getAttribute("value")->getIntValue(); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void GuardCommandType::doChecksum(Checksum &checksum) const {
	AttackCommandType::doChecksum(checksum);
	checksum.add<int>(maxDistance);
}
// =====================================================
// 	class CommandFactory
// =====================================================

CommandTypeFactory::CommandTypeFactory(){
	registerClass<StopCommandType>("stop");
	registerClass<MoveCommandType>("move");
	registerClass<AttackCommandType>("attack");
	registerClass<AttackStoppedCommandType>("attack_stopped");
	registerClass<BuildCommandType>("build");
	registerClass<HarvestCommandType>("harvest");
	registerClass<RepairCommandType>("repair");
	registerClass<ProduceCommandType>("produce");
	registerClass<UpgradeCommandType>("upgrade");
	registerClass<MorphCommandType>("morph");
	registerClass<CastSpellCommandType>("cast-spell");
	registerClass<GuardCommandType>("guard");
	registerClass<PatrolCommandType>("patrol");
	registerClass<SetMeetingPointCommandType>("set-meeting-point");
}

CommandTypeFactory &CommandTypeFactory::getInstance(){
	static CommandTypeFactory ctf;
	return ctf;
}

}}//end namespace
