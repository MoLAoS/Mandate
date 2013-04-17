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
#include "util.h"
#include "leak_dumper.h"
#include "faction.h"

using Glest::Util::Logger;
using namespace Shared::Util;
using namespace Glest::Graphics;

namespace Glest { namespace ProtoTypes {

using Shared::Util::mediaErrorLog;

void NameIdPair::doChecksum(Shared::Util::Checksum &checksum) const {
	checksum.add(m_id);
	checksum.add(m_name);
}

// =====================================================
// 	class DisplayableType
// =====================================================

bool DisplayableType::load(const XmlNode *baseNode, const string &dir) {
	string xmlPath = dir + "/" + basename(dir) + ".xml";
	string imgPath;
	try {
		const XmlNode *imageNode = baseNode->getChild("image");
		imgPath = dir + "/" + imageNode->getAttribute("path")->getRestrictedValue();
		image = g_renderer.getTexture2D(ResourceScope::GAME, imgPath);
		if (baseNode->getOptionalChild("name")) {
			m_name = baseNode->getChild("name")->getStringValue();
		}
	} catch (runtime_error &e) {
		g_logger.logXmlError(xmlPath, e.what());
		return false;
	}
	while (mediaErrorLog.hasError()) {
		MediaErrorLog::ErrorRecord record = mediaErrorLog.popError();
		g_logger.logMediaError(xmlPath, record.path, record.msg.c_str());
	}
	return true;
}

// =====================================================
// 	class RequirableType
// =====================================================
void UnitReq::init(const UnitType* type, int value, bool scope) {
    unitType = type;
    amount = value;
    local = scope;
}

void ItemReq::init(const ItemType* type, int value, bool scope) {
    itemType = type;
    amount = value;
    local = scope;
}

void UpgradeReq::init(const UpgradeType* type, int value) {
    upgradeType = type;
    stage = value;
}

string RequirableType::getReqDesc(const Faction *f, const FactionType *ft) const{
	stringstream ss;
	if (unitReqs.empty() && upgradeReqs.empty() && itemReqs.empty()) {
		return ss.str();
	}
	ss << g_lang.getFactionString(ft->getName(), m_name)
	   << " " << g_lang.get("Reqs") << ":\n";
	for (int i = 0; i < getUnitReqCount(); ++i) {
		ss << "  " << unitReqs[i].getUnitType()->getName() << ": " << unitReqs[i].getAmount()  << endl;
	}
	for (int i = 0; i < getItemReqCount(); ++i) {
		ss << "  " << itemReqs[i].getItemType()->getName() << ": " << itemReqs[i].getAmount()  << endl;
	}
	for (int i = 0; i < getUpgradeReqCount(); ++i) {
		ss << "  " << upgradeReqs[i].getUpgradeType()->getName() << ": " << upgradeReqs[i].getStage() << endl;
	}
	return ss.str();
}

bool RequirableType::load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = DisplayableType::load(baseNode, dir);

	try { // Unit requirements
		const XmlNode *unitRequirementsNode = baseNode->getChild("unit-requirements", 0, false);
		if(unitRequirementsNode) {
			for(int i = 0; i < unitRequirementsNode->getChildCount(); ++i) {
				const XmlNode *unitNode = unitRequirementsNode->getChild("unit", i);
				string name = unitNode->getRestrictedAttribute("name");
				bool scope = false;
				string local = unitNode->getRestrictedAttribute("scope");
				if (local == "local") {
                    scope = true;
				}
				int value = unitNode->getIntAttribute("amount");
				UnitReq unitReq;
				unitReq.init(ft->getUnitType(name), value, scope);
				unitReqs.push_back(unitReq);
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // Item requirements
		const XmlNode *itemRequirementsNode = baseNode->getChild("item-requirements", 0, false);
		if(itemRequirementsNode) {
			for(int i = 0; i < itemRequirementsNode->getChildCount(); ++i) {
				const XmlNode *itemNode = itemRequirementsNode->getChild("item", i);
				string name = itemNode->getRestrictedAttribute("name");
				bool scope = false;
				string local = itemNode->getRestrictedAttribute("scope");
				if (local == "local") {
                    scope = true;
				}
				int value = itemNode->getIntAttribute("amount");
				ItemReq itemReq;
				itemReq.init(ft->getItemType(name), value, scope);
				itemReqs.push_back(itemReq);
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // Upgrade requirements
		const XmlNode *upgradeRequirementsNode = baseNode->getChild("upgrade-requirements", 0, false);
		if(upgradeRequirementsNode) {
			for(int i = 0; i < upgradeRequirementsNode->getChildCount(); ++i) {
				const XmlNode *upgradeReqNode = upgradeRequirementsNode->getChild("upgrade", i);
				string name = upgradeReqNode->getRestrictedAttribute("name");
				int stage = upgradeReqNode->getIntAttribute("value");
				UpgradeReq upgradeReq;
				upgradeReq.init(ft->getUpgradeType(name), stage);
				upgradeReqs.push_back(upgradeReq);
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try { // Subfactions required
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
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void RequirableType::doChecksum(Checksum &checksum) const {
	NameIdPair::doChecksum(checksum);
	for (int i = 0; i < getUnitReqCount(); ++i) {
		checksum.add(unitReqs[i].getUnitType()->getName());
		checksum.add(unitReqs[i].getUnitType()->getId());
	}
	for (int i = 0; i < getItemReqCount(); ++i) {
		checksum.add(itemReqs[i].getItemType()->getName());
		checksum.add(itemReqs[i].getItemType()->getId());
	}
	for (int i = 0; i < getUpgradeReqCount(); ++i) {
		checksum.add(upgradeReqs[i].getUpgradeType()->getName());
		checksum.add(upgradeReqs[i].getUpgradeType()->getId());
	}
	checksum.add(subfactionsReqs);
}

// =====================================================
// 	class ProducibleType
// =====================================================

ProducibleType::ProducibleType() :
		RequirableType(),
		costs(),
		cancelImage(NULL),
		productionTime(0),
		advancesToSubfaction(-1),
		advancementIsImmediate(false) {
}

ProducibleType::~ProducibleType() {
}

ResourceAmount ProducibleType::getCost(int i, const Faction *f) const {
	Modifier mod = f->getCostModifier(this, costs[i].getType());
	ResourceAmount res(costs[i]);
	res.setAmount((((res.getAmount() + res.getAmountPlus()) * mod.getMultiplier()).intp() + mod.getAddition()) * res.getAmountMultiply().intp());
	return res;
}

ResourceAmount ProducibleType::getCost(const ResourceType *rt, const Faction *f) const {
	for (int i = 0; i < costs.size(); ++i) {
		if (costs[i].getType() == rt) {
			return getCost(i, f);
		}
	}
	return ResourceAmount();
}

ResourceAmount ProducibleType::getLocalCost(int i, const Faction *f) const {
	Modifier mod = f->getCostModifier(this, localCosts[i].getType());
	ResourceAmount res(localCosts[i]);
	res.setAmount((res.getAmount() * mod.getMultiplier()).intp() + mod.getAddition());
	return res;
}

ResourceAmount ProducibleType::getLocalCost(const ResourceType *rt, const Faction *f) const {
	for (int i = 0; i < localCosts.size(); ++i) {
		if (localCosts[i].getType() == rt) {
			return getCost(i, f);
		}
	}
	return ResourceAmount();
}

string ProducibleType::getReqDesc(const Faction *f, const FactionType *ft) const {
	Lang &lang = g_lang;
	stringstream ss;
	if (unitReqs.empty() && upgradeReqs.empty() && costs.empty()) {
		return ss.str();
	}
	ss << lang.getFactionString(ft->getName(), m_name)
	   << " " << g_lang.get("Reqs") << ":\n";
	for (int i=0; i < getCostCount(); ++i) {
		ResourceAmount r = getCost(i, f);
		string resName = lang.getFactionString(ft->getName(), r.getType()->getName());
		if (resName == r.getType()->getName()) {
			resName = lang.getTechString(r.getType()->getName());
			if (resName == r.getType()->getName()) {
				resName = formatString(resName);
			}
		}
		ss << "  " << resName << ": " << r.getAmount() << endl;
	}
	for (int i = 0; i < getUnitReqCount(); ++i) {
		ss << "  " << unitReqs[i].getUnitType()->getName() << ": " << unitReqs[i].getAmount()  << endl;
	}
	for (int i = 0; i < getItemReqCount(); ++i) {
		ss << "  " << itemReqs[i].getItemType()->getName() << ": " << itemReqs[i].getAmount()  << endl;
	}
	for (int i = 0; i < getUpgradeReqCount(); ++i) {
		ss << "  " << upgradeReqs[i].getUpgradeType()->getName() << ": " << upgradeReqs[i].getStage() << endl;
	}
	return ss.str();
}

bool ProducibleType::load(const XmlNode *baseNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
	string xmlPath = dir + "/" + basename(dir) + ".xml";
	bool loadOk = true;
	if (!RequirableType::load(baseNode, dir, techTree, factionType)) {
		loadOk = false;
	}

	// Production time
	try { productionTime = baseNode->getChildIntValue("time"); }
	catch (runtime_error e) {
		g_logger.logXmlError(xmlPath, e.what());
		loadOk = false;
	}
	// Cancel image
	string imgPath;
	try {
		const XmlNode *imageCancelNode = baseNode->getChild("image-cancel");
		imgPath = dir + "/" + imageCancelNode->getRestrictedAttribute("path");
		cancelImage = g_renderer.getTexture2D(ResourceScope::GAME, imgPath);
	} catch (runtime_error e) {
		g_logger.logXmlError(xmlPath, e.what());
		loadOk = false;
	}
	while (mediaErrorLog.hasError()) {
		MediaErrorLog::ErrorRecord record = mediaErrorLog.popError();
		g_logger.logMediaError(xmlPath, record.path, record.msg.c_str());
	}

	// Resource requirements
	try {
		const XmlNode *resourceRequirementsNode = baseNode->getChild("resource-requirements", 0, false);
		if(resourceRequirementsNode) {
			costs.resize(resourceRequirementsNode->getChildCount());
			for(int i = 0; i < costs.size(); ++i) {
				try {
					const XmlNode *resourceNode = resourceRequirementsNode->getChild("resource", i);
					string name = resourceNode->getAttribute("name")->getRestrictedValue();
					int amount = resourceNode->getAttribute("amount")->getIntValue();
                    int amount_plus = resourceNode->getAttribute("plus")->getIntValue();
                    fixed amount_multiply = resourceNode->getAttribute("multiply")->getFixedValue();
                    costs[i].init(techTree->getResourceType(name), amount, amount_plus, amount_multiply);
				} catch (runtime_error e) {
					g_logger.logXmlError(dir, e.what());
					loadOk = false;
				}
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	try {
		const XmlNode *localRequirementsNode = baseNode->getChild("local-requirements", 0, false);
		if(localRequirementsNode) {
			localCosts.resize(localRequirementsNode->getChildCount());
			for(int i = 0; i < localCosts.size(); ++i) {
				try {
					const XmlNode *resourceNode = localRequirementsNode->getChild("resource", i);
					string name = resourceNode->getAttribute("name")->getRestrictedValue();
					int amount = resourceNode->getAttribute("amount")->getIntValue();
                    int amount_plus = resourceNode->getAttribute("plus")->getIntValue();
                    fixed amount_multiply = resourceNode->getAttribute("multiply")->getFixedValue();
                    localCosts[i].init(techTree->getResourceType(name), amount, amount_plus, amount_multiply);
				} catch (runtime_error e) {
					g_logger.logXmlError(dir, e.what());
					loadOk = false;
				}
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
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
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void ProducibleType::doChecksum(Checksum &checksum) const {
	RequirableType::doChecksum(checksum);
	foreach_const (Costs, it, costs) {
		checksum.add(it->getType()->getId());
		checksum.add(it->getType()->getName());
		checksum.add(it->getAmount());
	}
	checksum.add(productionTime);
	checksum.add(advancesToSubfaction);
	checksum.add(advancementIsImmediate);
}

}}//end namespace
