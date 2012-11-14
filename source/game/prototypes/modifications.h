// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_MODIFICATIONS_H_
#define _GLEST_GAME_MODIFICATIONS_H_

#include "abilities.h"
#include "effect_type.h"

namespace Glest{ namespace ProtoTypes {

// =====================================================
// 	class Modification
// =====================================================
class Modification : public EnhancementType, public RequirableType {
private:
    typedef vector<DamageType> DamageBonuses;
    typedef vector<DamageType> ResistanceBonuses;
    typedef vector<ResourceAmount> Costs;

    DamageBonuses damageBonuses;
    ResistanceBonuses resistanceBonuses;

    Costs costs;

	EffectTypes effectTypes;

    const FactionType* m_factionType;
    string name;

public:
	const FactionType* getFactionType() const { return m_factionType; }
    string getModificationName() const {return name;}

    int getEffectTypeCount() const {return effectTypes.size();}
    const EffectTypes &getEffectTypes() const {return effectTypes;}
    const EffectType *getEffectType(int i) const {return effectTypes[i];}
	bool hasEffects() const {return effectTypes.size() > 0;}

    ResistanceBonuses getResistanceBonuses() const {return resistanceBonuses;}
    DamageType getResistanceBonus(int i) const {return resistanceBonuses[i];}

    int getResistanceBonusCount() const {return resistanceBonuses.size();}
    ResistanceBonuses getDamageBonuses() const {return resistanceBonuses;}
    DamageType getDamageBonus(int i) const {return damageBonuses[i];}
    int getDamageBonusCount() const {return damageBonuses.size();}

	void preLoad(const string &dir);
	bool load(const string &dir, const TechTree *techTree, const FactionType *factionType);
};

}}//end namespace

#endif