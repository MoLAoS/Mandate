// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_PRODUCIBLES_H_
#define _GLEST_GAME_PRODUCIBLES_H_

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <time.h>

#include "forward_decs.h"
#include "xml_parser.h"
#include "resource.h"

using std::map;
using std::vector;
using namespace Shared::Xml;
using namespace Glest::Entities;

namespace Glest { namespace ProtoTypes {
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
	fixed	             m_amount_multiply;
	int                  m_cap;

public:
	CreatedUnit() : m_type(0), m_amount(0), m_amount_plus(0), m_amount_multiply(0), m_cap(0) {}
	CreatedUnit(const CreatedUnit &that) : m_type(that.m_type), m_amount(that.m_amount),
	m_amount_plus(that.m_amount_plus), m_amount_multiply(that.m_amount_multiply), m_cap(that.m_cap) {}

	void init(const XmlNode *n, const Faction *f);
	void init(const UnitType *ut, const int amount, const int amount_plus, const fixed amount_multiply, const int cap);

	virtual void setAmount(int v) { m_amount = v; }
	int  getAmount() const { return m_amount; }
	int  getAmountPlus() const { return m_amount_plus; }
	fixed  getAmountMultiply() const { return m_amount_multiply; }
	int  getCap() const { return m_cap; }
	const UnitType *getType() const { return m_type; }

	void save(XmlNode *node) const;
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
	fixed	             m_amount_multiply;
	int                  m_cap;

public:
	CreatedItem() : m_type(0), m_amount(0), m_amount_plus(0), m_amount_multiply(0), m_cap(0) {}
	CreatedItem(const CreatedItem &that) : m_type(that.m_type), m_amount(that.m_amount),
	m_amount_plus(that.m_amount_plus), m_amount_multiply(that.m_amount_multiply), m_cap(that.m_cap) {}

	void init(const XmlNode *n, const Faction *f);
	void init(const ItemType *it, const int amount, const int amount_plus, const fixed amount_multiply, const int cap);

	virtual void setAmount(int v) { m_amount = v; }
	int  getAmount() const { return m_amount; }
	int  getAmountPlus() const { return m_amount_plus; }
	fixed  getAmountMultiply() const { return m_amount_multiply; }
	int  getCap() const { return m_cap; }
	const ItemType *getType() const { return m_type; }

	void save(XmlNode *node) const;
};

// =====================================================
// 	Helper Classes For Process
// =====================================================

class ResMade : public ResourceAmount {
private:
	int m_product;
	string m_name;

public:
	void init(const XmlNode *node, const TechTree *tt);
    void init(const ResourceType *rt, string name, int product, int add, fixed mult);

    int getProduct() const { return m_product; }
	void setProduct(int product) { m_product = product; }

    string getName() const { return m_name; }
	void setName(string name) { m_name = name; }

	void save(XmlNode *node) const;
};

class ResCost : public ResourceAmount {
private:
	bool m_consume;

public:
	void init(const XmlNode *node, const TechTree *tt);
    void init(const ResourceType *rt, bool consume, int cost, int add, fixed mult);

    bool getConsume() const { return m_consume; }
	void setConsume(bool consume) { m_consume = consume; }

	void save(XmlNode *node) const;
};

class ItemMade : public CreatedItem {
private:

public:
	void init(const XmlNode *n, const TechTree *tt);
	void init(const ItemType *it, int amount, int amount_plus, fixed amount_multiply);

	void save(XmlNode *node) const;
};

typedef vector<ItemMade> Items;
typedef	vector<ResMade> Products;
typedef vector<ResCost> Costs;

// =====================================================
// 	class ResBonus
// =====================================================
class ResBonus {
public:
    ResCost cost;
    ResMade product;

    int count;
    void init();
};

typedef vector<ResBonus> Bonuses;

// =====================================================
// 	class Process
// =====================================================
class Process {
private:
    bool local;
public:
    Costs costs;
    Bonuses bonuses;
    Products products;
    Items items;
    int count;

    bool getScope() const {return local;}

    void setScope(bool scope) { local = scope; }
    void setCount(int value) { count = value; }
};

// ===============================
// 	class ResourceStore
// ===============================
class ResourceStore : public ResourceAmount {
private:
    string m_status;
public:
    string getStatus() const {return m_status;}
    void init(string status, const ResourceType *rt, int required);
};

// =====================================================
// 	class ResourceProductionSystem
// =====================================================
class ResourceProductionSystem {
private:
	typedef vector<ResourceAmount>  StoredResources;
	typedef vector<ResourceAmount>  StarterResources;
	typedef vector<CreatedResource> CreatedResources;
	typedef vector<Timer>           CreatedResourceTimers;
	StoredResources storedResources;
    StarterResources starterResources;
	CreatedResources createdResources;
	mutable CreatedResourceTimers createdResourceTimers;
public:
	StoredResources getStoredResources() const {return storedResources;};
    StarterResources getStarterResources() const {return starterResources;};
	CreatedResources getCreatedResources() const {return createdResources;};
	// resources stored
	int getStoredResourceCount() const					{return storedResources.size();}
	ResourceAmount getStoredResource(int i, const Faction *f) const;
	int getStore(const ResourceType *rt, const Faction *f) const;
    // resources created
	int getCreatedResourceCount() const					{return createdResources.size();}
	ResourceAmount getCreatedResource(int i, const Faction *f) const;
	int getCreate(const ResourceType *rt, const Faction *f) const;
	// resources created timers
	int getCreatedResourceTimerCount()                 {return createdResourceTimers.size();}
	Timer getCreatedResourceTimer(int i, const Faction *f) const;
	// load resource system
	bool load(const XmlNode *resourceProductionNode, const string &dir, const TechTree *techTree, const FactionType *factionType);

    void update(CreatedResource cr, Unit *unit, int timer, TimerStep *timerStep) const;
};
// =====================================================
// 	class ItemProductionSystem
// =====================================================
class ItemProductionSystem {
private:
	typedef vector<CreatedItem> CreatedItems;
	typedef vector<Timer>       CreatedItemTimers;
	CreatedItems createdItems;
	mutable CreatedItemTimers createdItemTimers;
public:
    // items created
    CreatedItems getCreatedItems() const {return createdItems;}
	int getCreatedItemCount() const					{return createdItems.size();}
	CreatedItem getCreatedItem(int i, const Faction *f) const;
	int getCreateItem(const ItemType *it, const Faction *f) const;
	// items created timers
	int getItemTimerCount()                 {return createdItemTimers.size();}
	Timer getCreatedItemTimer(int i, const Faction *f) const;
	// load resource system
	bool load(const XmlNode *itemProductionNode, const string &dir, const TechTree *techTree, const FactionType *factionType);

    void update(CreatedItem ci, Unit *unit, int timer, TimerStep *timerStep) const;
};
// =====================================================
// 	class ProcessProductionSystem
// =====================================================
class ProcessProductionSystem {
private:
	typedef vector<Process> Processes;
	typedef vector<Timer>   ProcessTimers;
	Processes processes;
	ProcessTimers processTimers;
public:
    // processes
	int getProcessCount() const					{return processes.size();}
	Processes getProcesses() const {return processes;}
	Process getProcess(int i, const Faction *f) const;
	int getProcessing(const ResourceType *rt, const Faction *f) const;
	// process timers
	int getProcessTimerCount()                 {return processTimers.size();}
	Timer getProcessTimer(int i, const Faction *f) const;
	// load resource system
	bool load(const XmlNode *processProductionNode, const string &dir, const TechTree *techTree, const FactionType *factionType);

    void update(Process proc, Unit *unit, int timer, TimerStep *timerStep) const;
};
// =====================================================
// 	class UnitProductionSystem
// =====================================================
class UnitProductionSystem {
private:
	typedef vector<CreatedUnit> CreatedUnits;
	typedef vector<Timer>       CreatedUnitTimers;
	CreatedUnits createdUnits;
	mutable CreatedUnitTimers createdUnitTimers;
public:
    // units created
    CreatedUnits getCreatedUnits() const {return createdUnits;}
	int getCreatedUnitCount() const					{return createdUnits.size();}
	CreatedUnit getCreatedUnit(int i, const Faction *f) const;
	int getCreateUnit(const UnitType *ut, const Faction *f) const;
	// units created timers
	int getUnitTimerCount() const {return createdUnitTimers.size();}
	Timer getCreatedUnitTimer(int i, const Faction *f) const;
	// load resource system
	bool load(const XmlNode *unitProductionNode, const string &dir, const TechTree *techTree, const FactionType *factionType);

    void update(CreatedUnit *ci, Unit *unit, Timer *timer, TimerStep *timerStep) const;
};

}}// end namespace

#endif
