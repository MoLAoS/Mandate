// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_MANDATE_AI_SEARCH_SIM_H_
#define _GLEST_MANDATE_AI_SEARCH_SIM_H_

#include "forward_decs.h"
#include "entities_enums.h"

using namespace Glest::Search;
using namespace Glest::Entities;

namespace Glest { namespace Plan {
// ===============================
// 	class ExploredMap
// ===============================
class ExploredMap {
public:
    Vec2i findNearestUnexploredTile(Vec2i unitPos, Faction *faction, UnitDirection direction);
};

}}

#endif
