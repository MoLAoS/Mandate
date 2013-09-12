// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "specialization.h"
#include "tech_tree.h"

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class Specialization
// =====================================================
void Specialization::preLoad(const string &dir){
	name = basename(dir);
}

bool Specialization::load(const string &dir) {
	g_logger.logProgramEvent("Specialization: " + dir, true);
	bool loadOk = true;
	reset();
	string path = dir + ".xml";
	XmlTree xmlTree;
	try { xmlTree.load(path); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		g_logger.logError("Fatal Error: could not load " + path);
		return false;
	}
	const XmlNode *specializationNode;
	try { specializationNode = xmlTree.getRootNode(); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}
    name = basename(dir);

	const XmlNode *creatorCostNode = specializationNode->getChild("creator-cost", 0, false);
	if (creatorCostNode) {
        creatorCost.load(creatorCostNode);
	}

    try {
        const XmlNode *equipmentNode = specializationNode->getChild("equipment", 0, false);
        if (equipmentNode) {
            for (int i = 0, k = 0; i < equipmentNode->getChildCount(); ++i) {
                const XmlNode *typeNode = equipmentNode->getChild("type", i);
                string type = typeNode->getAttribute("type")->getRestrictedValue();
                int amount = typeNode->getAttribute("value")->getIntValue();
                for (int j = 0; j < amount; ++j) {
                    Equipment newEquipment;
                    equipment.push_back(newEquipment);
                    equipment[k].init(1, 0, type, 0);
                    ++k;
                }
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
	try {
        const XmlNode *statisticsNode = specializationNode->getChild("statistics", 0, false);
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
        const XmlNode *characterStatsNode = specializationNode->getChild("character-stats", 0, false);
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
        const XmlNode *knowledgeNode = specializationNode->getChild("knowledge", 0, false);
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
	const XmlNode *effectsNode = specializationNode->getChild("effects", 0, false);
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

void Specialization::reset() {
    characterStats.reset();
    //craftingStats.reset();
    statistics.cleanse();
}

void Specialization::save(XmlNode *node) const {
    node->addAttribute("type", name);
    if (!knowledge.isEmpty()) {
        knowledge.save(node->addChild("knowledge"));
    }
    if (!statistics.isEmpty()) {
        statistics.save(node->addChild("statistics"));
    }
    if (!characterStats.isEmpty()) {
        characterStats.save(node->addChild("character-stats"));
    }
    XmlNode *n;
    n = node->addChild("equipment");
    for (int i = 0; i < equipment.size();++i) {
        equipment[i].save(n->addChild("type"));
    }
}

}}//end namespace
