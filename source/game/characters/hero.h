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
#include "specialization.h"
#include "formations.h"
#include "cmd_types_hierarchy.h"
#include "abilities.h"

namespace Glest{ namespace ProtoTypes {
using namespace Hierarchy;
// ===============================
// 	class Sovereign
// ===============================
typedef vector<string> AddActions;
class Sovereign {
private:
    Specialization *specialization;
    CharacterStats characterStats;
    CraftingStats craftingStats;
    EffectTypes effectTypes;
    Statistics statistics;
    Equipments equipment;
    Knowledge knowledge;
    AddActions addSkills;
    AddActions addCommands;
    Traits traits;
    string sovName;
public:
    Specialization *getSpecialization() {return specialization;}
    const Specialization *getSpecialization() const {return specialization;}
    Trait *getTrait(int i) {return traits[i];}
    const Trait *getTrait(int i) const {return traits[i];}
    int getTraitCount() const {return traits.size();}
    const Knowledge *getKnowledge() {return &knowledge;}
    const Knowledge *getKnowledge() const {return &knowledge;}
    CraftingStats *getCraftingStats() {return &craftingStats;}
    CharacterStats *getCharacterStats() {return &characterStats;}
    int getAddSkillCount() const {return addSkills.size();}
    int getAddCommandCount() const {return addCommands.size();}
    string getAddSkill(int i) const {return addSkills[i];}
    string getAddCommand(int i) const {return addCommands[i];}
    Statistics *getStatistics() {return &statistics;}
    const Statistics *getStatistics() const {return &statistics;}
    int getEquipmentCount() const {return equipment.size();}
    Equipments *getEquipments() {return &equipment;}
    Equipment *getEquipment(int i) {return &equipment[i];}
    string getSovName() const {return sovName;}
	void preLoad(const string &dir);
	bool load(const string &dir, const FactionType *factionType);
	void save(XmlNode *node) const;

	void addName(string newName) {sovName = newName;}
	void addTrait(Trait *trait) {traits.push_back(trait);}
	void addSpecialization(Specialization *spec) {specialization = spec;}
	void addStatistics(const Statistics *statistic) {statistics.sum(statistic);}
	void addCharacterStats(const CharacterStats *charStats) {characterStats.sum(charStats);}
	void addKnowledge(const Knowledge *knowledges) {knowledge.sum(knowledges);}
	void addAction(const Actions *actions, string actionName);
	//void addCraftingStats()
	//void addEquipment();
	//void addEffect();
	//void AddAction();
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
    bool load(const XmlNode *leaderNode, const string &dir, const UnitType* unitType, const string &path);

};

// ===============================
// 	class Hero
// ===============================
class Hero {

};

}}//end namespace

#endif
