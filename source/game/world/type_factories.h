// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V2, see source/liscence.txt
// ==============================================================

#ifndef _GLEST_SIM_TYPE_FACTORY_
#define _GLEST_SIM_TYPE_FACTORY_

#include "factory.h"
#include "unit_type.h"
#include "upgrade_type.h"
#include "command_type.h"
#include "skill_type.h"

namespace Glest { namespace Sim {
using namespace ProtoTypes;

// ===============================
//  class TypeFactory
// ===============================

template<typename ProtoType>
class TypeFactory : private SingleTypeFactory<ProtoType> {
private:
	map<int, ProtoType*>	m_typeTable;
	map<ProtoType*, int32>	m_checksumTable;

public:
	TypeFactory() { }

	~TypeFactory() {
		deleteMapValues(m_typeTable.begin(), m_typeTable.end());
		m_typeTable.clear();
		m_checksumTable.clear();
	}

	ProtoType* newInstance(int id) {
		ProtoType *t = SingleTypeFactory<ProtoType>::newInstance();
		t->setId(id);
		m_typeTable[id] = t;
		return t;
	}

	void setChecksum(ProtoType *t) {
		assert(m_checksumTable.find(t) == m_checksumTable.end());
		Checksum checksum;
		t->doChecksum(checksum);
		m_checksumTable[t] = checksum.getSum();
	}

	ProtoType* getType(int id) {
		return m_typeTable[id];
	}

	int32 getChecksum(ProtoType *t) {
		assert(m_checksumTable.find(t) != m_checksumTable.end());
		return m_checksumTable[t];
	}
};

// ===============================
//  class CommandTypeFactory
// ===============================

class CommandTypeFactory: private MultiFactory<CommandType> {
private:
	vector<CommandType *> m_types;
	map<CommandType*, int32> m_checksumTable;
	int m_idCounter;

public:
	CommandTypeFactory();
	~CommandTypeFactory();

	CommandType* newInstance(string classId, UnitType *owner);
	CommandType* getType(int id) { return m_types[id]; }
	int getTypeCount() const	{ return m_types.size(); }
	int32 getChecksum(CommandType *ct);
	void setChecksum(CommandType *ct);
};

// ===============================
// 	class SkillTypeFactory
// ===============================

class SkillTypeFactory: public MultiFactory<SkillType> {
private:
	vector<SkillType*> m_types;
	map<SkillType *, int32> m_checksumTable;
	int m_idCounter;

public:
	SkillTypeFactory();
	~SkillTypeFactory();

	SkillType* newInstance(string classId);
	SkillType* getType(int id)	{ return m_types[id]; }
	int getTypeCount() const	{ return m_types.size(); }
	int32 getChecksum(SkillType *st);
	void setChecksum(SkillType *st);
};

WRAPPED_ENUM ( ProducibleClass, NONE, GENERATED, UPGRADE, UNIT );

// ===============================
// 	class MasterTypeFactory
// ===============================

class MasterTypeFactory {
private:
	int							m_idCounter;
	vector<ProducibleType*>		m_types;
	map<int, ProducibleClass>	m_classMap;

	TypeFactory<GeneratedType>  m_generatedTypeFactory;
	TypeFactory<UpgradeType>	m_upgradeTypeFactory;
	TypeFactory<UnitType>		m_unitTypeFactory;

public:
	MasterTypeFactory() : m_idCounter(0) {}

	ProducibleType* getType(int id) {
		RUNTIME_CHECK(id >= 0 && id < m_types.size());
		return m_types[id];
	}

	//ProducibleType* getType(string type) {
	//	for (int i=0; i < m_types.size(); ++i) {
	//		if (m_types[i]->getName() == type) {
	//			return m_types[i];
	//		}
	//	}
	//	throw runtime_error("Error, could not find producible type: " + type);
	//}

	int getTypeCount() const { return m_types.size(); }

	int32 getChecksum(ProducibleType *pt) {
		switch (m_classMap[pt->getId()]) {
			case ProducibleClass::GENERATED:
				return m_generatedTypeFactory.getChecksum(static_cast<GeneratedType*>(pt));
			case ProducibleClass::UPGRADE:
				return m_upgradeTypeFactory.getChecksum(static_cast<UpgradeType*>(pt));
			case ProducibleClass::UNIT:
				return m_unitTypeFactory.getChecksum(static_cast<UnitType*>(pt));
			default:
				throw runtime_error("Unknown producible type");
		}
	}

	bool isGeneratedType(const ProducibleType *pt) { return m_classMap[pt->getId()] == ProducibleClass::GENERATED;}
	bool isUpgradeType(const ProducibleType *pt) { return m_classMap[pt->getId()] == ProducibleClass::UPGRADE;}
	bool isUnitType(const ProducibleType *pt) { return m_classMap[pt->getId()] == ProducibleClass::UNIT;}

	GeneratedType* newGeneratedType() {
		m_types.push_back(m_generatedTypeFactory.newInstance(m_idCounter++));
		m_classMap[m_types.size() - 1] = ProducibleClass::GENERATED;
		return static_cast<GeneratedType*>(m_types.back());
	}

	UpgradeType* newUpgradeType() {
		m_types.push_back(m_upgradeTypeFactory.newInstance(m_idCounter++));
		m_classMap[m_types.size() - 1] = ProducibleClass::UPGRADE;
		return static_cast<UpgradeType*>(m_types.back());
	}

	UnitType*	newUnitType() {
		m_types.push_back(m_unitTypeFactory.newInstance(m_idCounter++));
		m_classMap[m_types.size() - 1] = ProducibleClass::UNIT;
		return static_cast<UnitType*>(m_types.back());
	}

	TypeFactory<GeneratedType>  &getGeneratedTypeFactory() { return m_generatedTypeFactory; }
	TypeFactory<UpgradeType>	&getUpgradeTypeFactory() { return m_upgradeTypeFactory; }
	TypeFactory<UnitType>		&getUnitTypeFactory() { return m_unitTypeFactory; }
};

}}

#endif
