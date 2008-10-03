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

#include "unit_reference.h"
#include "world.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class UnitReference
// =====================================================

UnitReference::UnitReference(){
	id= -1;
	faction= NULL;
}

UnitReference::UnitReference(const XmlNode *node, World *world) {
	id = node->getAttribute("id")->getIntValue();
	int factionId = node->getAttribute("faction")->getIntValue();
	faction = factionId == -1 ? NULL : world->getFaction(factionId);
}

void UnitReference::operator=(const Unit *unit){
	if(unit==NULL){
		id= -1;
		faction= NULL;
	}
	else{
		id= unit->getId();
		faction= unit->getFaction();
	}
}

Unit *UnitReference::getUnit() const{
	if(faction!=NULL){
		return faction->findUnit(id);
	}
	return NULL;
}

void UnitReference::save(XmlNode *node) const {
	node->addAttribute("id", id);
	node->addAttribute("faction", faction ? faction->getIndex() : -1);
}

}}//end namespace
