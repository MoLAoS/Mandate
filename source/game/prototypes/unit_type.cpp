// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "unit_type.h"

#include <cassert>

#include "util.h"
#include "upgrade_type.h"
#include "resource_type.h"
#include "sound.h"
#include "logger.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "resource.h"
#include "renderer.h"
#include "world.h"
#include "sim_interface.h"

#include "leak_dumper.h"

using namespace Shared::Xml;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace ProtoTypes {
// ===============================
// 	class BonusPower
// ===============================
bool BonusPower::load(const XmlNode *bonusPowerNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
    bool loadOk = true;
	try {
        const XmlNode *statisticsNode = bonusPowerNode->getChild("statistics", 0, false);
        if (statisticsNode) {
            if (!Statistics::load(statisticsNode, dir, techTree, factionType)) {
                loadOk = false;
            }
        }
    } catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *resourceProductionNode = bonusPowerNode->getChild("resource-production", 0, false);
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
        const XmlNode *itemProductionNode = bonusPowerNode->getChild("item-production", 0, false);
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
        const XmlNode *processingProductionNode = bonusPowerNode->getChild("processing", 0, false);
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
        const XmlNode *unitProductionNode = bonusPowerNode->getChild("unit-production", 0, false);
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
	return loadOk;
}

// ===============================
// 	class PetRule
// ===============================
/*
void PetRule::load(const XmlNode *prn, const string &dir, const TechTree *tt, const FactionType *ft){
	string unitTypeName = prn->getAttribute("type")->getRestrictedValue();
	type = ft->getUnitType(unitTypeName);
	count = prn->getAttribute("count")->getIntValue();
}
*/

// =====================================================
// 	class UnitType
// =====================================================

// ===================== PUBLIC ========================


// ==================== creation and loading ====================

UnitType::UnitType()
		: multiBuild(false), multiSelect(false)
		, armourType(0)
		, light(false), lightColour(0.f)
		, m_cloakType(0)
		, m_detectorType(0)
		, meetingPoint(false), meetingPointImage(0)
		, inhuman(0)
		, foundation("none")
		, bonusObjectName("")
		, m_cellMap(0), m_colourMap(0)
		, display(true) {
	reset();
}

UnitType::~UnitType(){
	deleteValues(selectionSounds.getSounds().begin(), selectionSounds.getSounds().end());
	deleteValues(commandSounds.getSounds().begin(), commandSounds.getSounds().end());
	delete m_cellMap;
	delete m_colourMap;
}

void UnitType::preLoad(const string &dir){
	m_name = basename(dir);
}

bool UnitType::load(const string &dir, const TechTree *techTree, const FactionType *factionType) {
	g_logger.logProgramEvent("Unit type: " + dir, true);
	bool loadOk = true;
	string path = dir + "/" + m_name + ".xml";
	XmlTree xmlTree;
	try { xmlTree.load(path); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		g_logger.logError("Fatal Error: could not load " + path);
		return false; // bail
	}
	const XmlNode *unitNode;
	try { unitNode = xmlTree.getRootNode(); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}
	const XmlNode *parametersNode;
	try { parametersNode = unitNode->getChild("parameters"); }
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}
    try { multiBuild = parametersNode->getOptionalBoolValue("multi-build"); }
    catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
    try {
        const XmlNode *foundationNode = parametersNode->getChild("foundation", 0, false);
        foundation = "none";
        if (foundationNode) {
            foundation = foundationNode->getAttribute("type")->getRestrictedValue();
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
    try { multiSelect= parametersNode->getChildBoolValue("multi-selection"); }
    catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
	const XmlNode *fieldNode = parametersNode->getChild("field", 0, false);
	if (fieldNode) {
		try {
			field = FieldNames.match(fieldNode->getRestrictedValue().c_str());
            if (field == Field::AIR) {
            zone = Zone::AIR;
            } else if (field == Field::LAND) {
            zone = Zone::LAND;
            } else if (field == Field::WALL || field == Field::STAIR) {
            zone = Zone::WALL;
            }
		} catch (runtime_error &e) {
			g_logger.logXmlError(path, e.what());
			loadOk = false;
		}
	} else {
		Fields fields;
		try {
			fields.load(parametersNode->getChild("fields"), dir, techTree, factionType);
			field = Field::INVALID;
			if (fields.get(Field::AMPHIBIOUS))		field = Field::AMPHIBIOUS;
			else if (fields.get(Field::ANY_WATER))	field = Field::ANY_WATER;
			else if (fields.get(Field::DEEP_WATER))	field = Field::DEEP_WATER;
			else if(fields.get(Field::LAND))		field = Field::LAND;
			else if(fields.get(Field::AIR))			field = Field::AIR;
			else throw runtime_error("unit prototypes must specify a field");
			zone = field == Field::AIR ? Zone::AIR : Zone::LAND;
		} catch (runtime_error e) {
			g_logger.logXmlError(path, e.what());
			loadOk = false;
		}
	}
	vector<string> deCloakOnSkills;
	vector<SkillClass> deCloakOnSkillClasses;
    try { properties.load(parametersNode->getChild("properties"), dir, techTree, factionType); }
    catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
	try {
		const XmlNode *tagsNode = parametersNode->getChild("tags", 0, false);
		if (tagsNode) {
			for (int i = 0; i < tagsNode->getChildCount(); ++i) {
				const XmlNode *tagNode = tagsNode->getChild("tag", i);
				string tag = tagNode->getRestrictedValue();
				m_tags.insert(tag);
			}
		}
	} catch (runtime_error &e) {
		g_logger.logXmlError(path, e.what());
		loadOk = false;
	}
    try {
        const XmlNode *cloakNode = parametersNode->getOptionalChild("cloak");
        if (cloakNode) {
            m_cloakType = g_prototypeFactory.newCloakType(this);
            m_cloakType->load(dir, cloakNode, techTree, deCloakOnSkills, deCloakOnSkillClasses);
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
    try {
        const XmlNode *detectNode = parametersNode->getOptionalChild("detector");
        if (detectNode) {
            m_detectorType = g_prototypeFactory.newDetectorType(this);
            m_detectorType->load(dir, detectNode, techTree);
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
    try {
        const XmlNode *meetingPointNode= parametersNode->getChild("meeting-point");
        meetingPoint= meetingPointNode->getAttribute("value")->getBoolValue();
        if (meetingPoint) {
            string imgPath = dir + "/" + meetingPointNode->getAttribute("image-path")->getRestrictedValue();
            meetingPointImage = g_renderer.getTexture2D(ResourceScope::GAME, imgPath);
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
	try {
		string armorTypeName = parametersNode->getChildRestrictedValue("armor-type");
		armourType = techTree->getArmourType(armorTypeName);
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *creatableTypeNode = unitNode->getChild("creatable-type");
        if (!CreatableType::load(creatableTypeNode, dir, techTree, factionType)) {
            loadOk = false;
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *resourceStoresNode = unitNode->getChild("resource-stores", 0, false);
        if (resourceStoresNode) {
            resourceStores.resize(resourceStoresNode->getChildCount());
            for (int i = 0; i < resourceStoresNode->getChildCount(); ++i) {
                const XmlNode *resourceStoreNode = resourceStoresNode->getChild("resource", i);
                string status = resourceStoreNode->getAttribute("status")->getRestrictedValue();
                string resType = resourceStoreNode->getAttribute("type")->getRestrictedValue();
                int required = resourceStoreNode->getAttribute("stored")->getIntValue();
                resourceStores[i].init(status, techTree->getResourceType(resType), required);
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *itemStoresNode = unitNode->getChild("item-stores", 0, false);
        if (itemStoresNode) {
            itemStores.resize(itemStoresNode->getChildCount());
            for (int i = 0; i < itemStoresNode->getChildCount(); ++i) {
                const XmlNode *itemStoreNode = itemStoresNode->getChild("item", i);
                string resType = itemStoreNode->getAttribute("type")->getRestrictedValue();
                int required = itemStoreNode->getAttribute("stored")->getIntValue();
                itemStores[i].init(factionType->getItemType(resType), required);
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
    try {
        const XmlNode *cellMapNode= parametersNode->getChild("cellmap", 0, false);
        if (cellMapNode && cellMapNode->getAttribute("value")->getBoolValue()) {
            m_cellMap = new PatchMap<1>(Rectangle(0,0, getSize(), getSize()), 0);
            for (int i=0; i < getSize(); ++i) {
                try {
                    const XmlNode *rowNode= cellMapNode->getChild("row", i);
                    string row= rowNode->getAttribute("value")->getRestrictedValue();
                    if (row.size() != getSize()) {
                        throw runtime_error("Cellmap row is not the same as unit size");
                    }
                    for (int j=0; j < row.size(); ++j) {
                        m_cellMap->setInfluence(Vec2i(j, i), row[j] == '0' ? 0 : 1);
                    }
                } catch (runtime_error e) {
                    g_logger.logXmlError(path, e.what());
                    loadOk = false;
                }
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
	try {
        inhuman = false;
	    const XmlNode *inhumanNode = unitNode->getChild("control", 0, false);
	    if (inhumanNode) {
            string control = inhumanNode->getAttribute("type")->getRestrictedValue();
            if (control == "inhuman") {
                inhuman = true;
            } else {
                inhuman = false;
            }
            live = 1;
            const XmlNode *personalityNode = inhumanNode->getChild("personality", 0, false);
            if (personalityNode) {
                personality = personalityNode->getAttribute("type")->getRestrictedValue();
                for (int i = 0; i < factionType->getPersonalities().size(); ++i) {
                    if (personality == factionType->getPersonalities()[i].getPersonalityName()) {
                        for (int k = 0; k < factionType->getPersonalities()[i].getGoals().size(); ++k) {
                            Focus goal = factionType->getPersonalities()[i].getGoal(k);
                            Goal goalName = goal.getName();
                            int goalImportance = goal.getImportance();
                            if (goalName == Goal::LIVE) {
                                live = goalImportance;
                            }
                        }
                    }
                }
            }
	    }
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
	    const XmlNode *dayNode = unitNode->getChild("day-power", 0, false);
	    if (dayNode) {
	        const XmlNode *enhancementNode = dayNode->getChild("enhancement", 0, false);
	        if (enhancementNode) {
                if(!dayPower.load(enhancementNode, dir, techTree, factionType)) {
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
	    const XmlNode *nightNode = unitNode->getChild("night-power", 0, false);
	    if (nightNode) {
	        const XmlNode *enhancementNode = nightNode->getChild("enhancement", 0, false);
	        if (enhancementNode) {
                if(!nightPower.load(enhancementNode, dir, techTree, factionType)) {
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
	    const XmlNode *bonusObjectNode = unitNode->getChild("bonus-object", 0, false);
	    if (bonusObjectNode) {
	        bonusObjectName = bonusObjectNode->getAttribute("object-name")->getRestrictedValue();
	        const XmlNode *enhancementNode = bonusObjectNode->getChild("enhancement", 0, false);
	        if (enhancementNode) {
                //if(!.load(enhancementNode, dir, techTree, factionType)) {
                    //loadOk = false;
                //}
	        }
	    }
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
	    const XmlNode *mageNode = unitNode->getChild("mage", 0, false);
	    if (mageNode) {
	        isMage = true;
	    }
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
    try {
        const XmlNode *leaderNode = unitNode->getChild("leader", 0, false);
        if (leaderNode) {
            isLeader = true;
            if (!leader.load(leaderNode, dir, techTree, factionType, this, path)) {
                loadOk = false;
            }
        }
	}
	catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		return false;
	}
	for (int i = 0; i < leader.squadCommands.size(); ++i) {
        addSquadCommand(leader.squadCommands[i]);
	}
	if (meetingPoint) {
		CommandType *smpct = g_prototypeFactory.newCommandType(CmdClass::SET_MEETING_POINT, this);
		addCommand(smpct);
		g_prototypeFactory.setChecksum(smpct);
	}
	try {
	    const XmlNode *heroNode = unitNode->getChild("hero", 0, false);
	    if (heroNode) {
	        isHero = true;
	    }
	}
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *itemLimitNode = unitNode->getChild("item-limit", 0, false);
        if (itemLimitNode) {
            itemLimit = itemLimitNode->getAttribute("limit")->getIntValue();
        } else {
            itemLimit = 0;
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
    try {
        const XmlNode *equipmentNode = unitNode->getChild("equipment", 0, false);
        if (equipmentNode) {
            for (int i = 0, k = 0; i < equipmentNode->getChildCount(); ++i) {
                const XmlNode *typeNode = equipmentNode->getChild("type", i);
                string type = typeNode->getAttribute("type")->getRestrictedValue();
                int amount = typeNode->getAttribute("value")->getIntValue();
                for (int j = 0; j < amount; ++j) {
                    Equipment newEquipment;
                    equipment.push_back(newEquipment);
                    equipment[k].init(1, 0, type, type);
                    ++k;
                }
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
    try {
        const XmlNode *starterItemsNode = unitNode->getChild("starter-items", 0, false);
        if (starterItemsNode) {
            for (int i = 0; i < starterItemsNode->getChildCount(); ++i) {
                const XmlNode *starterItemNode = starterItemsNode->getChild("type", i);
                string type = starterItemNode->getAttribute("type")->getRestrictedValue();
                starterItems.push_back(type);
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(dir, e.what());
        loadOk = false;
    }
    try {
        const XmlNode *levelsNode = unitNode->getChild("levels", 0, false);
        if(levelsNode) {
            levels.resize(levelsNode->getChildCount());
            for(int i=0; i<levels.size(); ++i){
                const XmlNode *levelNode= levelsNode->getChild("level", i);
                levels[i].load(levelNode, dir, techTree, factionType);
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
    try {
        const XmlNode *loadBonusesNode = unitNode->getChild("load-bonuses", 0, false);
        if(loadBonusesNode) {
            loadBonuses.resize(loadBonusesNode->getChildCount());
            for(int i = 0; i < loadBonuses.size(); ++i){
                const XmlNode *loadBonusNode = loadBonusesNode->getChild("load-bonus", i);
                loadBonuses[i].load(loadBonusNode, dir, techTree, factionType);
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
    try {
        const XmlNode *selectionSoundNode = unitNode->getChild("selection-sounds");
        if(selectionSoundNode->getAttribute("enabled")->getBoolValue()){
            selectionSounds.resize(selectionSoundNode->getChildCount());
            for(int i=0; i<selectionSounds.getSounds().size(); ++i){
                const XmlNode *soundNode= selectionSoundNode->getChild("sound", i);
                string path= soundNode->getAttribute("path")->getRestrictedValue();
                StaticSound *sound= new StaticSound();
                sound->load(dir + "/" + path);
                selectionSounds[i]= sound;
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
    try { // command sounds
        const XmlNode *commandSoundNode = unitNode->getChild("command-sounds");
        if(commandSoundNode->getAttribute("enabled")->getBoolValue()){
            commandSounds.resize(commandSoundNode->getChildCount());
            for(int i=0; i<commandSoundNode->getChildCount(); ++i){
                const XmlNode *soundNode= commandSoundNode->getChild("sound", i);
                string path= soundNode->getAttribute("path")->getRestrictedValue();
                StaticSound *sound= new StaticSound();
                sound->load(dir + "/" + path);
                commandSounds[i]= sound;
            }
        }
    } catch (runtime_error e) {
        g_logger.logXmlError(path, e.what());
        loadOk = false;
    }
	m_colourMap = new PatchMap<1>(Rectangle(0,0, getSize(), getSize()), 0);
	RectIterator iter(Rect2i(0, 0, getSize() - 1, getSize() - 1));
	while (iter.more()) {
		Vec2i pos = iter.next();
		if (!hasCellMap() || m_cellMap->getInfluence(pos)) {
			m_colourMap->setInfluence(pos, 1);
		} else {
			int ncount = 0;
			PerimeterIterator pIter(Vec2i(pos.x - 1, pos.y - 1), Vec2i(pos.x + 1, pos.y + 1));
			while (pIter.more()) {
				Vec2i nPos = pIter.next();
				if (m_cellMap->getInfluence(nPos)) {
					++ncount;
				}
			}
			if (ncount >= 2) {
				m_colourMap->setInfluence(pos, 1);
			}
		}
	}
	display = true;
	return loadOk;
}

void UnitType::addBeLoadedCommand() {
	CommandType *blct = g_prototypeFactory.newCommandType(CmdClass::BE_LOADED, this);
	const MoveSkillType *mst = static_cast<const MoveSkillType*>(CreatableType::getFirstStOfClass(SkillClass::MOVE));
	static_cast<BeLoadedCommandType*>(blct)->setMoveSkill(mst);
	CreatableType::addBeLoadedCommand(blct);
	g_prototypeFactory.setChecksum(blct);
}

void UnitType::doChecksum(Checksum &checksum) const {
	if (armourType) checksum.add(armourType->getName());
	checksum.add(light);
	checksum.add(lightColour);
	checksum.add(multiBuild);
	checksum.add(multiSelect);
	foreach_const (Levels, it, levels) {
		it->doChecksum(checksum);
	}
	checksum.add(meetingPoint);
}
// ==================== has ====================
bool UnitType::isOfClass(UnitClass uc) const{
	switch (uc) {
		case UnitClass::WARRIOR:
			return hasSkillClass(SkillClass::ATTACK)
				&& !hasSkillClass(SkillClass::HARVEST);
		case UnitClass::WORKER:
			return hasSkillClass(SkillClass::BUILD)
                || hasSkillClass(SkillClass::CONSTRUCT)
				|| hasSkillClass(SkillClass::REPAIR)
				|| hasSkillClass(SkillClass::HARVEST);
		case UnitClass::BUILDING:
			return hasSkillClass(SkillClass::BE_BUILT)
				&& !hasSkillClass(SkillClass::MOVE);
		case UnitClass::CARRIER:
			return hasSkillClass(SkillClass::LOAD)
				&& hasSkillClass(SkillClass::UNLOAD);
		default:
			throw runtime_error("Unknown UnitClass passed to UnitType::isOfClass()");
	}
	return false;
}

Vec2i rotateCellOffset(const Vec2i &offset, const int unitSize, const CardinalDir facing) {
	Vec2i result;
	switch (facing) {
		case CardinalDir::NORTH:
			result = offset;
			break;
		case CardinalDir::EAST:
			result.y = offset.x;
			result.x = unitSize - offset.y - 1;
			break;
		case CardinalDir::SOUTH:
			result.x = unitSize - offset.x - 1;
			result.y = unitSize - offset.y - 1;
			break;
		case CardinalDir::WEST:
			result.x = offset.y;
			result.y = unitSize - offset.x - 1;
			break;
	}
	return result;
}

bool UnitType::getCellMapCell(Vec2i pos, CardinalDir facing) const {
	RUNTIME_CHECK(m_cellMap != 0);
	return m_cellMap->getInfluence(rotateCellOffset(pos, getSize(), facing));
}

}}//end namespace
