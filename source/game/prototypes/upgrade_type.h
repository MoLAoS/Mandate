// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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

// ===============================
// 	class UpgradeType
// ===============================

/**
 * A specialization of EnhancementType that represents a permanent,
 * producable (i.e., researchable) upgrade to all of one or more unit
 * classes.
 */
class UpgradeType: public EnhancementType, public ProducibleType {
	friend class UpgradeTypeFactory;
private:
	vector<const UnitType*> effects;

public:
	void preLoad(const string &dir)			{name=basename(dir);}
	virtual bool load(const string &dir, const TechTree *techTree, const FactionType *factionType);

	//get all
	int getEffectCount() const				{return effects.size();}
	const UnitType * getEffect(int i) const	{return effects[i];}
	bool isAffected(const UnitType *unitType) const {
		return find(effects.begin(), effects.end(), unitType) != effects.end();
	}

	virtual void doChecksum(Checksum &checksum) const;

	//other methods
	string getDesc() const;
};



// ===============================
//  class UpgradeTypeFactory
// ===============================

class UpgradeTypeFactory : private SingleTypeFactory<UpgradeType> {
private:
	int idCounter;
	vector<UpgradeType *> types;

public:
	UpgradeTypeFactory() : idCounter(0) { }
	~UpgradeTypeFactory();

	UpgradeType *newInstance();
	UpgradeType* getType(int id);
};

}}//end namespace

#endif
