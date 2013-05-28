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

	void getDesc(string &str, const char *pre) const;

    void init(const string name, const int amount);
    void save(XmlNode *node) const;
};

// ===============================
// 	class Statistics
// ===============================
typedef vector<DamageType> DamageTypes;

class Statistics {
public:
    EnhancementType enhancement;
    DamageTypes damageTypes;
    DamageTypes resistances;
public:
    EnhancementType *getEnhancement() {return &enhancement;}
    const EnhancementType *getEnhancement() const {return &enhancement;}
    const DamageTypes *getDamageTypes() const {return &damageTypes;}
    const DamageTypes *getResistances() const {return &resistances;}
    int getDamageTypeCount() const {return damageTypes.size();}
    int getResistanceCount() const {return resistances.size();}
    const DamageType *getDamageType(int i) const {return &damageTypes[i];}
    const DamageType *getResistance(int i) const {return &resistances[i];}
    bool load(const XmlNode *baseNode, const string &dir);
    bool isEmpty() const;
    void sum(const Statistics *stats);
    void save(XmlNode *node) const;
    void addResistancesAndDamage(const Statistics *stats);
    void cleanse();
    void modify();
	void getDesc(string &str, const char *pre) const;
	void addStatic(const EnhancementType *e, fixed strength = 1);
	void addMultipliers(const EnhancementType *e, fixed strength = 1);
	void applyMultipliers(const EnhancementType *e);
	void clampMultipliers();
	void sanitise();
	void loadAllDamages(const TechTree *techTree);
};

// ===============================
// 	class Load Bonus
// ===============================
class LoadBonus {
private:
    string source;
    Statistics statistics;
public:
    const Statistics *getStatistics() const {return &statistics;}
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
	int expAdd;
	fixed expMult;
public:
	virtual bool load(const XmlNode *prn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const {
		NameIdPair::doChecksum(checksum);
	}
	const Statistics *getStatistics() const {return &statistics;}
	int getCount() const {return count;}
	int getExp() const {return exp;}
	int getExpAdd() const {return expAdd;}
	fixed getExpMult() const {return expMult;}
};

}}//end namespace

#endif
