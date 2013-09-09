// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "item_type.h"

#include <cassert>

#include "unit.h"
#include "world.h"
#include "upgrade.h"
#include "map.h"
#include "command.h"
#include "object.h"
#include "config.h"
#include "creatable_type.h"
#include "skill_type.h"
#include "core_data.h"
#include "renderer.h"
#include "script_manager.h"
#include "cartographer.h"
#include "game.h"
#include "earthquake_type.h"
#include "sound_renderer.h"
#include "sim_interface.h"
#include "user_interface.h"
#include "route_planner.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Glest::Net;

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class ItemType
// =====================================================

void ItemType::preLoad(const string &dir){
	m_name = basename(dir);
}

bool ItemType::load(const string &dir, const TechTree *techTree, const FactionType *factionType) {
	g_logger.logProgramEvent("Item type: " + dir, true);
	bool loadOk = true;
	string path = dir + "/" + m_name + ".xml";
	XmlTree xmlTree;
	try { xmlTree.load(path); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		g_logger.logError("Fatal Error: could not load " + path);
		return false;
	}
	const XmlNode *itemNode;
	try { itemNode = xmlTree.getRootNode(); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}
	const XmlNode *parametersNode;
	try { parametersNode = itemNode->getChild("parameters"); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false; // bail out
	}
	try {
        const XmlNode *typeTagNode = parametersNode->getChild("type-tag");
        if (typeTagNode) {
            typeTag = typeTagNode->getAttribute("type")->getRestrictedValue();
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *craftTypesNode = parametersNode->getChild("craft-types", 0, false);
        if (craftTypesNode) {
            craftTypes.resize(craftTypesNode->getChildCount());
            for (int i = 0; i < craftTypesNode->getChildCount(); ++i) {
                const XmlNode *craftTypeNode = craftTypesNode->getChild("craft-type", i);
                craftTypes[i] = craftTypeNode->getAttribute("type")->getRestrictedValue();
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *tierNode = parametersNode->getChild("tier");
        if (tierNode) {
            qualityTier = tierNode->getAttribute("value")->getIntValue();
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *creatableTypeNode = itemNode->getChild("creatable-type");
        if (!CreatableType::load(creatableTypeNode, dir, techTree, factionType, true)) {
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
