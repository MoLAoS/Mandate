// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_ABILITIES_H_
#define _GLEST_GAME_ABILITIES_H_

#include "statistics.h"

namespace Glest{ namespace ProtoTypes {
// ===============================
// 	class Timer
// ===============================
class Timer {
public:
    int timerValue;
    int currentStep;
    int initialTime;
    bool active;

    Timer() : timerValue(0), currentStep(0), initialTime(0), active(true) {}
	Timer(const Timer &that) : timerValue(that.timerValue), currentStep(that.currentStep), initialTime(that.initialTime), active(that.active) {}

	void init(const XmlNode *n, const TechTree *tt);
	void init(int timerValue, int currentStep, int initial, bool check);

    int getTimerValue() {return timerValue;}
    int getCurrentStep() {return currentStep;}
    int getInitialTime() {return initialTime;}
    bool getActive() {return active;}

    virtual void setTimerValue(int v) { timerValue = v; }
    virtual void setCurrentStep(int v) { currentStep = v; }
    virtual void setInitialTime(int v) { initialTime = v; }
    virtual void setActive(bool check) { active = check; }

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

// ===============================
// 	class ProductionSystemTimers
// ===============================
class ProductionSystemTimers {
public:
    CurrentStep currentSteps; /**< current timer step for resource creation */
    CurrentStep currentUnitSteps; /**< current timer step for unit creation */
    CurrentStep currentItemSteps; /**< current timer step for item creation */
    CurrentStep currentProcessSteps; /**< current timer step for resource processes */
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
// 	class ItemStore
// ===============================
class ItemStore {
private:
    const ItemType *m_type;
    int m_cap;
public:
    const ItemType *getType() const {return m_type;}
    int getCap() const {return m_cap;}
    void init(const ItemType *type, int cap);
};

// =====================================================
// 	class Equipment
// =====================================================
class Equipment {
private:
    int max;
    int current;
    string typeTag;
    Item *item;
    const ItemType *itemType;

public:
    int getMax() const {return max;}
    void setMax(int i) {max = max + i;}
    int getCurrent() const {return current;}
    void setCurrent(int i) {current = current + i;}
    string getTypeTag() const {return typeTag;}
    Item *getItem() const {return item;}
    const ItemType *getType() const {return itemType;}
    void setItem(Item *newItem) {item = newItem;}
    void setItemType(const ItemType *newItemType) {itemType = newItemType;}
    void getDesc(string &str, const char *pre, string name);
    void save(XmlNode *node) const;

    void init(int max, int current, string typeTag, const ItemType *itemType, Item *item = NULL);
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
