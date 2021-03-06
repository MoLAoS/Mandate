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
#include "tech_tree.h"

#include <cassert>

#include "util.h"
#include "resource.h"
#include "faction_type.h"
#include "logger.h"
#include "xml_parser.h"
#include "platform_util.h"
#include "program.h"

#include "leak_dumper.h"
#include "profiler.h"

using Glest::Util::Logger;
using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest { namespace ProtoTypes {
using Main::Program;

// =====================================================
// 	class TechTree
// =====================================================

bool TechTree::preload(const string &dir, const set<string> &factionNames){
	Logger &logger = Logger::getInstance();
	bool loadOk = true;
	XmlTree	xmlTree;
	string path;
	name = basename(dir);
	try {
		path = dir + "/" + name + ".xml";
		xmlTree.load(path);
	}
	catch (runtime_error &e) {
		g_logger.logXmlError ( path, "File missing or wrongly named." );
		return false;
	}
	const XmlNode *techTreeNode;
	try { techTreeNode= xmlTree.getRootNode(); }
	catch (runtime_error &e) {
		g_logger.logXmlError ( path, "File appears to lack contents." );
		return false;
	}

    const XmlNode *damagesNode = techTreeNode->getChild("damage-types");
    damageTypes.resize(damagesNode->getChildCount());
    resistances.resize(damagesNode->getChildCount());
	for (int i = 0; i < damagesNode->getChildCount(); ++i) {
	    const XmlNode *damageNode = damagesNode->getChild("damage-type", i);
        damageTypes[i].load(damageNode);
        resistances[i].load(damageNode);
	}

    // check for included factions
    vector<string> factionTypeNameList;
    vector<string> factionTypeNames;
	set<string>::const_iterator ftn;
    for (ftn = factionNames.begin(); ftn != factionNames.end(); ++ftn) {
        factionTypeNames.push_back(*ftn);
    }

    for (int i = 0; i < factionTypeNames.size(); ++i) {
        // 1. Open xml file to get included faction names
        string path = dir + "/factions/" + factionTypeNames[i] + "/" + factionTypeNames[i] + ".xml";
        XmlTree xmlTrees;
        try {
            xmlTrees.load(path);
        } catch (runtime_error e) {
            g_logger.logXmlError(path, "File missing or wrongly named.");
            return false; // bail
        }
        const XmlNode *factionNode;
        try {
            factionNode = xmlTrees.getRootNode();
        } catch (runtime_error e) {
            g_logger.logXmlError(path, "File appears to lack contents.");
            return false; // bail
        }
        try {
            const XmlNode *factionTypeNamesNode = factionNode->getChild("faction-type-names", 0, false);
            if (factionTypeNamesNode) {
                for (int i = 0; i < factionTypeNamesNode->getChildCount(); ++i) {
                    try {
                        const XmlNode *factionTypeNameNode = factionTypeNamesNode->getChild("faction-type", i, false);
                        if (factionTypeNameNode) {
                            string factionTypeName = factionTypeNameNode->getAttribute("name")->getRestrictedValue();
                            factionTypeNameList.push_back(factionTypeName);
                        }
                    } catch (runtime_error e) {
                    g_logger.logXmlError(path, e.what());
                    loadOk = false;
                    }
                }
            }
        } catch (runtime_error e) {
            g_logger.logXmlError(path, e.what());
            loadOk = false;
        }
    }

    for (int i = 0; i < factionTypeNames.size(); ++i) {
        factionTypeNameList.push_back(factionTypeNames[i]);
    }
	//load factions
	factionTypes.resize(factionTypeNameList.size());
	int numUnitTypes = 0;
	for (int i = 0; i < factionTypeNameList.size(); ++i) {
		if (!factionTypes[i].preLoad(dir + "/factions/" + factionTypeNameList[i], this)) {
			loadOk = false;
		} else {
			numUnitTypes += factionTypes[i].getUnitTypeCount();
		}
		if (!factionTypes[i].guiPreLoad(dir + "/factions/" + factionTypeNameList[i], this)) {
			loadOk = false;
		}
	}
	logger.getProgramLog().addUnitCount(numUnitTypes);

	if(resourceTypes.size() > 256) {
		throw runtime_error("Glest Advanced Engine currently only supports 256 resource types.");
	}
	return loadOk;
}

bool TechTree::load(const string &dir, const set<string> &factionNames){
	bool loadOk=true;

	Logger &logger = Logger::getInstance();
	logger.logProgramEvent("TechTree: "+ dir, true);

	//load resources
	vector<string> filenames;
	string str= dir + "/resources/*.";

	try {
		findAll(str, filenames);
		resourceTypes.resize(filenames.size());
	}
	catch(const exception &e) {
		throw runtime_error("Error loading Resource Types: "+ dir + "\n" + e.what());
	}
	if(resourceTypes.size() > 256) {
		g_logger.logXmlError(str, "Glest Advanced Engine currently only supports 256 resource types.");
		loadOk = false;
	} else {
		for(int i=0; i<filenames.size(); ++i){
			str=dir+"/resources/"+filenames[i];
			if (!resourceTypes[i].load(str, i)) {
				loadOk = false;
			}
			resourceTypeMap[filenames[i]] = &resourceTypes[i];
		}
	}

    // check for included factions
    vector<string> factionTypeNameList;
    vector<string> factionTypeNames;
	set<string>::const_iterator ftn;
    for (ftn = factionNames.begin(); ftn != factionNames.end(); ++ftn) {
        factionTypeNames.push_back(*ftn);
    }
    for (int i = 0; i < factionTypeNames.size(); ++i) {
        // 1. Open xml file to get included faction names
        string factn = factionTypeNames[i];
        string path = dir + "/factions/" + factn + "/" + factn + ".xml";
        XmlTree xmlTrees;
        try {
            xmlTrees.load(path);
        } catch (runtime_error e) {
            g_logger.logXmlError(path, "File missing or wrongly named.");
            return false; // bail
        }
        const XmlNode *factionNode;
        try {
            factionNode = xmlTrees.getRootNode();
        } catch (runtime_error e) {
            g_logger.logXmlError(path, "File appears to lack contents.");
            return false; // bail
        }
        try {
            const XmlNode *factionTypeNamesNode = factionNode->getChild("faction-type-names", 0, false);
            if (factionTypeNamesNode) {
                for (int i = 0; i < factionTypeNamesNode->getChildCount(); ++i) {
                    try {
                        const XmlNode *factionTypeNameNode = factionTypeNamesNode->getChild("faction-type", i, false);
                        if (factionTypeNameNode) {
                            string factionTypeName = factionTypeNameNode->getAttribute("name")->getRestrictedValue();
                            factionTypeNameList.push_back(factionTypeName);
                        }
                    } catch (runtime_error e) {
                    g_logger.logXmlError(path, e.what());
                    loadOk = false;
                    }
                }
            }
        } catch (runtime_error e) {
            g_logger.logXmlError(path, e.what());
            loadOk = false;
        }
    }

    for (int i = 0; i < factionTypeNames.size(); ++i) {
        factionTypeNameList.push_back(factionTypeNames[i]);
    }

	for (int i = 0; i < factionTypeNameList.size(); ++i) {
        factionTypeMap[factionTypeNameList[i]] = &factionTypes[i];
		if (!factionTypes[i].load(i, dir + "/factions/" + factionTypeNameList[i], this)) {
			loadOk = false;
		} else {
		}
	}
	for (int i = 0; i < factionTypeNameList.size(); ++i) {
        if (!factionTypes[i].loadSpecTraitSkills(i, dir + "/factions/" + factionTypeNameList[i], this)) {
            loadOk = false;
        }
	}
	set<string> names;
	for (int i = 0; i < factionTypeNameList.size(); ++i) {
        names.insert(factionTypeNameList[i]);
	}
	string techName = basename(dir);
	g_lang.loadFactionStrings(techName, names);

	XmlTree	xmlTechTree;
	string path;
	name = basename(dir);
	try {
		path = dir + "/" + name + ".xml";
		xmlTechTree.load(path);
	}
	catch (runtime_error &e) {
		g_logger.logXmlError ( path, "File missing or wrongly named." );
		return false;
	}
	const XmlNode *techTreeNode;
	try { techTreeNode= xmlTechTree.getRootNode(); }
	catch (runtime_error &e) {
		g_logger.logXmlError ( path, "File appears to lack contents." );
		return false;
	}

    const XmlNode *weaponsNode = techTreeNode->getChild("weapon-types");
    weaponStats.resize(weaponsNode->getChildCount());
	for (int i = 0; i < weaponsNode->getChildCount(); ++i) {
	    const XmlNode *weaponNode = weaponsNode->getChild("weapon-type", i);
	    string name = weaponNode->getAttribute("name")->getRestrictedValue();
        weaponStats[i].init(0, name);
	}

    const XmlNode *armorsNode = techTreeNode->getChild("armor-types");
    armorStats.resize(armorsNode->getChildCount());
	for (int i = 0; i < armorsNode->getChildCount(); ++i) {
	    const XmlNode *armorNode = armorsNode->getChild("armor-type", i);
	    string name = armorNode->getAttribute("name")->getRestrictedValue();
        armorStats[i].init(0, name);
	}

    const XmlNode *accessoriesNode = techTreeNode->getChild("accessory-types");
    accessoryStats.resize(accessoriesNode->getChildCount());
	for (int i = 0; i < accessoriesNode->getChildCount(); ++i) {
	    const XmlNode *accessoryNode = accessoriesNode->getChild("accessory-type", i);
	    string name = accessoryNode->getAttribute("name")->getRestrictedValue();
        accessoryStats[i].init(0, name);
	}

    const XmlNode *resourcesNode = techTreeNode->getChild("resource-types");
    resourceStats.resize(resourcesNode->getChildCount());
	for (int i = 0; i < resourcesNode->getChildCount(); ++i) {
	    const XmlNode *resourceNode = resourcesNode->getChild("resource-type", i);
        resourceStats[i].load(resourceNode);
	}

	return loadOk;
}

TechTree::~TechTree(){
	g_logger.logProgramEvent("~TechTree", !Program::getInstance()->isTerminating());
}

void TechTree::doChecksumResources(Checksum &checksum) const {
	foreach_const (ResourceTypes, it, resourceTypes) {
		it->doChecksum(checksum);
	}
}

void TechTree::doChecksumFaction(Checksum &checksum, int i) const {
	factionTypes[i].doChecksum(checksum);
}

void TechTree::doChecksum(Checksum &checksum) const {
	doChecksumResources(checksum);
	for (int i=0; i < factionTypes.size(); ++i) {
		factionTypes[i].doChecksum(checksum);
	}
}

// ==================== get ====================const FactionType *getFactionType(const string &name) const {
const FactionType *TechTree::getFactionType(const string &name) const {
    FactionTypeMap::const_iterator i = factionTypeMap.find(name);
    if(i != factionTypeMap.end()) {
        return i->second;
    } else {
        throw runtime_error("Faction Type not found: " + name);
    }
}

FactionType *TechTree::getFactionType(const string &name) {
    FactionTypeMap::const_iterator i = factionTypeMap.find(name);
    if(i != factionTypeMap.end()) {
        return i->second;
    } else {
        throw runtime_error("Faction Type not found: " + name);
    }
}

ResourceType *TechTree::getResourceType(const string &name) {
    ResourceTypeMap::const_iterator i = resourceTypeMap.find(name);
    if(i != resourceTypeMap.end()) {
        return i->second;
    } else {
        throw runtime_error("Resource Type not found: " + name);
    }
}

const ResourceType *TechTree::getResourceType(const string &name) const {
    ResourceTypeMap::const_iterator i = resourceTypeMap.find(name);
    if(i != resourceTypeMap.end()) {
        return i->second;
    } else {
        throw runtime_error("Resource Type not found: " + name);
    }
}

const ResourceType *TechTree::getTechResourceType(int i) const{
	for(int j=0; j<getResourceTypeCount(); ++j){
		const ResourceType *rt= getResourceType(j);
		if(rt->getClass()==ResourceClass::TECHTREE && rt->getResourceNumber()==i)
			return getResourceType(j);
	}
	return getFirstTechResourceType();
}

const ResourceType *TechTree::getFirstTechResourceType() const{
	for(int i=0; i<getResourceTypeCount(); ++i){
		const ResourceType *rt= getResourceType(i);
		if(rt->getResourceNumber()==1 && rt->getClass()==ResourceClass::TECHTREE)
			return getResourceType(i);
	}
	throw runtime_error("This tech tree has no tech resources, one at least is required");
}

//REFACTOR: Move to EffectTypeFactory
int TechTree::addEffectType(EffectType *et) {
	EffectTypeMap::const_iterator i;
	const string &name = et->getName();

	i = effectTypeMap.find(name);
	if(i != effectTypeMap.end()) {
		throw runtime_error("An effect named " + name
				+ " has already been specified.  Each effect must have a unique name.");
	} else {
		effectTypes.push_back(et);
		effectTypeMap[name] = et;
		// return the new id
		return effectTypes.size() - 1;
	}
}

}}//end namespace
