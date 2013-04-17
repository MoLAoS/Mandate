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
		: m_factionType(0)
		, maxStage(1) {
}

void UpgradeType::loadResourceModifier(const XmlNode *node, ResModifierMap &map, const TechTree *techTree) {
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

bool UpgradeType::loadNewStyle(const XmlNode *node, const string &dir, const TechTree *techTree,
							   const FactionType *factionType) {
	bool loadOk = true;

	m_enhancements.resize(node->getChildCount());
	m_unitsAffected.resize(node->getChildCount()); // one vector per enhancement
	for (int i=0; i < m_enhancements.size(); ++i) {
		const XmlNode *enhanceNode, *enhancementNode;
		try {
			enhanceNode = node->getChild("enhancement", i);
			enhancementNode = enhanceNode->getChild("effects");
		} catch (runtime_error &e) {
			g_logger.logXmlError(dir, e.what());
			loadOk = false;
			continue;
		}
		try {
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
					loadOk = false;
				}
			}
		} catch (runtime_error e) {
			g_logger.logXmlError(dir, e.what());
			loadOk = false;
		}
		const UnitType *ctype = factionType->getUnitType(m_unitsAffected[0][0]);
        try {
            const XmlNode *actionsNode = node->getChild("actions", 0, false);
            if (actionsNode) {
                if (!m_enhancements[i].actions.load(actionsNode, dir, techTree, factionType, true, ctype)) {
                    loadOk = false;
                }
            }
        } catch (runtime_error e) {
            g_logger.logXmlError(dir, e.what());
            loadOk = false;
        }
		if (!m_enhancements[i].m_enhancement.load(enhancementNode, dir, techTree, factionType)) {
			loadOk = false;
		}
		try {
			if (const XmlNode *costModsNode = enhancementNode->getOptionalChild("cost-modifiers")) {
				loadResourceModifier(costModsNode, m_enhancements[i].m_costModifiers, techTree);
			}
			if (const XmlNode *storeModsNode = enhancementNode->getOptionalChild("store-modifiers")) {
				loadResourceModifier(storeModsNode, m_enhancements[i].m_storeModifiers, techTree);
			}
			if (const XmlNode *createModsNode = enhancementNode->getOptionalChild("create-modifiers")) {
				loadResourceModifier(createModsNode, m_enhancements[i].m_createModifiers, techTree);
			}
		} catch (runtime_error e) {
			g_logger.logXmlError(dir, e.what());
			loadOk = false;
		}
	}
	return loadOk;
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
		upgradeNode = xmlTree.getRootNode();
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		g_logger.logError("Fatal Error: could not load " + path);
		return false;
	}
    const XmlNode *countNode;
    try { countNode = upgradeNode->getOptionalChild("stage"); }
    catch (runtime_error &e) {
		g_logger.logXmlError(dir, e.what());
		return false;
	}
    if (countNode) {
        try { maxStage = countNode->getChildIntValue("max-stage");
        } catch (runtime_error &e) {
		    g_logger.logXmlError(dir, e.what());
		    return false;
	    }
    }
	const XmlNode *namesNode;
    try { namesNode = upgradeNode->getOptionalChild("names");
    if(namesNode) {
        for (int i = 0; i < namesNode->getChildCount(); ++i) {
            const XmlNode *nameNode = namesNode->getChild(i);
            if (nameNode->getName() == "stage-name") {
                string name = nameNode->getAttribute("name")->getRestrictedValue();
                m_names.push_back(name);
            }
        }
    }
    } catch (runtime_error &e) {
		g_logger.logXmlError(dir, e.what());
		return false;
	}
	if (!ProducibleType::load(upgradeNode, dir, techTree, factionType)) {
		loadOk = false;
	}
	const XmlNode *enhancementsNode = upgradeNode->getChild("enhancements", 0, false);
    if (!loadNewStyle(enhancementsNode, dir, techTree, factionType)) {
        loadOk = false;
    }
	return loadOk;
}

string UpgradeType::getDescName(Faction *f) const{
	Lang &lang = Lang::getInstance();
	string str;// = getReqDesc(f);

    Faction::UpgradeStages::iterator fit;
	for(fit=f->upgradeStages.begin(); fit!=f->upgradeStages.end(); ++fit){
		if((*fit).getUpgradeType()==this){
			break;
		}
	}
    if(!(*fit).m_names.empty()) {
        for (int i = 0; i < (*fit).m_names.size(); ++i) {
            if (i == (*fit).getUpgradeStage()) {
                str += lang.get("Name") + ": ";
                str += (i == 0 ? "" : i == ((*fit).m_names.size()-1) ? " " : " ");
                str += lang.getFactionString(f->getType()->getName(), (*fit).m_names[i]);
            }
        }
    }
    return str;
}

void descResourceModifier(pair<const ResourceType*, Modifier> i_mod, string &io_res) {
	string resName = g_lang.getTechString(i_mod.first->getName());
	if (resName == i_mod.first->getName()) {
		resName = formatString(resName);
	}
	io_res += "\n" + resName + " ";
	const int add = i_mod.second.getAddition();
	if (add) {
		if (add > 0) {
			io_res += "+";
		}
		io_res += Conversion::toStr(add);
	}
	const fixed mult = i_mod.second.getMultiplier();
	if (mult != 1) {
		if (add) {
			io_res += ", ";
		}
		if (mult > 1) {
			io_res += "+";
		}
		io_res += intToStr(((mult - 1) * 100).intp()) + "%";
	}
}

string UpgradeType::getDesc(Faction *f) const {
	Lang &lang = Lang::getInstance();
	string str;
    Faction::UpgradeStages::iterator fit;
	for(fit=f->upgradeStages.begin(); fit!=f->upgradeStages.end(); ++fit){
		if((*fit).getUpgradeType()==this){
			break;
		}
	}

	if (!(*fit).m_enhancements.empty()) {
		for (int i=0; i < (*fit).m_enhancements.size(); ++i) {
			str += "\n" + lang.get("Affects") + ":";
			for (int j=0; j < (*fit).m_unitsAffected[i].size(); ++j) {
				str += (j == 0 ? " " : j == ((*fit).m_unitsAffected[i].size() - 1) ? " & " : ", ");
				str += lang.getFactionString(f->getType()->getName(), (*fit).m_unitsAffected[i][j]);
			}
			(*fit).m_enhancements[i].m_enhancement.getDesc(str, "\n");
			if (!(*fit).m_enhancements[i].m_costModifiers.empty()) {
				str += "\n" + lang.get("CostModifiers") + ":";
				foreach_const (ResModifierMap, it, (*fit).m_enhancements[i].m_costModifiers) {
					descResourceModifier(*it, str);
				}
			}
			if (!(*fit).m_enhancements[i].m_storeModifiers.empty()) {
				str += "\n" + lang.get("StoreModifiers") + ":";
				foreach_const (ResModifierMap, it, (*fit).m_enhancements[i].m_storeModifiers) {
					descResourceModifier(*it, str);
				}
			}
			if (!(*fit).m_enhancements[i].m_createModifiers.empty()) {
				str += "\n" + lang.get("CreateModifiers") + ":";
				foreach_const (ResModifierMap, it, (*fit).m_enhancements[i].m_createModifiers) {
					descResourceModifier(*it, str);
				}
			}
			if (i != (*fit).m_enhancements.size() - 1) {
				str += "\n";
			}
		}
        stringstream ss;
		ss << (*fit).getUpgradeStage();
		string string;
		ss >> string;
        str += "\n" + string;
	}
	return str;
}

void UpgradeType::doChecksum(Checksum &checksum) const {
	ProducibleType::doChecksum(checksum);
	foreach_const (Enhancements, it, m_enhancements) {
		it->m_enhancement.doChecksum(checksum);
	}
	///@todo resource mods

	// iterating over a std::map is not the same as std::set !!
	vector<int> enhanceIds;
	foreach_const (EnhancementMap, it, m_enhancementMap) {
		enhanceIds.push_back(it->first->getId());
		///@todo add EnhancementType index (it->second->getEnhancement()) ?
	}
	// sort first
	std::sort(enhanceIds.begin(), enhanceIds.end());
	foreach (vector<int>, it, enhanceIds) {
		checksum.add(*it);
	}
}

}}//end namespace
