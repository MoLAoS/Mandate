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
// =====================================================
// 	class CreatableType
// =====================================================
bool CreatableType::load(const XmlNode *creatableTypeNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
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
	halfSize = size / fixed(2);
	halfHeight = height / fixed(2);
	try {
        const XmlNode *statsNode = creatableTypeNode->getChild("stats", 0, false);
        if (statsNode) {
            if (!UnitStats::load(statsNode, dir, techTree, factionType)) {
                loadOk = false;
            }
        }
    } catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *enhancementNode = creatableTypeNode->getChild("enhancement", 0, false);
        if (enhancementNode) {
            if (!EnhancementType::load(enhancementNode, dir, techTree, factionType)) {
                loadOk = false;
            }
        }
    } catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *producibleNode = creatableTypeNode->getChild("producible");
        if (!ProducibleType::load(producibleNode, dir, techTree, factionType)) {
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
	    const XmlNode *resistancesNode = creatableTypeNode->getChild("resistances", 0, false);
	    if (resistancesNode) {
	        resistances.resize(resistancesNode->getChildCount());
            for (int i = 0; i < resistancesNode->getChildCount(); ++i) {
                const XmlNode *resistanceNode = resistancesNode->getChild("resistance", i);
                string resistanceTypeName = resistanceNode->getAttribute("type")->getRestrictedValue();
                int amount = resistanceNode->getAttribute("value")->getIntValue();
                resistances[i].init(resistanceTypeName, amount);
            }
	    }
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *damageTypesNode = creatableTypeNode->getChild("damage-types", 0, false);
        if (damageTypesNode) {
            damageTypes.resize(damageTypesNode->getChildCount());
            for (int i = 0; i < damageTypesNode->getChildCount(); ++i) {
                const XmlNode *damageTypeNode = damageTypesNode->getChild("damage-type", i);
                string damageTypeName = damageTypeNode->getAttribute("type")->getRestrictedValue();
                int amount = damageTypeNode->getAttribute("value")->getIntValue();
                damageTypes[i].init(damageTypeName, amount);
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
                    emanation->load(emanationNode, dir, techTree, factionType);
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
			effectType->load(effectNode, dir, techTree, factionType);
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
            vector<string> deCloakOnSkills;
            vector<SkillClass> deCloakOnSkillClasses;
            try {
                const XmlNode *skillsNode = actionsNode->getChild("skills");
                for (int i=0; i < skillsNode->getChildCount(); ++i) {
                    const XmlNode *skillNode = skillsNode->getChild("skill", i);
                    const XmlNode *typeNode = skillNode->getChild("type");
                    string classId = typeNode->getAttribute("value")->getRestrictedValue();
                    SkillType *skillType = g_prototypeFactory.newSkillType(SkillClassNames.match(classId.c_str()));
                    skillType->load(skillNode, dir, techTree, this);
                    skillTypes.push_back(skillType);
                    g_prototypeFactory.setChecksum(skillType);
                }
            } catch (runtime_error e) {
                g_logger.logXmlError(dir, e.what());
                loadOk = false;
            }
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
            try {
                const XmlNode *commandsNode = actionsNode->getChild("commands");
                for (int i = 0; i < commandsNode->getChildCount(); ++i) {
                    const XmlNode *commandNode = commandsNode->getChild(i);
                    if (commandNode->getName() != "command") continue;
                    string classId = commandNode->getChildRestrictedValue("type");
                    CommandType *commandType = g_prototypeFactory.newCommandType(CmdClassNames.match(classId.c_str()), this);
                    loadOk = commandType->load(commandNode, dir, techTree, this) && loadOk;
                    commandTypes.push_back(commandType);
                    g_prototypeFactory.setChecksum(commandType);
                }
            } catch (runtime_error e) {
                g_logger.logXmlError(dir, e.what());
                loadOk = false;
            }
            sortCommandTypes();
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
	return loadOk;
}

void CreatableType::setDeCloakSkills(const vector<string> &names, const vector<SkillClass> &classes) {
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

bool CreatableType::hasSkillClass(SkillClass skillClass) const {
	return !skillTypesByClass[skillClass].empty();
}

bool CreatableType::hasSkillType(const SkillType *st) const {
	assert(st);
	foreach_const (SkillTypes, it, skillTypesByClass[st->getClass()]) {
		if (*it == st) {
			return true;
		}
	}
	return false;
}

const SkillType *CreatableType::getSkillType(const string &skillName, SkillClass skillClass) const{
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

void CreatableType::addSkillType(SkillType *skillType) {
    skillTypes.push_back(skillType);
}

void CreatableType::sortSkillTypes() {
	foreach_enum (SkillClass, sc) {
		foreach (SkillTypes, it, skillTypes) {
			if ((*it)->getClass() == sc) {
				skillTypesByClass[sc].push_back(*it);
			}
		}
	}
	if (!skillTypesByClass[SkillClass::BE_BUILT].empty()) {
        startSkill = skillTypesByClass[SkillClass::BE_BUILT].front();
    } else {
        startSkill = skillTypesByClass[SkillClass::STOP].front();
    }
    foreach (SkillTypes, it, skillTypesByClass[SkillClass::ATTACK]) {
        if ((*it)->getProjectile()) {
            m_hasProjectileAttack = true;
            break;
        }
    }
}

void CreatableType::addCommand(CommandType *ct) {
    commandTypes.push_back(ct);
}

void CreatableType::addBeLoadedCommand(CommandType *ct) {
    commandTypes.push_back(ct);
    commandTypesByClass[CmdClass::BE_LOADED].push_back(ct);
}

void CreatableType::addSquadCommand(CommandType *ct) {
   squadCommands.push_back(ct);
   commandTypes.push_back(ct);
}

bool CreatableType::hasCommandType(const CommandType *ct) const {
	assert(ct);
	foreach_const (CommandTypes, it, commandTypesByClass[ct->getClass()]) {
		if (*it == ct) {
			return true;
		}
	}
	return false;
}

void CreatableType::sortCommandTypes() {
	foreach_enum (CmdClass, cc) {
		foreach (CommandTypes, it, commandTypes) {
			if ((*it)->getClass() == cc) {
				commandTypesByClass[cc].push_back(*it);
			}
		}
	}
}

const CommandType *CreatableType::getCommandType(const string &m_name) const {
	for (CommandTypes::const_iterator i = commandTypes.begin(); i != commandTypes.end(); ++i) {
		if ((*i)->getName() == m_name) {
			return (*i);
		}
	}
	return NULL;
}

const HarvestCommandType *CreatableType::getHarvestCommand(const ResourceType *rt) const {
	foreach_const (CommandTypes, it, commandTypesByClass[CmdClass::HARVEST]) {
		const HarvestCommandType *hct = static_cast<const HarvestCommandType*>(*it);
		if (hct->canHarvest(rt)) {
			return hct;
		}
	}
	return 0;
}

const AttackCommandType *CreatableType::getAttackCommand(Zone zone) const {
	foreach_const (CommandTypes, it, commandTypesByClass[CmdClass::ATTACK]) {
		const AttackCommandType *act = static_cast<const AttackCommandType*>(*it);
		if (act->getAttackSkillTypes()->getZone(zone)) {
			return act;
		}
	}
	return 0;
}

const RepairCommandType *CreatableType::getRepairCommand(const UnitType *repaired) const {
	foreach_const (CommandTypes, it, commandTypesByClass[CmdClass::REPAIR]) {
		const RepairCommandType *rct = static_cast<const RepairCommandType*>(*it);
		if (rct->canRepair(repaired)) {
			return rct;
		}
	}
	return 0;
}

}}//end namespace
