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

#include "element_type.h"
#include "checksum.h"
#include "xml_parser.h"
#include "unit_stats_base.h"

using Shared::Util::Checksum;
using namespace Shared::Xml;

namespace Glest { namespace Game {

class UnitType;
class Unit;

// ===============================
// 	class UpgradeType
// ===============================

/**
 * A specialization of EnhancementTypeBase that represents a permanent,
 * producable (i.e., researchable) upgrade to all of one or more unit
 * classes.
 */
class UpgradeType: public EnhancementTypeBase, public ProducibleType {
private:
    vector<const UnitType*> effects;

public:
	void preLoad(const string &dir)			{name=basename(dir);}
	virtual bool load(const string &dir, const TechTree *techTree, const FactionType *factionType, Checksum &checksum);

    //get all
	int getEffectCount() const				{return effects.size();}
	const UnitType * getEffect(int i) const	{return effects[i];}
	bool isAffected(const UnitType *unitType) const {
		return find(effects.begin(), effects.end(), unitType) != effects.end();
	}

    //other methods
	string getDesc() const;
};

}}//end namespace

#endif
