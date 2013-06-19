// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "statistics.h"
#include "tech_tree.h"
#include "faction.h"
#include "unit.h"
#include "world.h"
#include "sim_interface.h"
#include "character_creator.h"
#include "main_menu.h"
#include "menu_state_character_creator.h"

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class DamageType
// =====================================================
bool DamageType::load(const XmlNode *baseNode) {
    bool loadOk = true;

    type_name =  baseNode->getAttribute("type")->getRestrictedValue();

    const XmlAttribute *valueAttr = baseNode->getAttribute("value", false);
    if (valueAttr) {
        value = valueAttr->getIntValue();
    } else {
        value = 0;
    }

	const XmlNode *creatorCostNode = baseNode->getChild("creator-cost", 0, false);
	if (creatorCostNode) {
        creatorCost.load(creatorCostNode);
	}

    return loadOk;
}

void DamageType::init(string name, int amount) {
    type_name = name;
    value = amount;
}

void DamageType::save(XmlNode *node) const {
    node->addAttribute("type", type_name);
    node->addAttribute("value", value);
}

void DamageType::getDesc(string &str, const char *pre) const {
	str += pre;
	str += type_name;
	str += ": ";
	str += intToStr(value);
}

// ===============================
// 	class Statistics
// ===============================
bool Statistics::load(const XmlNode *baseNode, const string &dir) {
    bool loadOk = true;
	try {
        const XmlNode *enhancementNode = baseNode->getChild("enhancement", 0, false);
        if (enhancementNode) {
            if (!enhancement.load(enhancementNode, dir)) {
                loadOk = false;
            }
        }
    } catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	const TechTree *techTree = 0;
	if (g_program.getState()->isGameState()) {
	    techTree = g_world.getTechTree();
	} else if (g_program.getState()->isCCState()) {
	    MainMenu *charMenu = static_cast<MainMenu*>(g_program.getState());
	    MenuStateCharacterCreator *charState = static_cast<MenuStateCharacterCreator*>(charMenu->getState());
	    techTree = charState->getCharacterCreator()->getTechTree();
	} else {
	    MainMenu *charMenu = static_cast<MainMenu*>(g_program.getState());
	    MenuStateCharacterCreator *charState = static_cast<MenuStateCharacterCreator*>(charMenu->getState());
	    techTree = charState->getCharacterCreator()->getTechTree();
	}
	if (!techTree) {
        //throw runtime_error("no tech tree");
        assert(false);
	}
    loadAllDamages(techTree);
	try {
	    const XmlNode *resistancesNode = baseNode->getChild("resistances", 0, false);
	    if (resistancesNode) {
            for (int i = 0; i < resistancesNode->getChildCount(); ++i) {
                const XmlNode *resistanceNode = resistancesNode->getChild("resistance", i);
                string resistanceTypeName = resistanceNode->getAttribute("type")->getRestrictedValue();
                //int amount = resistanceNode->getAttribute("value")->getIntValue();
                for (int j = 0; j < resistances.size(); ++j) {
                    if (resistanceTypeName == resistances[j].getTypeName()) {
                        //resistances[j].setValue(amount);
                        resistances[j].load(resistanceNode);
                    }
                }
            }
	    }
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *damageTypesNode = baseNode->getChild("damage-types", 0, false);
        if (damageTypesNode) {
            for (int i = 0; i < damageTypesNode->getChildCount(); ++i) {
                const XmlNode *damageTypeNode = damageTypesNode->getChild("damage-type", i);
                string damageTypeName = damageTypeNode->getAttribute("type")->getRestrictedValue();
                //int amount = damageTypeNode->getAttribute("value")->getIntValue();
                for (int j = 0; j < damageTypes.size(); ++j) {
                    if (damageTypeName == damageTypes[j].getTypeName()) {
                        //damageTypes[j].setValue(amount);
                        damageTypes[j].load(damageTypeNode);
                    }
                }
            }
        }
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void Statistics::save(XmlNode *node) const {
    XmlNode *n;
    enhancement.save(node->addChild("enhancement"));
    if (!resistances.empty()) {
        n = node->addChild("resistances");
        for (int i = 0; i < resistances.size();++i) {
            resistances[i].save(n->addChild("resistance"));
        }
    }
    if (!damageTypes.empty()) {
        n = node->addChild("damage-types");
        for (int i = 0; i < damageTypes.size();++i) {
            damageTypes[i].save(n->addChild("damage-type"));
        }
    }
}

bool Statistics::isEmpty() const {
    bool empty = true;
    if (!enhancement.isEmpty()) empty = false;
    if (damageTypes.size() > 0) empty = false;
    if (resistances.size() > 0) empty = false;
    return empty;
}

void Statistics::addStatic(const EnhancementType *e, fixed strength) {
	enhancement.addStatic(e);
}

void Statistics::addMultipliers(const EnhancementType *e, fixed strength) {
	enhancement.addMultipliers(e);
}

void Statistics::applyMultipliers(const EnhancementType *e) {
	enhancement.applyMultipliers(e);
}

void Statistics::clampMultipliers() {
	enhancement.clampMultipliers();
}

void Statistics::sanitise() {
	enhancement.sanitiseEnhancement();
}

void Statistics::cleanse() {
	enhancement.reset();
    damageTypes.clear();
    resistances.clear();
}

void Statistics::modify() {
	enhancement.modify();
}

void Statistics::getDesc(string &str, const char *pre) const {
	enhancement.getDesc(str, pre);
	for (int i = 0; i < resistances.size(); ++i) {
	    if (resistances[i].getValue() > 0) {
            resistances[i].getDesc(str, pre);
        }
	}
	for (int i = 0; i < damageTypes.size(); ++i) {
	    if (damageTypes[i].getValue() > 0) {
            damageTypes[i].getDesc(str, pre);
        }
	}
}

void Statistics::loadAllDamages(const TechTree *techTree) {
    for (int i = 0; i < techTree->getDamageTypeCount(); ++i) {
        damageTypes.push_back(techTree->getDamageType(i));
    }
    for (int i = 0; i < techTree->getResistanceCount(); ++i) {
        resistances.push_back(techTree->getResistance(i));
    }
}

void Statistics::sum(const Statistics *stats) {
    enhancement.sum(stats->getEnhancement());
    addResistancesAndDamage(stats);
}

void Statistics::addResistancesAndDamage(const Statistics *stats) {
	for (int i = 0; i < stats->getResistanceCount(); ++i) {
        string name = stats->getResistance(i)->getTypeName();
        int value = stats->getResistance(i)->getValue();
        DamageType dType;
        dType.init(name, value);
        if (resistances.size() == 0) {
            resistances.push_back(dType);
        } else {
            for (int k = 0; k < resistances.size(); ++k) {
                if (resistances[k].getTypeName() == dType.getTypeName()) {
                    resistances[k].setValue(dType.getValue());
                    break;
                } else if (k == resistances.size()-1) {
                    resistances.push_back(dType);
                    break;
                }
            }
        }
    }
	for (int i = 0; i < stats->getDamageTypeCount(); ++i) {
        string name = stats->getDamageType(i)->getTypeName();
        int value = stats->getDamageType(i)->getValue();
        DamageType dType;
        dType.init(name, value);
        if (damageTypes.size() == 0) {
            damageTypes.push_back(dType);
        } else {
            for (int k = 0; k < damageTypes.size(); ++k) {
                if (damageTypes[k].getTypeName() == dType.getTypeName()) {
                    damageTypes[k].setValue(dType.getValue());
                    break;
                } else if (k == damageTypes.size()-1) {
                    damageTypes.push_back(dType);
                    break;
                }
            }
        }
    }
}

// ===============================
// 	class Load Bonus
// ===============================
bool LoadBonus::load(const XmlNode *loadBonusNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
	bool loadOk = true;
    const XmlNode *sourceNode = loadBonusNode->getChild("source");
    const XmlNode *loadableUnitNode = sourceNode->getChild("loadable-unit");
    source = loadableUnitNode->getAttribute("name")->getRestrictedValue();
    const XmlNode *statisticsNode = loadBonusNode->getChild("statistics", 0, false);
    if (statisticsNode) {
        statistics.load(statisticsNode, dir);
    }
	return loadOk;
}

// ===============================
// 	class Level
// ===============================
bool Level::load(const XmlNode *levelNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = true;
	count = 1;
	exp = 1;
	expAdd = 0;
	expMult = 1;
	try {
		m_name = levelNode->getAttribute("name")->getRestrictedValue();
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		exp = levelNode->getAttribute("exp")->getIntValue();
		const XmlAttribute *expAddNode = levelNode->getAttribute("add", false);
        if (expAddNode) {
            expAdd = expAddNode->getIntValue();
        }
		const XmlAttribute *expMultNode = levelNode->getAttribute("mult", false);
        if (expMultNode) {
            expMult = expMultNode->getIntValue();
        }
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		count = levelNode->getAttribute("count")->getIntValue();
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
	    const XmlNode *statsNode = levelNode->getChild("statistics");
        if (!statistics.load(statsNode, dir)) {
            loadOk = false;
        }
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

}}//end namespace
