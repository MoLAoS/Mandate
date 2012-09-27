// ====================================================================
//	This file is part of the Mandate Engine (www.lordofthedawn.com)
//
//	Copyright (C) 2012 Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//	It is released under the terms of the GNU General Public License 3
//
// ====================================================================

#ifndef _GLEST_GAME_HERO_H_
#define _GLEST_GAME_HERO_H_

#include "forward_decs.h"
#include "settlement.h"
#include "formations.h"
#include "cmd_types_hierarchy.h"
#include "abilities.h"

namespace Glest{ namespace ProtoTypes {
using namespace Hierarchy;

// ===============================
// 	class Mage
// ===============================

class MagicType {


};

class Mage {

};

// ===============================
// 	class Leader
// ===============================

class LeaderStats {

private:
    int awe;
    int fear;
    int leadership;
    int mindDefense;

public:
    SquadMoveCommandType squadMove;

public:
    int getAwe() {return awe;}
    int getFear() {return fear;}
    int getLeadership() {return leadership;}
    int getMindDefense() {return mindDefense;}

    void setAwe(int v) {awe =  v;}
    void setFear(int v) {fear =  v;}
    void setLeadership(int v) {leadership =  v;}
    void setMindDefense(int v) {mindDefense =  v;}
};

typedef vector<Formation> Formations;
typedef vector<FormationCommand> FormationCommands;

class Leader {
private:
	typedef vector<CommandType*> CommandTypes;
    LeaderStats leaderStats;
public:
    mutable Squad squad;
    mutable Settlement settlement;
    Formations formations;
    FormationCommands formationCommands;
    CommandTypes squadCommands;

public:
    bool load(const XmlNode *leaderNode, const string &dir, const TechTree *techtree,
              const FactionType *factionType, const UnitType* unitType, const string &path);

};

// ===============================
// 	class Hero
// ===============================

class Hero {

};

}}//end namespace

#endif
