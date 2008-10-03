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


using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class AttackSkillTypes & enum AttackSkillPreferenceFlags
// =====================================================


const char* AttackSkillPreferences::names[aspCount] = {
	"whenever-possible",
	"at-max-range",
	"on-large-units",
	"on-buildings",
	"when-damaged"
};

void AttackSkillTypes::init() {
	maxRange = 0;

	assert(types.size() == associatedPrefs.size());
	for(int i = 0; i < types.size(); ++i) {
		if(types[i]->getMaxRange() > maxRange) {
			maxRange = types[i]->getMaxRange();
		}
		fields.flags |= types[i]->getFields().flags;
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

const AttackSkillType *AttackSkillTypes::getPreferredAttack(const Unit *unit,
		const Unit *target, int rangeToTarget) const {
	const AttackSkillType *ast = NULL;

	if(types.size() == 1) {
		ast = types[0];
		return unit->getMaxRange(ast) >= rangeToTarget ? ast : NULL;
	}

	//a skill for use when damaged gets 1st priority.
	if(hasPreference(aspWhenDamaged) && unit->isDamaged()) {
		return getSkillForPref(aspWhenDamaged, rangeToTarget);
	}

	//If a skill in this collection is specified as use whenever possible and
	//the target resides in a field that skill can attack, we will only use that
	//skill if in range and return NULL otherwise.
	if(hasPreference(aspWheneverPossible)) {
		ast = getSkillForPref(aspWheneverPossible, 0);
		assert(ast);
		if(ast->getField(target->getCurrField())) {
			return unit->getMaxRange(ast) >= rangeToTarget ? ast : NULL;
		}
		ast = NULL;
	}

	if(hasPreference(aspOnBuilding) && unit->getType()->isOfClass(ucBuilding)) {
		ast = getSkillForPref(aspOnBuilding, rangeToTarget);
	}

	if(!ast && hasPreference(aspOnLarge) && unit->getType()->getSize() > 1) {
		ast = getSkillForPref(aspOnLarge, rangeToTarget);
	}

	//still haven't found an attack skill then use the 1st that's in range
	if(!ast) {
		for(int i = 0; i < types.size(); ++i) {
			if(unit->getMaxRange(types[i]) >= rangeToTarget && types[i]->getField(target->getCurrField())) {
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

CommandType::CommandType(CommandClass commandTypeClass, Clicks clicks, const char* typeName, bool queuable) {
	this->commandTypeClass = commandTypeClass;
	this->clicks = clicks;
	this->typeName = typeName;
	this->queuable = queuable;
}

//get
CommandClass CommandType::getClass() const{
	assert(this!=NULL);
	return commandTypeClass;
}

void CommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	switch(commandTypeClass) {
		case ccStop:
			unitUpdater->updateStop(unit);
			break;

		case ccMove:
			unitUpdater->updateMove(unit);
			break;

		case ccAttack:
			unitUpdater->updateAttack(unit);
			break;

		case ccAttackStopped:
			unitUpdater->updateAttackStopped(unit);
			break;

		case ccBuild:
			unitUpdater->updateBuild(unit);
			break;

		case ccHarvest:
			unitUpdater->updateHarvest(unit);
			break;

		case ccRepair:
			unitUpdater->updateRepair(unit);
			break;

		case ccProduce:
			unitUpdater->updateProduce(unit);
			break;

		case ccUpgrade:
			unitUpdater->updateUpgrade(unit);
			break;

		case ccMorph:
			unitUpdater->updateMorph(unit);
			break;

		case ccCastSpell:
			unitUpdater->updateCastSpell(unit);
			break;

		case ccGuard:
			unitUpdater->updateGuard(unit);
			break;

		case ccPatrol:
			unitUpdater->updatePatrol(unit);
			break;

		case ccSetMeetingPoint:
		case ccCount:
		case ccNull:
			assert(0);
			;
	}
}

void CommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	this->id= id;
	name= n->getChild("name")->getAttribute("value")->getRestrictedValue();

	//image
	const XmlNode *imageNode= n->getChild("image");
	image= Renderer::getInstance().newTexture2D(rsGame);
	image->load(dir+"/"+imageNode->getAttribute("path")->getRestrictedValue());

	RequirableType::load(n, dir, tt, ft);
}

// =====================================================
// 	class MoveBaseCommandType
// =====================================================

void MoveBaseCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	CommandType::load(id, n, dir, tt, ft, ut);

	//move
   	string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
	moveSkillType= static_cast<const MoveSkillType*>(ut.getSkillType(skillName, scMove));
}

// =====================================================
// 	class StopBaseCommandType
// =====================================================

void StopBaseCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	CommandType::load(id, n, dir, tt, ft, ut);

	//stop
   	string skillName= n->getChild("stop-skill")->getAttribute("value")->getRestrictedValue();
	stopSkillType= static_cast<const StopSkillType*>(ut.getSkillType(skillName, scStop));
}
// ===============================
// 	class AttackCommandTypeBase
// ===============================

void AttackCommandTypeBase::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	const AttackSkillType *ast;
	string skillName;
	const XmlNode *attackSkillNode = n->getChild("attack-skill", 0, false);

	//single attack skill
	if(attackSkillNode) {
	   	skillName = attackSkillNode->getAttribute("value")->getRestrictedValue();
		ast = (const AttackSkillType*)ut.getSkillType(skillName, scAttack);
		attackSkillTypes.push_back(ast, AttackSkillPreferences());

	//multiple attack skills
	} else {
		const XmlNode *flagsNode;
		const XmlNode *attackSkillsNode = n->getChild("attack-skills", 0, false);
		if(!attackSkillsNode) {
			throw runtime_error("Must specify either a single <attack-skill> node or an <attack-skills> node with nested <attack-skill>s.");
		}
		int count = attackSkillsNode->getChildCount();

		for(int i = 0; i < count; ++i) {
			AttackSkillPreferences prefs;
			attackSkillNode = attackSkillsNode->getChild("attack-skill", i);
			skillName = attackSkillNode->getAttribute("value")->getRestrictedValue();
			ast = (const AttackSkillType*)ut.getSkillType(skillName, scAttack);
			flagsNode = attackSkillNode->getChild("flags", 0, false);
			if(flagsNode) {
				prefs.load(flagsNode, dir, tt, ft);
			}
			attackSkillTypes.push_back(ast, prefs);
		}
	}
	attackSkillTypes.init();
}

/** Returns an attack skill for the given field if one exists. *//*
const AttackSkillType * AttackCommandTypeBase::getAttackSkillType(Field field) const {
	for(AttackSkillTypes::const_iterator i = attackSkillTypes.begin();
		   i != attackSkillTypes.end(); i++) {
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

void AttackCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	MoveBaseCommandType::load(id, n, dir, tt, ft, ut);
	AttackCommandTypeBase::load(id, n, dir, tt, ft, ut);
}

// =====================================================
// 	class AttackStoppedCommandType
// =====================================================

void AttackStoppedCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	AttackCommandTypeBase::load(id, n, dir, tt, ft, ut);
	StopBaseCommandType::load(id, n, dir, tt, ft, ut);
}

// =====================================================
// 	class BuildCommandType
// =====================================================

BuildCommandType::~BuildCommandType(){
	deleteValues(builtSounds.getSounds().begin(), builtSounds.getSounds().end());
	deleteValues(startSounds.getSounds().begin(), startSounds.getSounds().end());
}

void BuildCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	MoveBaseCommandType::load(id, n, dir, tt, ft, ut);

	//build
   	string skillName= n->getChild("build-skill")->getAttribute("value")->getRestrictedValue();
	buildSkillType= static_cast<const BuildSkillType*>(ut.getSkillType(skillName, scBuild));

	//buildings built
	const XmlNode *buildingsNode= n->getChild("buildings");
	for(int i=0; i<buildingsNode->getChildCount(); ++i){
		const XmlNode *buildingNode= buildingsNode->getChild("building", i);
		string name= buildingNode->getAttribute("name")->getRestrictedValue();
		buildings.push_back(ft->getUnitType(name));
	}

	//start sound
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

	//built sound
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
}

// =====================================================
// 	class HarvestCommandType
// =====================================================

void HarvestCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	MoveBaseCommandType::load(id, n, dir, tt, ft, ut);

	//harvest
   	string skillName= n->getChild("harvest-skill")->getAttribute("value")->getRestrictedValue();
	harvestSkillType= static_cast<const HarvestSkillType*>(ut.getSkillType(skillName, scHarvest));

	//stop loaded
   	skillName= n->getChild("stop-loaded-skill")->getAttribute("value")->getRestrictedValue();
	stopLoadedSkillType= static_cast<const StopSkillType*>(ut.getSkillType(skillName, scStop));

	//move loaded
   	skillName= n->getChild("move-loaded-skill")->getAttribute("value")->getRestrictedValue();
	moveLoadedSkillType= static_cast<const MoveSkillType*>(ut.getSkillType(skillName, scMove));

	//resources can harvest
	const XmlNode *resourcesNode= n->getChild("harvested-resources");
	for(int i=0; i<resourcesNode->getChildCount(); ++i){
		const XmlNode *resourceNode= resourcesNode->getChild("resource", i);
		harvestedResources.push_back(tt->getResourceType(resourceNode->getAttribute("name")->getRestrictedValue()));
	}

	maxLoad= n->getChild("max-load")->getAttribute("value")->getIntValue();
	hitsPerUnit= n->getChild("hits-per-unit")->getAttribute("value")->getIntValue();
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

void RepairCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	MoveBaseCommandType::load(id, n, dir, tt, ft, ut);

	//repair
   	string skillName= n->getChild("repair-skill")->getAttribute("value")->getRestrictedValue();
	repairSkillType= static_cast<const RepairSkillType*>(ut.getSkillType(skillName, scRepair));

	//repaired units
	const XmlNode *unitsNode= n->getChild("repaired-units");
	for(int i=0; i<unitsNode->getChildCount(); ++i){
		const XmlNode *unitNode= unitsNode->getChild("unit", i);
		repairableUnits.push_back(ft->getUnitType(unitNode->getAttribute("name")->getRestrictedValue()));
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
void ProduceCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	CommandType::load(id, n, dir, tt, ft, ut);

	//produce
   	string skillName= n->getChild("produce-skill")->getAttribute("value")->getRestrictedValue();
	produceSkillType= static_cast<const ProduceSkillType*>(ut.getSkillType(skillName, scProduce));

	string producedUnitName= n->getChild("produced-unit")->getAttribute("name")->getRestrictedValue();
	producedUnit= ft->getUnitType(producedUnitName);
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

void UpgradeCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){

	CommandType::load(id, n, dir, tt, ft, ut);

	//upgrade
   	string skillName= n->getChild("upgrade-skill")->getAttribute("value")->getRestrictedValue();
	upgradeSkillType= static_cast<const UpgradeSkillType*>(ut.getSkillType(skillName, scUpgrade));

	string producedUpgradeName= n->getChild("produced-upgrade")->getAttribute("name")->getRestrictedValue();
	producedUpgrade= ft->getUpgradeType(producedUpgradeName);

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

void MorphCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	CommandType::load(id, n, dir, tt, ft, ut);

	//morph skill
   	string skillName= n->getChild("morph-skill")->getAttribute("value")->getRestrictedValue();
	morphSkillType= static_cast<const MorphSkillType*>(ut.getSkillType(skillName, scMorph));

	//morph unit
   	string morphUnitName= n->getChild("morph-unit")->getAttribute("name")->getRestrictedValue();
	morphUnit= ft->getUnitType(morphUnitName);

	//discount
	discount= n->getChild("discount")->getAttribute("value")->getIntValue();
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

void CastSpellCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	MoveBaseCommandType::load(id, n, dir, tt, ft, ut);

	//cast spell
   	string skillName= n->getChild("cast-spell-skill")->getAttribute("value")->getRestrictedValue();
	castSpellSkillType= static_cast<const CastSpellSkillType*>(ut.getSkillType(skillName, scCastSpell));
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

void GuardCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	AttackCommandType::load(id, n, dir, tt, ft, ut);

	//distance
	maxDistance = n->getChild("max-distance")->getAttribute("value")->getIntValue();
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
