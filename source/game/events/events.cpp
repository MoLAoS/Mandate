// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "events.h"
#include "tech_tree.h"

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class Event Helper Classes
// =====================================================


// =====================================================
// 	class Event
// =====================================================
void Event::preLoad(const string &dir){
	m_name = basename(dir);
}

bool Event::load(const string &dir, const TechTree *techTree, const FactionType *factionType) {
	g_logger.logProgramEvent("Events: " + dir, true);
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
	const XmlNode *eventNode;
	try { eventNode = xmlTree.getRootNode(); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}

	const XmlNode *parametersNode;
	try { parametersNode = eventNode->getChild("parameters"); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}
	if (!RequirableType::load(parametersNode, dir, techTree, factionType)) {
		loadOk = false;
	}
	try {
	    const XmlNode *equipmentNode = parametersNode->getChild("equipment-types", 0, false);
		if(equipmentNode) {
			for(int i = 0; i < equipmentNode->getChildCount(); ++i) {
                try {
                    const XmlNode *typeNode = equipmentNode->getChild("equipment-type", i);
                    string name = typeNode->getAttribute("name")->getRestrictedValue();
                    string type = typeNode->getAttribute("type")->getRestrictedValue();
                    int number = typeNode->getAttribute("amount")->getIntValue();
                    Equipment newEquipment;
                    newEquipment.init(number, 0, name, type);
                    gear.push_back(newEquipment);
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
        const XmlNode *modificationsNode = parametersNode->getChild("modifications", 0, false);
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
                        modifications[i] = &getFactionType()->getModifications()[j];
                    }
                }
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
	try {
		const XmlNode *resourceRequirementsNode = parametersNode->getChild("resources", 0, false);
		if(resourceRequirementsNode) {
			resources.resize(resourceRequirementsNode->getChildCount());
			for(int i = 0; i < resources.size(); ++i) {
				try {
					const XmlNode *resourceNode = resourceRequirementsNode->getChild("resource", i);
					string rname = resourceNode->getAttribute("name")->getRestrictedValue();
					int amount = resourceNode->getAttribute("amount")->getIntValue();
                    int amount_plus = resourceNode->getAttribute("plus")->getIntValue();
                    fixed amount_multiply = resourceNode->getAttribute("multiply")->getFixedValue();
                    resources[i].init(techTree->getResourceType(rname), amount, amount_plus, amount_multiply);
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
    return loadOk;
}

}}//end namespace
