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
#include "components.h"

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

	//load tech tree xml info
	XmlTree	xmlTree;
	string path;
	name = basename(dir);
	try {
		path = dir + "/" + name + ".xml";
		xmlTree.load(path);
	}
	catch (runtime_error &e) {
		g_errorLog.addXmlError ( path, "File missing or wrongly named." );
		return false;
	}
	const XmlNode *techTreeNode;
	try { techTreeNode= xmlTree.getRootNode(); }
	catch (runtime_error &e) {
		g_errorLog.addXmlError ( path, "File appears to lack contents." );
		return false;
	}

	//attack types
	const XmlNode *attackTypesNode;
	try { 
		attackTypesNode= techTreeNode->getChild("attack-types");
		attackTypes.resize(attackTypesNode->getChildCount());
		for(int i=0; i<attackTypes.size(); ++i){
			const XmlNode *attackTypeNode= attackTypesNode->getChild("attack-type", i);
			string name;
			try { 
				name = attackTypeNode->getAttribute("name")->getRestrictedValue(); 
				attackTypes[i].setName(name);
				attackTypes[i].setId(i);
				attackTypeMap[name] = &attackTypes[i];
			}
			catch (runtime_error &e) { 
				g_errorLog.addXmlError(path, e.what());
				loadOk = false; 
			}
		}
	}
	catch (runtime_error &e) {
		g_errorLog.addXmlError(path, e.what());
		loadOk = false;
	}

	//armor types
	const XmlNode *armorTypesNode;
	try { 
		armorTypesNode= techTreeNode->getChild("armor-types"); 
		armorTypes.resize(armorTypesNode->getChildCount());
		for(int i=0; i<armorTypes.size(); ++i){
			string name ;
			try { 
				name = armorTypesNode->getChild("armor-type", i)->getRestrictedAttribute("name"); 
				armorTypes[i].setName(name);
				armorTypes[i].setId(i);
				armorTypeMap[name] = &armorTypes[i];
			}
			catch (runtime_error &e) { 
				g_errorLog.addXmlError(path, e.what());
				loadOk = false; 
			}
		}  
	}
	catch (runtime_error &e) {
		g_errorLog.addXmlError(path, e.what());
		loadOk = false;
	}

	try { //damage multipliers
		damageMultiplierTable.init(attackTypes.size(), armorTypes.size());
		const XmlNode *damageMultipliersNode= techTreeNode->getChild("damage-multipliers");
		for (int i = 0; i < damageMultipliersNode->getChildCount(); ++i) {
			try {
				const XmlNode *dmNode= damageMultipliersNode->getChild("damage-multiplier", i);
				const AttackType *attackType= getAttackType(dmNode->getRestrictedAttribute("attack"));
				const ArmourType *armourType= getArmourType(dmNode->getRestrictedAttribute("armor"));

				fixed fixedMult = dmNode->getFixedAttribute("value");
				//float multiplier= dmNode->getFloatAttribute("value");
				
				//cout << "Damage Multiplier as float: " << multiplier << ", as fixed " << fixedMult << endl;
								
				damageMultiplierTable.setDamageMultiplier(attackType, armourType, fixedMult);
			} catch (runtime_error e) {
				g_errorLog.addXmlError(path, e.what());
				loadOk = false;
			}
		}
	} catch (runtime_error &e) {
		g_errorLog.addXmlError(path, e.what());
		loadOk = false;
	}

	// this must be set before any unit types are loaded
//	UnitStats::setDamageMultiplierCount(getArmorTypeCount());

	//load factions
	factionTypes.resize(factionNames.size());
	int i = 0;
	set<string>::const_iterator fn;

	int numUnitTypes = 0;
	for (fn = factionNames.begin(); fn != factionNames.end(); ++fn, ++i) {
		if (!factionTypes[i].preLoad(dir + "/factions/" + *fn, this)) {
			loadOk = false;
		} else {
			numUnitTypes += factionTypes[i].getUnitTypeCount();
		}
	}
	logger.addUnitCount(numUnitTypes);

	if(resourceTypes.size() > 256) {
		throw runtime_error("Glest Advanced Engine currently only supports 256 resource types.");
	}
	return loadOk;
}

bool TechTree::load(const string &dir, const set<string> &factionNames){
	int i;
	set<string>::const_iterator fn;
	bool loadOk=true;
	
	Logger &logger = Logger::getInstance();
	logger.add("TechTree: "+ dir, true);
	
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
		g_errorLog.addXmlError(str, "Glest Advanced Engine currently only supports 256 resource types.");
		loadOk = false;
	} else {
		for(i=0; i<filenames.size(); ++i){
			str=dir+"/resources/"+filenames[i];
			if (!resourceTypes[i].load(str, i)) {
				loadOk = false;
			}
			resourceTypeMap[filenames[i]] = &resourceTypes[i];
		}
	}

	for (i = 0, fn = factionNames.begin(); fn != factionNames.end(); ++fn, ++i) {
		if (!factionTypes[i].load(i, dir + "/factions/" + *fn, this)) {
			loadOk = false;
		} else {
			factionTypeMap[*fn] = &factionTypes[i];
		}
	}
	return loadOk;
}

TechTree::~TechTree(){
	Logger::getInstance().add("~TechTree", !Program::getInstance()->isTerminating());
}

void TechTree::doChecksumResources(Checksum &checksum) const {
	foreach_const (ResourceTypes, it, resourceTypes) {
		it->doChecksum(checksum);
	}
}

void TechTree::doChecksumDamageMult(Checksum &checksum) const {
	checksum.add(desc);
	foreach_const (ArmorTypes, it, armorTypes) {
		it->doChecksum(checksum);
	}
	foreach_const (AttackTypes, it, attackTypes) {
		it->doChecksum(checksum);
	}
	foreach_const (ArmorTypes, armourIt, armorTypes) {
		const ArmourType *armourType 
			= (*const_cast<ArmorTypeMap*>(&armorTypeMap))[armourIt->getName()];
		foreach_const (AttackTypes, attackIt, attackTypes) {
			const AttackType *attackType 
				= (*const_cast<AttackTypeMap*>(&attackTypeMap))[attackIt->getName()];
			checksum.add(damageMultiplierTable.getDamageMultiplier(attackType, armourType));
		}
	}
}

void TechTree::doChecksumFaction(Checksum &checksum, int i) const {
	factionTypes[i].doChecksum(checksum);
}

void TechTree::doChecksum(Checksum &checksum) const {
	doChecksumDamageMult(checksum);
	doChecksumResources(checksum);
	for (int i=0; i < factionTypes.size(); ++i) {
		factionTypes[i].doChecksum(checksum);
	}
}

// ==================== get ====================

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
