// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_MANDATE_AI_SIM_H_
#define _GLEST_MANDATE_AI_SIM_H_

#include "forward_decs.h"
#include "mandate_ai_personalities.h"

using namespace Glest::Sim;
using namespace Glest::Entities;

namespace Glest { namespace Plan {

// ===============================
// 	class MandateAISim
// ===============================

typedef vector<Personality> Personalities;

class MandateAISim {
private:
    World *world;
    Faction *faction;
    Personalities personalities;
    GoalSystem goalSystem;

public:
    Personalities getPersonalities() const {return personalities;}
    Personality getPersonality(int i) const {return personalities[i];}

    void init(World *world, Faction *faction);
    Focus getTopGoal(Unit *unit, string personality);
    void update();
    void computeAction(Unit *unit, string personality, SkillClass currentAction);

};

}}

#endif
