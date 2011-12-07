// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V2, see source/liscence.txt
// ==============================================================

#ifndef _GLEST_SIM_TYPE_FACTORY_
#define _GLEST_SIM_TYPE_FACTORY_

#include "util.h"
#include "factory.h"
#include "unit_type.h"
#include "upgrade_type.h"
#include "command_type.h"
#include "skill_type.h"

#include "logger.h"

namespace Glest { namespace Sim {
using namespace ProtoTypes;

// ===================================================================
//  class EntityFactory, a factory class for transient instance types
// ===================================================================

template<typename Entity> class EntityFactory {

	friend class World; // needs to get and set id counters for save games

private:
	typedef std::map<int, Entity*>  ObjectTable;
	typedef std::list<Entity*>      ObjectList;

private:
	ObjectTable  m_objTable;
	ObjectList   m_allObjs;
	int          m_idCounter;
	const bool   m_destoryObjects;

private:
	void registerInstance(Entity *obj) {
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
	Entity* newInstance(Arg1 a1) {
		Entity *newbie = new Entity(a1);
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

	void deleteInstance(const Entity *ptr) {
		deleteInstance(ptr->getId());
	}

public:
	EntityFactory(bool deleteObjs = true) : m_idCounter(0), m_destoryObjects(deleteObjs) { }

	virtual ~EntityFactory() {
		if (m_destoryObjects) {
			for (typename ObjectList::iterator it = m_allObjs.begin(); it != m_allObjs.end(); ++it) {
				delete *it;
			}
		}
	}

	unsigned getInstanceCount() const { return m_allObjs.size(); }

	Entity* getInstance(int id) {
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
//  class SingleTypeFactory
// ===============================
/** Single Type Factory, for load once never write (non-transient) ProtoType classes
  * that are not part of a class hierarchy requiring a DynamicTypeFactory */

template<typename Type> class SingleTypeFactory {
private:
	typedef vector<Type*>             ObjectList;
	typedef map<const Type*, int32>   ChecksumMap;

private:
	ObjectList   m_types;
	ChecksumMap  m_checksumTable;

public:
	SingleTypeFactory() { }

	virtual ~SingleTypeFactory() {
		deleteValues(m_types);
	}

	Type* newInstance() {
		Type *newbie = new Type();
		m_types.push_back(newbie);
		newbie->setId(m_types.size() - 1);
		return newbie;
	}

	template <typename Arg1>
	Type* newInstance(Arg1 a1) {
		Type *newbie = new Type(a1);
		m_types.push_back(newbie);
		newbie->setId(m_types.size() - 1);
		return newbie;
	}

	Type* getType(int id)	{ ASSERT_RANGE(id, m_types.size()); return m_types[id]; }
	int getTypeCount() const	{ return m_types.size(); }

	int32 getChecksum(const Type *t) {
		return m_checksumTable[t];
	}
	void setChecksum(const Type *t) {
		Checksum checksum;
		t->doChecksum(checksum);
		m_checksumTable[t] = checksum.getSum();
	}

};

// ===============================
//  class DynamicTypeFactory
// ===============================
/** Dynamic Type Factory, for load once never write (non-transient) ProtoType classes,
  * Enum describes the derived class set, all of which must derive from (or be) 
  * BaseType, derived types must implement a static function 'Enum typeClass()' */
template<typename Enum, typename BaseType> class DynamicTypeFactory {
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
	DynamicTypeFactory() : m_idCounter(0) { }

	virtual ~DynamicTypeFactory() {
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
// 	class PrototypeFactory
// ===============================

typedef DynamicTypeFactory<SkillClass, SkillType>            SkillTypeFactory;
typedef DynamicTypeFactory<CmdClass, CommandType>            CommandTypeFactory;
typedef DynamicTypeFactory<ProducibleClass, ProducibleType>  ProducibleTypeFactory;
typedef DynamicTypeFactory<EffectClass, EffectType>	         EffectTypeFactory;
typedef SingleTypeFactory<CloakType>                         CloakTypeFactory;
typedef SingleTypeFactory<DetectorType>                      DetectorTypeFactory;

class PrototypeFactory {
protected:
	ProducibleTypeFactory  m_prodTypeFactory;
	SkillTypeFactory       m_skillTypeFactory;
	CommandTypeFactory     m_commandTypeFactory;
	EffectTypeFactory      m_effectTypeFactory;
	CloakTypeFactory       m_cloakTypeFactory;
	DetectorTypeFactory    m_detectorTypeFactory;

public:
	PrototypeFactory();


public:	// create objects

	// Producibles...
	GeneratedType* newGeneratedType() {
		return static_cast<GeneratedType*>(m_prodTypeFactory.newInstance(ProducibleClass::GENERATED));
	}
	UpgradeType* newUpgradeType() {
		return static_cast<UpgradeType*>(m_prodTypeFactory.newInstance(ProducibleClass::UPGRADE));
 	}
	UnitType*	newUnitType() {
		return static_cast<UnitType*>(m_prodTypeFactory.newInstance(ProducibleClass::UNIT));
 	}

	// skills
	SkillType*		newSkillType(SkillClass sc)		{ return m_skillTypeFactory.newInstance(sc); }

	// commands
	CommandType*	newCommandType(CmdClass cc, const UnitType *ut)	{
		CommandType *ct = m_commandTypeFactory.newInstance(cc);
		ct->setUnitType(ut);
		return ct;
	}

	// Effects
	EffectType*		newEffectType()		{ return m_effectTypeFactory.newInstance(EffectClass::EFFECT); }
	EmanationType*	newEmanationType()	{ 
		return static_cast<EmanationType*>(m_effectTypeFactory.newInstance(EffectClass::EMANATION));
 	}

	// Cloaks and Detectors
	CloakType* newCloakType(const UnitType *ut) { return m_cloakTypeFactory.newInstance(ut);  }
	DetectorType* newDetectorType(const UnitType *ut) { return m_detectorTypeFactory.newInstance(ut); }

public:
	// is derived producible type
	bool isGeneratedType(const ProducibleType *pt) { return pt->getClass() == ProducibleClass::GENERATED;}
	bool isUpgradeType(const ProducibleType *pt) { return pt->getClass() == ProducibleClass::UPGRADE;}
	bool isUnitType(const ProducibleType *pt) { return pt->getClass() == ProducibleClass::UNIT;}

	// get checksums
	int32 getChecksum(const ProducibleType *pt) { return m_prodTypeFactory.getChecksum(pt); }
	int32 getChecksum(const CommandType *ct) { return m_commandTypeFactory.getChecksum(ct); }
	int32 getChecksum(const SkillType *st) { return m_skillTypeFactory.getChecksum(st); }
	int32 getChecksum(const EffectType *et) { return m_effectTypeFactory.getChecksum(et); }
	int32 getChecksum(const CloakType *ct) { return m_cloakTypeFactory.getChecksum(ct); }
	int32 getChecksum(const DetectorType *dt) { return m_detectorTypeFactory.getChecksum(dt); }

	// set checksums
	void setChecksum(const SkillType *st) { m_skillTypeFactory.setChecksum(st); }
	void setChecksum(const CommandType *ct) { m_commandTypeFactory.setChecksum(ct); }
	void setChecksum(const ProducibleType *pt) { m_prodTypeFactory.setChecksum(pt); }
	void setChecksum(const EffectType *et) { m_effectTypeFactory.setChecksum(et); }
	void setChecksum(const CloakType *ct) { m_cloakTypeFactory.setChecksum(ct); }
	void setChecksum(const DetectorType *dt) { m_detectorTypeFactory.setChecksum(dt); }

	// get
	int getProdTypeCount() const { return m_prodTypeFactory.getTypeCount(); }
	int getSkillTypeCount() const { return m_skillTypeFactory.getTypeCount(); }
	int getCommandTypeCount() const { return m_commandTypeFactory.getTypeCount(); }
	int getCloakTypeCount() const { return m_cloakTypeFactory.getTypeCount(); }
	int getDetectorTypeCount() const { return m_detectorTypeFactory.getTypeCount(); }

	// get proto-types by id
	const ProducibleType* getProdType(int id) { return m_prodTypeFactory.getType(id); }
	const UnitType* getUnitType(int id) { return m_prodTypeFactory.getTypeById<UnitType>(id); }
	const SkillType* getSkillType(int id) { return m_skillTypeFactory.getType(id); }
	const CommandType* getCommandType(int id) { return m_commandTypeFactory.getType(id); }
	const CloakType* getCloakType(int i) { return m_cloakTypeFactory.getType(i); }
	const DetectorType* getDetectorType(int i) { return m_detectorTypeFactory.getType(i); }
};

}}

#endif
