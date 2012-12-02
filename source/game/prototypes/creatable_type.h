// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_CREATABLETYPE_H_
#define _GLEST_GAME_CREATABLETYPE_H_

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <time.h>

#include "forward_decs.h"
#include "unit_stats_base.h"
#include "element_type.h"
#include "modifications.h"
#include "producibles.h"
#include "vec.h"

using std::map;
using std::vector;

using Shared::Graphics::Texture2D;
using Shared::Math::Vec3f;
using Glest::Sim::World;
using Glest::Gui::Clicks;

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class CreatableType
// =====================================================
class CreatableType : public ProducibleType, public EnhancementType {
private:
    typedef vector<string> ModifyNames;
    typedef vector<Modification> Modifications;
    typedef vector<EmanationType*> Emanations;
    typedef vector<DamageType> Resistances;
    typedef vector<DamageType> DamageTypes;
	typedef vector<UnitsOwned> OwnedUnits;
    ResourceProductionSystem resourceGeneration;
    ItemProductionSystem itemGeneration;
    UnitProductionSystem unitGeneration;
    ProcessProductionSystem processingSystem;
private:
    Resistances resistances;
    DamageTypes damageTypes;
    Emanations emanations;
	OwnedUnits ownedUnits;
    Modifications modifications;
    ModifyNames modifyNames;
public:
	EffectTypes effectTypes;
private:
    const FactionType *m_factionType;
    int size;
    int height;
	fixed halfSize;
	fixed halfHeight;
	SkillType *startSkill;
	bool m_hasProjectileAttack;
public:
    ResourceProductionSystem getResourceProductionSystem() const {return resourceGeneration;}
    ItemProductionSystem getItemProductionSystem() const {return itemGeneration;}
    UnitProductionSystem getUnitProductionSystem() const {return unitGeneration;}
    ProcessProductionSystem getProcessProductionSystem() const {return processingSystem;}
	const Emanations &getEmanations() const {return emanations;}
	Resistances getResistances() const {return resistances;}
	DamageTypes getDamageTypes() const {return damageTypes;}
	OwnedUnits getOwnedUnits() const {return ownedUnits;}
	Modifications getModifications() const {return modifications;}
    const FactionType *getFactionType() const {return m_factionType;}
	int getSize() const {return size;}
	int getHeight() const {return height;}
	fixed getHalfSize() const							{return halfSize;}
	fixed getHalfHeight() const							{return halfHeight;}
	bool hasProjectileAttack() const				{return m_hasProjectileAttack;}
    const SkillType *getStartSkill() const				{return startSkill;}
	bool load(const XmlNode *creatableTypeNode, const string &dir, const TechTree *techTree, const FactionType *factionType);
private:
	typedef vector<SkillType*>          SkillTypes;
	SkillTypes skillTypes;
	SkillTypes skillTypesByClass[SkillClass::COUNT];
	typedef vector<CommandType*> CommandTypes;
	CommandTypes commandTypes;
	CommandTypes commandTypesByClass[CmdClass::COUNT];
	CommandTypes squadCommands;
public:
	int getSkillTypeCount() const						{return skillTypes.size();}
	const SkillType *getSkillType(int i) const			{return skillTypes[i];}
	const SkillType *getSkillType(const string &skillName, SkillClass skillClass = SkillClass::COUNT) const;
	const SkillType *getFirstStOfClass(SkillClass sc) const {
		return skillTypesByClass[sc].empty() ? 0 : skillTypesByClass[sc].front();
	}
	bool hasSkillType(const SkillType *skillType) const;
	bool hasSkillClass(SkillClass skillClass) const;
    void setDeCloakSkills(const vector<string> &names, const vector<SkillClass> &classes);
    void addSkillType(SkillType *skillType);
    void sortSkillTypes();
	int getCommandTypeCount() const						{return commandTypes.size();}
	const CommandType *getCommandType(int i) const		{return commandTypes[i];}
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
	const CommandType *getFirstCtOfClass(CmdClass cc) const {
		return commandTypesByClass[cc].empty() ? 0 : commandTypesByClass[cc].front();
	}
	const CommandType *getCommandType(const string &name) const;
	bool hasCommandType(const CommandType *ct) const;
	bool hasCommandClass(CmdClass cc) const		{return !commandTypesByClass[cc].empty();}
    void sortCommandTypes();
    const HarvestCommandType *getHarvestCommand(const ResourceType *resourceType) const;
	const AttackCommandType *getAttackCommand(Zone zone) const;
	const RepairCommandType *getRepairCommand(const UnitType *repaired) const;
    void addCommand(CommandType *ct);
    void addBeLoadedCommand(CommandType *ct);
    void addSquadCommand(CommandType *ct);
};

}}// end namespace

#endif
