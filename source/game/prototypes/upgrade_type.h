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

/** modifier pair (static addition and multiplier)
  * @todo move to unit_stats.h, use for all stat 'buffs' ? */
struct Modifier {
	int    m_addition;
	fixed  m_multiplier;

	int   getAddition()   const { return m_addition;   }
	fixed getMultiplier() const { return m_multiplier; }

	Modifier() : m_addition(0), m_multiplier(1) {}
	Modifier(int add, fixed mult) : m_addition(add), m_multiplier(mult) {}
	Modifier(const Modifier &that) : m_addition(that.m_addition), m_multiplier(that.m_multiplier) {}
};
//typedef pair<int, fixed> Modifier;

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

	ProducibleClass getClass() const override { return typeClass(); }

	const FactionType* getFactionType() const { return m_factionType; }

	// get all
	const EnhancementType* getEnhancement(const UnitType *ut) const {
		EnhancementMap::const_iterator it = m_enhancementMap.find(ut);
		if (it != m_enhancementMap.end()) {
			return it->second->getEnhancement();
		}
		return 0;
	}

	bool isAffected(const UnitType *unitType) const {
		return m_enhancementMap.find(unitType) != m_enhancementMap.end();
	}

	virtual void doChecksum(Checksum &checksum) const;

	//other methods
	string getDesc() const;
};

}} // namespace Glest::ProtoTypes

#endif
