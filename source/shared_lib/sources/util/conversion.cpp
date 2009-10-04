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

#include "pch.h"
#include "conversion.h"
#include "leak_dumper.h"

using namespace std;

namespace Shared { namespace Util {

const string Conversion::str_zero		= "0";
const string Conversion::str_false		= "false";
const string Conversion::str_one		= "1";
const string Conversion::str_true		= "true";
const string Conversion::str_bool		= "bool";
const string Conversion::str_int		= "int";
const string Conversion::str_uint		= "unsigned int";
const string Conversion::str_int64		= "int64";
const string Conversion::str_uint64		= "uint64";
const string Conversion::str_double		= "double";
const string Conversion::str_float		= "float";
const string Conversion::str_longdouble	= "long double";

// this function is outlined because we don't need this extra code inlined everywhere
__cold __noreturn void Conversion::throwException(const string &typeName, const string &s, int base) {
	std::stringstream str;
	str << "Error converting from string to " << typeName << " (base = " << base << "), found: " << s;
	throw runtime_error(str.str());
}

int Conversion::hexChar2Int(char c) {
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

}}//end namespace
