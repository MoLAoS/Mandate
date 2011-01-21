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

#ifndef _GLEST_GAME_UPGRADETYPE_H_
#define _GLEST_GAME_UPGRADETYPE_H_

#include <algorithm>

#include "element_type.h"
#include "checksum.h"
#include "xml_parser.h"
#include "unit_stats_base.h"
#include "factory.h"
#include "prototypes_enums.h"
#include "entities_enums.h"

using namespace Shared::Xml;
using Shared::Util::Checksum;
using Shared::Util::SingleTypeFactory;
using Glest::Entities::Unit;

namespace Glest { namespace ProtoTypes {

/** resource amount modifier */
typedef map<const ResourceType*, Modifier> ResModifierMap;

/** A unit type enhancement, an EnhancementType + resource cost modifiers + resource storage modifiers */
struct UpgradeEffect {
	EnhancementType  m_enhancement;
	ResModifierMap   m_costModifiers;
	ResModifierMap   m_storeModifiers;

	const EnhancementType* getEnhancement() const { return &m_enhancement; }

};

// ===============================
// 	class UpgradeType
// ===============================

/** A collection of EnhancementTypes and resource mods that represents a permanent,
  * producable (i.e., researchable) upgrade for one or more unit types. */
class UpgradeType : public ProducibleType {
private:
	typedef vector<UpgradeEffect> Enhancements;
	typedef map<const UnitType*, const UpgradeEffect*> EnhancementMap;
	typedef vector< vector<string> > AffectedUnits; // just names, used only in getDesc()

private:
	Enhancements       m_enhancements;
	EnhancementMap     m_enhancementMap;
	const FactionType *m_factionType;
	AffectedUnits      m_unitsAffected;

public:
	static ProducibleClass typeClass() { return ProducibleClass::UPGRADE; }

public:
	UpgradeType();
	void preLoad(const string &dir)			{ m_name = basename(dir); }
	virtual bool load(const string &dir, const TechTree *techTree, const FactionType *factionType);

	// get
	ProducibleClass getClass() const override                       { return typeClass(); }
	const FactionType* getFactionType() const                       { return m_factionType; }
	const EnhancementType* getEnhancement(const UnitType *ut) const {
		EnhancementMap::const_iterator it = m_enhancementMap.find(ut);
		if (it != m_enhancementMap.end()) {
			return it->second->getEnhancement();
		}
		return 0;
	}
	Modifier getCostModifier(const UnitType *ut, const ResourceType *rt) const {
		EnhancementMap::const_iterator uit = m_enhancementMap.find(ut);
		if (uit != m_enhancementMap.end()) {
			ResModifierMap::const_iterator rit = uit->second->m_costModifiers.find(rt);
			if (rit != uit->second->m_costModifiers.end()) {
				return rit->second;
			}
		}
		return Modifier(0, 1);
	}

	bool isAffected(const UnitType *unitType) const {
		return m_enhancementMap.find(unitType) != m_enhancementMap.end();
	}

	virtual void doChecksum(Checksum &checksum) const;
	string getDesc() const;
};

}} // namespace Glest::ProtoTypes

#endif
