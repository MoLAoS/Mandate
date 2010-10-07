// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "faction_type.h"

#include "logger.h"
#include "util.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "resource.h"
#include "platform_util.h"
#include "world.h"

#include "leak_dumper.h"


using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest { namespace ProtoTypes {

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

bool FactionType::preLoad(const string &dir, const TechTree *techTree) {
	name = basename(dir);

	// a1) preload units
	string unitsPath = dir + "/units/*.";
	vector<string> unitFilenames;
	bool loadOk = true;
	try { 
		findAll(unitsPath, unitFilenames); 
	} catch (runtime_error e) {
		g_errorLog.add(e.what());
		loadOk = false;
	}
	for (int i = 0; i < unitFilenames.size(); ++i) {
		string path = dir + "/units/" + unitFilenames[i];
		UnitType *ut = g_world.getMasterTypeFactory().newUnitType();
		unitTypes.push_back(ut);
		unitTypes.back()->preLoad(path);
	}
	// a2) preload upgrades
	string upgradesPath= dir + "/upgrades/*.";
	vector<string> upgradeFilenames;
	try { 
		findAll(upgradesPath, upgradeFilenames); 
	} catch (runtime_error e) {
		g_errorLog.add(e.what());
		loadOk = false;
	}
	for (int i = 0; i < upgradeFilenames.size(); ++i) {
		string path = dir + "/upgrades/" + upgradeFilenames[i];
		UpgradeType *ut = g_world.getMasterTypeFactory().newUpgradeType();
		upgradeTypes.push_back(ut);
		upgradeTypes.back()->preLoad(path);
	}
	return loadOk;
}

bool FactionType::preLoadGlestimals(const string &dir, const TechTree *techTree) {
	name = basename(dir);

	// a1) preload units
	string unitsPath = dir + "/glestimals/*.";
	vector<string> unitFilenames;
	bool loadOk = true;
	try { 
		findAll(unitsPath, unitFilenames); 
	} catch (runtime_error e) {
		g_errorLog.add(e.what());
		loadOk = false;
	}
	for (int i = 0; i < unitFilenames.size(); ++i) {
		string path = dir + "/glestimals/" + unitFilenames[i];
		UnitType *ut = g_world.getMasterTypeFactory().newUnitType();
		unitTypes.push_back(ut);
		unitTypes.back()->preLoad(path);
	}
	return loadOk;
}

//load a faction, given a directory
bool FactionType::load(int ndx, const string &dir, const TechTree *techTree) {
	Logger &logger = Logger::getInstance();
	logger.add("Faction type: "+ dir, true);
	id = ndx;
	name = basename(dir);

	bool loadOk = true;

	//open xml file
	string path = dir+"/"+name+".xml";
	XmlTree xmlTree;
	try { 
		xmlTree.load(path); 
	} catch (runtime_error e) { 
		g_errorLog.addXmlError(path, "File missing or wrongly named.");
		return false; // bail
	}
	const XmlNode *factionNode;
	try { 
		factionNode = xmlTree.getRootNode(); 
	} catch (runtime_error e) { 
		g_errorLog.addXmlError(path, "File appears to lack contents.");
		return false; // bail
	}
	//read subfaction list
	const XmlNode *subfactionsNode = factionNode->getChild("subfactions", 0, false);

	if (subfactionsNode) {
		for (int i = 0; i < subfactionsNode->getChildCount(); ++i) {
			// can't have more subfactions than an int has bits
			if (i >= sizeof(int) * 8) {
				throw runtime_error("Too many goddam subfactions.  Go write some code. " + dir);
			}
			subfactions.push_back(subfactionsNode->getChild("subfaction", i)->getAttribute("name")->getRestrictedValue());
		}
	}

	// progress : 0 - unitFileNames.size()

	// b1) load units
	for (int i = 0; i < unitTypes.size(); ++i) {
		string str = dir + "/units/" + unitTypes[i]->getName();
		if (!unitTypes[i]->load(str, techTree, this)) {
			loadOk = false;
		}
		g_world.getUnitTypeFactory().setChecksum(unitTypes[i]);
		logger.unitLoaded();
	}

	// b2) load upgrades
	for (int i = 0; i < upgradeTypes.size(); ++i) {
		string str = dir + "/upgrades/" + upgradeTypes[i]->getName();
		if (!upgradeTypes[i]->load(str, techTree, this)) {
			loadOk = false;
		}
		g_world.getUpgradeTypeFactory().setChecksum(upgradeTypes[i]);
	}

	//read starting resources
	try {
		const XmlNode *startingResourcesNode = factionNode->getChild("starting-resources");
		startingResources.resize(startingResourcesNode->getChildCount());
		for (int i = 0; i < startingResources.size(); ++i) {
			try {
				const XmlNode *resourceNode = startingResourcesNode->getChild("resource", i);
				string name = resourceNode->getAttribute("name")->getRestrictedValue();
				int amount = resourceNode->getAttribute("amount")->getIntValue();
				startingResources[i].init(techTree->getResourceType(name), amount);
			} catch (runtime_error e) {
				g_errorLog.addXmlError(path, e.what());
				loadOk = false;
			}
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(path, e.what());
		loadOk = false;
	}

	//read starting units
	try {
		const XmlNode *startingUnitsNode = factionNode->getChild("starting-units");
		for (int i = 0; i < startingUnitsNode->getChildCount(); ++i) {
			try {
				const XmlNode *unitNode = startingUnitsNode->getChild("unit", i);
				string name = unitNode->getAttribute("name")->getRestrictedValue();
				int amount = unitNode->getAttribute("amount")->getIntValue();
				startingUnits.push_back(PairPUnitTypeInt(getUnitType(name), amount));
			} catch (runtime_error e) {
				g_errorLog.addXmlError(path, e.what());
				loadOk = false;
			}
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(path, e.what());
		loadOk = false;
	}

	//read music
	try {
		const XmlNode *musicNode= factionNode->getChild("music");
		bool value = musicNode->getAttribute("value")->getBoolValue();
		if (value) {
			XmlAttribute *playListAttr = musicNode->getAttribute("play-list", false);
			if (playListAttr && playListAttr->getBoolValue()) {
				const XmlAttribute *shuffleAttrib = musicNode->getAttribute("shuffle", false);
				bool shuffle = (shuffleAttrib && shuffleAttrib->getBoolValue() ? true : false);

				vector<StrSound*> tracks;
				for (int i=0; i < musicNode->getChildCount(); ++i) {
					StrSound *sound = new StrSound();
					sound->open(dir+"/"+musicNode->getChild("music-file", i)->getAttribute("path")->getRestrictedValue());
					tracks.push_back(sound);
				}
				if (tracks.empty()) {
					throw runtime_error("No tracks in play-list!");
				}
				if (shuffle) {
					int seed = int(Chrono::getCurTicks());
					Random random(seed);
					Shared::Util::jumble(tracks, random);
				}
				vector<StrSound*>::iterator it = tracks.begin();
				vector<StrSound*>::iterator it2 = it + 1;
				while (it2 != tracks.end()) {
					(*it)->setNext(*it2);
					++it; ++it2;
				}
				music = tracks[0];
			} else {
				music = new StrSound();
				music->open(dir+"/"+musicNode->getAttribute("path")->getRestrictedValue());
			}
		}
	} catch (runtime_error e) { 
		g_errorLog.addXmlError(path, e.what());
		loadOk = false;
	}

	// notification of being attacked off screen
	try {
		const XmlNode *attackNoticeNode= factionNode->getChild("attack-notice", 0, false);
		if (attackNoticeNode && attackNoticeNode->getAttribute("enabled")->getBoolValue()) {
			attackNoticeDelay = attackNoticeNode->getAttribute("min-delay")->getIntValue();
			attackNotice = new SoundContainer();
			attackNotice->resize(attackNoticeNode->getChildCount());
			for (int i=0; i < attackNoticeNode->getChildCount(); ++i) {
				string path = attackNoticeNode->getChild("sound-file", i)->getAttribute("path")->getRestrictedValue();
				StaticSound *sound = new StaticSound();
				sound->load(dir + "/" + path);
				(*attackNotice)[i] = sound;
			}
			if (attackNotice->getSounds().size() == 0) {
				g_errorLog.addXmlError(path, "An enabled attack-notice must contain at least one sound-file.");
				loadOk = false;
			}
		}
	} catch (runtime_error &e) {
		g_errorLog.addXmlError(path, e.what());
		loadOk = false;
	}

	// notification of visual contact with enemy off screen
	try {
		const XmlNode *enemyNoticeNode= factionNode->getChild("enemy-notice", 0, false);
		if (enemyNoticeNode && enemyNoticeNode->getAttribute("enabled")->getRestrictedValue() == "true") {
			enemyNoticeDelay = enemyNoticeNode->getAttribute("min-delay")->getIntValue();
			enemyNotice = new SoundContainer();
			enemyNotice->resize(enemyNoticeNode->getChildCount());
			for (int i = 0; i < enemyNoticeNode->getChildCount(); ++i) {
				string path= enemyNoticeNode->getChild("sound-file", i)->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				(*enemyNotice)[i]= sound;
			}
			if (enemyNotice->getSounds().size() == 0) {
				g_errorLog.addXmlError(path, "An enabled enemy-notice must contain at least one sound-file.");
				loadOk = false;
			}
		}
	} catch (runtime_error &e) {
		g_errorLog.addXmlError(path, e.what());
		loadOk = false;
	}
	return loadOk;
}

bool FactionType::loadGlestimals(const string &dir, const TechTree *techTree) {
	Logger &logger = Logger::getInstance();
	logger.add("Glestimal Faction: " + dir, true);
	id = -1;
	name = basename(dir);
	bool loadOk = true;

	// load glestimals
	for (int i = 0; i < unitTypes.size(); ++i) {
		string str = dir + "/glestimals/" + unitTypes[i]->getName();
		if (unitTypes[i]->load(str, techTree, this, true)) {
			Checksum checksum;
			unitTypes[i]->doChecksum(checksum);
			g_world.getUnitTypeFactory().setChecksum(unitTypes[i]);
		} else {
			loadOk = false;
		}
		///@todo count glestimals
		//logger.unitLoaded();
	}
	return loadOk;
}

void FactionType::doChecksum(Checksum &checksum) const {
	checksum.add(name);
	foreach_const (UnitTypes, it, unitTypes) {
		(*it)->doChecksum(checksum);
	}
	foreach_const (UpgradeTypes, it, upgradeTypes) {
		(*it)->doChecksum(checksum);
	}
	foreach_const (StartingUnits, it, startingUnits) {
		checksum.add(it->first->getName());
		checksum.add<int>(it->second);
	}
	foreach_const (Resources, it, startingResources) {
		checksum.add(it->getType()->getName());
		checksum.add<int>(it->getAmount());
	}
	foreach_const (Subfactions, it, subfactions) {
		checksum.add(*it);
	}
}

FactionType::~FactionType() {
	while (music) {
		StrSound *delMusic = music;
		music = music->getNext();
		delete delMusic;
	}
	delete music;
	if (attackNotice) {
		deleteValues(attackNotice->getSounds().begin(), attackNotice->getSounds().end());
		delete attackNotice;
	}
	if (enemyNotice) {
		deleteValues(enemyNotice->getSounds().begin(), enemyNotice->getSounds().end());
		delete enemyNotice;
	}
}

// ==================== get ====================

int FactionType::getSubfactionIndex(const string &name) const {
    for (int i = 0; i < subfactions.size();i++) {
		if (subfactions[i] == name) {
            return i;
		}
    }
	throw runtime_error("Subfaction not found: " + name);
}

const UnitType *FactionType::getUnitType(const string &name) const{
    for (int i = 0; i < unitTypes.size(); ++i) {
		if (unitTypes[i]->getName() == name) {
            return unitTypes[i];
		}
    }
	throw runtime_error("Unit not found: " + name);
}

const UpgradeType *FactionType::getUpgradeType(const string &name) const{
    for (int i = 0; i < upgradeTypes.size(); ++i) {
		if (upgradeTypes[i]->getName() == name) {
            return upgradeTypes[i];
		}
    }
	throw runtime_error("Upgrade not found: " + name);
}

int FactionType::getStartingResourceAmount(const ResourceType *resourceType) const{
	for (int i = 0; i < startingResources.size(); ++i) {
		if (startingResources[i].getType() == resourceType) {
			return startingResources[i].getAmount();
		}
	}
	return 0;
}

}}//end namespace
