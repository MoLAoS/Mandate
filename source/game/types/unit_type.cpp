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
#include "unit_type.h"

#include <cassert>

#include "util.h"
#include "upgrade_type.h"
#include "resource_type.h"
#include "sound.h"
#include "logger.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "resource.h"
#include "renderer.h"

#include "leak_dumper.h"


using namespace Shared::Xml;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// ===============================
// 	class Level
// ===============================

bool Level::load(const XmlNode *levelNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = true;
	try { name = levelNode->getAttribute("name")->getRestrictedValue(); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError ( dir, e.what() );
		loadOk = false;
	}

	try { 
		kills = levelNode->getAttribute("kills")->getIntValue(); 
		const XmlAttribute *defaultsAtt = levelNode->getAttribute("defaults", 0);
		if(defaultsAtt && !defaultsAtt->getBoolValue()) {
			maxHpMult = 1.f;
			maxEpMult = 1.f;
			sightMult = 1.f;
			armorMult = 1.f;
			effectStrength = 0.0f;
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError ( dir, e.what() );
		loadOk = false;
	}

	if ( ! EnhancementTypeBase::load(levelNode, dir, tt, ft) )
		loadOk = false;
	return loadOk;
}

// ===============================
// 	class PetRule
// ===============================
/*
void PetRule::load(const XmlNode *prn, const string &dir, const TechTree *tt, const FactionType *ft){
string unitTypeName = prn->getAttribute("type")->getRestrictedValue();
type = ft->getUnitType(unitTypeName);
count = prn->getAttribute("count")->getIntValue();
}
*/

// =====================================================
// 	class UnitType
// =====================================================

// ===================== PUBLIC ========================


// ==================== creation and loading ====================

UnitType::UnitType(){
	reset();
	multiSelect= false;
	cellMap= NULL;
}

UnitType::~UnitType(){
	deleteValues(commandTypes.begin(), commandTypes.end());
	deleteValues(skillTypes.begin(), skillTypes.end());
	deleteValues(selectionSounds.getSounds().begin(), selectionSounds.getSounds().end());
	deleteValues(commandSounds.getSounds().begin(), commandSounds.getSounds().end());
	delete [] cellMap;
}

void UnitType::preLoad(const string &dir){
	name= basename(dir);
}

bool UnitType::load(int id, const string &dir, const TechTree *techTree, const FactionType *factionType){
	this->id = id;
	string path;

	Logger::getInstance().add("Unit type: " + dir, true);
	bool loadOk = true;
	//file load
	path= dir+"/"+name+".xml";

	//checksum.addFile(path, true);

	XmlTree xmlTree;
	try { xmlTree.load(path); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		return false; // bail
	}
	const XmlNode *unitNode;
	try { unitNode = xmlTree.getRootNode(); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		return false;
	}
	const XmlNode *parametersNode;
	try { parametersNode = unitNode->getChild("parameters"); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		return false; // bail out
	}
	if ( ! UnitStatsBase::load(parametersNode, dir, techTree, factionType) )
		loadOk = false;
	halfSize = size / 2.f;
	halfHeight = height / 2.f;
	//prod time
	try { productionTime= parametersNode->getChildIntValue("time"); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//multi-build
	try { multiBuild = parametersNode->getOptionalBoolValue("multi-build"); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//multi selection
	try { multiSelect= parametersNode->getChildBoolValue("multi-selection"); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//cellmap
	try {
		const XmlNode *cellMapNode= parametersNode->getChild("cellmap", 0, false);
		if(cellMapNode && cellMapNode->getAttribute("value")->getBoolValue()){
			cellMap= new bool[size*size];
			for(int i=0; i<size; ++i){
				try {
					const XmlNode *rowNode= cellMapNode->getChild("row", i);
					string row= rowNode->getAttribute("value")->getRestrictedValue();
					if(row.size()!=size){
						throw runtime_error("Cellmap row has not the same length as unit size");
					}
					for(int j=0; j<row.size(); ++j){
						cellMap[i*size+j]= row[j]=='0'? false: true;
					}
				}
				catch (runtime_error e) {
					Logger::getErrorLog().addXmlError(path, e.what());
					loadOk = false;
				}
			}
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//levels
	try {
		const XmlNode *levelsNode= parametersNode->getChild("levels", 0, false);
		if(levelsNode) {
			levels.resize(levelsNode->getChildCount());
			for(int i=0; i<levels.size(); ++i){
				const XmlNode *levelNode= levelsNode->getChild("level", i);
				levels[i].load(levelNode, dir, techTree, factionType);
			}
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//fields
	try { fields.load(parametersNode->getChild("fields"), dir, techTree, factionType); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//properties
	try { properties.load(parametersNode->getChild("properties"), dir, techTree, factionType); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//ProducibleType parameters
	try { ProducibleType::load(parametersNode, dir, techTree, factionType); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//resources stored
	try {
		const XmlNode *resourcesStoredNode= parametersNode->getChild("resources-stored", 0, false);
		if(resourcesStoredNode) {
			storedResources.resize(resourcesStoredNode->getChildCount());
			for(int i=0; i<storedResources.size(); ++i){
				const XmlNode *resourceNode= resourcesStoredNode->getChild("resource", i);
				string name= resourceNode->getAttribute("name")->getRestrictedValue();
				int amount= resourceNode->getAttribute("amount")->getIntValue();
				storedResources[i].init(techTree->getResourceType(name), amount);
			}
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//image
	try { DisplayableType::load(parametersNode, dir); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}

	//image cancel
	try {
		const XmlNode *imageCancelNode= parametersNode->getChild("image-cancel");
		cancelImage= Renderer::getInstance().newTexture2D(rsGame);
		cancelImage->load(dir+"/"+imageCancelNode->getAttribute("path")->getRestrictedValue());
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//meeting point
	try {
		const XmlNode *meetingPointNode= parametersNode->getChild("meeting-point");
		meetingPoint= meetingPointNode->getAttribute("value")->getBoolValue();
		if(meetingPoint){
			meetingPointImage= Renderer::getInstance().newTexture2D(rsGame);
			meetingPointImage->load(dir+"/"+meetingPointNode->getAttribute("image-path")->getRestrictedValue());
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//selection sounds
	try {
		const XmlNode *selectionSoundNode= parametersNode->getChild("selection-sounds");
		if(selectionSoundNode->getAttribute("enabled")->getBoolValue()){
			selectionSounds.resize(selectionSoundNode->getChildCount());
			for(int i=0; i<selectionSounds.getSounds().size(); ++i){
				const XmlNode *soundNode= selectionSoundNode->getChild("sound", i);
				string path= soundNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				selectionSounds[i]= sound;
			}
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//command sounds
	try {
		const XmlNode *commandSoundNode= parametersNode->getChild("command-sounds");
		if(commandSoundNode->getAttribute("enabled")->getBoolValue()){
			commandSounds.resize(commandSoundNode->getChildCount());
			for(int i=0; i<commandSoundNode->getChildCount(); ++i){
				const XmlNode *soundNode= commandSoundNode->getChild("sound", i);
				string path= soundNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				commandSounds[i]= sound;
			}
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	//skills
	try
	{
		const XmlNode *skillsNode= unitNode->getChild("skills");
		skillTypes.resize(skillsNode->getChildCount());
		for(int i=0; i<skillTypes.size(); ++i){
			const XmlNode *sn= skillsNode->getChild("skill", i);
			const XmlNode *typeNode= sn->getChild("type");
			string classId= typeNode->getAttribute("value")->getRestrictedValue();
			SkillType *skillType= SkillTypeFactory::getInstance().newInstance(classId);
			skillType->load(sn, dir, techTree, factionType);
			skillTypes[i]= skillType;
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		return false; // if skills are screwy, stop
	}

	//commands
	try {
		const XmlNode *commandsNode = unitNode->getChild("commands");
		commandTypes.resize(commandsNode->getChildCount());
		for (int i = 0; i < commandTypes.size(); ++i) {
			const XmlNode *commandNode = commandsNode->getChild("command", i);
			string classId = commandNode->getChildRestrictedValue("type");
			CommandType *commandType = CommandTypeFactory::getInstance().newInstance(classId);
			commandType->setUnitTypeAndIndex(this, i);
			commandType->load(commandNode, dir, techTree, factionType);
			commandTypes[i] = commandType;
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}

	if ( !loadOk ) return false; // unsafe to keep going...

	// if type has a meeting point, add a SetMeetingPoint command
	if(meetingPoint) {
		commandTypes.push_back(new SetMeetingPointCommandType());
		commandTypes.back()->setUnitTypeAndIndex(this, commandTypes.size() - 1);
	}

	computeFirstStOfClass();
	computeFirstCtOfClass();
	try { // Logger::addXmlError() expects a char*, so it's easier just to throw & catch ;)
		if(!getFirstStOfClass(SkillClass::STOP)) {
			throw runtime_error("Every unit must have at least one stop skill: "+ path);
		}
		if(!getFirstStOfClass(SkillClass::DIE)) {
			throw runtime_error("Every unit must have at least one die skill: "+ path);
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	// if it's mobile and doesn't have a fall down skill, give it one
	if(!firstSkillTypeOfClass[SkillClass::FALL_DOWN] && firstSkillTypeOfClass[SkillClass::MOVE]) {
		skillTypes.push_back(new FallDownSkillType(firstSkillTypeOfClass[SkillClass::DIE]));
	}

	if(!firstSkillTypeOfClass[SkillClass::GET_UP] && firstSkillTypeOfClass[SkillClass::MOVE]) {
		skillTypes.push_back(new GetUpSkillType(firstSkillTypeOfClass[SkillClass::MOVE]));
	}

	//push dummy wait for server skill type and recalculate first of skill cache
	skillTypes.push_back(new WaitForServerSkillType(getFirstStOfClass(SkillClass::STOP)));
	computeFirstStOfClass();

	/*
	//petRules
	const XmlNode *petRulesNode= unitNode->getChild("pet-rules", 0, false);
	if(petRulesNode) {
	PetRule *petRule;
	petRules.resize(petRulesNode->getChildCount());
	for(int i=0; i<petRules.size(); ++i) {
	const XmlNode *n= petRulesNode->getChild("pet-rule", i);
	petRules.push_back(petRule = new PetRule());
	petRule->load(n, dir, techTree, factionType);
	}
	}
	*/

	//emanations
	try {
		const XmlNode *emanationsNode = parametersNode->getChild("emanations", 0, false);
		if (emanationsNode) {
			emanations.resize(emanationsNode->getChildCount());
			for (int i = 0; i < emanationsNode->getChildCount(); ++i) {
				try {
					const XmlNode *emanationNode = emanationsNode->getChild("emanation", i);
					Emanation *emanation = new Emanation();
					emanation->load(emanationNode, dir, techTree, factionType);
					emanations[i] = emanation;
				}
				catch (runtime_error e) {
					Logger::getErrorLog().addXmlError(path, e.what());
					loadOk = false;
				}
			}
		}
	}
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false;
	}
	return loadOk;   
}

void UnitType::doChecksum(Checksum &checksum) const {
	ProducibleType::doChecksum(checksum);
	UnitStatsBase::doChecksum(checksum);

	checksum.add<bool>(multiBuild);
	checksum.add<bool>(multiSelect);
	foreach_const (SkillTypes, it, skillTypes) {
		(*it)->doChecksum(checksum);
	}
	foreach_const (CommandTypes, it, commandTypes) {
		(*it)->doChecksum(checksum);
	}
	foreach_const (StoredResources, it, storedResources) {
		checksum.addString(it->getType()->getName());
		checksum.add<int>(it->getAmount());
	}
	foreach_const (Levels, it, levels) {
		it->doChecksum(checksum);
	}
	foreach_const (Emanations, it, emanations) {
		(*it)->doChecksum(checksum);
	}

	//meeting point
	checksum.add<bool>(meetingPoint);
	checksum.add<float>(halfSize);
	checksum.add<float>(halfHeight);

}

// ==================== get ====================



const CommandType *UnitType::getCommandType(const string &name) const {
	for(CommandTypes::const_iterator i = commandTypes.begin(); i != commandTypes.end(); ++i) {
		if((*i)->getName() == name) {
			return (*i);
		}
	}
	return NULL;
}

const HarvestCommandType *UnitType::getFirstHarvestCommand(const ResourceType *resourceType) const{
	for(int i=0; i<commandTypes.size(); ++i){
		if(commandTypes[i]->getClass()== CommandClass::HARVEST){
			const HarvestCommandType *hct= static_cast<const HarvestCommandType*>(commandTypes[i]);
			if(hct->canHarvest(resourceType)){
				return hct;
			}
		}
	}
	return NULL;
}

const AttackCommandType *UnitType::getFirstAttackCommand(Zone zone) const{
	for(int i=0; i<commandTypes.size(); ++i){
		if(commandTypes[i]->getClass()== CommandClass::ATTACK){
			const AttackCommandType *act= static_cast<const AttackCommandType*>(commandTypes[i]);
			if(act->getAttackSkillTypes()->getZone(zone)){
				return act;
			}
		}
	}
	return NULL;
}


const RepairCommandType *UnitType::getFirstRepairCommand(const UnitType *repaired) const{
	for(int i=0; i<commandTypes.size(); ++i){
		if(commandTypes[i]->getClass()== CommandClass::REPAIR){
			const RepairCommandType *rct= static_cast<const RepairCommandType*>(commandTypes[i]);
			if(rct->isRepairableUnitType(repaired)){
				return rct;
			}
		}
	}
	return NULL;
}

int UnitType::getStore(const ResourceType *rt) const{
	for(int i=0; i<storedResources.size(); ++i){
		if(storedResources[i].getType()==rt){
			return storedResources[i].getAmount();
		}
	}
	return 0;
}

const SkillType *UnitType::getSkillType(const string &skillName, SkillClass skillClass) const{
	for(int i=0; i<skillTypes.size(); ++i){
		if(skillTypes[i]->getName()==skillName){
			if(skillTypes[i]->getClass() == skillClass || skillClass == SkillClass::COUNT){
				return skillTypes[i];
			}
			else{
				throw runtime_error("Skill \""+skillName+"\" is not of class \""+SkillType::skillClassToStr(skillClass));
			}
		}
	}
	throw runtime_error("No skill named \""+skillName+"\"");
}


// ==================== has ====================

bool UnitType::hasSkillClass(SkillClass skillClass) const {
	return firstSkillTypeOfClass[skillClass] != NULL;
}

bool UnitType::hasCommandType(const CommandType *commandType) const {
	assert(commandType != NULL);
	for (int i = 0; i < commandTypes.size(); ++i) {
		if (commandTypes[i] == commandType) {
			return true;
		}
	}
	return false;
}

bool UnitType::hasCommandClass(CommandClass commandClass) const{
	return firstCommandTypeOfClass[commandClass] != NULL;
}

bool UnitType::hasSkillType(const SkillType *skillType) const{
	assert(skillType != NULL);
	for (int i = 0; i < skillTypes.size(); ++i) {
		if (skillTypes[i] == skillType) {
			return true;
		}
	}
	return false;
}

bool UnitType::isOfClass(UnitClass uc) const{
	switch (uc) {
		case UnitClass::WARRIOR:
			return hasSkillClass(SkillClass::ATTACK) && !hasSkillClass(SkillClass::HARVEST);
		case UnitClass::WORKER:
			return hasSkillClass(SkillClass::BUILD) || hasSkillClass(SkillClass::REPAIR);
		case UnitClass::BUILDING:
			return hasSkillClass(SkillClass::BE_BUILT) && !hasSkillClass(SkillClass::MOVE);
		default:
			throw runtime_error("Unknown UnitClass passed to UnitType::isOfClass()");
	}
	return false;
}

// ==================== PRIVATE ====================

void UnitType::computeFirstStOfClass(){
	for (int j = 0; j < SkillClass::COUNT; ++j) {
		firstSkillTypeOfClass[j] = NULL;
		for (int i= 0; i < skillTypes.size(); ++i) {
			if (skillTypes[i]->getClass() == enum_cast<SkillClass>(j)) {
				firstSkillTypeOfClass[j] = skillTypes[i];
				break;
			}
		}
	}
}

void UnitType::computeFirstCtOfClass() {
	for (int j = 0; j < CommandClass::COUNT; ++j) {
		firstCommandTypeOfClass[j]= NULL;
		for (int i = 0; i < commandTypes.size(); ++i) {
			if (commandTypes[i]->getClass() == enum_cast<CommandClass>(j)) {
				firstCommandTypeOfClass[j] = commandTypes[i];
				break;
			}
		}
	}
}

const CommandType* UnitType::findCommandTypeById(int id) const {
	for (int i = 0; i < getCommandTypeCount(); ++i) {
		const CommandType* commandType = getCommandType(i);
		if (commandType->getId() == id) {
			return commandType;
		}
	}
	return NULL;
}

}}//end namespace
