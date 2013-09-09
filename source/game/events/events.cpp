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
#include "faction.h"
#include "world.h"
#include "menu_state_character_creator.h"
#include "character_creator.h"
#include "main_menu.h"
#include "sim_interface.h"

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class Event Helper Classes
// =====================================================
void EventItem::init(const ItemType* type, int value, bool scope) {
    itemType = type;
    amount = value;
    local = scope;
}

void EventItem::getDesc(string &str, const char *pre) {
    str += pre;
    str += getItemType()->getName();
    str += ": ";
    str += intToStr(getAmount());
}

void EventUnit::init(const UnitType* type, int value, bool scope) {
    unitType = type;
    amount = value;
    local = scope;
}

void EventUnit::getDesc(string &str, const char *pre) {
    str += pre;
    str += getUnitType()->getName();
    str += ": ";
    str += intToStr(getAmount());
}

void EventUpgrade::init(const UpgradeType* type, int value) {
    upgradeType = type;
    stage = value;
}

void EventUpgrade::getDesc(string &str, const char *pre) {
    str += pre;
    str += getUpgradeType()->getName();
    str += ": ";
    str += intToStr(getStage());
}

bool Triggers::load(const XmlNode *baseNode, const string &dir, const FactionType *ft, bool add) {
    bool loadOk = true;
	try {
		const XmlNode *unitRequirementsNode = baseNode->getChild("unit-requirements", 0, false);
		if(unitRequirementsNode) {
		    unitTriggers.resize(unitRequirementsNode->getChildCount());
			for(int i = 0; i < unitRequirementsNode->getChildCount(); ++i) {
				const XmlNode *unitNode = unitRequirementsNode->getChild("unit", i);
				string name = unitNode->getRestrictedAttribute("name");
				bool scope = unitNode->getAttribute("local")->getBoolValue();
				int value = unitNode->getIntAttribute("amount");
				unitTriggers[i].init(ft->getUnitType(name), value, scope);
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try {
		const XmlNode *itemRequirementsNode = baseNode->getChild("item-requirements", 0, false);
		if(itemRequirementsNode) {
		    itemTriggers.resize(itemRequirementsNode->getChildCount());
			for(int i = 0; i < itemRequirementsNode->getChildCount(); ++i) {
				const XmlNode *itemNode = itemRequirementsNode->getChild("item", i);
				string name = itemNode->getRestrictedAttribute("name");
				bool scope = itemNode->getAttribute("local")->getBoolValue();
				int value = itemNode->getIntAttribute("amount");
				itemTriggers[i].init(ft->getItemType(name), value, scope);
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	try {
		const XmlNode *upgradeRequirementsNode = baseNode->getChild("upgrade-requirements", 0, false);
		if(upgradeRequirementsNode) {
		    upgradeTriggers.resize(upgradeRequirementsNode->getChildCount());
			for(int i = 0; i < upgradeRequirementsNode->getChildCount(); ++i) {
				const XmlNode *upgradeReqNode = upgradeRequirementsNode->getChild("upgrade", i);
				string name = upgradeReqNode->getRestrictedAttribute("name");
				int stage = upgradeReqNode->getIntAttribute("value");
				upgradeTriggers[i].init(ft->getUpgradeType(name), stage);
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}
	try {
		const XmlNode *resourceRequirementsNode = baseNode->getChild("resource-requirements", 0, false);
		if(resourceRequirementsNode) {
			resourceTriggers.resize(resourceRequirementsNode->getChildCount());
			for(int i = 0; i < resourceTriggers.size(); ++i) {
				try {
					const XmlNode *resourceNode = resourceRequirementsNode->getChild("resource", i);
					string name = resourceNode->getAttribute("name")->getRestrictedValue();
					int amount = resourceNode->getAttribute("amount")->getIntValue();
                    int amount_plus = resourceNode->getAttribute("plus")->getIntValue();
                    fixed amount_multiply = resourceNode->getAttribute("multiply")->getFixedValue();
                    if (&g_world) {
                        resourceTriggers[i].init(g_world.getTechTree()->getResourceType(name), amount, amount_plus, amount_multiply);
                    } else {

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
		const XmlNode *localRequirementsNode = baseNode->getChild("local-requirements", 0, false);
		if(localRequirementsNode) {
			localResTriggers.resize(localRequirementsNode->getChildCount());
			for(int i = 0; i < localResTriggers.size(); ++i) {
				try {
					const XmlNode *resourceNode = localRequirementsNode->getChild("resource", i);
					string name = resourceNode->getAttribute("name")->getRestrictedValue();
					int amount = resourceNode->getAttribute("amount")->getIntValue();
                    int amount_plus = resourceNode->getAttribute("plus")->getIntValue();
                    fixed amount_multiply = resourceNode->getAttribute("multiply")->getFixedValue();
                    if (&g_world) {
                        localResTriggers[i].init(g_world.getTechTree()->getResourceType(name), amount, amount_plus, amount_multiply);
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
	return loadOk;
}
// =====================================================
// 	class Choice
// =====================================================
void Choice::processChoice(Faction *faction, EventTargetList targetList) {
    for (int i = 0; i < faction->getUnitCount(); ++i) {
        Unit *possibleTarget = faction->getUnit(i);
        const UnitType *possibleType = possibleTarget->getType();
        for (int j = 0; j < targetList.size(); ++j) {
            if (targetList[j]->getType() == possibleType) {
                for (int k = 0; k < items.size(); ++k) {
                    EventItem *eventItem = &items[k];
                    if (eventItem->getScope() == true) {
                        for (int l = 0; l < eventItem->getAmount(); ++l) {
                            Item *newItem = g_world.newItem(faction->getItemCount(), eventItem->getItemType(), faction);
                            faction->addItem(newItem);
                            possibleTarget->accessStorageAdd(faction->getItemCount()-1);
                        }
                    }
                }
            }
        }
    }
    if (sovereign) {
        Unit *unit = 0;
        for (int i = 0; i < faction->getUnitCount(); ++i) {
            Unit *test = faction->getUnit(i);
            if (test->getType()->isSovereign) {
                unit = test;
            }
        }
        if (unit->isCharacter()) {
            if (!knowledge.isEmpty()) {
                unit->addKnowledge(&knowledge);
            }
            if (!characterStats.isEmpty()) {
                unit->addCharacterStats(&characterStats);
            }
        }
        for (int i = 0; i < getResourceCount(); ++i) {
            const ResourceAmount *res = getResource(i);
            unit->incResourceAmount(res->getType(), res->getAmount());
        }
        for (int i = 0; i < items.size(); ++i) {
            EventItem *eventItem = &items[i];
            if (eventItem->getScope() == false) {
                for (int j = 0; j < eventItem->getAmount(); ++j) {
                    Item *newItem = g_world.newItem(faction->getItemCount(), eventItem->getItemType(), faction);
                    faction->addItem(newItem);
                    unit->accessStorageAdd(faction->getItemCount()-1);
                }
            }
        }
        for (int i = 0; i < units.size(); ++i) {
            EventUnit *eventUnit = &units[i];
            Vec2i locate = unit->getPos();
            for (int j = 0; j < eventUnit->getAmount(); ++j) {
                Unit *createdUnit = g_world.newUnit(locate, eventUnit->getUnitType(), faction, g_world.getMap(), CardinalDir::NORTH);
                g_world.placeUnit(unit->getCenteredPos(), 10, createdUnit);
                createdUnit->setOwner(unit);
                for (int z = 0; z < unit->ownedUnits.size(); ++z) {
                    if (unit->ownedUnits[z].getType() == createdUnit->getType()) {
                        unit->ownedUnits[z].incOwned();
                    }
                }
                createdUnit->create();
                createdUnit->born();
                ScriptManager::onUnitCreated(createdUnit);
                g_simInterface.getStats()->produce(unit->getFactionIndex());
            }
        }
    } else {
        for (int i = 0; i < getResourceCount(); ++i) {
            const ResourceAmount *res = getResource(i);
            faction->incResourceAmount(res->getType(), res->getAmount());
        }
    }
}

void Choice::getDesc(string &str, const char *pre) {
    if (!knowledge.isEmpty()) {
        knowledge.getDesc(str, pre);
        str += pre;
    }
    if (!characterStats.isEmpty()) {
        characterStats.getDesc(str, pre);
        str += pre;
    }
    if (getResourceCount() > 0) {
        str += "Resources:";
        for (int i = 0; i < getResourceCount(); ++i) {
            str += pre;
            str += getResource(i)->getType()->getName();
            str += ": ";
            str += intToStr(getResource(i)->getAmount());
        }
    }
    for (int i = 0; i < items.size(); ++i) {
        items[i].getDesc(str, pre);
    }
    for (int i = 0; i < units.size(); ++i) {
        units[i].getDesc(str, pre);
    }
    for (int i = 0; i < upgrades.size(); ++i) {
        upgrades[i].getDesc(str, pre);
    }
    for (int i = 0; i < gear.size(); ++i) {
        gear[i].getDesc(str, pre, "");
    }
}

bool Choice::load(const XmlNode *baseNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
    bool loadOk = true;
    flavorTitle = baseNode->getAttribute("title")->getRestrictedValue();
    string imgPath = dir + "/" + baseNode->getAttribute("image-path")->getRestrictedValue();
    flavorImage = g_renderer.getTexture2D(ResourceScope::GAME, imgPath);
    sovereign = false;
	const XmlNode *sovNode = baseNode->getChild("sovereign", 0, false);
	if (sovNode) {
        sovereign = sovNode->getAttribute("check")->getBoolValue();
	}
	try {
		const XmlNode *resourceRequirementsNode = baseNode->getChild("resources", 0, false);
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
	try {
		const XmlNode *itemRequirementsNode = baseNode->getChild("items", 0, false);
		if(itemRequirementsNode) {
			items.resize(itemRequirementsNode->getChildCount());
			for(int i = 0; i < items.size(); ++i) {
				try {
					const XmlNode *itemNode = itemRequirementsNode->getChild("item", i);
					string rname = itemNode->getAttribute("name")->getRestrictedValue();
					int amount = itemNode->getAttribute("amount")->getIntValue();
					bool scope = itemNode->getAttribute("local")->getBoolValue();
                    items[i].init(factionType->getItemType(rname), amount, scope);
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
		const XmlNode *unitRequirementsNode = baseNode->getChild("units", 0, false);
		if(unitRequirementsNode) {
			units.resize(unitRequirementsNode->getChildCount());
			for(int i = 0; i < units.size(); ++i) {
				try {
					const XmlNode *unitNode = unitRequirementsNode->getChild("unit", i);
					string rname = unitNode->getAttribute("name")->getRestrictedValue();
					int amount = unitNode->getAttribute("amount")->getIntValue();
					bool scope = unitNode->getAttribute("local")->getBoolValue();
                    units[i].init(factionType->getUnitType(rname), amount, scope);
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
	    const XmlNode *traitsNode = baseNode->getChild("traits", 0, false);
	    if (traitsNode) {
            traits.resize(traitsNode->getChildCount());
            for (int i = 0; i < traitsNode->getChildCount(); ++i) {
                int id = traitsNode->getChild("trait", i)->getAttribute("id")->getIntValue();
                if (!&g_world) {
                    MainMenu *charMenu = static_cast<MainMenu*>(g_program.getState());
                    MenuStateCharacterCreator *charState = static_cast<MenuStateCharacterCreator*>(charMenu->getState());
                    CharacterCreator *charCreator = charState->getCharacterCreator();
                    TechTree *tt = charCreator->getTechTree();
                    traits[i] = tt->getTraitById(id);
                } else {
                    traits[i] = g_world.getTechTree()->getTraitById(id);
                }
            }
	    }
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		return false;
	}
	try {
        const XmlNode *knowledgeNode = baseNode->getChild("knowledge", 0, false);
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
	try {
        const XmlNode *characterStatsNode = baseNode->getChild("character-stats", 0, false);
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
	    const XmlNode *equipmentNode = baseNode->getChild("equipment-types", 0, false);
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
        const XmlNode *modificationsNode = baseNode->getChild("modifications", 0, false);
        if(modificationsNode) {
            modifications.resize(modificationsNode->getChildCount());
            modifyNames.resize(modificationsNode->getChildCount());
            for(int i = 0; i < modificationsNode->getChildCount(); ++i){
                const XmlNode *modificationNode = modificationsNode->getChild("modification", i);
                string mname = modificationNode->getAttribute("name")->getRestrictedValue();
                modifyNames[i] = mname;
                for (int j = 0; j < factionType->getModifications().size(); ++j) {
                    string modName = factionType->getModifications()[j].getModificationName();
                    if (modName == mname) {
                        modifications[i] = &factionType->getModifications()[j];
                    }
                }
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
	return loadOk;
}


// =====================================================
// 	class EventType
// =====================================================
bool EventType::checkTriggers(Faction *faction, EventTargetList *validTargetList) const {
    EventTargetList baseTargetList;
    for (int i = 0; i < getTargetTypeCount(); ++i) {
        const UnitType *targetType = getTargetType(i);
        for (int j = 0; j < faction->getUnitCount(); ++j) {
            Unit *unit = faction->getUnit(j);
            const UnitType *unitType = unit->getType();
            if (unitType == targetType) {
                baseTargetList.push_back(unit);
            }
        }
    }
    for (int i = 0; i < triggers.getUnitTriggerCount(); ++i) {
        if (!triggers.getUnitTrigger(i)->getScope()) {
            if (faction->getCountOfUnitType(triggers.getUnitTrigger(i)->getUnitType()) < triggers.getUnitTrigger(i)->getAmount()) {
                return false;
            }
        }
    }
    for (int i = 0; i < triggers.getItemTriggerCount(); ++i) {
        if (!triggers.getItemTrigger(i)->getScope()) {
            if (faction->getCountOfItemType(triggers.getItemTrigger(i)->getItemType()) < triggers.getItemTrigger(i)->getAmount()) {
                return false;
            }
        }
    }
    for (int i = 0; i < triggers.getUpgradeTriggerCount(); ++i) {
        if (faction->getUpgradeManager()->isUpgraded(triggers.getUpgradeTrigger(i)->getUpgradeType())) {
        } else if (faction->getUpgradeManager()->isPartial(triggers.getUpgradeTrigger(i)->getUpgradeType())) {
            int stage = faction->getCurrentStage(triggers.getUpgradeTrigger(i)->getUpgradeType());
            if (triggers.getUpgradeTrigger(i)->getStage() == stage) {
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
    for (int i = 0; i < triggers.getResourceTriggerCount(); ++i) {
        if (faction->getSResource(triggers.getResourceTrigger(i)->getType())->getAmount() < triggers.getResourceTrigger(i)->getAmount()) {
            return false;
        }
    }
    for (int j = 0; j < baseTargetList.size(); ++j) {
        bool valid = true;
        Unit *potentialUnit = baseTargetList[j];
        for (int i = 0; i < triggers.getUnitTriggerCount(); ++i) {
            if (triggers.getUnitTrigger(i)->getScope()) {
                int unitCount = 0;
                UnitIdList garrisonedUnits = potentialUnit->getGarrisonedUnits();
                if (!garrisonedUnits.empty()) {
                    foreach (UnitIdList, it, garrisonedUnits) {
                        Unit *garrisonedUnit = g_world.getUnit(*it);
                        if (garrisonedUnit->getType() == triggers.getUnitTrigger(i)->getUnitType()) {
                            ++unitCount;
                        }
                    }
                }
                if (unitCount < triggers.getUnitTrigger(i)->getAmount()) {
                    valid = false;
                }
            }
        }
        for (int i = 0; i < triggers.getItemTriggerCount(); ++i) {
            int itemCount = 0;
            if (triggers.getItemTrigger(i)->getScope()) {
                for (int k = 0; k < potentialUnit->getStoredItems().size(); ++k) {
                    if (triggers.getItemTrigger(i)->getItemType() == potentialUnit->getStoredItem(k)->getType()) {
                        ++itemCount;
                    }
                }
                if (itemCount < triggers.getItemTrigger(i)->getAmount()) {
                    valid = false;
                }
            }
        }
        for (int i = 0; i < triggers.getResourceTriggerCount(); ++i) {
            if (potentialUnit->getSResource(triggers.getResourceTrigger(i)->getType())->getAmount() < triggers.getResourceTrigger(i)->getAmount()) {
                valid = false;
            }
        }
        if (valid = true) {
            validTargetList->push_back(potentialUnit);
        }
    }
	return true;
}

void EventType::preLoad(const string &dir){
	m_name = basename(dir);
}

bool EventType::load(const string &dir, const TechTree *techTree, const FactionType *factionType) {
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
	loadOk = DisplayableType::load(eventNode, dir, false);
	const XmlNode *targetTypesNode = eventNode->getChild("target-types", 0, false);
	if (targetTypesNode) {
	    targetTypes.resize(targetTypesNode->getChildCount());
	    for (int i = 0; i < targetTypesNode->getChildCount(); ++i) {
            const XmlNode *targetTypeNode = targetTypesNode->getChild("target-type", i);
            string targetTypeName = targetTypeNode->getAttribute("type")->getRestrictedValue();
            const UnitType *targetType = factionType->getUnitType(targetTypeName);
            targetTypes[i] = targetType;
	    }
    }
	const XmlNode *triggersNode = eventNode->getChild("triggers", 0, false);
	if (triggersNode) {
        if (!triggers.load(triggersNode, dir, factionType, false)) {
            loadOk = false;
        }
	}
	const XmlNode *choicesNode = eventNode->getChild("choices", 0, false);
	if (choicesNode) {
	    choices.resize(choicesNode->getChildCount());
	    for (int i = 0; i < choicesNode->getChildCount(); ++i) {
            const XmlNode *choiceNode = choicesNode->getChild("choice", i);
            if (!choices[i].load(choiceNode, dir, techTree, factionType)) {
                loadOk = false;
            }
	    }
	}
    return loadOk;
}

}}//end namespace
