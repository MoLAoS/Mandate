// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_CONVERSION_H_
#define _SHARED_UTIL_CONVERSION_H_

#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "types.h"
#include "fixed.h"
#include "vec.h"

using std::string;
using std::runtime_error;
using std::stringstream;
using Shared::Platform::int64;
using Shared::Platform::uint64;
using Shared::Math::Vec2i;
using Shared::Math::Vec3f;
using Shared::Math::fixed;

#if defined(WIN32) || defined(WIN64)
#	define strtoull(np,ep,b) _strtoui64(np,ep,b)
#	define strtof(np,ep) ((float)strtod(np,ep))
#	define strtold(np,ep) ((long double)strtod(np,ep))
#endif

namespace Shared { namespace Util {

const int maxStrSize = 256;

/**
 * Contains static functions to primatives to and from string representation.  Ideally, this class
 * would not be needed if we were using iostreams instead of appending string objects, but it's
 * better than having a multitude of global functions.
 */
namespace Conversion {

	bool			strToBool(const string &s);
	int				strToInt(const string &s, int base = 10);
	unsigned int	strToUInt(const string &s, unsigned int base = 10);
	int64			strToInt64(const string &s, int base = 10);
	uint64			strToUInt64(const string &s, int base = 10);
	double			strToDouble(const string &s);
	float			strToFloat(const string &s);
	fixed			strToFixed(const string &s);
	long double		strToLongDouble(const string &s);

	const string &toStr(bool b);

	string toStr(int i);
	string toStr(unsigned int i);
	string toStr(int64 i);
	string toStr(uint64 i);
	string toHex(int i);
	string toHex(unsigned int i);
	string toHex(int64 i);
	string toHex(uint64 i);
	string toStr(fixed f);
	string toStr(float f);

	// ugly (i.e., fat) catch-all
	template<typename T> string toStr(T v) {
		stringstream str;
		str << v;
		return str.str();
	}

	int hexChar2Int(char c);

	inline int hexPair2Int(const char *hex) {
		return (hexChar2Int(hex[0]) << 4) + hexChar2Int(hex[1]);
	}

};

inline string intToStr(int i) {return Conversion::toStr(i);}

inline string Vec2iToStr(Vec2i v) {
	char str[maxStrSize];
	sprintf(str, "(%d,%d)", v.x, v.y);
	return str; 
}

inline string Vec3fToStr(Vec3f v) {
	char str[maxStrSize];
	sprintf(str, "(%g,%g,%g)", v.x, v.y, v.z);
	return str; 
}

}}//end namespace

#endif
