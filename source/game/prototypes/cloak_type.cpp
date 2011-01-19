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

namespace Glest { namespace ProtoTypes {

using namespace Graphics;

CloakType::CloakType(const UnitType *ut)
		: m_unitType(ut)
		, m_class(CloakClass::INVALID)
		, m_tags(0)
		, m_cost(0)
		, m_cloakSound(0)
		, m_deCloakSound(0)
		, m_image(0)
		, m_shader(0) {
}

CloakType::~CloakType() {
	delete m_cloakSound;
	delete m_deCloakSound;
	delete m_shader;
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
		if (childNode->getName() == "de-cloak") {
			const XmlNode *deCloakNode = cloakNode->getChild("de-cloak", i);
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
		} else if (childNode->getName() == "image") {
			m_image = g_renderer.newTexture2D(ResourceScope::GAME);
			string path = dir + "/" + childNode->getRestrictedAttribute("path");
			m_image->getPixmap()->load(path);
		} else if (childNode->getName() == "cloak-sound") {
			m_cloakSound = new StaticSound();
			string path = dir + "/" + childNode->getRestrictedAttribute("path");
			m_cloakSound->load(path);
		} else if (childNode->getName() == "de-cloak-sound") {
			m_deCloakSound = new StaticSound();
			string path = dir + "/" + childNode->getRestrictedAttribute("path");
			m_deCloakSound->load(path);
		} else if (childNode->getName() == "visible-shader") {
			string vert = childNode->getChild("vertex-shader")->getAttribute("path")->getRestrictedValue();
			string frag = childNode->getChild("fragment-shader")->getAttribute("path")->getRestrictedValue();
			vert = cleanPath(dir + "/" + vert);
			frag = cleanPath(dir + "/" + frag);
			m_shader = new DefaultShaderProgram();
			m_shader->load(vert, frag);
		}
	}
}

void CloakType::doChecksum(Checksum &cs) const {
	cs.add(m_unitType->getId());
	cs.add(m_class);
	cs.add(m_tags);
	cs.add(m_cost);
}

}}

