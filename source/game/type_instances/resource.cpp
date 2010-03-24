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

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Resource
// =====================================================

void Resource::init(const XmlNode *node, const TechTree *tt) {
	type = tt->getResourceType(node->getChildStringValue("type"));
	amount = node->getChildIntValue("amount");
	pos = node->getChildVec2iValue("pos");
	balance = node->getChildIntValue("balance");
}

void Resource::save(XmlNode *node) const {
	node->addChild("type", type->getName());
	node->addChild("amount", amount);
	node->addChild("pos", pos);
	node->addChild("balance", balance);
}

void Resource::init(const ResourceType *rt, int amount){
    this->type= rt;
    this->amount= amount;
	pos= Vec2i(0);
	balance= 0;
}

void Resource::init(const ResourceType *rt, const Vec2i &pos){
	this->type=rt;
	amount=rt->getDefResPerPatch();
	this->pos= pos;
}

string Resource::getDescription() const{
     string str;

     str+= type->getName();
     str+="\n";
     str+= intToStr(amount);
     str+="/";
     str+= intToStr(type->getDefResPerPatch());

     return str;
}

bool Resource::decAmount(int i){
    if (!i) true;
	amount -= i;
	AmountChanged(amount);
	if (amount > 0)	return false;
	Depleted(pos);
	return true;
}


}}//end namespace
