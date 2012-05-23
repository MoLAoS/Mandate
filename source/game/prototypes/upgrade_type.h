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

///@todo: ResourceModifierType ...
/** resource amount modifier */
typedef map<const ResourceType*, Modifier> ResModifierMap;

/** A unit type enhancement, an EnhancementType + resource cost modifiers + resource storage modifiers + resource creation modifiers */
struct UpgradeEffect {
	EnhancementType  m_enhancement;
	ResModifierMap   m_costModifiers;
	ResModifierMap   m_storeModifiers;
	ResModifierMap   m_createModifiers;

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
    typedef vector<string> Names; // just names, used only in getNameDesc()

private:
	EnhancementMap     m_enhancementMap;
	const FactionType *m_factionType;
    Names              m_names;
    mutable int        upgradeStage;
public:
	Enhancements       m_enhancements;
	AffectedUnits      m_unitsAffected;


private:
	bool loadNewStyle(const XmlNode *node, const string &dir, const TechTree *techTree, const FactionType *factionType);
	bool loadOldStyle(const XmlNode *node, const string &dir, const TechTree *techTree, const FactionType *factionType);
	void loadResourceModifier(const XmlNode *node, ResModifierMap &map, const TechTree *techTree);

public:
	static ProducibleClass typeClass() { return ProducibleClass::UPGRADE; }

public:
	UpgradeType();
	void preLoad(const string &dir)			{ m_name = basename(dir); }
	virtual bool load(const string &dir, const TechTree *techTree, const FactionType *factionType);
    int maxStage;

	// get
	ProducibleClass getClass() const override                       { return typeClass(); }
	const FactionType* getFactionType() const                       { return m_factionType; }
	const EnhancementType* getEnhancement(const UnitType *ut) const;
	Modifier getCostModifier(const UnitType *ut, const ResourceType *rt) const;
	Modifier getStoreModifier(const UnitType *ut, const ResourceType *rt) const;
    Modifier getCreateModifier(const UnitType *ut, const ResourceType *rt) const;
    int getMaxStage() const { return maxStage; }
    int getUpgradeStage() const { return upgradeStage; }

	bool isAffected(const UnitType *unitType) const {
		return m_enhancementMap.find(unitType) != m_enhancementMap.end();
	}

	virtual void doChecksum(Checksum &checksum) const;
	string getDesc(const Faction *f) const;
    string getDescName(const Faction *f) const;

    //set
    virtual void setUpgradeStage(int v) const { upgradeStage = v; }
};

}} // namespace Glest::ProtoTypes

#endif
