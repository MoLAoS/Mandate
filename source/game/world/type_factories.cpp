// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V2, see source/liscence.txt
// ==============================================================

#include "pch.h"
#include "type_factories.h"
#include "leak_dumper.h"

namespace Glest { namespace Sim {

// =====================================================
// 	class MasterTypeFactory
// =====================================================

MasterTypeFactory::MasterTypeFactory() {
	// Register Skill Types
	m_skillTypeFactory.registerClass<StopSkillType>();
	m_skillTypeFactory.registerClass<MoveSkillType>();
	m_skillTypeFactory.registerClass<AttackSkillType>();
	m_skillTypeFactory.registerClass<BuildSkillType>();
	m_skillTypeFactory.registerClass<BeBuiltSkillType>();
	m_skillTypeFactory.registerClass<HarvestSkillType>();
	m_skillTypeFactory.registerClass<RepairSkillType>();
	m_skillTypeFactory.registerClass<ProduceSkillType>();
	m_skillTypeFactory.registerClass<UpgradeSkillType>();
	m_skillTypeFactory.registerClass<MorphSkillType>();
	m_skillTypeFactory.registerClass<DieSkillType>();
	m_skillTypeFactory.registerClass<LoadSkillType>();
	m_skillTypeFactory.registerClass<UnloadSkillType>();
	m_skillTypeFactory.registerClass<CastSpellSkillType>();

	// Register Command Types
	m_commandTypeFactory.registerClass<StopCommandType>();
	m_commandTypeFactory.registerClass<MoveCommandType>();
	m_commandTypeFactory.registerClass<AttackCommandType>();
	m_commandTypeFactory.registerClass<AttackStoppedCommandType>();
	m_commandTypeFactory.registerClass<BuildCommandType>();
	m_commandTypeFactory.registerClass<HarvestCommandType>();
	m_commandTypeFactory.registerClass<RepairCommandType>();
	m_commandTypeFactory.registerClass<ProduceCommandType>();
	m_commandTypeFactory.registerClass<GenerateCommandType>();
	m_commandTypeFactory.registerClass<UpgradeCommandType>();
	m_commandTypeFactory.registerClass<MorphCommandType>();
	m_commandTypeFactory.registerClass<LoadCommandType>();
	m_commandTypeFactory.registerClass<UnloadCommandType>();
	m_commandTypeFactory.registerClass<BeLoadedCommandType>();
	m_commandTypeFactory.registerClass<GuardCommandType>();
	m_commandTypeFactory.registerClass<PatrolCommandType>();
	m_commandTypeFactory.registerClass<CastSpellCommandType>();
	m_commandTypeFactory.registerClass<SetMeetingPointCommandType>();

	// Register Producible Types
	m_prodTypeFactory.registerClass<GeneratedType>();
	m_prodTypeFactory.registerClass<UpgradeType>();
	m_prodTypeFactory.registerClass<UnitType>();

	// Register Effect and Emanation Types
	m_effectTypeFactory.registerClass<EffectType>();
	m_effectTypeFactory.registerClass<EmanationType>();
}

}}
