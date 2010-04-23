// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "damage_multiplier.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class DamageMultiplierTable
// =====================================================

DamageMultiplierTable::DamageMultiplierTable() 
		: values(0) {
}

DamageMultiplierTable::~DamageMultiplierTable() {
	delete [] values;
}

void DamageMultiplierTable::init(int attackTypeCount, int armorTypeCount){
	this->attackTypeCount= attackTypeCount;
	this->armorTypeCount= armorTypeCount;

	int valueCount= attackTypeCount*armorTypeCount;
	values= new fixed[valueCount];
	for(int i=0; i<valueCount; ++i){
		values[i] = 1;
	}
}

fixed DamageMultiplierTable::getDamageMultiplier(const AttackType *att, const ArmourType *art) const {
	return values[attackTypeCount*art->getId()+att->getId()];
}

void DamageMultiplierTable::setDamageMultiplier(const AttackType *att, const ArmourType *art, fixed value) {
	values[attackTypeCount*art->getId()+att->getId()]= value;
}

}}//end namespaces
