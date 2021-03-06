// ====================================================================
//	This file is part of the Mandate Engine (www.lordofthedawn.com)
//
//	Copyright (C) 2012 Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//	It is released under the terms of the GNU General Public License 3
//
// ====================================================================

#ifndef _GLEST_GAME_COHORT_H_
#define _GLEST_GAME_COHORT_H_

#include "forward_decs.h"


namespace Glest { namespace Hierarchy {
using namespace Entities;

// ===============================
// 	class Cohort
// ===============================

class Cohort {
private:
    typedef vector<Unit*> Centurions;
    Centurions centurions;
    Unit *commander;

public:

};

}}//end namespace

#endif
