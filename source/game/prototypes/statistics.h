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
// 	class DamageType
// ===============================
/**< class that deals with damage type resistance and attack damage types */
class DamageType {
private:
    string type_name;
    int value;

public:
    string getTypeName() const {return type_name;}
    int getValue() const {return value;}
    void setValue(int i) {value = value + i;}

    void init(const string name, const int amount);

};

// ===============================
// 	class Statistics
// ===============================
typedef vector<DamageType> DamageTypes;

class Statistics : public EnhancementType {
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

// ===============================
// 	class Load Bonus
// ===============================
class LoadBonus : public Statistics {
private:
    string source;
public:
	string getSource() const {return source;}
	virtual bool load(const XmlNode *loadBonusNode, const string &dir, const TechTree *tt, const FactionType *ft);
};

// ===============================
// 	class Level
// ===============================

class Level: public NameIdPair {
private:
    Statistics statistics;
    int count;
	int exp;
public:
	virtual bool load(const XmlNode *prn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const {
		NameIdPair::doChecksum(checksum);
	}
	const Statistics *getStatistics() const {return &statistics;}
	int getCount() const {return count;}
	int getExp() const {return exp;}
};

}}//end namespace

#endif
