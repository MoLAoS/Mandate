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
}

void ResourceAmount::save(XmlNode *node) const {
	node->addChild("type", m_type->getName());
	node->addChild("amount", m_amount);
}

void ResourceAmount::init(const ResourceType *rt, int amount) {
    m_type = rt;
    m_amount = amount;
}

bool ResourceAmount::decAmount(int i) {
	if (!i) { // just test
		return !m_amount;
	}
	RUNTIME_CHECK(m_amount >= i);
	m_amount -= i;
	if (m_amount) {
		return false;
	}
	return true;
}

// =====================================================
// 	class MapResource
// =====================================================

void MapResource::init(const XmlNode *n, const TechTree *tt) {
	ResourceAmount::init(n, tt);
	m_pos = n->getChildVec2iValue("pos");
}

void MapResource::init(const ResourceType *rt, const Vec2i &pos) {
	ResourceAmount::init(rt, rt->getDefResPerPatch());
	m_pos = pos;
}

void MapResource::save(XmlNode *n) const {
	ResourceAmount::save(n);
	n->addChild("pos", m_pos);
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
	ResourceAmount::init(rt, v);
	m_balance = 0;
	m_storage = 0;
}

void StoredResource::save(XmlNode *n) const {
	ResourceAmount::save(n);
	n->addChild("balance", m_balance);
	n->addChild("storage", m_storage);
}


}} // end namespace
