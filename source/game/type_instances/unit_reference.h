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
#include "socket.h"

namespace Glest{ namespace Game{

using Shared::Xml::XmlNode;
using Shared::Platform::int32;
using Shared::Platform::int8;

class Faction;
class World;
class Unit;

// =====================================================
// 	class UnitReference
// =====================================================

//REFACTOR redunant info, id alone is sufficient, look up in one map
class UnitReference {
private:
	int id;
	Faction *faction;

public:
	UnitReference(): id(-1), faction(NULL) {}
	UnitReference(const XmlNode *node);
	UnitReference(const Unit* unit)			{*this = unit;}
//	UnitReference(int id, Faction *faction): id(id), faction(faction) {}

	void operator=(const Unit *unit);
	operator Unit *() const					{return getUnit();}
	int getUnitId() const					{return id;}
	Faction *getFaction() const				{return faction;}
	Unit *getUnit() const;
	void save(XmlNode *node) const;

	size_t getNetSize() const				{return sizeof(int32) + sizeof(int8);}
};


}}//end namespace

#endif
