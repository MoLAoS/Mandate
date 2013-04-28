// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_STATISTICS_H_
#define _GLEST_GAME_STATISTICS_H_

#include "unit_stats_base.h"

namespace Glest{ namespace ProtoTypes {
// ===============================
// 	class Quality
// ===============================
class CraftType {
private:
    string name;
    Stat stat;
public:
    string getName() {return name;}
    Stat *getStat() {return &stat;}
};

class Quality {
public:
    DamageTypes damageTypes;
    DamageTypes resistances;
public:
    const DamageTypes *getDamageTypes() const {return &damageTypes;}
    const DamageTypes *getResistances() const {return &resistances;}
    int getDamageTypeCount() const {return damageTypes.size();}
    int getResistanceCount() const {return resistances.size();}
    const DamageType *getDamageType(int i) const {return &damageTypes[i];}
    const DamageType *getResistance(int i) const {return &resistances[i];}
    bool load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft);
    void sum(const Statistics *stats);
    void addResistancesAndDamage(const Statistics *stats);
};

}}//end namespace

#endif
