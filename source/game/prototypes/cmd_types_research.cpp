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
// 	class ResearchCommandType
// =====================================================
bool ResearchCommandType::load(const XmlNode *n, const string &dir, const FactionType *ft, const CreatableType *ct) {
	bool loadOk = CommandType::load(n, dir, ft, ct);
	//produce
	try {
		string skillName = n->getChild("research-skill")->getAttribute("value")->getRestrictedValue();
		m_researchSkillType = static_cast<const ResearchSkillType*>(creatableType->getActions()->getSkillType(skillName, SkillClass::RESEARCH));
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what ());
		loadOk = false;
	}

    FactionType *factionType = g_world.getTechTree()->getFactionType(ft->getName());
	if (n->getOptionalChild("research")) {
		try {
			int researchId = n->getChild("research")->getAttribute("id")->getIntValue();
                if (researchId == -1) {
                    throw runtime_error("id = -1");
                }
			m_researchTypes.push_back(factionType->getTraitById(researchId));
		} catch (runtime_error e) {
			g_logger.logXmlError(dir, e.what ());
			loadOk = false;
		}
		clicks = Clicks::ONE;
	} else {
		try {
			const XmlNode *un = n->getChild("researches");
			for (int i=0; i < un->getChildCount(); ++i) {
				const XmlNode *createNode = un->getChild("research", i);
				int intId = createNode->getAttribute("id")->getIntValue();
                if (intId == -1) {
                    throw runtime_error("id = -1");
                }
				m_researchTypes.push_back(factionType->getTraitById(intId));
				string name = factionType->getTraitById(intId)->getName();
				try {
					m_tipKeys[name] = createNode->getRestrictedAttribute("tip");
				} catch (runtime_error &e) {
					m_tipKeys[name] = "";
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

const ProducibleType* ResearchCommandType::getProduced(int i) const {
	return m_researchTypes[i];
}

void ResearchCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(m_researchSkillType->getName());
}

void ResearchCommandType::getDesc(string &str, const Unit *unit) const {
	string msg;
	m_researchSkillType->getDesc(msg, unit);
	if (m_researchTypes.size() == 1) {
		str += "\n" + m_researchTypes[0]->getReqDesc(unit->getFaction(), unit->getType()->getFactionType());
	}
}

string ResearchCommandType::getReqDesc(const Faction *f, const FactionType *ft) const {
	string res = RequirableType::getReqDesc(f, ft);
	if (m_researchTypes.size() == 1) {
		res += m_researchTypes[0]->getReqDesc(f, ft);
	}
	return res;
}

void ResearchCommandType::descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	string msg;
	if (!pt) {
		m_researchSkillType->getDesc(msg, unit);
	}
	callback->addElement(msg);
}

void ResearchCommandType::subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const {
	if (!pt) {
		Lang &lang = g_lang;
		const string factionName = unit->getFaction()->getType()->getName();
		callback->addElement(g_lang.get("Research") + ":");
		for (int i = 0; i < m_researchTypes.size(); ++i) {
			callback->addItem(m_researchTypes[i], lang.getTranslatedFactionName(factionName, m_researchTypes[i]->getName()));
		}
	}
}

void ResearchCommandType::update(Unit *unit) const {
	_PROFILE_COMMAND_UPDATE();
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);
	if (unit->getCurrSkill()->getClass() != SkillClass::RESEARCH) {
		unit->setCurrSkill(m_researchSkillType);
	} else {
		unit->update2();
		//if (command->getProdType() == 0) {
            //throw runtime_error("no prod type");
		//}
		if (unit->getProgress2() > unit->currentResearch->getProductionTime()) {
            unit->addHeroClass(unit->currentResearch);
            unit->currentResearch = 0;
            unit->finishCommand();
            unit->setCurrSkill(SkillClass::STOP);
		}
    }
}

}}//end namespace
