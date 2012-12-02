// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_MANDATE_AI_PERSONALITIES_H_
#define _GLEST_MANDATE_AI_PERSONALITIES_H_

#include "forward_decs.h"
#include "xml_parser.h"
#include "mandate_ai_search_sim.h"

using namespace Glest::ProtoTypes;
using namespace Glest::Entities;
using namespace Shared::Xml;

namespace Glest { namespace Plan {
// ===============================
// 	class Focus
// ===============================
class Focus {
private:
    int importance;
    Goal name;
public:
    int getImportance() const {return importance;}
    Goal getName() const {return name;}

    void init(Goal newName, int newImportance);
};

// ===============================
// 	class Personality
// ===============================

typedef vector<Focus> Goals;

class Personality {
private:
    string personalityName;
    Goals goals;
public:
    string getPersonalityName() const {return personalityName;}
    Goals getGoals() const {return goals;}
    Focus getGoal(int i) const {return goals[i];}
    void load(const XmlNode *node, const TechTree* techTree, const FactionType* factionType);
};

// ===============================
// 	class Goal System
// ===============================
typedef vector<string> GoalList;

class GoalSystem {
private:
    GoalList goalList;
    ExploredMap exploredMap;

public:
    void init();
    void computeAction(Unit *unit, Focus focus);
    void ownerLoad(Unit*unit);
    void ownerUnload(Unit *unit);
    void shop(Unit *unit);
    Unit *findShop(Unit *unit);
    Unit *findNearbyAlly(Unit *unit, Focus focus);
    Unit *findBuilding(Unit *unit);
    Unit *findProducer(Unit *unit);
    const CommandType *selectHealSpell(Unit *unit, Unit* target);
    const CommandType *selectBuffSpell(Unit *unit, Unit* target);
    const CommandType *selectAttackSpell(Unit *unit, Unit* target);
    Unit *findCity(Unit *unit);
    Unit *findLair(Unit *unit);
    Unit *findCreature(Unit *unit);
    UnitDirection newDirection(UnitDirection oldDirection);
    void clearSimAi(Unit *unit, Goal goal);
};

}}

#endif
