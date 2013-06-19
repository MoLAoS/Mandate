// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_ACTIONS_H_
#define _GLEST_GAME_ACTIONS_H_

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <time.h>

#include "forward_decs.h"
#include "simulation_enums.h"

using namespace Glest::Sim;
using Shared::Xml::XmlNode;

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class Actions
// =====================================================
typedef vector<SkillType*>      SkillTypes;
typedef vector<CommandType*>    CommandTypes;

class Actions {
private:
	SkillTypes skillTypes;
	SkillTypes skillTypesByClass[SkillClass::COUNT];
	CommandTypes commandTypes;
	CommandTypes commandTypesByClass[CmdClass::COUNT];
	CommandTypes squadCommands;
public:
	int getSkillTypeCount() const						{return skillTypes.size();}
	SkillTypes getSkillTypeCountOfClass(SkillClass skillClass) const {return skillTypesByClass[skillClass];}
	SkillType *getSkillType(int i) const			{return skillTypes[i];}
	SkillType *getSkillType(int i)      			{return skillTypes[i];}
	SkillType *getSkillType(const string &skillName, SkillClass skillClass = SkillClass::COUNT) const;
	SkillType *getFirstStOfClass(SkillClass sc) const {
		return skillTypesByClass[sc].empty() ? 0 : skillTypesByClass[sc].front();
	}
	bool hasSkillType(const SkillType *skillType) const;
	bool hasSkillClass(SkillClass skillClass) const;
    void setDeCloakSkills(const vector<string> &names, const vector<SkillClass> &classes);
    void addSkillType(SkillType *skillType);
    void sortSkillTypes();
	int getCommandTypeCount() const				{return commandTypes.size();}
	CommandType *getCommandType(int i) const	{return commandTypes[i];}
	CommandType *getCommandType(int i)          {return commandTypes[i];}
	template <typename ConcreteType>
	int getCommandTypeCount() const {
		return commandTypesByClass[ConcreteType::typeClass()].size();
	}
    template <typename ConcreteType>
	const ConcreteType* getCommandType(int i) const {
		return static_cast<const ConcreteType*>(commandTypesByClass[ConcreteType::typeClass()][i]);
	}
	const CommandTypes& getCommandTypes(CmdClass cc) const {
		return commandTypesByClass[cc];
	}
	CommandType *getFirstCtOfClass(CmdClass cc) const {
		return commandTypesByClass[cc].empty() ? 0 : commandTypesByClass[cc].front();
	}
	CommandType *getCommandType(const string &name) const;
	bool hasCommandType(const CommandType *ct) const;
	bool hasCommandClass(CmdClass cc) const		{return !commandTypesByClass[cc].empty();}
    void sortCommandTypes();
    const HarvestCommandType *getHarvestCommand(const ResourceType *resourceType) const;
	const AttackCommandType *getAttackCommand(Zone zone) const;
	const RepairCommandType *getRepairCommand(const UnitType *repaired) const;
    void addCommand(CommandType *ct);
    void addBeLoadedCommand(CommandType *ct);
    void addSquadCommand(CommandType *ct);

	bool load(const XmlNode *actionsNode, const string &dir, bool isItem, const FactionType *ft, const CreatableType *cType);
	void save(XmlNode *node) const;
	void getDesc(string &str, const char *pre);
};

}}// end namespace

#endif
