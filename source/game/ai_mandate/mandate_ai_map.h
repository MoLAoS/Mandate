// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_MANDATE_AI_MAP_H_
#define _GLEST_MANDATE_AI_MAP_H_


#include "map.h"
#include "settlement.h"

namespace Glest { namespace Plan {

typedef vector<Cell*> CellList;
typedef vector<Cell*> ResList;
typedef vector<Cell*> UnitList;
typedef vector<Cell*> BuildingList;
typedef vector<Cell*> LairList;
typedef vector<Settlement*> SettlementList;

class Section {
public:
    CellList cellList;
    ResList resourceList;
    UnitList unitList;
    BuildingList buildingList;
    LairList lairList;
    SettlementList settlementList;

    void addCell(Cell *cell);
    void addResource(Cell *cell);
    void addBuilding(Cell *cell);
    void addLair(Cell *cell);
    void addSettlement(Settlement *settlement);
};

typedef vector<Section*> Sections;

class Area {
public:
    Sections sections;

    void addSection(Section *section);

};

typedef vector<Area> Areas;

class AIMapAreas {
public:
    Areas areas;

    void addArea(Area *area);
};

}}

#endif
