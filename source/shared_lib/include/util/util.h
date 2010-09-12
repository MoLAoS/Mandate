// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//				  2009 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_UTIL_H_
#define _SHARED_UTIL_UTIL_H_

#include <string>
#include <stdexcept>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>
#include <vector>
#include <algorithm>

#include "math_util.h"
#include "lang_features.h"

#if defined(WIN32) || defined(WIN64)
	#include <list>
	#define slist list

	using std::slist;
#else
	#include <ext/slist>

	using __gnu_cxx::slist;
#endif

using std::string;
using std::vector;
using std::runtime_error;

namespace Shared { namespace Xml {
	class XmlNode;
}}

#define WRAPPED_ENUM(Name,...)							\
	struct Name {										\
		enum Enum { INVALID = -1, __VA_ARGS__, COUNT };	\
		Name() : value(INVALID) {}						\
		Name(Enum val) : value(val) {}					\
		explicit Name(int i) {							\
			if (i >= 0 && i < COUNT) value = Enum(i);	\
			else value = INVALID;						\
		}												\
		operator Enum() const { return value; }			\
		void operator++() {								\
			if (value < COUNT) {						\
				value = Enum(value + 1);				\
			}											\
		}												\
		void operator--() {								\
			if (value > 0) {							\
				value = Enum(value - 1);				\
			}											\
		}												\
	private:											\
		Enum value;										\
	};

#define STRINGY_ENUM(Name,...)							\
	WRAPPED_ENUM(Name,__VA_ARGS__)						\
	STRINGY_ENUM_NAMES(Name, Name::COUNT, __VA_ARGS__);

#ifdef NDEBUG
#	define REGULAR_ENUM WRAPPED_ENUM
#else
#	define REGULAR_ENUM STRINGY_ENUM
#endif

#ifdef GAME_CONSTANTS_DEF
#	define STRINGY_ENUM_NAMES(name, count, ...) EnumNames<name> name##Names(#__VA_ARGS__, count, true)
#else
#	define STRINGY_ENUM_NAMES(name, count, ...)	extern EnumNames<name> name##Names
#endif

template<typename E>
E enum_cast(unsigned i) {
	return i < E::COUNT ? static_cast<typename E::Enum>(i) : E::INVALID;
}

namespace Shared { namespace Util {

const string sharedLibVersionString= "v0.5";

// =====================================================
//	class EnumNames
// =====================================================

class EnumNamesBase {
private:
	const char *valueList;
	const char **names;
	const char *qualifiedList;
	const char **qualifiedNames;
	size_t count;

public:
	EnumNamesBase(const char *valueList, size_t count, bool lazy, const char *enumName = NULL);
	~EnumNamesBase();

protected:
	const char *get(int i, size_t count) const { // using count as local instead of data member to compile out memory access (as long as a constant is passed for count)
		if (!names) {
			const_cast<EnumNamesBase*>(this)->init();
		}
		if (i < 0 || i >= int(count)) {
			return "invalid value";
		}
		return qualifiedNames ? qualifiedNames[i] : names[i];
	}

	int _match(const char *value) const;

private:
	void init();
};

template<typename E>
class EnumNames : public EnumNamesBase {
public:
	EnumNames(const char *valueList, size_t count, bool lazy, const char *enumName = NULL) 
		: EnumNamesBase(valueList, count, lazy, enumName) {}
	~EnumNames() {}

	const char* operator[](int i) const {return get(i, E::COUNT);} 
	const char* operator[](E e) const {return get(e, E::COUNT);} // passing E::COUNT here will inline the value in EnumNamesBase::get()
	E match(const char *value) const {return enum_cast<E>(_match(value));} // this will inline a function call to the fairly large _match() function
};

#define foreach(CollectionClass, it, collection) for(CollectionClass::iterator it = (collection).begin(); it != (collection).end(); ++it)
#define foreach_rev(CollectionClass, it, collection) for(CollectionClass::reverse_iterator it = (collection).rbegin(); it != (collection).rend(); ++it)
#define foreach_const(CollectionClass, it, collection) for(CollectionClass::const_iterator it = (collection).begin(); it != (collection).end(); ++it)
#define foreach_enum(Enum, val) for(Enum val(0); val < Enum::COUNT; ++val)

void findAll(const string &path, vector<string> &results, bool cutExtension = false);

//string fcs
string cleanPath(const string &s);
string dirname(const string &s);
string basename(const string &s);
string lastDir(const string &s);

inline string lastFile(const string &s){
	return lastDir(s);
}

string cutLastFile(const string &s);
string cutLastExt(const string &s);
string ext(const string &s);
string replaceBy(const string &s, char c1, char c2);

//#if defined(WIN32) || defined(WIN64)
inline string intToHex(int addr) {
	static char hexBuffer[32];
	sprintf(hexBuffer, "%.8X", addr);
	return string(hexBuffer);
}
//#endif

inline string toLower(const string &str){
	string s = str;
	std::transform(s.begin(), s.end(), s.begin(), (int(*)(int))std::tolower);
	return s;
}

inline void copyStringToBuffer(char *buffer, int bufferSize, const string& s) {
	strncpy(buffer, s.c_str(), bufferSize-1);
	buffer[bufferSize-1]= '\0';
}

extern const char uu_base64[64];
void uuencode(char *dest, size_t *destSize, const void *src, size_t n);
void uudecode(void *dest, size_t *destSize, const char *src);

// ==================== numeric fcs ====================

template <typename T> inline T clamp(const T &val, const T &min, const T &max) {
	return	val < min	? min
		 :	val > max	? max
						: val;
}

inline float saturate(float value) {
	return clamp(value, 0.f, 1.f);
}

inline int round(float f){
     return int(roundf(f));
}

//misc
bool fileExists(const string &path);

template<typename  FwdIt>
void deleteValues(FwdIt beginIt, FwdIt endIt){
	for(FwdIt it= beginIt; it!=endIt; ++it){
		delete *it;
	}
}

template<typename ContainerType>
void deleteValues(ContainerType container){
	for (typename ContainerType::iterator it = container.begin(); it != container.end(); ++it) {
		delete *it;
	}
}

template<typename T>
void deleteMapValues(T beginIt, T endIt){
	for(T it= beginIt; it!=endIt; ++it){
		delete it->second;
	}
}

template<typename MapType>
void deleteMapValues(MapType map) {
	for (typename MapType::iterator it = map.begin(); it != map.end(); ++it) {
		delete it->second;
	}
}

}}//end namespace

#endif
