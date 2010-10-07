// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//				  2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "conversion.h"
#include "leak_dumper.h"

namespace Shared { namespace Util {

namespace Conversion {

#define  str_zero		 "0"
#define  str_false		 "false"
#define  str_one		 "1"
#define  str_true		 "true"
#define  str_bool		 "bool"
#define  str_int		 "int"
#define  str_uint		 "unsigned int"
#define  str_int64		 "int64"
#define  str_uint64		 "uint64"
#define  str_double		 "double"
#define  str_float		 "float"
#define  str_longdouble	 "long double"

// overloaded functions to fill in strToX()'s call to strto_
void strto_(const char *nptr, char **endptr, int base, long int &dest) {
	dest = strtoul(nptr, endptr, base);
}

void strto_(const char *nptr, char **endptr, int base, long long int &dest) {
	dest = strtoull(nptr, endptr, base);
}

void strto_(const char *nptr, char **endptr, int base, unsigned long int &dest) {
	dest = strtoul(nptr, endptr, base);
}

void strto_(const char *nptr, char **endptr, int base, unsigned long long int &dest) {
	dest = strtoull(nptr, endptr, base);
}

void strto_(const char *nptr, char **endptr, int base, double &dest) {
	dest = strtod(nptr, endptr);
}

void strto_(const char *nptr, char **endptr, int base, float &dest) {
	dest = strtof(nptr, endptr);
}

void strto_(const char *nptr, char **endptr, int base, long double &dest) {
	dest = strtold(nptr, endptr);
}

// this function is outlined because we don't need this extra code inlined everywhere
void throwException(const string &typeName, const string &s, int base) {
	std::stringstream str;
	str << "Error converting from string to " << typeName << " (base = " << base << "), found: " << s;
	throw runtime_error(str.str());
}

template<typename T>
T strToX(const string &s, int base, const string &typeName) {
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

int hexChar2Int(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	} else if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	} else {
		throw runtime_error("bad hex value");
	}
}

bool strToBool(const string &s) {
	if (s == str_zero || s == str_false) {
		return false;
	}
	if (s == str_one || s == str_true) {
		return true;
	}

	throwException(str_bool, s, 1);
	return false;
}

int strToInt(const string &s, int base) {
	return strToX<long int>(s, base, str_int);
}

unsigned int strToUInt(const string &s, unsigned int base) {
	return strToX<unsigned long int>(s, base, str_uint);
}

int64 strToInt64(const string &s, int base) {
	return strToX<int64>(s, base, str_int64);
}

uint64 strToUInt64(const string &s, int base) {
	return strToX<uint64>(s, base, str_uint64);
}

double strToDouble(const string &s) {
	return strToX<double>(s, 10, str_double);
}

fixed strToFixed(const string &s) {
	size_t n = s.find_first_of('.');
	if (n == string::npos) {
		return fixed(strToInt(s));
	}
	string iPart = s.substr(0, n);
	string fPart = s.substr(n + 1);
	fixed res = strToInt(iPart);
	fixed denom = 10;
	for (int i = 0; i < fPart.size() - 1; ++i) {
		denom *= 10;
	}
	fixed numer = strToInt(fPart);
	return res + numer / denom;
}

float strToFloat(const string &s) {
	return strToX<float>(s, 10, str_float);
}

long double strToLongDouble(const string &s) {
	return strToX<long double>(s, 10, str_longdouble);
}

const string &toStr(bool b) {
	static string s_true = str_true;
	static string s_false = str_false;
	return  b ? s_true : s_false;
}

string toStr(int i) {
	char str[13];
	sprintf(str, "%d", i);
	return str;
}

string toStr(unsigned int i) {
	char str[13];
	sprintf(str, "%u", i);
	return str;
}

string toStr(int64 i) {
	char str[32];
	sprintf(str, "%lld", static_cast<long long int>(i));
	return str;
}

string toStr(uint64 i) {
	char str[32];
	sprintf(str, "%llu", static_cast<unsigned long long int>(i));
	return str;
}

string toHex(int i) {
	char str[10];
	sprintf(str, "%x", static_cast<unsigned int>(i));
	return str;
}

string toHex(unsigned int i) {
	char str[10];
	sprintf(str, "%x", i);
	return str;
}

string toHex(int64 i) {
	char str[18];
	sprintf(str, "%llx", static_cast<long long unsigned int>(i));
	return str;
}

string toHex(uint64 i) {
	char str[18];
	sprintf(str, "%llx", static_cast<long long unsigned int>(i));
	return str;
}

string toStr(fixed f) {
	static char buffer[128];
	double val = double(f.intp()) + double(f.frac()) / double(f.max_frac());
	sprintf(buffer, "%g", val);
	return string(buffer);
}

string toStr(float f) {
	char str[24];
	sprintf(str, "%.2f", f);
	return str;
}

string trimStr(string str)
{
	string trimmedStr = str;

	while (!trimmedStr.empty() && isspace(trimmedStr[0])) {
		trimmedStr.erase(0, 1);
	}

	while (!trimmedStr.empty() && isspace(trimmedStr[trimmedStr.size() - 1])) {
		trimmedStr.erase(trimmedStr.size() - 1);
	}

	return trimmedStr;
}

}

}}//end namespace
