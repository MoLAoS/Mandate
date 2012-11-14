// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "modifications.h"
#include "tech_tree.h"

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class Modification
// =====================================================
void Modification::preLoad(const string &dir){
	name = basename(dir);
}

bool Modification::load(const string &dir, const TechTree *techTree, const FactionType *factionType) {
	g_logger.logProgramEvent("Modification: " + dir, true);
	bool loadOk = true;

	m_factionType = factionType;
	string path = dir + "/" + name + ".xml";

	XmlTree xmlTree;
	try { xmlTree.load(path); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		g_logger.logError("Fatal Error: could not load " + path);
		return false;
	}
	const XmlNode *modificationNode;
	try { modificationNode = xmlTree.getRootNode(); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}
	const XmlNode *parametersNode;
	try { parametersNode = modificationNode->getChild("parameters"); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}
	try {
		const XmlNode *resourceRequirementsNode = parametersNode->getChild("resource-requirements", 0, false);
		if(resourceRequirementsNode) {
			costs.resize(resourceRequirementsNode->getChildCount());
			for(int i = 0; i < costs.size(); ++i) {
				try {
					const XmlNode *resourceNode = resourceRequirementsNode->getChild("resource", i);
					string name = resourceNode->getAttribute("name")->getRestrictedValue();
					int amount = resourceNode->getAttribute("amount")->getIntValue();
                    int amount_plus = resourceNode->getAttribute("plus")->getIntValue();
                    float amount_multiply = resourceNode->getAttribute("multiply")->getFloatValue();
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
	if (!RequirableType::load(parametersNode, dir, techTree, factionType)) {
		loadOk = false;
	}
	try {
        const XmlNode *enhancementNode = parametersNode->getChild("enhancement", 0, false);
	    if (enhancementNode) {
            if (!EnhancementType::load(enhancementNode, dir, techTree, factionType)) {
                loadOk = false;
            }
	    }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	const XmlNode *effectsNode = parametersNode->getChild("effects", 0, false);
	if (effectsNode) {
		effectTypes.resize(effectsNode->getChildCount());
		for(int i=0; i < effectsNode->getChildCount(); ++i) {
			const XmlNode *effectNode = effectsNode->getChild("effect", i);
			EffectType *effectType = new EffectType();
			effectType->load(effectNode, dir, techTree, factionType);
			effectTypes[i] = effectType;
		}
	}
	try {
	    const XmlNode *damageTypesNode = parametersNode->getChild("damage-types", 0, false);
	    if (damageTypesNode) {
	        damageBonuses.resize(damageTypesNode->getChildCount());
            for (int i = 0; i < damageTypesNode->getChildCount(); ++i) {
                const XmlNode *damageTypeNode = damageTypesNode->getChild("damage-type", i);
                string damageTypeName = damageTypeNode->getAttribute("type")->getRestrictedValue();
                int amount = damageTypeNode->getAttribute("value")->getIntValue();
                damageBonuses[i].init(damageTypeName, amount);
            }
	    }
	}
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
	    const XmlNode *resistancesNode = parametersNode->getChild("resistances", 0, false);
	    if (resistancesNode) {
	        resistanceBonuses.resize(resistancesNode->getChildCount());
            for (int i = 0; i < resistancesNode->getChildCount(); ++i) {
                const XmlNode *resistanceNode = resistancesNode->getChild("resistance", i);
                string resistanceTypeName = resistanceNode->getAttribute("type")->getRestrictedValue();
                int amount = resistanceNode->getAttribute("value")->getIntValue();
                resistanceBonuses[i].init(resistanceTypeName, amount);
            }
	    }
	}
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
    return loadOk;
}

}}//end namespace
