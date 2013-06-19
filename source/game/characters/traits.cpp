// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "traits.h"
#include "tech_tree.h"

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class Traits
// =====================================================
void Trait::preLoad(const string &dir){
	m_name = basename(dir);
}

void Trait::save(XmlNode *node) const {
    node->addAttribute("name", name);
    node->addAttribute("progress", progress);
    if (!knowledge.isEmpty()) {
        knowledge.save(node->addChild("knowledge"));
    }
    if (!statistics.isEmpty()) {
        statistics.save(node->addChild("statistics"));
    }
    if (!characterStats.isEmpty()) {
        characterStats.save(node->addChild("character-stats"));
    }
    if (!equipment.empty()) {
        XmlNode *n;
        n = node->addChild("equipment");
        for (int i = 0; i < equipment.size();++i) {
            equipment[i].save(n->addChild("type"));
        }
    }
}

bool Trait::load(const string &dir) {
	g_logger.logProgramEvent("Traits: " + dir, true);
	bool loadOk = true;

	string path = dir + ".xml";

	m_name = basename(dir);
	name = m_name;

	XmlTree xmlTree;
	try { xmlTree.load(path); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		g_logger.logError("Fatal Error: could not load " + path);
		return false;
	}
	const XmlNode *traitNode;
	try { traitNode = xmlTree.getRootNode(); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}
	//if (!RequirableType::load(traitNode, dir)) {
		//loadOk = false;
	//}
	id = traitNode->getAttribute("id")->getIntValue();
	progress = traitNode->getAttribute("progress")->getIntValue();

	const XmlNode *creatorCostNode = traitNode->getChild("creator-cost", 0, false);
	if (creatorCostNode) {
        creatorCost.load(creatorCostNode);
	}

    try {
        const XmlNode *equipmentNode = traitNode->getChild("equipment", 0, false);
        if (equipmentNode) {
            for (int i = 0, k = 0; i < equipmentNode->getChildCount(); ++i) {
                const XmlNode *typeNode = equipmentNode->getChild("type", i);
                string type = typeNode->getAttribute("type")->getRestrictedValue();
                int amount = typeNode->getAttribute("value")->getIntValue();
                for (int j = 0; j < amount; ++j) {
                    Equipment newEquipment;
                    equipment.push_back(newEquipment);
                    equipment[k].init(1, 0, type, type);
                    ++k;
                }
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
	try {
        const XmlNode *characterStatsNode = traitNode->getChild("character-stats", 0, false);
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
        const XmlNode *statisticsNode = traitNode->getChild("statistics", 0, false);
	    if (statisticsNode) {
            if (!statistics.load(statisticsNode, dir)) {
                loadOk = false;
            }
	    }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *actionsNode = traitNode->getChild("actions", 0, false);
	    if (actionsNode) {
	        const CreatableType *ct = 0;
	        const FactionType *ft = 0;
            if (!actions.load(actionsNode, dir, true, ft, ct)) {
                loadOk = false;
            }
	    }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *knowledgeNode = traitNode->getChild("knowledge", 0, false);
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
	const XmlNode *effectsNode = traitNode->getChild("effects", 0, false);
	if (effectsNode) {
		effectTypes.resize(effectsNode->getChildCount());
		for(int i=0; i < effectsNode->getChildCount(); ++i) {
			const XmlNode *effectNode = effectsNode->getChild("effect", i);
			EffectType *effectType = new EffectType();
			effectType->load(effectNode, dir);
			effectTypes[i] = effectType;
		}
	}
    return loadOk;
}

void Trait::getDesc(string &str, const char *pre) {
    characterStats.getDesc(str, pre);
    statistics.enhancement.getDesc(str, pre);
    for (int i = 0; i < equipment.size(); ++i) {
        equipment[i].getDesc(str, pre, "");
    }
    knowledge.getDesc(str, pre);
    actions.getDesc(str, pre);
}

}}//end namespace
