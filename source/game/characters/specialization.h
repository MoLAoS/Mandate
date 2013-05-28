// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_SPECIALIZATION_H_
#define _GLEST_GAME_SPECIALIZATION_H_

#include "pch.h"
#include "abilities.h"
#include "character.h"
#include "actions.h"
#include "traits.h"
#include "effect_type.h"

namespace Glest{ namespace ProtoTypes {
// =====================================================
// 	class Specialization
// =====================================================
class Specialization {
private:
    CharacterStats characterStats;
    CraftingStats craftingStats;
    EffectTypes effectTypes;
    Statistics statistics;
    Equipments equipment;
    Knowledge knowledge;
    Actions actions;
    Traits traits;
    string name;
public:
    CraftingStats *getCraftingStats() {return &craftingStats;}
    CharacterStats *getCharacterStats() {return &characterStats;}
    Actions *getActions() {return &actions;}
    Knowledge *getKnowledge() {return &knowledge;}
    Statistics *getStatistics() {return &statistics;}
    int getEquipmentCount() {return equipment.size();}
    Equipments *getEquipments() {return &equipment;}
    Equipment *getEquipment(int i) {return &equipment[i];}
    string getSpecName() const {return name;}
    void reset();
	void save(XmlNode *node) const;
	void preLoad(const string &dir);
	bool load(const string &dir);
};

}}//end namespace

#endif
