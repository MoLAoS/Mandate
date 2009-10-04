// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "properties.h"

#include <fstream>
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <locale>

#include "conversion.h"

#include "leak_dumper.h"


using namespace std;

namespace Shared { namespace Util {

// =====================================================
// class Properties
// =====================================================

Properties::Properties() {}

void Properties::load(const string &path, bool trim) {
	locale loc;
	ifstream fileStream;
	char lineBuffer[maxLine];
	string line, key, value;
	int pos;

	this->path = path;

//	fileStream.exceptions(ios::failbit | ios::badbit);
	fileStream.open(path.c_str(), ios_base::in);
	if (fileStream.fail()) {
		throw runtime_error("Can't open propertyMap file: " + path);
	}

	propertyMap.clear();
	while (!fileStream.eof()) {
		fileStream.getline(lineBuffer, maxLine);

		lineBuffer[maxLine-1] = '\0';

		if (lineBuffer[0] == ';' || lineBuffer[0] == '#') {
			continue;
		}

		// FIXME: Is this neccisary? I thought ifstream was supposed to do this for us unless you
		// pass ios_base::binary when opening the file (i.e., the default file mode is "text").
		// gracefully handle win32 \r\n line endings
		size_t len = strlen(lineBuffer);
		if (len > 0 && lineBuffer[len - 1] == '\r') {
			lineBuffer[len-1] = 0;
		}

		line = lineBuffer;
		pos = line.find('=');

		if (pos == string::npos) {
			continue;
		}

		key = line.substr(0, pos);
		value = line.substr(pos + 1);
		if(trim) {
			while (!key.empty() && isspace( key[0], loc )) {
				key.erase(0, 1);
			}
			while (!key.empty() && isspace( key[key.size() - 1], loc )) {
				key.erase(key.size() - 1);
			}
			while (!value.empty() && isspace( value[0], loc )) {
				value.erase(0, 1);
			}
			while (!value.empty() && isspace( value[value.size() - 1], loc )) {
				value.erase(value.size() - 1);
			}
		}
		propertyMap.insert(PropertyPair(key, value));
		propertyVector.push_back(PropertyPair(key, value));
	}

	fileStream.close();
}

void Properties::save(const string &path) {
	ofstream fileStream;

	fileStream.open(path.c_str(), ios_base::out | ios_base::trunc);

	fileStream << "; === propertyMap File === \n";
	fileStream << '\n';

	for (PropertyMap::iterator pi = propertyMap.begin(); pi != propertyMap.end(); ++pi) {
		fileStream << pi->first << '=' << pi->second << '\n';
	}

	fileStream.close();
}

void Properties::clear(){
	propertyMap.clear();
	propertyVector.clear();
}

const string *Properties::_getString(const string &key, bool required) const {
	PropertyMap::const_iterator it;
	it = propertyMap.find(key);
	if(it == propertyMap.end()) {
		if(required) {
			throw runtime_error("Value not found in propertyMap: " + key + ", loaded from: " + path);
		}
		return NULL;
	} else {
		return &it->second;
	}
}

template<typename T>
static inline void checkRange(const T &val, const T *pMin, const T *pMax) {
	if((pMin && val < *pMin) || (pMax && val > *pMax)) {
		stringstream str;
		str << "Value out of range: " << val << "(min = ";
		if(pMin) {
			str << *pMin;
		} else {
			str << "none";
		}
		str << ", max = ";
		if(pMax) {
			str << *pMax << ")";
		} else {
			str << "none)";
		}
		throw range_error(str.str());
	}
}

bool Properties::_getBool(const string &key, const bool *pDef) const {
	try {
		const string *pstrVal = _getString(key, !pDef);
		if(!pstrVal) {
			assert(pDef);
			return *pDef;
		}
		return Conversion::strToBool(*pstrVal);
	} catch (exception &e) {
		throw runtime_error("Error accessing value: " + key + " in: " + path + "\n" + e.what());
	}
}

int Properties::_getInt(const string &key, const int *pDef, const int *pMin, const int *pMax) const {
	try {
		const string *pstrVal = _getString(key, !pDef);
		if(!pstrVal) {
			assert(pDef);
			return *pDef;
		}
		int val =  Conversion::strToInt(*pstrVal);
		checkRange<int>(val, pMin, pMax);
		return val;
	} catch (exception &e) {
		throw runtime_error("Error accessing value: " + key + " in: " + path + "\n" + e.what());
	}
}

float Properties::_getFloat(const string &key, const float *pDef, const float *pMin, const float *pMax) const {
	try {
		const string *pstrVal = _getString(key, !pDef);
		if(!pstrVal) {
			assert(pDef);
			return *pDef;
		}
		float val =  Conversion::strToFloat(*pstrVal);
		checkRange<float>(val, pMin, pMax);
		return val;
	} catch (exception &e) {
		throw runtime_error("Error accessing value: " + key + " in: " + path + "\n" + e.what());
	}
}

const string &Properties::_getString(const string &key, const string *pDef) const {
	try {
		const string *pstrVal = _getString(key, !pDef);
		if(!pstrVal) {
			assert(pDef);
			return *pDef;
		}
		return *pstrVal;
	} catch (exception &e) {
		throw runtime_error("Error accessing value: " + key + " in: " + path + "\n" + e.what());
	}
}

string Properties::toString() const {
	stringstream str;

	for (PropertyMap::const_iterator pi = propertyMap.begin(); pi != propertyMap.end(); pi++)
		str << pi->first << "=" << pi->second << endl;

	return str.str();
}

}}//end namepsace
