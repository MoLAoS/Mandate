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
	m_name = basename(dir);
}

bool Modification::load(const string &dir, const TechTree *techTree, const FactionType *factionType) {
	g_logger.logProgramEvent("Modification: " + dir, true);
	bool loadOk = true;

	m_factionType = factionType;
	string path = dir + "/" + m_name + ".xml";

	name = m_name;

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
	if (!RequirableType::load(parametersNode, dir, techTree, factionType)) {
		loadOk = false;
	}
	try {
	    const XmlNode *serviceNode = parametersNode->getChild("service");
	    service = serviceNode->getAttribute("scope")->getRestrictedValue();
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
	    const XmlNode *equipmentNode = parametersNode->getChild("equipment-types", 0, false);
		if(equipmentNode) {
			for(int i = 0; i < equipmentNode->getChildCount(); ++i) {
                try {
                    const XmlNode *typeNode = equipmentNode->getChild("equipment-type", i);
                    string ename = typeNode->getAttribute("name")->getRestrictedValue();
                    equipment.push_back(ename);
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
		const XmlNode *resourceRequirementsNode = parametersNode->getChild("resource-requirements", 0, false);
		if(resourceRequirementsNode) {
			costs.resize(resourceRequirementsNode->getChildCount());
			for(int i = 0; i < costs.size(); ++i) {
				try {
					const XmlNode *resourceNode = resourceRequirementsNode->getChild("resource", i);
					string rname = resourceNode->getAttribute("name")->getRestrictedValue();
					int amount = resourceNode->getAttribute("amount")->getIntValue();
                    int amount_plus = resourceNode->getAttribute("plus")->getIntValue();
                    fixed amount_multiply = resourceNode->getAttribute("multiply")->getFixedValue();
                    costs[i].init(techTree->getResourceType(rname), amount, amount_plus, amount_multiply);
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
        const XmlNode *statisticsNode = parametersNode->getChild("statistics", 0, false);
	    if (statisticsNode) {
            if (!Statistics::load(statisticsNode, dir, techTree, factionType)) {
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
    return loadOk;
}

}}//end namespace
