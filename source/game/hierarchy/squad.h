// ====================================================================
//	This file is part of the Mandate Engine (www.lordofthedawn.com)
//
//	Copyright (C) 2012 Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//	It is released under the terms of the GNU General Public License 3
//
// ====================================================================

#ifndef _GLEST_GAME_SQUAD_H_
#define _GLEST_GAME_SQUAD_H_

#include "forward_decs.h"


namespace Glest { namespace Hierarchy {
using namespace Entities;

// ===============================
// 	class Squad
// ===============================

class Squad {
private:
    typedef vector<Unit*> Subordinates;
    typedef vector<Vec2i> LeaderOffsets;

public:
    mutable LeaderOffsets leaderOffsets;
    mutable Subordinates subordinates;
    const Unit *leader;

public:
    void create(const Unit *leader);

};

}}//end namespace

#endif
