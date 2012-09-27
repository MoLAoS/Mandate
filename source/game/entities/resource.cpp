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

#include "pch.h"
#include "resource.h"

#include "conversion.h"
#include "resource_type.h"
#include "tech_tree.h"
#include "logger.h"

#include "leak_dumper.h"

namespace Glest { namespace Entities {

using namespace Shared::Graphics;
using namespace Shared::Util;
using Glest::Util::Logger;

// =====================================================
// 	class ResourceAmount
// =====================================================

void ResourceAmount::init(const XmlNode *node, const TechTree *tt) {
	m_type = tt->getResourceType(node->getChildStringValue("type"));
	m_amount = node->getChildIntValue("amount");
	m_amount_plus = node->getChildIntValue("plus");
	m_amount_multiply = node->getChildFloatValue("multiply");
}

void ResourceAmount::save(XmlNode *node) const {
	node->addChild("type", m_type->getName());
	node->addChild("amount", m_amount);
	node->addChild("plus", m_amount_plus);
	node->addChild("multiply", m_amount_multiply);
}

void ResourceAmount::init(const ResourceType *rt, int amount, int amount_plus, float amount_multiply) {
    m_type = rt;
    m_amount = amount;
    m_amount_plus = amount_plus;
    m_amount_multiply = amount_multiply;
}

// =====================================================
// 	class MapResource
// =====================================================

void MapResource::init(const XmlNode *n, const TechTree *tt) {
	ResourceAmount::init(n, tt);
	assert(m_amount > 0);
	m_pos = n->getChildVec2iValue("pos");
}

void MapResource::init(const ResourceType *rt, const Vec2i &pos) {
	ResourceAmount::init(rt, rt->getDefResPerPatch(), 0, 0);
	assert(m_amount > 0);
	m_pos = pos;
}

void MapResource::save(XmlNode *n) const {
	ResourceAmount::save(n);
	n->addChild("pos", m_pos);
}

bool MapResource::decrement() {
	assert(m_amount > 0);
	--m_amount;
	if (m_amount) {
		return false;
	}
	Depleted(m_pos);
	return true;
}

ostream& operator<<(ostream &stream, const MapResource &res) {
	return stream << "Type: " << res.getType()->getName() << " Pos: " << res.getPos()
		<< " Amount: " << res.getAmount();
}

// =====================================================
// 	class StoredResource
// =====================================================

void StoredResource::init(const XmlNode *n, const TechTree *tt) {
	ResourceAmount::init(n, tt);
	m_balance = n->getChildIntValue("balance");
	m_storage = n->getChildIntValue("storage");
}

void StoredResource::init(const ResourceType *rt, int v) {
	ResourceAmount::init(rt, v, 0, 0);
	m_balance = 0;
	m_storage = 0;
}

void StoredResource::save(XmlNode *n) const {
	ResourceAmount::save(n);
	n->addChild("balance", m_balance);
	n->addChild("storage", m_storage);
}

// =====================================================
// 	class CreatedResource
// =====================================================

void CreatedResource::init(const XmlNode *n, const TechTree *tt) {
	ResourceAmount::init(n, tt);
	m_creation = n->getChildIntValue("creation");
}

void CreatedResource::init(const ResourceType *rt, int v) {
	ResourceAmount::init(rt, v, 0, 0);
	m_creation = 0;
}

void CreatedResource::save(XmlNode *n) const {
	ResourceAmount::save(n);
	n->addChild("creation", m_creation);
}

// =====================================================
// 	Helper Classes For Process
// =====================================================

void ResMade::init(const XmlNode *n, const TechTree *tt) {
	ResourceAmount::init(n, tt);
	m_product = n->getChildIntValue("product");
}

void ResMade::init(const ResourceType *rt, string n, int v, int a, int b) {
	ResourceAmount::init(rt, v, 0, 0);
    m_product = 0;
    m_name = n;
}

void ResMade::save(XmlNode *n) const {
	ResourceAmount::save(n);
	n->addChild("product", m_product);
}

void ResCost::init(const XmlNode *n, const TechTree *tt) {
	ResourceAmount::init(n, tt);
	m_cost = n->getChildIntValue("cost");
}

void ResCost::init(const ResourceType *rt, string n, int v, int a, int b) {
	ResourceAmount::init(rt, v, 0, 0);
	m_cost = 0;
	m_name = n;
}

void ResCost::save(XmlNode *n) const {
	ResourceAmount::save(n);
	n->addChild("cost", m_cost);
}

// =====================================================
// 	class Process
// =====================================================

}} // end namespace
