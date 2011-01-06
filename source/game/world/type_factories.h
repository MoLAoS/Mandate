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

#include "logger.h"

namespace Glest { namespace Sim {
using namespace ProtoTypes;
using Util::Logger;

// ===================================================================
//  class StaticFactory, a factory class for transient instance types
// ===================================================================

template<typename StaticType> class StaticFactory {

	friend class World; // needs to get and set id counters for save games

private:
	typedef std::map<int, StaticType*>  ObjectTable;
	typedef std::vector<StaticType*>    ObjectList;

private:
	ObjectTable  m_objTable;
	ObjectList   m_allObjs;
	int       m_idCounter;

private:
	void registerInstance(StaticType *obj) {
		if (obj->getId() == -1) {
			obj->setId(m_idCounter++);
		} else {
			RUNTIME_CHECK(m_objTable.find(obj->getId()) == m_objTable.end());
		}
		m_allObjs.push_back(obj);
		m_objTable[obj->getId()] = obj;
	}

protected:
	void setIdCounter(int idCount) { m_idCounter = idCount; }
	int  getIdCounter() const      { return m_idCounter;    }

public:
	template <typename Arg1>
	StaticType* newInstance(Arg1 a1) {
		StaticType *newbie = new StaticType(a1);
		registerInstance(newbie);
		return newbie;
	}

	void deleteInstance(int id) {
		typename ObjectTable::iterator it = m_objTable.find(id);
		RUNTIME_CHECK(it != m_objTable.end());
		m_allObjs.erase(std::find(m_allObjs.begin(), m_allObjs.end(), it->second));
		delete it->second;
		m_objTable.erase(it);
	}

	void deleteInstance(const StaticType *ptr) {
		deleteInstance(ptr->getId());
	}

public:
	StaticFactory() : m_idCounter(0) { }

	virtual ~StaticFactory() {
		for (typename ObjectList::iterator it = m_allObjs.begin(); it != m_allObjs.end(); ++it) {
			delete *it;
		}
	}

	unsigned getInstanceCount() const { return m_allObjs.size(); }

	StaticType* getInstance(int id) {
		typename ObjectTable::iterator it = m_objTable.find(id);
		if (it != m_objTable.end()) {
			return it->second;
		} else {
			return 0;
		}
	}

	typename ObjectList::const_iterator begin() const { return m_allObjs.begin(); }
	typename ObjectList::const_iterator end() const { return m_allObjs.end();}
};

// ===============================
//  class DynamicFactory
// ===============================
/** Dynamic Type Factory, Enum describes the derived class set, all of which must derive
  * from (or be) BaseType, derived types must implement a static function 'Enum typeClass()' */
template<typename Enum, typename BaseType> class DynamicFactory {
private:
	typedef map<Enum, SingleFactoryBase*>		Factories;
	typedef pair<Enum, SingleFactoryBase*>		FactoryPair;
	typedef vector<BaseType*>					ObjectList;
	typedef map<const BaseType*, int32>			ChecksumMap;

private:
	Factories		m_factories;
	ObjectList		m_allObjs;
	ObjectList		m_objsByType[Enum::COUNT];
	ChecksumMap		m_checksumTable;
	int				m_idCounter;

public:
	DynamicFactory() : m_idCounter(0) { }

	virtual ~DynamicFactory() {
		deleteValues(m_allObjs);
		deleteMapValues(m_factories);
	}

	template<typename DerivedType>
	void registerClass() {
		Enum typeClass = DerivedType::typeClass();
		assert(m_factories.find(typeClass) == m_factories.end());
		m_factories.insert(FactoryPair(typeClass, new SingleFactory<DerivedType>()));
	}

	BaseType* newInstance(Enum typeClass) {
		typename Factories::iterator it = m_factories.find(typeClass);
		if (it == m_factories.end()) {
			throw UnknownType("Invalid type class: " + intToStr(typeClass));
		}
		BaseType *newbie = static_cast<BaseType*>(it->second->newInstance());
		m_objsByType[typeClass].push_back(newbie);
		m_allObjs.push_back(newbie);
		newbie->setId(m_idCounter++);
		return newbie;
	}

	BaseType* newInstance(Enum typeClass, const string &name) {
		BaseType *newbie = newInstance(typeClass);
		newbie->setName(name);
		return newbie;
	}

	BaseType* getType(int id)	{ return m_allObjs[id]; }
	BaseType* getObject(int id) { return getType(id); }
	int getTypeCount() const	{ return m_allObjs.size(); }

	template <typename DerivedType>
	int getTypeCount()		{ return m_objsByType[DerivedType::typeClass()].size(); }

	template <typename DerivedType>
	DerivedType* getTypeById(int id) {
		BaseType *btp = m_allObjs[id];
		if (btp->getClass() == DerivedType::typeClass()) {
			return static_cast<DerivedType*>(btp);
		}
		return 0;
	}

	template <typename DerivedType>
	DerivedType* getTypeByIndex(int ndx) {
		Enum e = DerivedType::typeClass();
		if (e != Enum::INVALID) {
			return static_cast<DerivedType*>(m_objsByType[e][ndx]);
		}
		return 0;
	}

	int32 getChecksum(const BaseType *t) {
		return m_checksumTable[t];
	}
	void setChecksum(const BaseType *t) {
		Checksum checksum;
		t->doChecksum(checksum);
		m_checksumTable[t] = checksum.getSum();
	}

};

// ===============================
// 	class MasterTypeFactory
// ===============================

typedef DynamicFactory<SkillClass, SkillType>            SkillTypeFactory;
typedef DynamicFactory<CommandClass, CommandType>        CommandTypeFactory;
typedef DynamicFactory<ProducibleClass, ProducibleType>  ProducibleTypeFactory;
typedef DynamicFactory<EffectClass, EffectType>	         EffectTypeFactory;

class MasterTypeFactory {
protected:
	ProducibleTypeFactory  m_prodTypeFactory;
	SkillTypeFactory       m_skillTypeFactory;
	CommandTypeFactory     m_commandTypeFactory;
	EffectTypeFactory      m_effectTypeFactory;

public:
	MasterTypeFactory();

	// get
	ProducibleType* getProducibleType(int id) { return m_prodTypeFactory.getType(id); }
	int getProdTypeCount() const { return m_prodTypeFactory.getTypeCount(); }
	int32 getChecksum(const ProducibleType *pt) { return m_prodTypeFactory.getChecksum(pt); }
	int32 getChecksum(const CommandType *ct) { return m_commandTypeFactory.getChecksum(ct); }
	int32 getChecksum(const SkillType *st) { return m_skillTypeFactory.getChecksum(st); }

	// is
	bool isGeneratedType(const ProducibleType *pt) { return pt->getClass() == ProducibleClass::GENERATED;}
	bool isUpgradeType(const ProducibleType *pt) { return pt->getClass() == ProducibleClass::UPGRADE;}
	bool isUnitType(const ProducibleType *pt) { return pt->getClass() == ProducibleClass::UNIT;}

public:	// create objects
	GeneratedType* newGeneratedType() {
		ProducibleType *pt = m_prodTypeFactory.newInstance(ProducibleClass::GENERATED);
		return static_cast<GeneratedType*>(pt);
	}
	UpgradeType* newUpgradeType() {
		ProducibleType *pt = m_prodTypeFactory.newInstance(ProducibleClass::UPGRADE);
		return static_cast<UpgradeType*>(pt);
 	}
	UnitType*	newUnitType() {
		ProducibleType *pt = m_prodTypeFactory.newInstance(ProducibleClass::UNIT);
		return static_cast<UnitType*>(pt);
 	}
	SkillType*		newSkillType(SkillClass sc)		{ return m_skillTypeFactory.newInstance(sc); }
	CommandType*	newCommandType(CommandClass cc, const UnitType *ut)	{
		CommandType *ct = m_commandTypeFactory.newInstance(cc);
		ct->setUnitType(ut);
		return ct;
	}

	EffectType*		newEffectType()		{ return m_effectTypeFactory.newInstance(EffectClass::EFFECT); }
	EmanationType*	newEmanationType()	{ 
		return static_cast<EmanationType*>(m_effectTypeFactory.newInstance(EffectClass::EMANATION));
 	}

	// checksums
	void setChecksum(const SkillType *st) { m_skillTypeFactory.setChecksum(st); }
	void setChecksum(const CommandType *ct) { m_commandTypeFactory.setChecksum(ct); }
	void setChecksum(const ProducibleType *pt) { m_prodTypeFactory.setChecksum(pt); }
	void setChecksum(const EffectType *et) { m_effectTypeFactory.setChecksum(et); }

	// get
	const ProducibleType* getProdType(int id) { return m_prodTypeFactory.getType(id); }
	const UnitType* getUnitType(int id) { return m_prodTypeFactory.getTypeById<UnitType>(id); }
	const SkillType* getSkillType(int id) { return m_skillTypeFactory.getType(id); }
	int getSkillTypeCount() const { return m_skillTypeFactory.getTypeCount(); }
	const CommandType* getCommandType(int id) { return m_commandTypeFactory.getType(id); }
	int getCommandTypeCount() const { return m_commandTypeFactory.getTypeCount(); }

};

}}

#endif
