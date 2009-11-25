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


using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest{ namespace Game{

// =====================================================
// 	class TechTree
// =====================================================

bool TechTree::load(const string &dir, const set<string> &factionNames, Checksum &checksum){
	string str;
	vector<string> filenames;
	Logger::getInstance().add("TechTree: "+ dir, true);
	bool loadOk = true;
	//load resources
	str= dir + "/resources/*.";

	try {
		findAll(str, filenames);
		resourceTypes.resize(filenames.size());
	} 
	catch(const exception &e) {
		throw runtime_error("Error loading Resource Types: "+ dir + "\n" + e.what());
	}
	for(int i=0; i<filenames.size(); ++i){
		str=dir+"/resources/"+filenames[i];
		if ( ! resourceTypes[i].load(str, i, checksum) ) loadOk = false;
		resourceTypeMap[filenames[i]] = &resourceTypes[i];
	}

	//load tech tree xml info
	XmlTree	xmlTree;
	string path;
	try {
		path= dir+"/"+basename(dir)+".xml";
		checksum.addFile(path, true);
		xmlTree.load(path);
	}
	catch ( runtime_error &e ) {
		Logger::getErrorLog().addXmlError ( path, "File missing or wrongly named." );
		return false;
	}
	const XmlNode *techTreeNode;
	try { techTreeNode= xmlTree.getRootNode(); }
	catch ( runtime_error &e ) {
		Logger::getErrorLog().addXmlError ( path, "File appears to lack contents." );
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
			catch ( runtime_error &e ) { 
				Logger::getErrorLog().addXmlError ( path, e.what() );
				loadOk = false; 
			}
		}
	}
	catch ( runtime_error &e ) {
		Logger::getErrorLog().addXmlError ( path, e.what() );
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
			catch ( runtime_error &e ) { 
				Logger::getErrorLog().addXmlError ( path, e.what() );
				loadOk = false; 
			}
		}  
	}
	catch ( runtime_error &e ) {
		Logger::getErrorLog().addXmlError ( path, e.what() );
		loadOk = false;
	}

	try { //damage multipliers
		damageMultiplierTable.init(attackTypes.size(), armorTypes.size());
		const XmlNode *damageMultipliersNode= techTreeNode->getChild("damage-multipliers");
		for(int i=0; i<damageMultipliersNode->getChildCount(); ++i){
			try {
				const XmlNode *dmNode= damageMultipliersNode->getChild("damage-multiplier", i);
				const AttackType *attackType= getAttackType(dmNode->getRestrictedAttribute("attack"));
				const ArmorType *armorType= getArmorType(dmNode->getRestrictedAttribute("armor"));
				float multiplier= dmNode->getFloatAttribute("value");
				damageMultiplierTable.setDamageMultiplier(attackType, armorType, multiplier);
			}
			catch ( runtime_error e ) {
				Logger::getErrorLog().addXmlError ( path, e.what() );
				loadOk = false;
			}
		}
	}
	catch ( runtime_error &e ) {
		Logger::getErrorLog().addXmlError ( path, e.what() );
		loadOk = false;
	}

	// this must be set before any unit types are loaded
	UnitStatsBase::setDamageMultiplierCount(getArmorTypeCount());

	//load factions
	factionTypes.resize(factionNames.size());
	int i = 0;
	set<string>::const_iterator fn;
	for(fn = factionNames.begin(); fn != factionNames.end(); ++fn, ++i) {
		if ( ! factionTypes[i].load(dir + "/factions/" + *fn, this, checksum) )
			loadOk = false;
		else
			factionTypeMap[*fn] = &factionTypes[i];
	}

	if(resourceTypes.size() > 256) {
		throw runtime_error("Glest Advanced Engine currently only supports 256 resource types.");
	}
	return loadOk;
}

TechTree::~TechTree(){
	Logger::getInstance().add("Tech tree", !Program::getInstance()->isTerminating());
}


// ==================== get ====================

const ResourceType *TechTree::getTechResourceType(int i) const{

     for(int j=0; j<getResourceTypeCount(); ++j){
          const ResourceType *rt= getResourceType(j);
          if(rt->getResourceNumber()==i && rt->getClass()==rcTech)
               return getResourceType(j);
     }

     return getFirstTechResourceType();
}

const ResourceType *TechTree::getFirstTechResourceType() const{
     for(int i=0; i<getResourceTypeCount(); ++i){
          const ResourceType *rt= getResourceType(i);
          if(rt->getResourceNumber()==1 && rt->getClass()==rcTech)
               return getResourceType(i);
     }

	 throw runtime_error("This tech tree has not tech resources, one at least is required");
}

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
