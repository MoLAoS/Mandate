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

#include "forward_decs.h"
#include "element_type.h"
#include "actions.h"

#include "checksum.h"
#include "xml_parser.h"
#include "statistics.h"
#include "factory.h"
#include "prototypes_enums.h"
#include "entities_enums.h"

using namespace Shared::Xml;
using Shared::Util::Checksum;
using Shared::Util::SingleTypeFactory;
using Glest::Entities::Unit;
using Glest::Sim::World;

namespace Glest { namespace ProtoTypes {

typedef map<const ResourceType*, Modifier> ResModifierMap;

struct UpgradeEffect {
	Statistics       m_statistics;
	ResModifierMap   m_costModifiers;
	ResModifierMap   m_storeModifiers;
	ResModifierMap   m_createModifiers;
    Actions          actions;

    Actions *getActions() {return &actions;}
	const Statistics* getStatistics() const { return &m_statistics; }
};

// ===============================
// 	class UpgradeType
// ===============================

/** A collection of EnhancementTypes and resource mods that represents a permanent,
  * producable (i.e., researchable) upgrade for one or more unit types. */
class UpgradeUnitCombo {
private:
    const UnitType *m_ut;
    const UpgradeEffect *m_ue;
public:
    void init(const UnitType *ut, UpgradeEffect *ue) {m_ut = ut; m_ue = ue;}

    const UnitType *getUnitType() const {return m_ut;}
    const UpgradeEffect *getUpgradeEffect() const {return m_ue;}

	bool affectsThis(const UnitType *ut, const UpgradeEffect *ue) const {
        if (m_ut == ut && m_ue == ue) {
            return true;
        }
        return false;
	}
};

class UpgradeType : public ProducibleType {
private:
	typedef vector<UpgradeEffect> Upgrades;
	typedef vector<UpgradeUnitCombo> UpgradeUnitMap;
	typedef vector< vector<string> > AffectedUnits;
    typedef vector<string> Names;

public:
	UpgradeUnitMap     m_upgradeMap;
    Names              m_names;

	Upgrades           m_upgrades;
	AffectedUnits      m_unitsAffected;

private:
	void loadResourceModifier(const XmlNode *node, ResModifierMap &map);

public:
	UpgradeType();
	void preLoad(const string &dir)			{ m_name = basename(dir); }
	bool load(const string &dir);
    int maxStage;

    bool isAffected(const UnitType *ut) const {
        for (int i = 0; i < m_upgradeMap.size(); ++i) {
            if (m_upgradeMap[i].getUnitType() == ut) {
                return true;
            }
        }
        return false;
    }

    const Statistics *getStatistics(const UnitType *ut) const {
        for (int i = 0; i < m_upgradeMap.size(); ++i) {
            if (m_upgradeMap[i].getUnitType() == ut) {
                return m_upgradeMap[i].getUpgradeEffect()->getStatistics();
            }
        }
        return 0;
    }

	// get
	ProducibleClass getClass() const override   { return typeClass(); }
	static ProducibleClass typeClass()          { return ProducibleClass::UPGRADE; }
	Modifier getCostModifier(const UnitType *ut, const ResourceType *rt) const;
	Modifier getStoreModifier(const UnitType *ut, const ResourceType *rt) const;
    Modifier getCreateModifier(const UnitType *ut, const ResourceType *rt) const;
    int getMaxStage() const { return maxStage; }

	virtual void doChecksum(Checksum &checksum) const;
	string getDesc(Faction *f) const;
    string getDescName(Faction *f) const;
};

}} // namespace Glest::ProtoTypes

#endif
