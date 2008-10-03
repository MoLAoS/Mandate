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

void Level::load(const XmlNode *levelNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	name = levelNode->getAttribute("name")->getRestrictedValue();
	kills = levelNode->getAttribute("kills")->getIntValue();

	const XmlAttribute *defaultsAtt = levelNode->getAttribute("defaults", 0);
	if(defaultsAtt && !defaultsAtt->getBoolValue()) {
		maxHpMult = 1.f;
		maxEpMult = 1.f;
		sightMult = 1.f;
		armorMult = 1.f;
		effectStrength = 0.0f;
	}

	EnhancementTypeBase::load(levelNode, dir, tt, ft);
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
	name= lastDir(dir);
}

void UnitType::load(int id, const string &dir, const TechTree *techTree, const FactionType *factionType, Checksum &checksum){
	this->id = id;
	string path;

	try {
		Logger::getInstance().add("Unit type: " + dir, true);

		//file load
		path= dir+"/"+name+".xml";

		checksum.addFile(path, true);

		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *unitNode= xmlTree.getRootNode();

		const XmlNode *parametersNode= unitNode->getChild("parameters");

		UnitStatsBase::load(parametersNode, dir, techTree, factionType);

		//prod time
		productionTime= parametersNode->getChildIntValue("time");

		//multi-build
		multiBuild = parametersNode->getOptionalBoolValue("multi-build");

		//multi selection
		multiSelect= parametersNode->getChildBoolValue("multi-selection");

		//cellmap
		const XmlNode *cellMapNode= parametersNode->getChild("cellmap", 0, false);
		if(cellMapNode && cellMapNode->getAttribute("value")->getBoolValue()){
			cellMap= new bool[size*size];
			for(int i=0; i<size; ++i){
				const XmlNode *rowNode= cellMapNode->getChild("row", i);
				string row= rowNode->getAttribute("value")->getRestrictedValue();
				if(row.size()!=size){
					throw runtime_error("Cellmap row has not the same length as unit size");
				}
				for(int j=0; j<row.size(); ++j){
					cellMap[i*size+j]= row[j]=='0'? false: true;
				}
			}
		}

		//levels
		const XmlNode *levelsNode= parametersNode->getChild("levels", 0, false);
		if(levelsNode) {
			levels.resize(levelsNode->getChildCount());
			for(int i=0; i<levels.size(); ++i){
				const XmlNode *levelNode= levelsNode->getChild("level", i);
				levels[i].load(levelNode, dir, techTree, factionType);
			}
		}

		//fields
		fields.load(parametersNode->getChild("fields"), dir, techTree, factionType);

		//properties
		properties.load(parametersNode->getChild("properties"), dir, techTree, factionType);

		//ProducibleType parameters
		ProducibleType::load(parametersNode, dir, techTree, factionType);

		//resources stored
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

		//image
		const XmlNode *imageNode= parametersNode->getChild("image");
		image= Renderer::getInstance().newTexture2D(rsGame);
		image->load(dir+"/"+imageNode->getAttribute("path")->getRestrictedValue());

		//image cancel
		const XmlNode *imageCancelNode= parametersNode->getChild("image-cancel");
		cancelImage= Renderer::getInstance().newTexture2D(rsGame);
		cancelImage->load(dir+"/"+imageCancelNode->getAttribute("path")->getRestrictedValue());

		//meeting point
		const XmlNode *meetingPointNode= parametersNode->getChild("meeting-point");
		meetingPoint= meetingPointNode->getAttribute("value")->getBoolValue();
		if(meetingPoint){
			meetingPointImage= Renderer::getInstance().newTexture2D(rsGame);
			meetingPointImage->load(dir+"/"+meetingPointNode->getAttribute("image-path")->getRestrictedValue());
		}

		//selection sounds
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

		//command sounds
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

		//skills
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

		//commands
		const XmlNode *commandsNode= unitNode->getChild("commands");
		commandTypes.resize(commandsNode->getChildCount());
		for(int i=0; i<commandTypes.size(); ++i){
			const XmlNode *commandNode= commandsNode->getChild("command", i);
			const XmlNode *typeNode= commandNode->getChild("type");
			string classId= typeNode->getAttribute("value")->getRestrictedValue();
			CommandType *commandType= CommandTypeFactory::getInstance().newInstance(classId);
			commandType->load(i, commandNode, dir, techTree, factionType, *this);
			commandTypes[i]= commandType;
		}

		computeFirstStOfClass();
		computeFirstCtOfClass();

		if(!getFirstStOfClass(scStop)) {
			throw runtime_error("Every unit must have at least one stop skill: "+ path);
		}
		if(!getFirstStOfClass(scDie)) {
			throw runtime_error("Every unit must have at least one die skill: "+ path);
		}

		// if it's mobile and doesn't have a fall down skill, give it one
		if(!firstSkillTypeOfClass[scFallDown] && firstSkillTypeOfClass[scMove]) {
			skillTypes.push_back(new FallDownSkillType(firstSkillTypeOfClass[scDie]));
		}

		if(!firstSkillTypeOfClass[scGetUp] && firstSkillTypeOfClass[scMove]) {
			skillTypes.push_back(new GetUpSkillType(firstSkillTypeOfClass[scMove]));
		}

		//push dummy wait for server skill type and recalculate first of skill cache
		skillTypes.push_back(new WaitForServerSkillType(getFirstStOfClass(scStop)));
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
		const XmlNode *emanationsNode = parametersNode->getChild("emanations", 0, false);

		if (emanationsNode) {
			emanations.resize(emanationsNode->getChildCount());

			for (int i = 0; i < emanationsNode->getChildCount(); ++i) {
				const XmlNode *emanationNode = emanationsNode->getChild("emanation", i);
				Emanation *emanation = new Emanation();
				emanation->load(emanationNode, dir, techTree, factionType);
				emanations[i] = emanation;
			}
		}
	}
	//Exception handling (conversions and so on);
	catch(const exception &e){
		throw runtime_error("Error loading UnitType: " + path + "\n" + e.what());
	}
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
		if(commandTypes[i]->getClass()== ccHarvest){
			const HarvestCommandType *hct= static_cast<const HarvestCommandType*>(commandTypes[i]);
			if(hct->canHarvest(resourceType)){
				return hct;
			}
		}
	}
	return NULL;
}

const AttackCommandType *UnitType::getFirstAttackCommand(Field field) const{
	for(int i=0; i<commandTypes.size(); ++i){
		if(commandTypes[i]->getClass()== ccAttack){
			const AttackCommandType *act= static_cast<const AttackCommandType*>(commandTypes[i]);
			if(act->getAttackSkillTypes()->getField(field)){
				return act;
			}
		}
	}
	return NULL;
}

const RepairCommandType *UnitType::getFirstRepairCommand(const UnitType *repaired) const{
	for(int i=0; i<commandTypes.size(); ++i){
		if(commandTypes[i]->getClass()== ccRepair){
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
			if(skillTypes[i]->getClass() == skillClass || skillClass == scCount){
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

bool UnitType::hasSkillClass(SkillClass skillClass) const{
    return firstSkillTypeOfClass[skillClass]!=NULL;
}

bool UnitType::hasCommandType(const CommandType *commandType) const{
    assert(commandType!=NULL);
    for(int i=0; i<commandTypes.size(); ++i){
        if(commandTypes[i]==commandType){
            return true;
        }
    }
    return false;
}

bool UnitType::hasCommandClass(CommandClass commandClass) const{
	return firstCommandTypeOfClass[commandClass]!=NULL;
}

bool UnitType::hasSkillType(const SkillType *skillType) const{
    assert(skillType!=NULL);
    for(int i=0; i<skillTypes.size(); ++i){
        if(skillTypes[i]==skillType){
            return true;
        }
    }
    return false;
}

bool UnitType::isOfClass(UnitClass uc) const{
	switch(uc){
	case ucWarrior:
		return hasSkillClass(scAttack) && !hasSkillClass(scHarvest);
	case ucWorker:
		return hasSkillClass(scBuild) || hasSkillClass(scRepair);
	case ucBuilding:
		return hasSkillClass(scBeBuilt) && !hasSkillClass(scMove);
	default:
		assert(false);
	}
	return false;
}

// ==================== PRIVATE ====================

void UnitType::computeFirstStOfClass(){
	for(int j= 0; j<scCount; ++j){
        firstSkillTypeOfClass[j]= NULL;
        for(int i= 0; i<skillTypes.size(); ++i){
            if(skillTypes[i]->getClass()== SkillClass(j)){
                firstSkillTypeOfClass[j]= skillTypes[i];
                break;
            }
        }
    }
}

void UnitType::computeFirstCtOfClass(){
    for(int j=0; j<ccCount; ++j){
        firstCommandTypeOfClass[j]= NULL;
        for(int i=0; i<commandTypes.size(); ++i){
            if(commandTypes[i]->getClass()== CommandClass(j)){
                firstCommandTypeOfClass[j]= commandTypes[i];
                break;
            }
        }
    }
}

const CommandType* UnitType::findCommandTypeById(int id) const{
	for(int i=0; i<getCommandTypeCount(); ++i){
		const CommandType* commandType= getCommandType(i);
		if(commandType->getId()==id){
			return commandType;
		}
	}
	return NULL;
}

}}//end namespace
