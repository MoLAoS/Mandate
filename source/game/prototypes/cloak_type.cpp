// ==============================================================
//	This file is part of the Glest Advanced Engine
//
//	Copyright (C) 2010 James McCulloch
//
//	GPL V2. see source/liscence.txt
// ==============================================================

#include "pch.h"

#include "cloak_type.h"
#include "renderer.h"
#include "world.h"

namespace Glest { namespace ProtoTypes {

using namespace Graphics;
using Sim::World;

// =====================================================
//  class CloakType
// =====================================================

CloakType::CloakType(const UnitType *ut)
		: m_unitType(ut)
		, m_class(CloakClass::INVALID)
		, m_group(-1)
		, m_cost(0)
		, m_cloakSound(0)
		, m_deCloakSound(0)
		, m_allyShader(0)
		, m_enemyShader(0) {
}

CloakType::~CloakType() {
	delete m_cloakSound;
	delete m_deCloakSound;
	delete m_allyShader;
	delete m_enemyShader;
}

void CloakType::load(const string &dir, const XmlNode *cloakNode, const TechTree *tt,
		 vector<string> &out_decloakSkillNames, vector<SkillClass> &out_decloakSkillClasses) {
	RequirableType::load(cloakNode, dir, tt, m_unitType->getFactionType());
	string ct = cloakNode->getRestrictedAttribute("type");
	m_class = CloakClassNames.match(ct.c_str());
	if (m_class == CloakClass::INVALID) {
		throw runtime_error("Invalid CloakClass: " + ct);
	}
	if (m_class == CloakClass::ENERGY) {
		m_cost = cloakNode->getIntAttribute("cost");
	}
	for (int i=0; i < cloakNode->getChildCount(); ++i) {
		const XmlNode *childNode = cloakNode->getChild(i);

		if (childNode->getName() == "de-cloak") { // de-cloak on skill name/class
			const XmlNode *deCloakNode = childNode;
			const XmlAttribute *skillNameAttrib = deCloakNode->getAttribute("skill-name", false);
			if (skillNameAttrib) {
				string str = skillNameAttrib->getValue();
				out_decloakSkillNames.push_back(str);
			} else {
				string str = deCloakNode->getRestrictedAttribute("skill-class");
				SkillClass sc = SkillClassNames.match(str.c_str());
				if (sc == SkillClass::INVALID) {
					throw runtime_error("Invlaid SkillClass: " + str);
				}
				out_decloakSkillClasses.push_back(sc);
			}

		} else if (childNode->getName() == "group") { // cloak group
			string key = childNode->getRestrictedValue();
			m_group = g_world.getCloakGroupId(key);

		} else if (childNode->getName() == "cloak-sound") { // sounds
			m_cloakSound = new StaticSound();
			string path = dir + "/" + childNode->getRestrictedAttribute("path");
			m_cloakSound->load(path);
		} else if (childNode->getName() == "de-cloak-sound") {
			m_deCloakSound = new StaticSound();
			string path = dir + "/" + childNode->getRestrictedAttribute("path");
			m_deCloakSound->load(path);

		} else if (childNode->getName() == "ally-shader") { // custom shaders
			if (childNode->getBoolValue()) {
				string path = dir + "/" + childNode->getAttribute("dir")->getRestrictedValue();
				string name = childNode->getAttribute("name")->getRestrictedValue();
				path = cleanPath(path);
				GlslProgram *program = new GlslProgram(name);
				program->load(path + "/" + name + ".vs", true);
				program->load(path + "/" + name + ".fs", false);
				program->link();

				// wrap-up better...
			}
		} else if (childNode->getName() == "enemy-shader") {
			if (childNode->getBoolValue()) {
				//string path = dir + "/" + childNode->getAttribute("path")->getRestrictedValue();
				//try {
				//	m_enemyShaders = new UnitShaderSet(cleanPath(path));
				//} catch (runtime_error &e) {
				//	delete m_allyShaders;
				//	m_enemyShaders = 0;
				//}
			}
		}
	}
	if (m_group == -1) {
		throw runtime_error("Error: cloak must specify group.");
	}
}

void CloakType::doChecksum(Checksum &cs) const {
	cs.add(m_unitType->getId());
	cs.add(m_class);
	cs.add(m_group);
	cs.add(m_cost);
}

// =====================================================
//  class DetectorType
// =====================================================

DetectorType::DetectorType(const UnitType *ut)
		: RequirableType()
		, m_unitType(ut)
		, m_class(DetectorClass::INVALID)
		, m_cost(0)
		, m_detectionOnSound(0)
		, m_detectionOffSound(0)
		, m_detectionMadeSound(0) {
}

DetectorType::~DetectorType() {
	delete m_detectionOnSound;
	delete m_detectionOffSound;
	delete m_detectionMadeSound;
}

void DetectorType::load(const string &dir, const XmlNode *node, const TechTree *tt) {
	RequirableType::load(node, dir, tt, m_unitType->getFactionType());
	string dt = node->getRestrictedAttribute("type");
	m_class = DetectorClassNames.match(dt.c_str());
	if (m_class == DetectorClass::INVALID) {
		throw runtime_error("Invalid Detector type: '" + dt + "'");
	}

	if (m_class != DetectorClass::PERMANENT) {
		throw runtime_error("Only Detector type 'permanent' is currently supported [got: '" + dt + "']");
	}

	if (m_class == DetectorClass::ENERGY) {
		m_cost = node->getIntAttribute("cost");
	}
	for (int i=0; i < node->getChildCount(); ++i) {
		const XmlNode *childNode = node->getChild(i);

		if (childNode->getName() == "groups") { // detects cloaks from multiple groups
			for (int j=0; j < childNode->getChildCount(); ++j) {
				const XmlNode *groupNode = childNode->getChild("group", j);
				string key = groupNode->getRestrictedValue();
				m_groups.push_back(g_world.getCloakGroupId(key));
			}
		} else if (childNode->getName() == "group") { // detects cloaks from one group
			string key = childNode->getRestrictedValue();
			m_groups.push_back(g_world.getCloakGroupId(key));

		} else if (childNode->getName() == "detect-sound") { // sounds
			m_detectionMadeSound = new StaticSound();
			string path = dir + "/" + childNode->getRestrictedAttribute("path");
			m_detectionMadeSound->load(path);
		} else if (childNode->getName() == "activate-sound") {
			m_detectionOnSound = new StaticSound();
			string path = dir + "/" + childNode->getRestrictedAttribute("path");
			m_detectionOnSound->load(path);
		} else if (childNode->getName() == "deactivate-sound") {
			m_detectionOffSound = new StaticSound();
			string path = dir + "/" + childNode->getRestrictedAttribute("path");
			m_detectionOffSound->load(path);
		}
	}
	if (m_groups.empty()) {
		// error, detector must detect at least one cloak group...
	}
}

bool DetectorType::detectsCloakGroup(int group) const {
	foreach_const (Groups, it, m_groups) {
		if (*it == group) {
			return true;
		}
	}
	return false;
}

void DetectorType::doChecksum(Checksum &cs) const {
	cs.add(m_unitType->getId());
	cs.add(m_class);
	cs.add(m_cost);
	foreach_const (Groups, it, m_groups) {
		cs.add(*it);
	}
}


}}

