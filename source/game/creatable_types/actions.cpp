// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "actions.h"

#include <cassert>

#include "util.h"
#include "logger.h"
#include "xml_parser.h"
#include "sim_interface.h"
#include "command_type.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class Actions
// =====================================================
bool Actions::load(const XmlNode *actionsNode, const string &dir, bool isItem, const FactionType *ft, const CreatableType *cType) {
    bool loadOk = true;
    vector<string> deCloakOnSkills;
    vector<SkillClass> deCloakOnSkillClasses;
    try {
        const XmlNode *skillsNode = actionsNode->getChild("skills", 0, false);
        if (skillsNode) {
            for (int i=0; i < skillsNode->getChildCount(); ++i) {
                const XmlNode *skillNode = skillsNode->getChild("skill", i);
                const XmlNode *typeNode = skillNode->getChild("type");
                string classId = typeNode->getAttribute("value")->getRestrictedValue();
                SkillType *skillType = g_prototypeFactory.newSkillType(SkillClassNames.match(classId.c_str()));
                skillType->load(skillNode, dir, ft, cType);
                skillTypes.push_back(skillType);
                g_prototypeFactory.setChecksum(skillType);
            }
            if (!isItem) {
                sortSkillTypes();
                setDeCloakSkills(deCloakOnSkills, deCloakOnSkillClasses);
                try {
                    if(!getFirstStOfClass(SkillClass::STOP)) {
                        throw runtime_error("Every unit must have at least one stop skill: "+ dir);
                    }
                    if(!getFirstStOfClass(SkillClass::DIE)) {
                        throw runtime_error("Every unit must have at least one die skill: "+ dir);
                    }
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
        const XmlNode *commandsNode = actionsNode->getChild("commands", 0, false);
        if (commandsNode) {
            for (int i = 0; i < commandsNode->getChildCount(); ++i) {
                const XmlNode *commandNode = commandsNode->getChild(i);
                if (commandNode->getName() != "command") continue;
                string classId = commandNode->getChildRestrictedValue("type");
                CommandType *commandType = g_prototypeFactory.newCommandType(CmdClassNames.match(classId.c_str()), cType);
                loadOk = commandType->load(commandNode, dir, ft, cType) && loadOk;
                commandTypes.push_back(commandType);
                //g_prototypeFactory.setChecksum(commandType);
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
    sortCommandTypes();
    return loadOk;
}

void Actions::save(XmlNode *node) const {

}

void Actions::setDeCloakSkills(const vector<string> &names, const vector<SkillClass> &classes) {
	foreach_const (vector<string>, it, names) {
		bool found = false;
		foreach (SkillTypes, sit, skillTypes) {
			if (*it == (*sit)->getName()) {
				found = true;
				(*sit)->setDeCloak(true);
				break;
			}
		}
		if (!found) {
			throw runtime_error("de-cloak is set for skill: skill_name, which was not found.");
		}
	}
	foreach_const (vector<SkillClass>, it, classes) {
		foreach (SkillTypes, sit, skillTypesByClass[*it]) {
			(*sit)->setDeCloak(true);
		}
	}
}

bool Actions::hasSkillClass(SkillClass skillClass) const {
	return !skillTypesByClass[skillClass].empty();
}

bool Actions::hasSkillType(const SkillType *st) const {
	assert(st);
	foreach_const (SkillTypes, it, skillTypesByClass[st->getClass()]) {
		if (*it == st) {
			return true;
		}
	}
	return false;
}

SkillType *Actions::getSkillType(const string &skillName, SkillClass skillClass) const{
	for (int i=0; i < skillTypes.size(); ++i) {
		if (skillTypes[i]->getName() == skillName) {
			if (skillTypes[i]->getClass() == skillClass || skillClass == SkillClass::COUNT) {
				return skillTypes[i];
			} else {
				throw runtime_error("Skill '" + skillName + "' is not of class " + SkillClassNames[skillClass]);
			}
		}
	}
	throw runtime_error("No skill named '" + skillName + "'");
}

void Actions::addSkillType(SkillType *skillType) {
    skillTypes.push_back(skillType);
}

void Actions::sortSkillTypes() {
	foreach_enum (SkillClass, sc) {
		foreach (SkillTypes, it, skillTypes) {
			if ((*it)->getClass() == sc) {
				skillTypesByClass[sc].push_back(*it);
			}
		}
	}
}

void Actions::addCommand(CommandType *ct) {
    commandTypes.push_back(ct);
}

void Actions::addBeLoadedCommand(CommandType *ct) {
    commandTypes.push_back(ct);
    commandTypesByClass[CmdClass::BE_LOADED].push_back(ct);
}

void Actions::addSquadCommand(CommandType *ct) {
   squadCommands.push_back(ct);
   commandTypes.push_back(ct);
}

bool Actions::hasCommandType(const CommandType *ct) const {
	assert(ct);
	foreach_const (CommandTypes, it, commandTypesByClass[ct->getClass()]) {
		if (*it == ct) {
			return true;
		}
	}
	return false;
}

void Actions::sortCommandTypes() {
	foreach_enum (CmdClass, cc) {
		foreach (CommandTypes, it, commandTypes) {
			if ((*it)->getClass() == cc) {
				commandTypesByClass[cc].push_back(*it);
			}
		}
	}
}

CommandType *Actions::getCommandType(const string &m_name) const {
	for (CommandTypes::const_iterator i = commandTypes.begin(); i != commandTypes.end(); ++i) {
		if ((*i)->getName() == m_name) {
			return (*i);
		}
	}
	return NULL;
}

const HarvestCommandType *Actions::getHarvestCommand(const ResourceType *rt) const {
	foreach_const (CommandTypes, it, commandTypesByClass[CmdClass::HARVEST]) {
		const HarvestCommandType *hct = static_cast<const HarvestCommandType*>(*it);
		if (hct->canHarvest(rt)) {
			return hct;
		}
	}
	return 0;
}

const AttackCommandType *Actions::getAttackCommand(Zone zone) const {
	foreach_const (CommandTypes, it, commandTypesByClass[CmdClass::ATTACK]) {
		const AttackCommandType *act = static_cast<const AttackCommandType*>(*it);
		if (act->getAttackSkillTypes()->getZone(zone)) {
			return act;
		}
	}
	return 0;
}

const RepairCommandType *Actions::getRepairCommand(const UnitType *repaired) const {
	foreach_const (CommandTypes, it, commandTypesByClass[CmdClass::REPAIR]) {
		const RepairCommandType *rct = static_cast<const RepairCommandType*>(*it);
		if (rct->canRepair(repaired)) {
			return rct;
		}
	}
	return 0;
}

}}//end namespace
