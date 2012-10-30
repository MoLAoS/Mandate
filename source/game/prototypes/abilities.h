// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_ABILITIES_H_
#define _GLEST_GAME_ABILITIES_H_

#include "unit_stats_base.h"

namespace Glest{ namespace ProtoTypes {

// ===============================
// 	class Load Bonus
// ===============================
/** resource amount modifier */
typedef map<const ResourceType*, Modifier> ResModifierMap;
/** A unit type enhancement, an EnhancementType + resource cost modifiers + resource storage modifiers + resource creation modifiers */
struct GarrisonEffect {
	EnhancementType  m_enhancement;
	ResModifierMap   m_costModifiers;
	ResModifierMap   m_storeModifiers;
	ResModifierMap   m_createModifiers;
	const EnhancementType* getEnhancement() const { return &m_enhancement; }
};

class LoadBonus {
public:
    string source;
    typedef GarrisonEffect Enhancement;
    Enhancement m_enhancement;
	LoadBonus();
	virtual bool load(const XmlNode *loadBonusNode, const string &dir, const TechTree *tt, const FactionType *ft);
    bool loadNewStyle(const XmlNode *node, const string &dir, const TechTree *techTree, const FactionType *factionType);
	void loadResourceModifier(const XmlNode *node, ResModifierMap &map, const TechTree *techTree);
	const Enhancement* getEnhancement() const {return &m_enhancement;}
	string getSource() const {return source;}
};

// ===============================
// 	class Timer
// ===============================

class Timer {
public:
    int timerValue;
    int currentStep;

    Timer() : timerValue(0), currentStep(0) {}
	Timer(const Timer &that) : timerValue(that.timerValue), currentStep(that.currentStep) {}

	void init(const XmlNode *n, const TechTree *tt);
	void init(int timerValue, int currentStep);

    int getTimerValue() {return timerValue;}
    int getCurrentStep() {return currentStep;}

    virtual void setTimerValue(int v) { currentStep = v; }
    virtual void setCurrentStep(int v) { currentStep = v; }

	void save(XmlNode *node) const;
};

// ===============================
// 	class TimerStep
// ===============================
/**< class that makes up current step vector */

class TimerStep {

public:
    mutable int currentStep;
    int getCurrentStep() {return currentStep;}
    void setCurrentStep(int i) {currentStep = i;}
};

typedef vector<TimerStep> CurrentStep;

// =====================================================
// 	class CreatedUnit
//
/// Amount of a given UnitType
// =====================================================

class CreatedUnit {
protected:
	const UnitType       *m_type;
	int	                 m_amount;
	int	                 m_amount_plus;
	float	             m_amount_multiply;
	int                  m_cap;

public:
	CreatedUnit() : m_type(0), m_amount(0), m_amount_plus(0), m_amount_multiply(0), m_cap(0) {}
	CreatedUnit(const CreatedUnit &that) : m_type(that.m_type), m_amount(that.m_amount),
	m_amount_plus(that.m_amount_plus), m_amount_multiply(that.m_amount_multiply), m_cap(that.m_cap) {}

	void init(const XmlNode *n, const Faction *f);
	void init(const UnitType *ut, const int amount, const int amount_plus, const float amount_multiply, const int cap);

	virtual void setAmount(int v) { m_amount = v; }
	int  getAmount() const { return m_amount; }
	int  getAmountPlus() const { return m_amount_plus; }
	float  getAmountMultiply() const { return m_amount_multiply; }
	int  getCap() const { return m_cap; }
	const UnitType *getType() const { return m_type; }

	void save(XmlNode *node) const;
};

// =====================================================
// 	class UnitsOwned
//
/// Amount of a given UnitType
// =====================================================

class UnitsOwned {
private:
	const UnitType *m_type;
	int	m_owned;
	int m_limit;

public:
	UnitsOwned() : m_type(0), m_owned(0), m_limit(0) {}
	UnitsOwned(const UnitsOwned &that) : m_type(that.m_type), m_owned(that.m_owned), m_limit(that.m_limit) {}

	void init(const XmlNode *n, const Faction *f);
	void init(const UnitType *ut, const int owned, const int limit);

	const UnitType *getType() const { return m_type; }
	int  getOwned() const { return m_owned; }
	int  getLimit() const { return m_limit; }

	void incOwned() {++m_owned;}
	void decOwned() {--m_owned;}
	void setLimit(int i) {m_limit = i;}

	void save(XmlNode *node) const;
};

// ===============================
// 	class ProductionRoute
// ===============================
/**< class that deals with the route of transport carts */

class ProductionRoute {
private:
    int storeId;
    int producerId;
    Vec2i destination;

public:
    int getStoreId() const {return storeId;}
    int getProducerId() const {return producerId;}
    Vec2i getDestination() const {return destination;}

    void setStoreId(int v) {storeId = v;}
    void setProducerId(int v) {producerId = v;}
    void setDestination(Vec2i pos) {destination = pos;}
};

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

// =====================================================
// 	class CreatedItem
//
/// Amount of a given ItemType
// =====================================================

class CreatedItem {
protected:
	const ItemType       *m_type;
	int	                 m_amount;
	int	                 m_amount_plus;
	float	             m_amount_multiply;
	int                  m_cap;

public:
	CreatedItem() : m_type(0), m_amount(0), m_amount_plus(0), m_amount_multiply(0), m_cap(0) {}
	CreatedItem(const CreatedItem &that) : m_type(that.m_type), m_amount(that.m_amount),
	m_amount_plus(that.m_amount_plus), m_amount_multiply(that.m_amount_multiply), m_cap(that.m_cap) {}

	void init(const XmlNode *n, const Faction *f);
	void init(const ItemType *it, const int amount, const int amount_plus, const float amount_multiply, const int cap);

	virtual void setAmount(int v) { m_amount = v; }
	int  getAmount() const { return m_amount; }
	int  getAmountPlus() const { return m_amount_plus; }
	float  getAmountMultiply() const { return m_amount_multiply; }
	int  getCap() const { return m_cap; }
	const ItemType *getType() const { return m_type; }

	void save(XmlNode *node) const;
};

// =====================================================
// 	class Equipment
// =====================================================
class Equipment {
private:
    int max;
    int current;
    string name;
    string typeTag;

public:
    int getMax() const {return max;}
    void setMax(int i) {max = max + i;}
    int getCurrent() const {return current;}
    void setCurrent(int i) {current = current + i;}
    string getName() const {return name;}
    void setName(string newName) {name = newName;}
    string getTypeTag() const {return typeTag;}

    void init(int max, int current, string name, string typeTag);
};

// =====================================================
// 	class Attacker
// =====================================================
class Attacker {
private:
    Unit *attacker;
    int timer;
public:
    Unit *getUnit() const {return attacker;}
    int getTimer() const {return timer;}
    void incTimer() { ++timer; }
    void resetTimer() {timer = 0;}
    void init(Unit* unit, int value);
};

}}//end namespace

#endif
