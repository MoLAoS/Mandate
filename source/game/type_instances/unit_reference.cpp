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
#include "unit_reference.h"
#include "world.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class UnitReference
// =====================================================

UnitReference::UnitReference(const XmlNode *node) {
	assert(World::getCurrWorld());
	id = node->getAttribute("id")->getIntValue();
	int factionId = node->getAttribute("faction")->getIntValue();
	assert(factionId < World::getCurrWorld()->getFactionCount());
	faction = factionId == -1 ? NULL : World::getCurrWorld()->getFaction(factionId);
}

void UnitReference::operator=(const Unit * unit){
	if (unit) {
		id = unit->getId();
		faction = unit->getFaction();
	} else {
		id = -1;
		faction = NULL;
	}
}

Unit *UnitReference::getUnit() const {
	if(faction) {
		return faction->findUnit(id);
	}
	return NULL;
}

void UnitReference::save(XmlNode *node) const {
	node->addAttribute("id", id);
	node->addAttribute("faction", faction ? faction->getIndex() : -1);
}

}}//end namespace
