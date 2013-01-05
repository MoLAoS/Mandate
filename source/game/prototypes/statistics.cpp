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

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class DamageType
// =====================================================
void DamageType::init(string name, int amount) {
    type_name = name;
    value = amount;
}
// ===============================
// 	class Statistics
// ===============================
bool Statistics::load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft) {
    bool loadOk = true;
	try {
        const XmlNode *enhancementNode = baseNode->getChild("enhancement", 0, false);
        if (enhancementNode) {
            if (!EnhancementType::load(enhancementNode, dir, tt, ft)) {
                loadOk = false;
            }
        }
    } catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
	    const XmlNode *resistancesNode = baseNode->getChild("resistances", 0, false);
	    if (resistancesNode) {
	        resistances.resize(resistancesNode->getChildCount());
            for (int i = 0; i < resistancesNode->getChildCount(); ++i) {
                const XmlNode *resistanceNode = resistancesNode->getChild("resistance", i);
                string resistanceTypeName = resistanceNode->getAttribute("type")->getRestrictedValue();
                int amount = resistanceNode->getAttribute("value")->getIntValue();
                resistances[i].init(resistanceTypeName, amount);
            }
	    }
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *damageTypesNode = baseNode->getChild("damage-types", 0, false);
        if (damageTypesNode) {
            damageTypes.resize(damageTypesNode->getChildCount());
            for (int i = 0; i < damageTypesNode->getChildCount(); ++i) {
                const XmlNode *damageTypeNode = damageTypesNode->getChild("damage-type", i);
                string damageTypeName = damageTypeNode->getAttribute("type")->getRestrictedValue();
                int amount = damageTypeNode->getAttribute("value")->getIntValue();
                damageTypes[i].init(damageTypeName, amount);
            }
        }
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void Statistics::sum(const Statistics *stats) {
    EnhancementType::sum(stats);
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
    const XmlNode *enhancementNode = loadBonusNode->getChild("enhancement", 0, false);
    if (enhancementNode) {
        EnhancementType::load(enhancementNode, dir, techTree, factionType);
    }
	return loadOk;
}

// ===============================
// 	class Level
// ===============================
bool Level::load(const XmlNode *levelNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = true;
	try {
		m_name = levelNode->getAttribute("name")->getRestrictedValue();
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		exp = levelNode->getAttribute("exp")->getIntValue();
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
        if (!statistics.load(levelNode, dir, tt, ft)) {
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
