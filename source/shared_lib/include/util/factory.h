// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_FACTORY_
#define _SHARED_UTIL_FACTORY_

#include <map>
#include <string>
#include <stdexcept>

#include "util.h"

using std::map;
using std::string;
using std::pair;
using std::runtime_error;

namespace Shared{ namespace Util{

// =====================================================
//	class SingleFactoryBase
// =====================================================

class SingleFactoryBase{
public:
	virtual ~SingleFactoryBase(){}
	virtual void *newInstance()= 0;
};

// =====================================================
//	class SingleFactory
// =====================================================

template<typename T>
class SingleFactory: public SingleFactoryBase{
public:
	virtual void *newInstance()	{return new T();}
};

template <typename T>
class SingleTypeFactory {
	SingleFactory<T> singleFactory;
public:
	T* newInstance() {
		return static_cast<T*>(singleFactory.newInstance());
	}
};

// =====================================================
//	class MultiFactory
// =====================================================

class UnknownType: public std::exception {
public:
	UnknownType(const string &typeName) : typeName(typeName) {}
	~UnknownType() throw() {}
	const char* what() const throw() {
		static char msgBuf[512];
		sprintf(msgBuf, "Unknown class identifier: %s.", typeName.c_str());
		return msgBuf;
	}

private:
	string typeName;
};

template<typename T>
class MultiFactory {
private:
	typedef map<string, SingleFactoryBase*> Factories;
	typedef pair<string, SingleFactoryBase*> FactoryPair;

private:
	Factories factories;

public:
	virtual ~MultiFactory(){
		for(Factories::iterator it= factories.begin(); it!=factories.end(); ++it){
			delete it->second;
		}
	}

	template<typename R>
	void registerClass(string classId) {
		classId = formatString(classId);
		factories.insert(FactoryPair(classId, new SingleFactory<R>()));
	}

	//
	///@todo This is broken, it relies on classes deriving from T _first_
	// if any other base class comes first this breaks.
	//
	// To fix properly, we will need to use a new single factory type, avoiding any of
	// that evil casting to/from 'void*'
	//
	// template <typename BaseClass, typename DerivedClass>
	// class SingeleDerivedFactory {
	//		...
	//		BaseClass* newInstance() { return static_cast<BaseClass*>(new DerivedClass()); }
	//		...
	//

	T* newInstance(string classId) {
		classId = formatString(classId);
		Factories::iterator it= factories.find(classId);
		if (it == factories.end()) {
			throw UnknownType(classId);
		}
		return static_cast<T*>(it->second->newInstance());
	}

	bool isClassId(string classId) {
		classId = formatString(classId);
		return (factories.find(classId) != factories.end());
	}
};

}}//end namespace

#endif
