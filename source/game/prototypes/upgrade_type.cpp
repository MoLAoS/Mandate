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
#include "upgrade_type.h"

#include <algorithm>
#include <cassert>

#include "unit_type.h"
#include "util.h"
#include "logger.h"
#include "lang.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "resource.h"
#include "renderer.h"

#include "leak_dumper.h"

using Glest::Util::Logger;
using namespace Shared::Util;
using namespace Shared::Xml;
using namespace Glest::Graphics;

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class UpgradeType
// =====================================================

// ==================== misc ====================

UpgradeType::UpgradeType()
		: m_factionType(0) {
}

void loadResourceModifier(const XmlNode *node, ResModifierMap &map, const TechTree *techTree) {
	for (int i=0; i < node->getChildCount(); ++i) {
		const XmlNode *resNode = node->getChild(i);
		if (resNode->getName() == "resource") {
			string resName = resNode->getAttribute("name")->getRestrictedValue();
			int addition = 0;
			if (const XmlAttribute *addAttrib = resNode->getAttribute("addition", false)) {
				addition = addAttrib->getIntValue();
			}
			fixed mult = 1;
			if (const XmlAttribute *multAttrib = resNode->getAttribute("multiplier", false)) {
				mult = multAttrib->getFixedValue();
			}
			const ResourceType *rt = techTree->getResourceType(resName);
			if (map.find(rt) != map.end()) {
				throw runtime_error("duplicate resource node '" + resName + "'");
			}
			map[rt] = Modifier(addition, mult);
		}
	}
}

bool UpgradeType::load(const string &dir, const TechTree *techTree, const FactionType *factionType) {
	string path;
	m_factionType = factionType;

	g_logger.logProgramEvent("Upgrade type: "+ dir, true);
	path = dir + "/" + m_name + ".xml";
	bool loadOk = true;

	XmlTree xmlTree;
	const XmlNode *upgradeNode;
	try { 
		xmlTree.load(path);
		upgradeNode= xmlTree.getRootNode();
	}
	catch (runtime_error e) { 
		g_logger.logXmlError(dir, e.what());
		g_logger.logError("Fatal Error: could not load " + path);
		return false;
	}

	// ProducibleType parameters (unit/upgrade reqs and resource reqs)
	if (!ProducibleType::load(upgradeNode, dir, techTree, factionType)) {
		loadOk = false;
	}

	// enhancements...
	const XmlNode *enhancementsNode = upgradeNode->getChild("enhancements", 0, false);

	if (enhancementsNode) { // Nu skool.
		m_enhancements.resize(enhancementsNode->getChildCount());
		m_unitsAffected.resize(enhancementsNode->getChildCount());
		for (int i=0; i < m_enhancements.size(); ++i) {
			const XmlNode *enhanceNode, *enhancementNode;
			try {
				enhanceNode = enhancementsNode->getChild("enhancement", i);
				enhancementNode = enhanceNode->getChild("effects");
			} catch (runtime_error &e) {
				g_logger.logXmlError(dir, e.what());
				loadOk = false;
				continue;
			}
			if (!m_enhancements[i].m_enhancement.load(enhancementNode, dir, techTree, factionType)) {
				loadOk = false;
			}
			try { // resource cost and storage modifiers
				if (const XmlNode *costModsNode = enhancementNode->getOptionalChild("cost-modifiers")) {
					loadResourceModifier(costModsNode, m_enhancements[i].m_costModifiers, techTree);
				}
				if (const XmlNode *storeModsNode = enhancementNode->getOptionalChild("store-modifiers")) {
					loadResourceModifier(storeModsNode, m_enhancements[i].m_storeModifiers, techTree);
				}
			} catch (runtime_error e) { 
				g_logger.logXmlError(dir, e.what());
				loadOk = false;
			} 
			try { // Units affected by this upgrade
				const XmlNode *affectsNode = enhanceNode->getChild("affects", 0);
				for (int j=0; j < affectsNode->getChildCount(); ++j) {
					const XmlNode *affectNode = affectsNode->getChild(j);
					if (affectNode->getName() == "unit") {
						string name = affectNode->getAttribute("name")->getRestrictedValue();
						const UnitType *ut = factionType->getUnitType(name);
						m_enhancementMap[ut] = &m_enhancements[i];
						m_unitsAffected[i].push_back(name);
					} else if (affectNode->getName() == "tag") {
						string tag = affectNode->getAttribute("name")->getRestrictedValue();
						for (int k=0; k < factionType->getUnitTypeCount(); ++k) {
							const UnitType *ut = factionType->getUnitType(k);
							if (ut->hasTag(tag)) {
								m_enhancementMap[ut] = &m_enhancements[i];
								m_unitsAffected[i].push_back(ut->getName());
							}
						}
					} else {
						string msg = "Unknown affect node, expected 'unit' or 'tag', got '"
							+ affectNode->getName() + "'";
						g_logger.logXmlError(dir, msg.c_str());
					}
				}
			} catch (runtime_error e) { 
				g_logger.logXmlError(dir, e.what());
				return false;
			}
		}
	} else { // Old skool.
		m_enhancements.resize(1);
		m_unitsAffected.resize(1);

		// values
		// maintain backward compatibility using legacy format
		EnhancementType &e = m_enhancements[0].m_enhancement;
		e.setMaxHp(upgradeNode->getOptionalIntValue("max-hp"));
		e.setMaxEp(upgradeNode->getOptionalIntValue("max-ep"));
		e.setSight(upgradeNode->getOptionalIntValue("sight"));
		e.setAttackStrength(upgradeNode->getOptionalIntValue("attack-strength"));
		e.setAttackRange(upgradeNode->getOptionalIntValue("attack-range"));
		e.setArmor(upgradeNode->getOptionalIntValue("armor"));
		e.setMoveSpeed(upgradeNode->getOptionalIntValue("move-speed"));
		e.setProdSpeed(upgradeNode->getOptionalIntValue("production-speed"));

		//initialize values using new format if nodes are present
		if (upgradeNode->getChild("static-modifiers", 0, false)
		|| upgradeNode->getChild("multipliers", 0, false)
		|| upgradeNode->getChild("point-boosts", 0, false)) {
			if (!e.load(upgradeNode, dir, techTree, factionType)) {
				loadOk = false;
			}
		}
		try { // Units affected by this upgrade
			const XmlNode *effectsNode= upgradeNode->getChild("effects", 0, false);
			if (effectsNode) {
				for (int i=0; i<effectsNode->getChildCount(); ++i) {
					const XmlNode *unitNode = effectsNode->getChild("unit", i);
					string name = unitNode->getAttribute("name")->getRestrictedValue();
					const UnitType *ut = factionType->getUnitType(name);
					m_enhancementMap[ut] = &m_enhancements[0];
					m_unitsAffected[0].push_back(name);
				}
			}
		} catch (runtime_error e) { 
			g_logger.logXmlError ( dir, e.what() );
			return false;
		}

	}
	return loadOk;
}

void UpgradeType::doChecksum(Checksum &checksum) const {
	ProducibleType::doChecksum(checksum);
	foreach_const (Enhancements, it, m_enhancements) {
		it->m_enhancement.doChecksum(checksum);
	}
	///@todo resource mods
	foreach_const (EnhancementMap, it, m_enhancementMap) {
		checksum.add(it->first->getId());
	}
}

string UpgradeType::getDesc(const Faction *f) const {
	Lang &lang = Lang::getInstance();
	string str = getReqDesc(f);

	if (!m_enhancements.empty()) {
		for (int i=0; i < m_enhancements.size(); ++i) {
			str += "\n" + lang.get("Affects") + ":";
			for (int j=0; j < m_unitsAffected[i].size(); ++j) {
				str += "\n" + m_unitsAffected[i][j];
			}
			m_enhancements[i].m_enhancement.getDesc(str, "\n");
		}
	}
	return str;
}

const EnhancementType* UpgradeType::getEnhancement(const UnitType *ut) const {
	EnhancementMap::const_iterator it = m_enhancementMap.find(ut);
	if (it != m_enhancementMap.end()) {
		return it->second->getEnhancement();
	}
	return 0;
}
Modifier UpgradeType::getCostModifier(const UnitType *ut, const ResourceType *rt) const {
	EnhancementMap::const_iterator uit = m_enhancementMap.find(ut);
	if (uit != m_enhancementMap.end()) {
		ResModifierMap::const_iterator rit = uit->second->m_costModifiers.find(rt);
		if (rit != uit->second->m_costModifiers.end()) {
			return rit->second;
		}
	}
	return Modifier(0, 1);
}
Modifier UpgradeType::getStoreModifier(const UnitType *ut, const ResourceType *rt) const {
	EnhancementMap::const_iterator uit = m_enhancementMap.find(ut);
	if (uit != m_enhancementMap.end()) {
		ResModifierMap::const_iterator rit = uit->second->m_storeModifiers.find(rt);
		if (rit != uit->second->m_storeModifiers.end()) {
			return rit->second;
		}
	}
	return Modifier(0, 1);
}


}}//end namespace
