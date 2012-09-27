// ====================================================================
//	This file is part of the Mandate Engine (www.lordofthedawn.com)
//
//	Copyright (C) 2012 Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//	It is released under the terms of the GNU General Public License 3
//
// ====================================================================

#include "pch.h"
#include "squad.h"

namespace Glest { namespace Hierarchy {
using namespace Entities;
// =====================================================
// 	class Squad
// =====================================================

void Squad::create(const Unit *newLeader) {
    leader = newLeader;
    //subordinates.resize(100);
    //leaderOffsets.resize(100);
}

}}//end namespace
