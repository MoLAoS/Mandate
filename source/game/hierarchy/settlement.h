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

#ifndef _GLEST_GAME_SETTLEMENT_H_
#define _GLEST_GAME_SETTLEMENT_H_

#include "forward_decs.h"

namespace Glest { namespace Hierarchy {

using namespace Entities;

// ===============================
// 	class Settlement
// ===============================

typedef vector<Unit*> Peripherals;
typedef vector<Unit*> Structures;

class Settlement {
private:
    Unit *center;
    int radius;

public:
    Peripherals peripherals;
    Structures structures;

public:
    Unit *getCenter() {return center;}
    Peripherals getStructures() {return peripherals;}
    int getRadius() {return radius;}
    Structures getSettlement() {return structures;}

    void setCenter(Unit *unit);

};

}}//end namespace

#endif
