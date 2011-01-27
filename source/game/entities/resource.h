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

using Shared::Xml::XmlNode;
using Shared::Math::Vec2i;
using namespace Glest::ProtoTypes;

namespace Glest { namespace Entities {

// =====================================================
// 	class ResourceAmount
//
/// Amount of a given ResourceType
// =====================================================

class ResourceAmount {
protected:
	const ResourceType *m_type;
	int		m_amount;

public:
	ResourceAmount() : m_type(0), m_amount(0) {}
	ResourceAmount(const ResourceAmount &that) : m_type(that.m_type), m_amount(that.m_amount) {}

	void init(const XmlNode *n, const TechTree *tt);
	void init(const ResourceType *rt, const int amount);

	void setAmount(int v) { m_amount = v; }
	int  getAmount() const { return m_amount; }
	const ResourceType *getType() const { return m_type; }

	virtual bool decAmount(int v);
	void save(XmlNode *node) const;
};

// =====================================================
// 	class MapResource
// =====================================================

class MapResource : public ResourceAmount {
	Vec2i	m_pos;

public:
	sigslot::signal<Vec2i>	Depleted;

	void init(const XmlNode *n, const TechTree *tt);
	void init(const ResourceType *rt, const Vec2i &pos);

	Vec2i getPos() const { return m_pos; }

	void save(XmlNode *n) const;

	bool decAmount(int v) override {
		if (ResourceAmount::decAmount(v)) {
			Depleted(m_pos);
			return true;
		}
		return false;
	}
};

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

}}// end namespace

#endif
