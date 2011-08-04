// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008-2010 Daniel Santos <daniel.santos@pobox.com>
//				  2009-2011 James McCulloch <silnarm at gmail>
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
#include <deque>
#include <list>
#include <map>
#include <algorithm>

#include "math_util.h"
#include "lang_features.h"
#include "random.h"
#include "timer.h"
#include "conversion.h"

using std::list;
using std::string;
using std::map;
using std::vector;
using std::runtime_error;

namespace Shared { namespace Xml {
	class XmlNode;
}}

#define ASSERT_RANGE(var, size)	assert(var >= 0 && var < size)

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

extern const string sharedLibVersionString;

// some helpers to find stuff in vectors

template<typename C, typename T>
bool find(C &c, const T &val) {
	return std::find(c.begin(), c.end(), val) != c.end();
}

template<typename C, typename T>
bool find(C &c, const T &val, typename C::iterator &out) {
	out = std::find(c.begin(), c.end(), val);
	return out != c.end();
}


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
	E match(const string &val) const { return match(val.c_str()); }
};

//
// A run-time assert, that can be changed to assert() one day...
// ... if we ever get to 1.0 maybe ;)
//
// Update: is assert() now to 'crash' debug builds and get stack traces...
//
#define RUNTIME_CHECK(x)                                                    \
	if (!(x)) {                                                             \
		std::stringstream ss;                                               \
		ss << "In " << __FUNCTION__ << " () [" << __FILE__ << " : "         \
			<< __LINE__ << "]\nRuntime check fail: "#x;                     \
		g_logger.logError(ss.str());                                        \
		assert(false);                                                      \
		throw runtime_error(ss.str());	                                    \
	}	

// and another one, with a custom error message
#define RUNTIME_CHECK_MSG(x, msg)                                           \
	if (!(x)) {                                                             \
		std::stringstream ss;                                               \
		ss << "In " << __FUNCTION__ << " () [" << __FILE__ << " : "         \
			<< __LINE__ << "]\nRuntime check fail: "#x << "\n" << msg;      \
		g_logger.logError(ss.str());                                        \
		assert(false);                                                      \
		throw runtime_error(ss.str());	                                    \
	}	


//
// Memory checking stuff
//
#if _GAE_DEBUG_EDITION_ && !_GAE_LEAK_DUMP_

#	define MEMORY_CHECK_DECLARATIONS(Class)			\
		static void* operator new(size_t n);		\
		static void  operator delete(void *ptr);	\
		static void* operator new[](size_t n);		\
		static void  operator delete[](void *ptr);	\
		static size_t getAllocatedMemSize();		\
		static size_t getAllocCount();				\
		static size_t getDeAllocCount();			

	typedef map<void*, size_t> AllocationMap;

#	define MEMORY_CHECK_IMPLEMENTATION(Class)						\
		static size_t			s_allocTotal = 0;					\
		static size_t			s_allocCount = 0;					\
		static size_t			s_deAllocCount = 0;					\
		static AllocationMap	s_allocMap;							\
		size_t Class::getAllocatedMemSize() {return s_allocTotal;}	\
		size_t Class::getAllocCount() {return s_allocCount;}		\
		size_t Class::getDeAllocCount()	{return s_deAllocCount;}	\
		void* Class::operator new(size_t n) {						\
			void *res = ::operator new(n);							\
			if (res) {												\
				s_allocMap[res] = n;								\
				s_allocTotal += n;									\
			}														\
			++s_allocCount;											\
			return res;												\
		}															\
		void  Class::operator delete(void *ptr) {					\
			if (!ptr) return;										\
			AllocationMap::iterator it = s_allocMap.find(ptr);		\
			INVARIANT(it != s_allocMap.end(), "Bad delete!");		\
			INVARIANT(it->second != 0, "Bad alloc, size == 0.");	\
			s_allocTotal -= it->second;								\
			s_allocMap.erase(it);									\
			++s_deAllocCount;										\
			::operator delete(ptr);									\
		}															\
		void* Class::operator new[](size_t n) {						\
			void *res = ::operator new[](n);						\
			if (res) {												\
				s_allocMap[res] = n;								\
				s_allocTotal += n;									\
			}														\
			return res;												\
		}															\
		void Class::operator delete[](void *ptr) {					\
			if (!ptr) return;										\
			AllocationMap::iterator it = s_allocMap.find(ptr);		\
			INVARIANT(it != s_allocMap.end(), "Bad delete!");		\
			INVARIANT(it->second != 0, "Bad alloc, size == 0.");	\
			s_allocTotal -= it->second;								\
			s_allocMap.erase(it);									\
			::operator delete[](ptr);								\
		}															
#else // !_GAE_DEBUG_EDITION_ || _GAE_LEAK_DUMP_
#	define MEMORY_CHECK_DECLARATIONS(Class)
#	define MEMORY_CHECK_IMPLEMENTATION(Class)
#endif

//
// Some basic foreach loops
//

#define foreach(CollectionClass, it, collection) for(CollectionClass::iterator it = (collection).begin(); it != (collection).end(); ++it)
#define foreach_rev(CollectionClass, it, collection) for(CollectionClass::reverse_iterator it = (collection).rbegin(); it != (collection).rend(); ++it)
#define foreach_const(CollectionClass, it, collection) for(CollectionClass::const_iterator it = (collection).begin(); it != (collection).end(); ++it)
#define foreach_enum(Enum, val) for(Enum val(0); val < Enum::COUNT; ++val)

//
// global error log for shared lib media loading functions
//

struct MediaErrorLog {
public:
	struct ErrorRecord {
		string msg;
		string path;
	};

private:
	std::deque<ErrorRecord> errors;

public:
	MediaErrorLog() {}

	void add(string msg, string path) {
		errors.push_back(ErrorRecord());
		errors.back().msg = msg;
		errors.back().path = path;
	}

	bool hasError() const { return !errors.empty(); }
	ErrorRecord popError() { ErrorRecord rec = errors.front(); errors.pop_front(); return rec; }
};

extern MediaErrorLog mediaErrorLog;


//
// Util finctions
//

/// check existence of a file 
///@todo move into Shared::PhysFS?
bool fileExists(const string &path);
///@todo move into Shared::PhysFS?
/** Find all files in a directory
  * @param out_results stores the found paths, can be 0 size if doThrow = false
  */
void findAll(const string &path, vector<string> &out_results, bool cutExtension = false, bool doThrow = true);

// path string utils
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


// misc string utils

string formatString(const string &str);

string replaceBy(const string &s, char c1, char c2);

inline string intToHex(int addr) {
	static char hexBuffer[32];
	sprintf(hexBuffer, "%.8X", addr);
	return string(hexBuffer);
}

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

inline void trimString(string &str) {
	while (!str.empty() && *(str.end() - 1) == ' ') {
		str.erase(str.end() - 1);
	}
	while (!str.empty() && *(str.begin()) == ' ') {
		str.erase(str.begin());
	}
}


inline void trimTrailingNewlines(string &str) {
	while (!str.empty() && *(str.end() - 1) == '\n') {
		str.erase(str.end() - 1);
	}
}

// numeric util functions

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


inline float remap(float in, const float oldMin, const float oldMax, const float newMin, const float newMax) {
	float t = (in - oldMin) / (oldMax - oldMin);
	return newMin + t * (newMax - newMin);
}

// delete stuff in std containers

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

// 'jumble' a std container (random accessible ones only)
template <typename T>
void jumble(vector<T> &list, Random &rand) {
	for (int i=0; i < list.size(); ++i) {
		int j = rand.randRange(0, list.size() - 1);
		if (i == j) continue;
		std::swap(list[i], list[j]);
	}
}

}

namespace Debug {

using Platform::Chrono;
using namespace Util;

inline string formatName(const string &str) {
	string res = str;
	foreach (string, c, res) {
		if (*c == '_') {
			*c = ' ';
		}
	}
	return res;
}

inline string formatTime(int64 ms) {
	if (ms < 0) {
		ms = 0;
	}
	int sec = ms / 1000;
	if (sec) {
		int millis = ms % 1000;
		return intToStr(sec) + " s, " + intToStr(millis) + " ms.";
	}
	return intToStr(int(ms)) + " ms.";
}

struct OneTimeTimer {
	string name;
	Chrono chrono;
	std::ostream& out;

	OneTimeTimer(const string &name, std::ostream& out) : name(name), out(out) {chrono.start();}
	~OneTimeTimer() {
		chrono.stop();
		out << formatName(name) << ": " << formatTime(chrono.getAccumTime()) << endl;
	}
};

}} // end namespace Shared

#define ONE_TIME_TIMER(name, ostream) Shared::Debug::OneTimeTimer oneTimeTimer_##name(#name, ostream)

#endif
