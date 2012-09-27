// ====================================================================
//	This file is part of the Mandate Engine (www.lordofthedawn.com)
//
//	Copyright (C) 2012 Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//	It is released under the terms of the GNU General Public License 3
//
// ====================================================================

#include "pch.h"
#include "hero.h"
#include "util.h"
#include "sound.h"
#include "tech_tree.h"
#include "renderer.h"
#include "world.h"
#include "sim_interface.h"

namespace Glest { namespace ProtoTypes {
using namespace Hierarchy;

// ===============================
// 	class Hero
// ===============================

// ===============================
// 	class Mage
// ===============================

// ===============================
// 	class Leader
// ===============================

bool Leader::load(const XmlNode *leaderNode, const string &dir, const TechTree *techTree,
                  const FactionType *factionType, const UnitType *unitType, const string &path) {
    bool loadOk = true;
    try { const XmlNode *formationsNode = leaderNode->getChild("formations", 0, false);
    if (formationsNode) {
        formations.resize(formationsNode->getChildCount());
        for (int i = 0; i < formationsNode->getChildCount(); ++i) {
            const XmlNode *formationNode = formationsNode->getChild("formation", i);
            if (!formations[i].load(formationNode, dir)) {
                loadOk = false;
            }
        }
    }
    }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}

	try {
	    formationCommands.resize(formations.size());
        for (int i = 0; i < formations.size(); ++i) {
            Formation newFormation = formations[i];
            formationCommands[i].init(newFormation, Clicks::ONE);
        }
    }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}



    try { const XmlNode *squadNode = leaderNode->getChild("squad", 0, false);
    if (squadNode) {
        /*squadCommands.resize(1);
        for (int i = 0; i < squadCommands.size(); ++i) {
            if (!squadCommands[i].load(squadNode, dir)) {
                loadOk = false;
            }
        }
    }*/
		const XmlNode *squadCommandsNode = squadNode->getChild("squad_commands", 0, false);
		if (squadCommandsNode) {
		for (int i = 0; i < squadCommandsNode->getChildCount(); ++i) {
			const XmlNode *squadCommandNode = squadCommandsNode->getChild(i);
			if (squadCommandNode->getName() != "command") continue;
			string classId = squadCommandNode->getChildRestrictedValue("type");
			CommandType *commandType = g_prototypeFactory.newCommandType(CmdClassNames.match(classId.c_str()), unitType);
			loadOk = commandType->load(squadCommandNode, dir, techTree, factionType) && loadOk;
			squadCommands.push_back(commandType);
			g_prototypeFactory.setChecksum(commandType);
		}
		}
    }
	} catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		loadOk = false;
	}


    return loadOk;
}

}}//end namespace
