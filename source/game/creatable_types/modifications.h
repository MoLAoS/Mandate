// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_MODIFICATIONS_H_
#define _GLEST_GAME_MODIFICATIONS_H_

#include "statistics.h"
#include "effect_type.h"

namespace Glest{ namespace ProtoTypes {

// =====================================================
// 	class Modification
// =====================================================
class Modification : public RequirableType {
private:
    Statistics statistics;
    typedef vector<ResourceAmount> Costs;
    typedef vector<string> Equipment;
    Costs costs;
    string service;
	EffectTypes effectTypes;
    Equipment equipment;
    const FactionType* m_factionType;
    string name;
public:
    const Statistics *getStatistics() const {return &statistics;}
	const FactionType* getFactionType() const { return m_factionType; }
    string getModificationName() const {return name;}
    Equipment getEquipment() const {return equipment;}
    string getEquipment(int i) const {return equipment[i];}
    int getEquipmentSize() const {return equipment.size();}
    string getService() const {return service;}
    int getCostCount() const {return costs.size();}
    ResourceAmount getCost(int i) const {return costs[i];}
    int getEffectTypeCount() const {return effectTypes.size();}
    const EffectTypes &getEffectTypes() const {return effectTypes;}
    const EffectType *getEffectType(int i) const {return effectTypes[i];}
	bool hasEffects() const {return effectTypes.size() > 0;}
	void preLoad(const string &dir);
	bool load(const string &dir);
};

}}//end namespace

#endif
