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

#include "element_type.h"

#include <cassert>

#include "resource_type.h"
#include "upgrade_type.h"
#include "unit_type.h"
#include "resource.h"
#include "tech_tree.h"
#include "logger.h"
#include "lang.h"
#include "renderer.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class DisplayableType
// =====================================================

DisplayableType::DisplayableType(){
	image= NULL;
}

// =====================================================
// 	class RequirableType
// =====================================================

string RequirableType::getReqDesc() const{
	bool anyReqs= false;

	string reqString;
	for(int i=0; i<getUnitReqCount(); ++i){
        reqString+= getUnitReq(i)->getName();
        reqString+= "\n";
		anyReqs= true;
    }

    for(int i=0; i<getUpgradeReqCount(); ++i){
        reqString+= getUpgradeReq(i)->getName();
        reqString+= "\n";
		anyReqs= true;
    }

	string str= getName();
	if(anyReqs){
		return str + " " + Lang::getInstance().get("Reqs") + ":\n" + reqString;
	}
	else{
		return str;
	}
}

void RequirableType::load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	//unit requirements
	const XmlNode *unitRequirementsNode= baseNode->getChild("unit-requirements", 0, false);
	if(unitRequirementsNode) {
		for(int i=0; i<unitRequirementsNode->getChildCount(); ++i){
			const XmlNode *unitNode= 	unitRequirementsNode->getChild("unit", i);
			string name= unitNode->getAttribute("name")->getRestrictedValue();
			unitReqs.push_back(ft->getUnitType(name));
		}
	}

	//upgrade requirements
	const XmlNode *upgradeRequirementsNode= baseNode->getChild("upgrade-requirements", 0, false);
	if(upgradeRequirementsNode) {
		for(int i=0; i<upgradeRequirementsNode->getChildCount(); ++i){
			const XmlNode *upgradeReqNode= upgradeRequirementsNode->getChild("upgrade", i);
			string name= upgradeReqNode->getAttribute("name")->getRestrictedValue();
			upgradeReqs.push_back(ft->getUpgradeType(name));
		}
	}

	//subfactions required
	const XmlNode *subfactionsNode= baseNode->getChild("subfaction-restrictions", 0, false);
	if(subfactionsNode) {
		subfactionsReqs = 0;
		for(int i=0; i<subfactionsNode->getChildCount(); ++i){
			string name= subfactionsNode->getChild("subfaction", i)->getAttribute("name")->getRestrictedValue();
			subfactionsReqs |= 1 << ft->getSubfactionIndex(name);
		}
	} else {
		subfactionsReqs	= -1;	//all subfactions
	}
}

// =====================================================
// 	class ProducibleType
// =====================================================

ProducibleType::ProducibleType(){
	cancelImage= NULL;
	advancesToSubfaction= 0;
	advancementIsImmediate= false;
}

ProducibleType::~ProducibleType(){
}

string ProducibleType::getReqDesc() const{
    string str= getName()+" "+Lang::getInstance().get("Reqs")+":\n";
    for(int i=0; i<getCostCount(); ++i){
        if(getCost(i)->getAmount()!=0){
            str+= getCost(i)->getType()->getName();
            str+= ": "+ intToStr(getCost(i)->getAmount());
            str+= "\n";
        }
    }

    for(int i=0; i<getUnitReqCount(); ++i){
        str+= getUnitReq(i)->getName();
        str+= "\n";
    }

    for(int i=0; i<getUpgradeReqCount(); ++i){
        str+= getUpgradeReq(i)->getName();
        str+= "\n";
    }

    return str;
}

void ProducibleType::load(const XmlNode *baseNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
	RequirableType::load(baseNode, dir, techTree, factionType);

	//resource requirements
	const XmlNode *resourceRequirementsNode= baseNode->getChild("resource-requirements", 0, false);
	if(resourceRequirementsNode) {
		costs.resize(resourceRequirementsNode->getChildCount());
		for(int i=0; i<costs.size(); ++i){
			const XmlNode *resourceNode= resourceRequirementsNode->getChild("resource", i);
			string name= resourceNode->getAttribute("name")->getRestrictedValue();
			int amount= resourceNode->getAttribute("amount")->getIntValue();
			costs[i].init(techTree->getResourceType(name), amount);
		}
	}

	//subfaction advancement
	const XmlNode *advancementNode = baseNode->getChild("advances-to-subfaction", 0, false);
	if(advancementNode) {
		advancesToSubfaction = factionType->getSubfactionIndex(advancementNode->
				getAttribute("name")->getRestrictedValue());
		advancementIsImmediate = advancementNode->getAttribute("is-immediate")->getBoolValue();
	}
}

}}//end namespace
