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
// 	class SkillTypeFactory
// =====================================================

SkillTypeFactory::SkillTypeFactory()
		: m_idCounter(0) {
	registerClass<StopSkillType>("stop");
	registerClass<MoveSkillType>("move");
	registerClass<AttackSkillType>("attack");
	registerClass<BuildSkillType>("build");
	registerClass<BeBuiltSkillType>("be-built");
	registerClass<HarvestSkillType>("harvest");
	registerClass<RepairSkillType>("repair");
	registerClass<ProduceSkillType>("produce");
	registerClass<UpgradeSkillType>("upgrade");
	registerClass<MorphSkillType>("morph");
	registerClass<DieSkillType>("die");
	registerClass<LoadSkillType>("load");
	registerClass<UnloadSkillType>("unload");
	registerClass<CastSpellSkillType>("cast-spell");
}

SkillTypeFactory::~SkillTypeFactory() {
	deleteValues(m_types);
	m_types.clear();
	m_checksumTable.clear();
}

SkillType* SkillTypeFactory::newInstance(string classId) {
	SkillType *st = MultiFactory<SkillType>::newInstance(classId);
	st->setId(m_idCounter++);
	m_types.push_back(st);
	return st;
}

int32 SkillTypeFactory::getChecksum(SkillType *st) {
	assert(m_checksumTable.find(st) != m_checksumTable.end());
	return m_checksumTable[st];
}

void SkillTypeFactory::setChecksum(SkillType *st) {
	assert(m_checksumTable.find(st) == m_checksumTable.end());
	Checksum checksum;
	st->doChecksum(checksum);
	m_checksumTable[st] = checksum.getSum();
}

// =====================================================
// 	class CommandTypeFactory
// =====================================================

CommandTypeFactory::CommandTypeFactory()
		: m_idCounter(0) {
	registerClass<StopCommandType>("stop");
	registerClass<MoveCommandType>("move");
	registerClass<TeleportCommandType>("teleport");
	registerClass<AttackCommandType>("attack");
	registerClass<AttackStoppedCommandType>("attack-stopped");
	registerClass<BuildCommandType>("build");
	registerClass<HarvestCommandType>("harvest");
	registerClass<RepairCommandType>("repair");
	registerClass<ProduceCommandType>("produce");
	registerClass<GenerateCommandType>("generate");
	registerClass<UpgradeCommandType>("upgrade");
	registerClass<MorphCommandType>("morph");
	registerClass<LoadCommandType>("load");
	registerClass<UnloadCommandType>("unload");
	registerClass<BeLoadedCommandType>("be-loaded");
	registerClass<GuardCommandType>("guard");
	registerClass<PatrolCommandType>("patrol");
	registerClass<CastSpellCommandType>("cast-spell");
	registerClass<SetMeetingPointCommandType>("set-meeting-point");
}

CommandTypeFactory::~CommandTypeFactory() {
	deleteValues(m_types);
	m_types.clear();
	m_checksumTable.clear();
}

CommandType* CommandTypeFactory::newInstance(string classId, UnitType *owner) {
	CommandType *ct = MultiFactory<CommandType>::newInstance(classId);
	ct->setIdAndUnitType(m_idCounter++, owner);
	m_types.push_back(ct);
	return ct;
}

int32 CommandTypeFactory::getChecksum(CommandType *ct) {
	assert(m_checksumTable.find(ct) != m_checksumTable.end());
	return m_checksumTable[ct];
}

void CommandTypeFactory::setChecksum(CommandType *ct) {
	assert(m_checksumTable.find(ct) == m_checksumTable.end());
	Checksum checksum;
	ct->doChecksum(checksum);
	m_checksumTable[ct] = checksum.getSum();
}


}}
