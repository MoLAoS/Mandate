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

#ifndef _SHARED_UTIL_CONVERSION_H_
#define _SHARED_UTIL_CONVERSION_H_

#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>

using std::string;
using std::runtime_error;

namespace Shared { namespace Util {

const int maxStrSize = 256;

inline bool strToBool(const string &s) {
	if (s == "0" || s == "false") {
		return false;
	}

	if (s == "1" || s == "true") {
		return true;
	}

	throw runtime_error("Error converting string to bool, expected 0, 1, true or false, found: " + s);
}

inline int strToInt(const string &s) {
	char *endChar;
	int intValue = strtol(s.c_str(), &endChar, 10);

	if (*endChar) {
		throw runtime_error("Error converting from string to int, found: " + s);
	}

	return intValue;
}


inline float strToFloat(const string &s) {
	char *endChar;
	float floatValue = static_cast<float>(strtod(s.c_str(), &endChar));

	if (*endChar) {
		throw runtime_error("Error converting from string to float, found: " + s);
	}

	return floatValue;
}

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

inline string boolToStr(bool b) {
	return b ? "true" : "false";
}

inline string intToStr(int i) {
	char str[maxStrSize];
	sprintf(str, "%d", i);
	return str;
}

inline string intToHex(int i) {
	char str[maxStrSize];
	sprintf(str, "%x", i);
	return str;
}

inline string floatToStr(float f) {
	char str[maxStrSize];
	sprintf(str, "%.2f", f);
	return str;
}

inline string doubleToStr(double d){
	char str[maxStrSize];
	sprintf(str, "%.2f", d);
	return str; 
}

/*
bool strToBool(const string &s);
int strToInt(const string &s);
float strToFloat(const string &s);

bool strToBool(const string &s, bool *b);
bool strToInt(const string &s, int *i);
bool strToFloat(const string &s, float *f);

string boolToStr(bool b);
string intToStr(int i);
string intToHex(int i);
string floatToStr(float f);
*/
}
}//end namespace

#endif
