// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "creatable_type.h"

#include <cassert>

#include "util.h"
#include "logger.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "sim_interface.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Glest::Net;

namespace Glest { namespace ProtoTypes {

void CitizenModifier::reset() {
    characterStats.reset();
    knowledge.reset();
    statistics.cleanse();
}

void CitizenModifier::sum(const CitizenModifier *citizenModifier) {
    statistics.sum(citizenModifier->getStatistics());
    knowledge.sum(citizenModifier->getKnowledge());
    characterStats.sum(citizenModifier->getCharacterStats());
}

bool CitizenModifier::load(const XmlNode *baseNode, const string &dir) {
    bool loadOk = true;
	try {
        const XmlNode *statisticsNode = baseNode->getChild("statistics", 0, false);
        if (statisticsNode) {
            if (!statistics.load(statisticsNode, dir)) {
                loadOk = false;
            }
        }
    } catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *characterStatsNode = baseNode->getChild("character-stats", 0, false);
	    if (characterStatsNode) {
            if (!characterStats.load(characterStatsNode, dir)) {
                loadOk = false;
            }
	    }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *knowledgeNode = baseNode->getChild("knowledge", 0, false);
	    if (knowledgeNode) {
            if (!knowledge.load(knowledgeNode)) {
                loadOk = false;
            }
	    }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}
// =====================================================
// 	class CreatableType
// =====================================================
bool CreatableType::load(const XmlNode *creatableTypeNode, const string &dir, const TechTree *techTree, const FactionType *factionType, bool isItem) {
    bool loadOk = true;
	m_factionType = factionType;
	size = 0;
	height = 0;
	try { size = creatableTypeNode->getChildIntValue("size"); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	// height
	try { height = creatableTypeNode->getChildIntValue("height"); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *statisticsNode = creatableTypeNode->getChild("statistics", 0, false);
        if (statisticsNode) {
            if (!statistics.load(statisticsNode, dir)) {
                loadOk = false;
            }
        }
    } catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *producibleNode = creatableTypeNode->getChild("producible");
        if (!ProducibleType::load(producibleNode, dir)) {
            loadOk = false;
        }
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
    try {
        const XmlNode *unitsOwnedNode= creatableTypeNode->getChild("units-owned", 0, false);
        if (unitsOwnedNode) {
            ownedUnits.resize(unitsOwnedNode->getChildCount());
            for(int i = 0; i<ownedUnits.size(); ++i){
                const XmlNode *ownedNode = unitsOwnedNode->getChild("unit", i);
                string name = ownedNode->getAttribute("name")->getRestrictedValue();
                int limit = ownedNode->getAttribute("limit")->getIntValue();
                ownedUnits[i].init(factionType->getUnitType(name), 0, limit);
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
    try {
        const XmlNode *emanationsNode = creatableTypeNode->getChild("emanations", 0, false);
        if (emanationsNode) {
            emanations.resize(emanationsNode->getChildCount());
            for (int i = 0; i < emanationsNode->getChildCount(); ++i) {
                try {
                    const XmlNode *emanationNode = emanationsNode->getChild("emanation", i);
                    EmanationType *emanation = g_prototypeFactory.newEmanationType();
                    emanation->load(emanationNode, dir);
                    emanations[i] = emanation;
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
        const XmlNode *modificationsNode = creatableTypeNode->getChild("modifications", 0, false);
        if(modificationsNode) {
            modifications.resize(modificationsNode->getChildCount());
            modifyNames.resize(modificationsNode->getChildCount());
            for(int i = 0; i < modificationsNode->getChildCount(); ++i){
                const XmlNode *modificationNode = modificationsNode->getChild("modification", i);
                string mname = modificationNode->getAttribute("name")->getRestrictedValue();
                modifyNames[i] = mname;
                for (int j = 0; j < getFactionType()->getModifications().size(); ++j) {
                    string modName = getFactionType()->getModifications()[j].getModificationName();
                    if (modName == mname) {
                        modifications[i] = getFactionType()->getModifications()[j];
                    }
                }
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
	const XmlNode *effectsNode = creatableTypeNode->getChild("effects", 0, false);
	if (effectsNode) {
		effectTypes.resize(effectsNode->getChildCount());
		for(int i=0; i < effectsNode->getChildCount(); ++i) {
			const XmlNode *effectNode = effectsNode->getChild("effect", i);
			EffectType *effectType = new EffectType();
			effectType->load(effectNode, dir);
			effectTypes[i] = effectType;
		}
	}
	try {
        const XmlNode *resourceProductionNode = creatableTypeNode->getChild("resource-production", 0, false);
        if (resourceProductionNode) {
            if (!resourceGeneration.load(resourceProductionNode, dir, techTree, factionType)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *itemProductionNode = creatableTypeNode->getChild("item-production", 0, false);
        if (itemProductionNode) {
            if (!itemGeneration.load(itemProductionNode, dir, techTree, factionType)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *processingProductionNode = creatableTypeNode->getChild("processing", 0, false);
        if (processingProductionNode) {
            if (!processingSystem.load(processingProductionNode, dir, techTree, factionType)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *unitProductionNode = creatableTypeNode->getChild("unit-production", 0, false);
        if (unitProductionNode) {
            if (!unitGeneration.load(unitProductionNode, dir, techTree, factionType)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *actionsNode = creatableTypeNode->getChild("actions", 0, false);
        if (actionsNode) {
            actions.load(actionsNode, dir, isItem, factionType, this);
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
	return loadOk;
}

}}//end namespace
