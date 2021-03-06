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
#include "world.h"
#include "character_creator.h"
#include "main_menu.h"
#include "menu_state_character_creator.h"

#include "leak_dumper.h"

using Glest::Util::Logger;
using namespace Shared::Util;
using namespace Shared::Xml;
using namespace Glest::Graphics;

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class UpgradeType
// =====================================================
void UpgradeEffect::init() {
    for (int i = 1; i < m_statisticsSet.size(); ++i) {
        m_statisticsSet[i] = m_statisticsSet[i-1];
        m_statisticsSet[i].modify();
    }
}
// ==================== misc ====================

UpgradeType::UpgradeType()
		: maxStage(1) {
}

void UpgradeType::loadResourceModifier(const XmlNode *node, ResModifierMap &map) {
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
			const TechTree* tt = 0;
			if (&g_world) {
                tt = g_world.getTechTree();
			} else {
                MainMenu *charMenu = static_cast<MainMenu*>(g_program.getState());
                MenuStateCharacterCreator *charState = static_cast<MenuStateCharacterCreator*>(charMenu->getState());
                CharacterCreator *charCreator = charState->getCharacterCreator();
                tt = charCreator->getTechTree();
			}
			const ResourceType *rt = tt->getResourceType(resName);
			if (map.find(rt) != map.end()) {
				throw runtime_error("duplicate resource node '" + resName + "'");
			}
			map[rt] = Modifier(addition, mult);
		}
	}
}

bool UpgradeType::load(const string &dir) {
	string path;
	g_logger.logProgramEvent("Upgrade type: "+ dir, true);
	path = dir + "/" + m_name + ".xml";
	bool loadOk = true;
	XmlTree xmlTree;
	const XmlNode *upgradeTypeNode;
	try {
		xmlTree.load(path);
		upgradeTypeNode = xmlTree.getRootNode();
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		g_logger.logError("Fatal Error: could not load " + path);
		return false;
	}
	const FactionType *ft = 0;
	if (&g_world) {
        for (int i = 0; i < g_world.getTechTree()->getFactionTypeCount(); ++i) {
            ft = g_world.getTechTree()->getFactionType(i);
            for (int j = 0; j < ft->getUpgradeTypeCount(); ++j) {
                if (m_name == ft->getUpgradeType(i)->getName()) {
                    break;
                }
            }
        }
	} else {
        MainMenu *charMenu = static_cast<MainMenu*>(g_program.getState());
        MenuStateCharacterCreator *charState = static_cast<MenuStateCharacterCreator*>(charMenu->getState());
        CharacterCreator *charCreator = charState->getCharacterCreator();
        const TechTree *tt = charCreator->getTechTree();
        for (int i = 0; i < tt->getFactionTypeCount(); ++i) {
            ft = tt->getFactionType(i);
            for (int j = 0; j < ft->getUnitTypeCount(); ++j) {
                if (m_name == ft->getUnitType(i)->getName()) {
                    break;
                }
            }
        }
	}
    if (ft == 0) {
        loadOk = false;
    }
    const XmlNode *countNode;
    try { countNode = upgradeTypeNode->getOptionalChild("stage"); }
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
    try { namesNode = upgradeTypeNode->getOptionalChild("names");
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
	if (!ProducibleType::load(upgradeTypeNode, dir)) {
		loadOk = false;
	}
	const XmlNode *upgradesNode = upgradeTypeNode->getChild("upgrades", 0, false);
	m_upgradeMap.resize(upgradesNode->getChildCount());
	m_upgrades.resize(upgradesNode->getChildCount());
	m_unitsAffected.resize(upgradesNode->getChildCount());
	for (int i=0; i < m_upgrades.size(); ++i) {
		const XmlNode *upgradeNode, *affectedNode;
		try {
			upgradeNode = upgradesNode->getChild("upgrade", i);
		} catch (runtime_error &e) {
			g_logger.logXmlError(dir, e.what());
			loadOk = false;
			continue;
		}
		try {
			const XmlNode *affectsNode = upgradeNode->getChild("affects", 0);
			for (int j=0; j < affectsNode->getChildCount(); ++j) {
				const XmlNode *affectNode = affectsNode->getChild(j);
				if (affectNode->getName() == "unit") {
					string name = affectNode->getAttribute("name")->getRestrictedValue();
					const UnitType *ut = ft->getUnitType(name);
					m_upgradeMap[i].init(ut, &m_upgrades[i]);
					m_unitsAffected[i].push_back(name);
				} else if (affectNode->getName() == "tag") {
					string tag = affectNode->getAttribute("name")->getRestrictedValue();
					for (int k=0; k < ft->getUnitTypeCount(); ++k) {
						const UnitType *ut = ft->getUnitType(k);
						if (ut->hasTag(tag)) {
                            m_upgradeMap[i].init(ut, &m_upgrades[i]);
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
		const UnitType *ctype = ft->getUnitType(m_unitsAffected[0][0]);
		try {
            const XmlNode *statisticsNode = upgradeNode->getChild("statistics", 0, false);
            if (statisticsNode) {
                m_upgrades[i].m_statisticsSet.resize(maxStage);
                if (!m_upgrades[i].m_statisticsSet[0].load(statisticsNode, dir)) {
                    loadOk = false;
                }
            }
        } catch (runtime_error e) {
            g_logger.logXmlError(dir, e.what());
            loadOk = false;
        }
        try {
            const XmlNode *actionsNode = upgradeNode->getChild("actions", 0, false);
            if (actionsNode) {
                m_upgrades[i].actionsSet.resize(maxStage);
                const CreatableType *ct = 0;
                if (!m_upgrades[i].actionsSet[0].load(actionsNode, dir, true, ft, ct)) {
                    loadOk = false;
                }
            }
        } catch (runtime_error e) {
            g_logger.logXmlError(dir, e.what());
            loadOk = false;
        }
		try {
		    m_upgrades[i].m_costModifiers.resize(1);
			if (const XmlNode *costModsNode = upgradeNode->getOptionalChild("cost-modifiers")) {
				loadResourceModifier(costModsNode, m_upgrades[i].m_costModifiers[0]);
			}
			m_upgrades[i].m_storeModifiers.resize(1);
			if (const XmlNode *storeModsNode = upgradeNode->getOptionalChild("store-modifiers")) {
				loadResourceModifier(storeModsNode, m_upgrades[i].m_storeModifiers[0]);
			}
			m_upgrades[i].m_createModifiers.resize(1);
			if (const XmlNode *createModsNode = upgradeNode->getOptionalChild("create-modifiers")) {
				loadResourceModifier(createModsNode, m_upgrades[i].m_createModifiers[0]);
			}
		} catch (runtime_error e) {
			g_logger.logXmlError(dir, e.what());
			loadOk = false;
		}
		m_upgrades[i].init();
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
    if(!(*fit).getUpgradeType()->m_names.empty()) {
        for (int i = 0; i < (*fit).getUpgradeType()->m_names.size(); ++i) {
            if (i == (*fit).getUpgradeStage()) {
                str += lang.get("Name") + ": ";
                str += (i == 0 ? "" : i == ((*fit).getUpgradeType()->m_names.size()-1) ? " " : " ");
                str += lang.getFactionString(f->getType()->getName(), (*fit).getUpgradeType()->m_names[i]);
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
    if ((*fit).getUpgradeStage() < (*fit).getMaxStage()) {
        if (!(*fit).m_upgrades.empty()) {
            for (int i=0; i < (*fit).m_upgrades.size(); ++i) {
                str += "\n" + lang.get("Affects") + ":";
                for (int j=0; j < (*fit).m_unitsAffected[i].size(); ++j) {
                    str += (j == 0 ? " " : j == ((*fit).m_unitsAffected[i].size() - 1) ? " & " : ", ");
                    str += lang.getFactionString(f->getType()->getName(), (*fit).m_unitsAffected[i][j]);
                }
                (*fit).m_upgrades[i].m_statisticsSet[(*fit).getUpgradeStage()].getDesc(str, "\n");
                if (!(*fit).m_upgrades[i].m_costModifiers[0].empty()) {
                    str += "\n" + lang.get("CostModifiers") + ":";
                    foreach_const (ResModifierMap, it, (*fit).m_upgrades[i].m_costModifiers[0]) {
                        descResourceModifier(*it, str);
                    }
                }
                if (!(*fit).m_upgrades[i].m_storeModifiers[0].empty()) {
                    str += "\n" + lang.get("StoreModifiers") + ":";
                    foreach_const (ResModifierMap, it, (*fit).m_upgrades[i].m_storeModifiers[0]) {
                        descResourceModifier(*it, str);
                    }
                }
                if (!(*fit).m_upgrades[i].m_createModifiers[0].empty()) {
                    str += "\n" + lang.get("CreateModifiers") + ":";
                    foreach_const (ResModifierMap, it, (*fit).m_upgrades[i].m_createModifiers[0]) {
                        descResourceModifier(*it, str);
                    }
                }
                if (i != (*fit).m_upgrades.size() - 1) {
                    str += "\n";
                }
            }
            stringstream ss;
            ss << (*fit).getUpgradeStage();
            string string;
            ss >> string;
            str += "\n" + string;
        }
    }
	return str;
}

Modifier UpgradeType::getCostModifier(const UnitType *ut, const ResourceType *rt) const {
	for (int i = 0; i < m_upgradeMap.size(); ++i) {
		if (m_upgradeMap[i].getUnitType() == ut) {
		    ResModifierMap::const_iterator rit = m_upgradeMap[i].getUpgradeEffect()->m_costModifiers[0].find(rt);
            if (rit != m_upgradeMap[i].getUpgradeEffect()->m_costModifiers[0].end()) {
                return rit->second;
            }
		}
	}
	return Modifier(0, 1);
}

Modifier UpgradeType::getStoreModifier(const UnitType *ut, const ResourceType *rt) const {
	for (int i = 0; i < m_upgradeMap.size(); ++i) {
		if (m_upgradeMap[i].getUnitType() == ut) {
		    ResModifierMap::const_iterator rit = m_upgradeMap[i].getUpgradeEffect()->m_storeModifiers[0].find(rt);
            if (rit != m_upgradeMap[i].getUpgradeEffect()->m_storeModifiers[0].end()) {
                return rit->second;
            }
		}
	}
	return Modifier(0, 1);
}

Modifier UpgradeType::getCreateModifier(const UnitType *ut, const ResourceType *rt) const {
	for (int i = 0; i < m_upgradeMap.size(); ++i) {
		if (m_upgradeMap[i].getUnitType() == ut) {
		    ResModifierMap::const_iterator rit = m_upgradeMap[i].getUpgradeEffect()->m_createModifiers[0].find(rt);
            if (rit != m_upgradeMap[i].getUpgradeEffect()->m_createModifiers[0].end()) {
                return rit->second;
            }
		}
	}
	return Modifier(0, 1);
}

void UpgradeType::doChecksum(Checksum &checksum) const {
	ProducibleType::doChecksum(checksum);
	foreach_const (Upgrades, it, m_upgrades) {
		it->m_statisticsSet[0].getEnhancement()->doChecksum(checksum);
	}
}

}}//end namespace
