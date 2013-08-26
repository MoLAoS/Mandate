// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_TRAITS_H_
#define _GLEST_GAME_TRAITS_H_

#include "character.h"
#include "effect_type.h"

namespace Glest{ namespace ProtoTypes {
// =====================================================
// 	class Traits
// =====================================================
class Trait : public RequirableType {
private:
    CharacterStats characterStats;
    CraftingStats craftingStats;
    EffectTypes effectTypes;
    Statistics statistics;
    Equipments equipment;
    Knowledge knowledge;
    Actions actions;
    int progress;
    string name;
    int id;
    Stat creatorCost;
public:
	const Stat *getCreatorCost() const {return &creatorCost;}
    int getId() const {return id;}
    int getProgress() {return progress;}
    string getTraitName() const {return name;}
    int getEquipmentCount() {return equipment.size();}
    Equipment *getEquipment(int i) {return &equipment[i];}
    Actions *getActions() {return &actions;}
    Statistics *getStatistics() {return &statistics;}
    int getEffectTypeCount() const {return effectTypes.size();}
    const EffectTypes &getEffectTypes() const {return effectTypes;}
    const EffectType *getEffectType(int i) const {return effectTypes[i];}
	bool hasEffects() const {return effectTypes.size() > 0;}
	void save(XmlNode *node) const;
	void getDesc(string &str, const char *pre);
	void preLoad(const string &dir, int i);
	bool load(const string &dir);
};

typedef vector<Trait> ListTraits;
typedef vector<Trait*> Traits;

}}//end namespace

#endif
