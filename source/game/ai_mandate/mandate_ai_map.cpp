// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "mandate_ai.h"

namespace Glest { namespace Plan {

// ==============================
// Class Section
// ==============================

void Section::addCell(Cell *newCell) {
    cellList.push_back(newCell);
}

void Section::addResource(Cell *newResource) {
    resourceList.push_back(newResource);
}

void Section::addBuilding(Cell *newBuilding) {
    buildingList.push_back(newBuilding);
}

void Section::addLair(Cell *newLair) {
    lairList.push_back(newLair);
}

void Section::addSettlement(Settlement *newSettlement) {
    settlementList.push_back(newSettlement);
}

}}
