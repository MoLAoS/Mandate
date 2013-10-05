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
#include "character_creator.h"
#include "main_menu.h"
#include "menu_state_character_creator.h"

namespace Glest { namespace ProtoTypes {
using namespace Hierarchy;
// ===============================
// 	class Sovereign
// ===============================
bool Sovereign::load(const string &dir, const FactionType *factionType) {
    bool loadOk = true;
    string path = dir + "/sovereign.xml";

	sovName = basename(dir);

	XmlTree xmlTree;
	try { xmlTree.load(path); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		g_logger.logError("Fatal Error: could not load " + path);
		return false;
	}
	const XmlNode *sovereignNode, *characterNode;
	try { characterNode = xmlTree.getRootNode(); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}
    sovereignNode = characterNode->getChild("sovereign");
	try {
	    const XmlNode *specializationNode = sovereignNode->getChild("specialization");
	    string specName = specializationNode->getAttribute("name")->getRestrictedValue();
        if (!&g_world) {
            MainMenu *charMenu = static_cast<MainMenu*>(g_program.getState());
            MenuStateCharacterCreator *charState = static_cast<MenuStateCharacterCreator*>(charMenu->getState());
            CharacterCreator *charCreator = charState->getCharacterCreator();
            FactionType *ft = charCreator->getFactionType();
            specialization = ft->getSpecialization(specName);
        } else {
            specialization = g_world.getTechTree()->getFactionType(factionType->getName())->getSpecialization(specName);
	    }
	}
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}

	try {
	    const XmlNode *traitsNode = sovereignNode->getChild("traits");
	    traits.resize(traitsNode->getChildCount());
        for (int i = 0; i < traitsNode->getChildCount(); ++i) {
            int id = traitsNode->getChild("trait", i)->getAttribute("id")->getIntValue();
            if (!&g_world) {
                MainMenu *charMenu = static_cast<MainMenu*>(g_program.getState());
                MenuStateCharacterCreator *charState = static_cast<MenuStateCharacterCreator*>(charMenu->getState());
                CharacterCreator *charCreator = charState->getCharacterCreator();
                FactionType *ft = charCreator->getFactionType();
                traits[i] = ft->getTraitById(id);
            } else {
                traits[i] = g_world.getTechTree()->getFactionType(factionType->getName())->getTraitById(id);
            }
        }
	}
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}

	try {
        const XmlNode *statisticsNode = sovereignNode->getChild("statistics", 0, false);
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

	}
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}

	try {
	    const XmlNode *actionsNode = sovereignNode->getChild("actions");
	    const XmlNode *skillsNode = actionsNode->getChild("skills");
	    addSkills.resize(skillsNode->getChildCount());
	    for (int i = 0; i < skillsNode->getChildCount(); ++i) {
	        const XmlNode *skillNode = skillsNode->getChild("skill", i);
            string skillName = skillNode->getAttribute("name")->getRestrictedValue();
            addSkills[i] = skillName;
	    }
	    const XmlNode *commandsNode = actionsNode->getChild("commands");
	    addCommands.resize(commandsNode->getChildCount());
	    for (int i = 0; i < commandsNode->getChildCount(); ++i) {
	        const XmlNode *commandNode = commandsNode->getChild("command", i);
            string commandName = commandNode->getAttribute("name")->getRestrictedValue();
            addCommands[i] = commandName;
	    }
	}
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}

	try {

	}
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}

    return loadOk;
}

void Sovereign::save(XmlNode *node) const {
    node->addAttribute("name", sovName);
    XmlNode *n;
    n = node->addChild("specialization");
    n->addAttribute("name", specialization->getSpecName());
    n = node->addChild("traits");
    for (int i = 0; i < traits.size();++i) {
        XmlNode *traitNode = n->addChild("trait");
        traitNode->addAttribute("id", intToStr(traits[i]->getId()));
        traitNode->addAttribute("name", traits[i]->getName());
    }
    if (!characterStats.isEmpty()) {
        characterStats.save(node->addChild("characterStats"));
    }
    if (!statistics.isEmpty()) {
        statistics.save(node->addChild("statistics"));
    }
    if (!knowledge.isEmpty()) {
        knowledge.save(node->addChild("knowledge"));
    }
    if (equipment.size() > 0) {
        n = node->addChild("equipment");
        for (int i = 0; i < equipment.size(); ++i) {
            equipment[i].save(n->addChild("type"));
        }
    }
    //n = node->addChild("effect-types");
    //for (int i = 0; i < effectTypes.size(); ++i) {
        //effectTypes[i]->save(n->addChild("effect-type"));
    //}
    //craftingStats.save(node->addChild("craftingStats"));
    //actions.save(node->addChild("actions"));
}

void Sovereign::addAction(const Actions *addedActions, string actionName) {
}

// ===============================
// 	class Hero
// ===============================

// ===============================
// 	class Mage
// ===============================

// ===============================
// 	class Leader
// ===============================

bool Leader::load(const XmlNode *leaderNode, const string &dir, const UnitType *unitType, const string &path) {
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
                loadOk = commandType->load(squadCommandNode, dir, unitType->getFactionType(), unitType) && loadOk;
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
