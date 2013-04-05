// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "command_type.h"

#include <algorithm>
#include <cassert>
#include <climits>

#include "upgrade_type.h"
#include "unit_type.h"
#include "item_type.h"
#include "sound.h"
#include "sound_renderer.h"
#include "util.h"
#include "leak_dumper.h"
#include "graphics_interface.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "renderer.h"
#include "world.h"
#include "game.h"
#include "route_planner.h"
#include "script_manager.h"

#include "leak_dumper.h"
#include "logger.h"
#include "sim_interface.h"

using std::min;
using namespace Glest::Sim;
using namespace Shared::Util;

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class CreateItemCommandType
// =====================================================


bool CreateItemCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const CreatableType *ct) {
	bool loadOk = CommandType::load(n, dir, tt, ct);
    const FactionType *ft = ct->getFactionType();	try {
    const XmlNode *childNode = n->getChild("child-structure", 0, false);
	    if (childNode) {
            m_child = childNode->getAttribute("child-building")->getBoolValue();
	    } else {
            m_child = false;
	    }
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	//produce
	try {
		string skillName = n->getChild("produce-skill")->getAttribute("value")->getRestrictedValue();
		m_produceSkillType = static_cast<const ProduceSkillType*>(creatableType->getSkillType(skillName, SkillClass::PRODUCE));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

	if (n->getOptionalChild("created-item")) {
		try {
			string createdItemName = n->getChild("created-item")->getAttribute("name")->getRestrictedValue();
			m_createdItems.push_back(ft->getItemType(createdItemName));
			if (const XmlAttribute *attrib = n->getChild("created-item")->getAttribute("number", false)) {
				m_createdNumbers.push_back(attrib->getIntValue());
			} else {
				m_createdNumbers.push_back(1);
			}
		} catch (runtime_error e) {
			g_logger.logXmlError(dir, e.what ());
			loadOk = false;
		}
		clicks = Clicks::ONE;
	} else {
		try {
			const XmlNode *un = n->getChild("created-items");
			for (int i=0; i < un->getChildCount(); ++i) {
				const XmlNode *createNode = un->getChild("created-item", i);
				string name = createNode->getAttribute("name")->getRestrictedValue();
				m_createdItems.push_back(ft->getItemType(name));
				try {
					m_tipKeys[name] = createNode->getRestrictedAttribute("tip");
				} catch (runtime_error &e) {
					m_tipKeys[name] = "";
				}
				if (const XmlAttribute *attrib = createNode->getAttribute("number", false)) {
					m_createdNumbers.push_back(attrib->getIntValue());
				} else {
					m_createdNumbers.push_back(1);
				}
			}
		} catch (runtime_error e) {
			g_logger.logXmlError(dir, e.what ());
			loadOk = false;
		}
		clicks = Clicks::TWO;
	}
	// finished sound
	try {
		const XmlNode *finishSoundNode = n->getOptionalChild("finished-sound");
		if (finishSoundNode && finishSoundNode->getAttribute("enabled")->getBoolValue()) {
			m_finishedSounds.resize(finishSoundNode->getChildCount());
			for (int i=0; i < finishSoundNode->getChildCount(); ++i) {
				const XmlNode *soundFileNode = finishSoundNode->getChild("sound-file", i);
				string path = soundFileNode->getAttribute("path")->getRestrictedValue();
				StaticSound *sound = new StaticSound();
				sound->load(dir + "/" + path);
				m_finishedSounds[i] = sound;
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

const ProducibleType* CreateItemCommandType::getProduced(int i) const {
	return m_createdItems[i];
}

void CreateItemCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(m_produceSkillType->getName());
	foreach_const (vector<const ItemType*>, it, m_createdItems) {
		checksum.add((*it)->getName());
	}
}

void CreateItemCommandType::getDesc(string &str, const Unit *unit) const {
	string msg;
	m_produceSkillType->getDesc(msg, unit);
	if (m_createdItems.size() == 1) {
		str += "\n" + m_createdItems[0]->getReqDesc(unit->getFaction(), unit->getType()->getFactionType());
	}
}

string CreateItemCommandType::getReqDesc(const Faction *f, const FactionType *ft) const {
	string res = RequirableType::getReqDesc(f, ft);
	if (m_createdItems.size() == 1) {
		res += m_createdItems[0]->getReqDesc(f, ft);
	}
	return res;
}

void CreateItemCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	if (!pt) {
		m_produceSkillType->getDesc(msg, unit);
	} else {
		// do the time-to-produce calc...
		int framesPerCycle = int(floorf((1.f / (float(unit->getSpeed(m_produceSkillType)) / 4000.f)) + 1.f));
		int timeToBuild = pt->getProductionTime() * framesPerCycle / 40;
		msg = g_lang.get("TimeToBuild") + ": " + intToStr(timeToBuild);
	}
	callback->addElement(msg);
}

void CreateItemCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	if (!pt) {
		Lang &lang = g_lang;
		const string factionName = unit->getFaction()->getType()->getName();
		callback->addElement(g_lang.get("Created") + ":");
		foreach_const (vector<const ItemType*>, it, m_createdItems) {
			callback->addItem(*it, lang.getTranslatedFactionName(factionName, (*it)->getName()));
		}
	}
}

int CreateItemCommandType::getCreatedNumber(const ItemType *it) const {
	for (int i=0; i < m_createdItems.size(); ++i) {
		if (m_createdItems[i] == it) {
			return m_createdNumbers[i];
		}
	}
	throw runtime_error("ItemType not in created command.");
}

void CreateItemCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	if (unit->getCurrSkill()->getClass() != SkillClass::PRODUCE) {
		unit->setCurrSkill(m_produceSkillType);
	} else {
		unit->update2();
		const FactionType *ft = unit->getFaction()->getType();
		const ItemType *prodType = static_cast<const ItemType*>(command->getProdType());
		if (unit->getProgress2() > prodType->getProductionTime()) {
			for (int i=0; i < getCreatedNumber(prodType); ++i) {
                if (unit->getItemLimit() > unit->getItemsStored() && prodType->getTypeTag() != "stock") {
                    Item *newItem = g_world.newItem(unit->getFaction()->getItemCount(), prodType, unit->getFaction());
                    unit->getFaction()->addItem(newItem);
                    unit->accessStorageAdd(unit->getFaction()->getItemCount()-1);
                } else if (prodType->getTypeTag() == "stock") {
                }
            }
            unit->finishCommand();
            unit->setCurrSkill(SkillClass::STOP);
		}
    }
}

}}//end namespace
