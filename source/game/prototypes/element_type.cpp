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

using Glest::Util::Logger;
using namespace Shared::Util;
using namespace Glest::Graphics;

namespace Glest { namespace ProtoTypes {

void NameIdPair::doChecksum(Shared::Util::Checksum &checksum) const {
	checksum.add<int>(id);
	checksum.add(name);
}

// =====================================================
// 	class DisplayableType
// =====================================================

bool DisplayableType::load(const XmlNode *baseNode, const string &dir) {
	try {
		const XmlNode *imageNode = baseNode->getChild("image");
		image = Renderer::getInstance().newTexture2D(ResourceScope::GAME);
		image->load(dir + "/" + imageNode->getAttribute("path")->getRestrictedValue());
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError ( dir, e.what() );
		return false;
	}
	return true;
}

// =====================================================
// 	class RequirableType
// =====================================================

string RequirableType::getReqDesc() const{
	stringstream ss;
	ss << name;
	if (unitReqs.empty() && upgradeReqs.empty()) {
		return ss.str();
	}
	ss << " " << Lang::getInstance().get("Reqs") << ":\n";
	foreach_const (UnitReqs, it, unitReqs) {
		ss << (*it)->getName() << endl;
	}
	foreach_const (UpgradeReqs, it, upgradeReqs) {
		ss << (*it)->getName() << endl;
	}
	return ss.str();
}

bool RequirableType::load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = true;
	//unit requirements
	try {
		const XmlNode *unitRequirementsNode = baseNode->getChild("unit-requirements", 0, false);
		if(unitRequirementsNode) {
			for(int i = 0; i < unitRequirementsNode->getChildCount(); ++i) {
				const XmlNode *unitNode = unitRequirementsNode->getChild("unit", i);
				string name = unitNode->getRestrictedAttribute("name");
				unitReqs.push_back(ft->getUnitType(name));
			}
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//upgrade requirements
	try {
		const XmlNode *upgradeRequirementsNode = baseNode->getChild("upgrade-requirements", 0, false);
		if(upgradeRequirementsNode) {
			for(int i = 0; i < upgradeRequirementsNode->getChildCount(); ++i) {
				const XmlNode *upgradeReqNode = upgradeRequirementsNode->getChild("upgrade", i);
				string name = upgradeReqNode->getRestrictedAttribute("name");
				upgradeReqs.push_back(ft->getUpgradeType(name));
			}
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	//subfactions required
	try { 
		const XmlNode *subfactionsNode = baseNode->getChild("subfaction-restrictions", 0, false);
		if(subfactionsNode) {
			for(int i = 0; i < subfactionsNode->getChildCount(); ++i) {
				string name = subfactionsNode->getChild("subfaction", i)->getRestrictedAttribute("name");
				subfactionsReqs |= 1 << ft->getSubfactionIndex(name);
			}
		} else {
			subfactionsReqs = -1; //all subfactions
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void RequirableType::doChecksum(Checksum &checksum) const {
	NameIdPair::doChecksum(checksum);
	foreach_const (UnitReqs, it, unitReqs) {
		checksum.add((*it)->getName());
		checksum.add<int>((*it)->getId());
	}
	foreach_const (UpgradeReqs, it, upgradeReqs) {
		checksum.add((*it)->getName());
		checksum.add<int>((*it)->getId());
	}
	checksum.add<int>(subfactionsReqs);
}

// =====================================================
// 	class ProducibleType
// =====================================================

ProducibleType::ProducibleType() :
		RequirableType(),
		costs(),
		cancelImage(NULL),
		productionTime(0),
		advancesToSubfaction(0),
		advancementIsImmediate(false) {
}

ProducibleType::~ProducibleType() {
}

string ProducibleType::getReqDesc() const {
	stringstream ss;
	ss << name;
	if (unitReqs.empty() && upgradeReqs.empty() && costs.empty()) {
		return ss.str();
	}
	ss << " " << Lang::getInstance().get("Reqs") << ":\n";
	foreach_const (Costs, it, costs) {
		ss << it->getType()->getName() << ": " << it->getAmount() << endl;
	}
	foreach_const (UnitReqs, it, unitReqs) {
		ss << (*it)->getName() << endl;
	}
	foreach_const (UpgradeReqs, it, upgradeReqs) {
		ss << (*it)->getName() << endl;
	}
	return ss.str();
}

bool ProducibleType::load(const XmlNode *baseNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
	bool loadOk = true;
	if ( ! RequirableType::load(baseNode, dir, techTree, factionType) )
		loadOk = false;

	//resource requirements
	try {
		const XmlNode *resourceRequirementsNode = baseNode->getChild("resource-requirements", 0, false);
		if(resourceRequirementsNode) {
			costs.resize(resourceRequirementsNode->getChildCount());
			for(int i = 0; i < costs.size(); ++i) {
				try {
					const XmlNode *resourceNode = resourceRequirementsNode->getChild("resource", i);
					string name = resourceNode->getAttribute("name")->getRestrictedValue();
					int amount = resourceNode->getAttribute("amount")->getIntValue();
					costs[i].init(techTree->getResourceType(name), amount);
				} catch (runtime_error e) {
					Logger::getErrorLog().addXmlError ( dir, e.what() );
					loadOk = false;
				}
			}
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError ( dir, e.what() );
		loadOk = false;
	}

	//subfaction advancement
	try {
		const XmlNode *advancementNode = baseNode->getChild("advances-to-subfaction", 0, false);
		if(advancementNode) {
			advancesToSubfaction = factionType->getSubfactionIndex(
				advancementNode->getAttribute("name")->getRestrictedValue());
			advancementIsImmediate = advancementNode->getAttribute("is-immediate")->getBoolValue();
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError ( dir, e.what() );
		loadOk = false;
	}
	return loadOk;
}

void ProducibleType::doChecksum(Checksum &checksum) const {
	RequirableType::doChecksum(checksum);
	foreach_const (Costs, it, costs) {
		checksum.add<int>(it->getType()->getId());
		checksum.add(it->getType()->getName());
		checksum.add<int>(it->getAmount());
	}
	checksum.add<int>(productionTime);
	checksum.add<int>(advancesToSubfaction);
	checksum.add<bool>(advancementIsImmediate);
}

}}//end namespace