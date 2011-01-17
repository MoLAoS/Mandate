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
			const XmlNode *enhanceNode = enhancementsNode->getChild("enhancement", i);
			if (!m_enhancements[0].load(enhanceNode->getChild("effects"), dir, techTree, factionType)) {
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

		// values
		// maintain backward compatibility using legacy format
		m_enhancements[0].setMaxHp(upgradeNode->getOptionalIntValue("max-hp"));
		m_enhancements[0].setMaxEp(upgradeNode->getOptionalIntValue("max-ep"));
		m_enhancements[0].setSight(upgradeNode->getOptionalIntValue("sight"));
		m_enhancements[0].setAttackStrength(upgradeNode->getOptionalIntValue("attack-strength"));
		m_enhancements[0].setAttackRange(upgradeNode->getOptionalIntValue("attack-range"));
		m_enhancements[0].setArmor(upgradeNode->getOptionalIntValue("armor"));
		m_enhancements[0].setMoveSpeed(upgradeNode->getOptionalIntValue("move-speed"));
		m_enhancements[0].setProdSpeed(upgradeNode->getOptionalIntValue("production-speed"));

		//initialize values using new format if nodes are present
		if (upgradeNode->getChild("static-modifiers", 0, false)
		|| upgradeNode->getChild("multipliers", 0, false)
		|| upgradeNode->getChild("point-boosts", 0, false)) {
			if (!m_enhancements[0].load(upgradeNode, dir, techTree, factionType)) {
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
		it->doChecksum(checksum);
	}
	foreach_const (EnhancementMap, it, m_enhancementMap) {
		checksum.add(it->first->getId());
	}
}

string UpgradeType::getDesc() const {
	Lang &lang = Lang::getInstance();
	string str = getReqDesc();

	if (!m_enhancements.empty()) {
		for (int i=0; i < m_enhancements.size(); ++i) {
			str += "\n" + lang.get("Affects") + ":";
			for (int j=0; j < m_unitsAffected[i].size(); ++j) {
				str += "\n" + m_unitsAffected[i][j];
			}
			m_enhancements[i].getDesc(str, "\n");
		}
	}
	return str;
}

}}//end namespace
