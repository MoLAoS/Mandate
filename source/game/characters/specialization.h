// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_SPECIALIZATION_H_
#define _GLEST_GAME_SPECIALIZATION_H_

#include "statistics.h"
#include "effect_type.h"

namespace Glest{ namespace ProtoTypes {

// =====================================================
// 	class Specialization
// =====================================================
class Specialization {
private:
    typedef vector<string> Skills;
    Statistics statistics;
    Equipment equipment;
    Skills skills;
    string name;
public:
    string getSpecName() const {return name;}
    Statistics *getStatistics() {return &statistics;}
    Equipment getEquipment() const {return equipment;}
	void preLoad(const string &dir);
	bool load(const string &dir, const TechTree *techTree, const FactionType *factionType);
};

}}//end namespace

#endif
