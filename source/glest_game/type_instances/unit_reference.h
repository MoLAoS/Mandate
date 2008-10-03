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

#ifndef _GLEST_GAME_UNIT_REFERENCE_H_
#define _GLEST_GAME_UNIT_REFERENCE_H_

#include "xml_parser.h"

namespace Glest{ namespace Game{

using Shared::Xml::XmlNode;

class Faction;
class World;
class Unit;

// =====================================================
// 	class UnitReference
// =====================================================

class UnitReference {
private:
	int id;
	Faction *faction;

public:
	UnitReference();
	UnitReference(const XmlNode *node, World *world);
	UnitReference(const Unit* unit)			{*this = unit;}

	void operator=(const Unit *unit);
	operator Unit *() const					{return getUnit();}
	Unit *getUnit() const;
	void save(XmlNode *node) const;
};


}}//end namespace

#endif
