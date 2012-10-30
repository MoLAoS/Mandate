// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _GLEST_GAME_RESOURCE_H_
#define _GLEST_GAME_RESOURCE_H_

#include <string>

using std::string;

#include "vec.h"
#include "xml_parser.h"
#include "sigslot.h"
#include "forward_decs.h"

namespace Glest { namespace Entities {

using Shared::Xml::XmlNode;
using Shared::Math::Vec2i;
using namespace Glest::ProtoTypes;

// =====================================================
// 	class ResourceAmount
//
/// Amount of a given ResourceType
// =====================================================

class ResourceAmount {
protected:
	const ResourceType  *m_type;
	int	                 m_amount;
	int	                 m_amount_plus;
	float	             m_amount_multiply;

public:
	ResourceAmount() : m_type(0), m_amount(0), m_amount_plus(0), m_amount_multiply(0) {}
	ResourceAmount(const ResourceAmount &that) : m_type(that.m_type), m_amount(that.m_amount),
	m_amount_plus(that.m_amount_plus), m_amount_multiply(that.m_amount_multiply) {}

	void init(const XmlNode *n, const TechTree *tt);
	void init(const ResourceType *rt, const int amount, const int amount_plus, const float amount_multiply);

	virtual void setAmount(int v) { m_amount = v; }
	int  getAmount() const { return m_amount; }
	int  getAmountPlus() const { return m_amount_plus; }
	float  getAmountMultiply() const { return m_amount_multiply; }
	const ResourceType *getType() const { return m_type; }

	void save(XmlNode *node) const;
};

// =====================================================
// 	class MapResource
// =====================================================

class MapResource : public ResourceAmount {
private:
	Vec2i	m_pos;

private:
	MapResource(const MapResource &r) { assert(false); }

public:
	MapResource() : ResourceAmount(), m_pos(-1) {}

	sigslot::signal<Vec2i>	Depleted;

	void init(const XmlNode *n, const TechTree *tt);
	void init(const ResourceType *rt, const Vec2i &pos);

	Vec2i getPos() const { return m_pos; }

	void save(XmlNode *n) const;

	bool decrement();
};

ostream& operator<<(ostream &stream, const MapResource &res);

// =====================================================
// 	class StoredResource
//
/// amount, balance & storage capacity of a given ResourceType
/// stored by a faction
// =====================================================

class StoredResource : public ResourceAmount {
private:
	int m_balance;
	int m_storage;

public:
	void init(const XmlNode *node, const TechTree *tt);
    void init(const ResourceType *rt, int amount);

	int getBalance() const { return m_balance; }
	void setBalance(int balance) { m_balance = balance; }

	int getStorage() const { return m_storage; }
	void setStorage(int storage) { m_storage = storage; }

	void save(XmlNode *node) const;
};

// =====================================================
// 	class CreatedResource
// =====================================================

class CreatedResource : public ResourceAmount {
private:
	int m_creation;

public:

    bool local;

    void setScope(bool scope) { local = scope; }

	void init(const XmlNode *node, const TechTree *tt);
    void init(const ResourceType *rt, int amount);

    int getCreation() const { return m_creation; }
	void setCreation(int creation) { m_creation = creation; }

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
    void init(const ResourceType *rt, string name, int product, int add, int mult);

    int getProduct() const { return m_product; }
	void setProduct(int product) { m_product = product; }

    string getName() const { return m_name; }
	void setName(string name) { m_name = name; }

	void save(XmlNode *node) const;
};

class ResCost : public ResourceAmount {
private:
	int m_cost;
	string m_name;

public:
	void init(const XmlNode *node, const TechTree *tt);
    void init(const ResourceType *rt, string name, int cost, int add, int mult);

    int getCost() const { return m_cost; }
	void setCost(int cost) { m_cost = cost; }

    string getName() const { return m_name; }
	void setName(string name) { m_name = name; }

	void save(XmlNode *node) const;
};

class ItemMade {
private:
	int m_item;
	string m_name;
	const ItemType *m_type;
	int	m_amount;
	int	m_amount_plus;
	float m_amount_multiply;

public:
	void init(const XmlNode *n, const TechTree *tt);
	void init(const ItemType *it, string name, int amount, int amount_plus, float amount_multiply);

	virtual void setAmount(int v) { m_amount = v; }
	int  getAmount() const { return m_amount; }
	int  getAmountPlus() const { return m_amount_plus; }
	float  getAmountMultiply() const { return m_amount_multiply; }
	const ItemType *getType() const { return m_type; }

    int getItem() const { return m_item; }
	void setItem(int item) { m_item = item; }

    string getName() const { return m_name; }
	void setName(string name) { m_name = name; }

	void save(XmlNode *node) const;
};

typedef vector<ItemMade> Items;
typedef	vector<ResMade> Products;
typedef vector<ResCost> Costs;

// =====================================================
// 	class Process
// =====================================================

class Process {
public:
    Costs costs;
    Products products;
    Items items;
    bool local;

    void setScope(bool scope) { local = scope; }
};

}}// end namespace

#endif
