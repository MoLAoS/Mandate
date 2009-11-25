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

using std::string;
using std::runtime_error;
using std::stringstream;
using Shared::Platform::int64;
using Shared::Platform::uint64;

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
class Conversion {
private:
	// Note: static const data members used to prevent replication of strings accross object files
	// caused by inline functions using them.
	static const string str_zero;
	static const string str_false;
	static const string str_one;
	static const string str_true;

	static const string str_bool;
	static const string str_int;
	static const string str_uint;
	static const string str_int64;
	static const string str_uint64;
	static const string str_double;
	static const string str_float;
	static const string str_longdouble;

private:
	Conversion();

public:

	static bool strToBool(const string &s) {
		if (s == str_zero || s == str_false) {
			return false;
		}
		if (s == str_one || s == str_true) {
			return true;
		}
		throwException(str_bool, s, 1);
		return false;
	}

	static int strToInt(const string &s, int base = 10) {
		return strToX<long int>(s, base, str_int);
	}

	static unsigned int strToUInt(const string &s, unsigned int base = 10) {
		return strToX<unsigned long int>(s, base, str_uint);
	}

	static int64 strToInt64(const string &s, int base = 10) {
		return strToX<int64>(s, base, str_int64);
	}

	static uint64 strToUInt64(const string &s, int base = 10) {
		return strToX<uint64>(s, base, str_uint64);
	}

	static double strToDouble(const string &s) {
		return strToX<double>(s, 10, str_double);
	}

	static float strToFloat(const string &s) {
		return strToX<float>(s, 10, str_float);
	}

	static long double strToLongDouble(const string &s) {
		return strToX<long double>(s, 10, str_longdouble);
	}

	/*
	inline bool strToBool(const string &s, bool *b) {
		if (s == "0" || s == "false") {
			*b = false;
			return true;
		}

		if (s == "1" || s == "true") {
			*b = true;
			return true;
		}

		return false;
	}

	inline bool strToInt(const string &s, int *i) {
		char *endChar;
		*i = strtol(s.c_str(), &endChar, 10);

		return !*endChar;
	}

	inline bool strToFloat(const string &s, float *f) {
		char *endChar;
		*f = static_cast<float>(strtod(s.c_str(), &endChar));

		return !*endChar;
	}
	*/
	static const string &toStr(bool b) {
		return b ? str_true : str_false;
	}

	static string toStr(int i) {
		char str[13];
		sprintf(str, "%d", i);
		return str;
	}

	static string toStr(unsigned int i) {
		char str[13];
		sprintf(str, "%u", i);
		return str;
	}

	static string toStr(int64 i) {
		char str[32];
		sprintf(str, "%lld", static_cast<long long int>(i));
		return str;
	}

	static string toStr(uint64 i) {
		char str[32];
		sprintf(str, "%llu", static_cast<unsigned long long int>(i));
		return str;
	}

	static string toHex(int i) {
		char str[10];
		sprintf(str, "%x", static_cast<unsigned int>(i));
		return str;
	}

	static string toHex(unsigned int i) {
		char str[10];
		sprintf(str, "%x", i);
		return str;
	}

	static string toHex(int64 i) {
		char str[18];
		sprintf(str, "%llx", static_cast<long long unsigned int>(i));
		return str;
	}

	static string toHex(uint64 i) {
		char str[18];
		sprintf(str, "%llx", static_cast<long long unsigned int>(i));
		return str;
	}

	static string toStr(float f) {
		char str[24];
		sprintf(str, "%.2f", f);
		return str;
	}

	// ugly (i.e., fat) catch-all
	template<typename T>
	static string toStr(T v) {
		stringstream str;
		str << v;
		return str.str();
	}

	static int hexChar2Int(char c);

	static int hexPair2Int(const char *hex) {
		return (hexChar2Int(hex[0]) << 4) + hexChar2Int(hex[1]);
	}

private:
	// overloaded functions to fill in strToX()'s call to strto_
	static void strto_(const char *nptr, char **endptr, int base, long int &dest) {
		dest = strtoul(nptr, endptr, base);
	}

	static void strto_(const char *nptr, char **endptr, int base, long long int &dest) {
		dest = strtoull(nptr, endptr, base);
	}

	static void strto_(const char *nptr, char **endptr, int base, unsigned long int &dest) {
		dest = strtoul(nptr, endptr, base);
	}

	static void strto_(const char *nptr, char **endptr, int base, unsigned long long int &dest) {
		dest = strtoull(nptr, endptr, base);
	}

	static void strto_(const char *nptr, char **endptr, int base, double &dest) {
		dest = strtod(nptr, endptr);
	}

	static void strto_(const char *nptr, char **endptr, int base, float &dest) {
		dest = strtof(nptr, endptr);
	}

	static void strto_(const char *nptr, char **endptr, int base, long double &dest) {
		dest = strtold(nptr, endptr);
	}

	template<typename T>
	static T strToX(const string &s, int base, const string &typeName) {
		char *endChar;
		T ret;
		strto_(s.c_str(), &endChar, base, ret);

		if (*endChar) {
			// make an actual function call to throw exception so we can keep the size of this
			// inline smaller
			throwException(typeName, s, base);
		}

		return ret;
	}

	static void throwException(const string &typeName, const string &s, int base);
};

inline string intToStr(int i) {return Conversion::toStr(i);}

}}//end namespace

#endif
