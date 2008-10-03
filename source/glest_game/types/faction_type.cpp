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

#include "faction_type.h"

#include "logger.h"
#include "util.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "resource.h"
#include "platform_util.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest{ namespace Game{

// ======================================================
//          Class FactionType
// ======================================================

FactionType::FactionType(){
	music= NULL;
	attackNotice = NULL;
	enemyNotice = NULL;
	attackNoticeDelay = 0;
	enemyNoticeDelay = 0;
	subfactions.push_back(string("base"));
}

//load a faction, given a directory
void FactionType::load(const string &dir, const TechTree *techTree, Checksum* checksum){

    Logger::getInstance().add("Faction type: "+ dir, true);
    name= lastDir(dir);

	// a1) preload units
	string unitsPath= dir + "/units/*.";
	vector<string> unitFilenames;
    findAll(unitsPath, unitFilenames);
	unitTypes.resize(unitFilenames.size());
    for(int i=0; i<unitTypes.size(); ++i){
		string str= dir + "/units/" + unitFilenames[i];
		unitTypes[i].preLoad(str);
    }

	// a2) preload upgrades
	string upgradesPath= dir + "/upgrades/*.";
	vector<string> upgradeFilenames;
    findAll(upgradesPath, upgradeFilenames);
	upgradeTypes.resize(upgradeFilenames.size());
    for(int i=0; i<upgradeTypes.size(); ++i){
		string str= dir + "/upgrades/" + upgradeFilenames[i];
		upgradeTypes[i].preLoad(str);
    }

	//open xml file
    string path= dir+"/"+name+".xml";

	checksum->addFile(path);

	XmlTree xmlTree;
	xmlTree.load(path);
	const XmlNode *factionNode= xmlTree.getRootNode();

	//read subfaction list
	const XmlNode *subfactionsNode= factionNode->getChild("subfactions", 0, false);
	if(subfactionsNode) {
		for(int i=0; i<subfactionsNode->getChildCount(); ++i){
			// can't have more subfactions than an int has bits
			if(i >= sizeof(int) * 8) {
				throw runtime_error("Too many goddam subfactions.  Go write some code. " + dir);
			}
			subfactions.push_back(subfactionsNode->getChild("subfaction", i)->getAttribute("name")->getRestrictedValue());
		}
	}

	// b1) load units
	try{
		for(int i=0; i<unitTypes.size(); ++i){
            string str= dir + "/units/" + unitTypes[i].getName();
            unitTypes[i].load(i, str, techTree, this, checksum);
        }
    }
	catch(const exception &e){
		throw runtime_error("Error loading units: "+ dir + "\n" + e.what());
	}

	// b2) load upgrades
	try{
		for(int i=0; i<upgradeTypes.size(); ++i){
            string str= dir + "/upgrades/" + upgradeTypes[i].getName();
            upgradeTypes[i].load(str, techTree, this, checksum);
        }
    }
	catch(const exception &e){
		throw runtime_error("Error loading upgrades: "+ dir + "\n" + e.what());
	}

	//read starting resources
	const XmlNode *startingResourcesNode= factionNode->getChild("starting-resources");

	startingResources.resize(startingResourcesNode->getChildCount());
	for(int i=0; i<startingResources.size(); ++i){
		const XmlNode *resourceNode= startingResourcesNode->getChild("resource", i);
		string name= resourceNode->getAttribute("name")->getRestrictedValue();
		int amount= resourceNode->getAttribute("amount")->getIntValue();
		startingResources[i].init(techTree->getResourceType(name), amount);
	}

	//read starting units
	const XmlNode *startingUnitsNode= factionNode->getChild("starting-units");
	for(int i=0; i<startingUnitsNode->getChildCount(); ++i){
		const XmlNode *unitNode= startingUnitsNode->getChild("unit", i);
		string name= unitNode->getAttribute("name")->getRestrictedValue();
		int amount= unitNode->getAttribute("amount")->getIntValue();
		startingUnits.push_back(PairPUnitTypeInt(getUnitType(name), amount));
	}

	//read music
	const XmlNode *musicNode= factionNode->getChild("music");
	bool value= musicNode->getAttribute("value")->getBoolValue();
	if(value){
		music= new StrSound();
		music->open(dir+"/"+musicNode->getAttribute("path")->getRestrictedValue());
	}

	//notification of being attacked off screen
	const XmlNode *attackNoticeNode= factionNode->getChild("attack-notice", 0, false);
	if(attackNoticeNode && attackNoticeNode->getAttribute("enabled")->getRestrictedValue() == "true") {
		attackNoticeDelay = attackNoticeNode->getAttribute("min-delay")->getIntValue();
		attackNotice = new SoundContainer();
		attackNotice->resize(attackNoticeNode->getChildCount());
		for(int i=0; i<attackNoticeNode->getChildCount(); ++i){
			string path= attackNoticeNode->getChild("sound-file", i)->getAttribute("path")->getRestrictedValue();
			StaticSound *sound= new StaticSound();
			sound->load(dir + "/" + path);
			(*attackNotice)[i]= sound;
		}
		if(attackNotice->getSounds().size() == 0) {
			throw runtime_error("An enabled attack-notice must contain at least one sound-file: "+ dir);
		}
	}

	//notification of visual contact with enemy off screen
	const XmlNode *enemyNoticeNode= factionNode->getChild("enemy-notice", 0, false);
	if(enemyNoticeNode && enemyNoticeNode->getAttribute("enabled")->getRestrictedValue() == "true") {
		enemyNoticeDelay = enemyNoticeNode->getAttribute("min-delay")->getIntValue();
		enemyNotice = new SoundContainer();
		enemyNotice->resize(enemyNoticeNode->getChildCount());
		for(int i=0; i<enemyNoticeNode->getChildCount(); ++i){
			string path= enemyNoticeNode->getChild("sound-file", i)->getAttribute("path")->getRestrictedValue();
			StaticSound *sound= new StaticSound();
			sound->load(dir + "/" + path);
			(*enemyNotice)[i]= sound;
		}
		if(enemyNotice->getSounds().size() == 0) {
			throw runtime_error("An enabled enemy-notice must contain at least one sound-file: "+ dir);
		}
	}
}

FactionType::~FactionType(){
	delete music;
	if(attackNotice) {
		deleteValues(attackNotice->getSounds().begin(), attackNotice->getSounds().end());
		delete attackNotice;
	}
	if(enemyNotice) {
		deleteValues(enemyNotice->getSounds().begin(), enemyNotice->getSounds().end());
		delete enemyNotice;
	}
}

// ==================== get ====================

int FactionType::getSubfactionIndex(const string &name) const {
    for(int i=0; i<subfactions.size();i++){
		if(subfactions[i]==name){
            return i;
		}
    }
	throw runtime_error("Subfaction not found: "+name);
}

const UnitType *FactionType::getUnitType(const string &name) const{
    for(int i=0; i<unitTypes.size();i++){
		if(unitTypes[i].getName()==name){
            return &unitTypes[i];
		}
    }
	throw runtime_error("Unit not found: "+name);
}

const UpgradeType *FactionType::getUpgradeType(const string &name) const{
    for(int i=0; i<upgradeTypes.size();i++){
		if(upgradeTypes[i].getName()==name){
            return &upgradeTypes[i];
		}
    }
	throw runtime_error("Upgrade not found: "+name);
}

int FactionType::getStartingResourceAmount(const ResourceType *resourceType) const{
	for(int i=0; i<startingResources.size(); ++i){
		if(startingResources[i].getType()==resourceType){
			return startingResources[i].getAmount();
		}
	}
	return 0;
}

}}//end namespace
