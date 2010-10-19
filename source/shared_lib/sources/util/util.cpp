// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "types.h"
#include "util.h"

#include <ctime>
#include <cassert>
#include <list>
#include <stdexcept>
#include <cstring>
#include <cstdio>

#include "leak_dumper.h"
#include "platform_util.h"
#include "random.h"
#include "xml_parser.h"
#include "zlib.h"
#include "FSFactory.hpp"

#if !defined(WIN32) && !defined(WIN64)
#	include <glob.h>
#endif

#include "leak_dumper.h"

using namespace Shared::Platform;
using Shared::Xml::XmlNode;

namespace Shared{ namespace Util{

using namespace PhysFS;

MediaErrorLog mediaErrorLog;

EnumNamesBase::EnumNamesBase(const char *valueList, size_t count, bool lazy, const char *enumName)
		: valueList(valueList)
		, names(NULL)
		, qualifiedList(NULL)
		, qualifiedNames(NULL)
		, count(count) {
	if(!lazy) {
		init();
	}
	if (enumName) {
		if (lazy) {
			throw runtime_error("Qualified names and Lazy loading not simultaneously supported.");
		}
		qualifiedList = const_cast<const char*>(new char[(strlen(enumName) + 2) * count + strlen(valueList) + 1]);
		qualifiedNames = const_cast<const char**>(new char*[count]);
		char *tmp = strcpy(new char[strlen(valueList) + 1], valueList);
		char *tok = strtok(tmp,", ");
		char *ptr = const_cast<char*>(qualifiedList);
		int tokens = 0;
		while (tok) {
			qualifiedNames[tokens] = ptr;
			tokens++;
			while (isspace(*tok)) tok++;
			ptr += sprintf(ptr, "%s::%s", enumName, tok);
			*ptr++ = 0;
			tok = strtok(NULL, ", ");
		}
		delete [] tmp;
	}
}

EnumNamesBase::~EnumNamesBase() {
	if (names) {
		delete [] names;
		delete [] valueList;
	}
	if (qualifiedNames) {
		delete [] qualifiedNames;
		delete [] qualifiedList;
	}
}


int EnumNamesBase::_match(const char *value) const {
	if (!names) {
		const_cast<EnumNamesBase*>(this)->init();
	}
	for (int i=0; i < count; ++i) {
		const char *ptr1 = names[i];
		const char *ptr2 = value;
		bool same = true;
		if (!*ptr1 || !*ptr2) {
			continue;
		}
		while (*ptr1 && *ptr2) {
			if (isalpha(*ptr1) && isalpha(*ptr2)) {
				if (tolower(*ptr1) != tolower(*ptr2)) {
					same = false;
					break;
				}
			} else if (!(*ptr1 == '_' && (*ptr2 == ' ' || *ptr2 == '_' || *ptr2 == '-'))) {
				same = false;
				break;
			}
			++ptr1;
			++ptr2;
			if ((!*ptr1 && *ptr2 && !isspace(*ptr2)) || (*ptr1 && !*ptr2)) {
				same = false;
			}
		}
		if (same) {
			return i;
		}
	}
	return -1;
}

void EnumNamesBase::init() {
	size_t curName = 0;
	bool needStart = true;

	assert(!names);
	names = new const char *[count];
	valueList = strcpy(new char[strlen(valueList) + 1], valueList);

	for (char *p = const_cast<char*>(valueList); *p; ++p) {
		if (isspace(*p)) { // I don't want to have any leading or trailing whitespace
			*p = 0;
		} else if (needStart) {
			// do some basic sanity checking, even though the compiler should catch any such errors
			assert(isalpha(*p) || *p == '_');
			assert(curName < count);
			names[curName++] = p;
			needStart = false;
		} else if (*p == ',') {
			assert(!needStart);
			needStart = true;
			*p = 0;
		}
	}
	assert(curName == count);
}

int Random::rand() {
	lastNumber = (a * lastNumber + b) % m;
	return lastNumber;
}

int Random::randRange(int min, int max) {
	assert(min <= max);
	int diff = max - min;
	assert(diff < m);
	if (!diff) return min;
	int res = min + (rand() % (diff + 1));
	assert(res >= min && res <= max);
	return res;
}

float Random::randRange(float min, float max) {
	assert(min <= max);
	float rand01 = float(Random::rand()) / (m - 1);
	float res = min + (max - min) * rand01;
	assert(res >= min && res <= max);
	return res;
}

fixed Random::randPercent() {
	fixed res = rand() % 100;
	int32 frac_bit = rand() % (fixed::scale_factor());
	res.raw() += frac_bit;
	return res;
}

//finds all filenames like path and stores them in results
void findAll(const string &path, vector<string> &results, bool cutExtension){
	if (FSFactory::getInstance()->usePhysFS) {
		results.clear();
		vector<string> strings = FSFactory::findAll(path, cutExtension);
		results.insert(results.begin(), strings.begin(), strings.end());
		if (results.empty()) {
			throw runtime_error("No files found: " + path);
		}
	} else {
		slist<string> l;
		DirIterator di;
		char *p = initDirIterator(path, di);

		if(!p) {
			throw runtime_error("No files found: " + path);
		}

		do {
			// exclude current and parent directory as well as any files that start
			// with a dot, but do not exclude files with a leading relative path
			// component, although the path component will be stripped later.
			if(p[0] == '.' && !(p[1] == '/' || (p[1] == '.' && p[2] == '/'))) {
				continue;
			}
			char* begin = p;
			char* dot = NULL;

			for( ; *p != 0; ++p) {
				// strip the path component
				switch(*p) {
				case '/':
					begin = p + 1;
					break;
				case '.':
					dot = p;
				}
			}
			// this may zero out a dot preceding the base file name, but we
			// don't care
			if(cutExtension && dot) {
				*dot = 0;
			}
			l.push_front(begin);
		} while((p = getNextFile(di)));

		freeDirIterator(di);

		l.sort();
		results.clear();
		for(slist<string>::iterator li = l.begin(); li != l.end(); ++li) {
			results.push_back(*li);
		}
	}
}

/**
 * Cleans up a path.
 * - converting any backslashes to slashes
 * - removes duplicate path delimiters
 * - eliminates all entries that specify the current directory
 * - if a "paraent directory" entry is found (two dots) and a path element exists before it, then
 *   both are removed (i.e., factored out)
 * - If the resulting path is the current directory, then an empty string is returned.
 */
string cleanPath(const string &s) {
	// empty input gets empty output
	if(!s.length()) {
		return s;
	}

	string result;
	vector<const char *> elements;
	char *buf = new char[s.length() + 1];
	strcpy(buf, s.c_str());
	char *data;
	bool isAbsolute;

	isAbsolute = (s.at(0) == '/' || s.at(0) == '\\');

	for(char *p =  strtok_r(buf, "\\/", &data); p; p = strtok_r(NULL, "\\/", &data)) {
		// skip duplicate path delimiters
		if(strlen(p) == 0) {
			continue;

		// skip entries that just say "the current directory"
		} else if(!strcmp(p, ".")) {
			continue;

		// If an entry referrs to the parent directory and we have that, then we just drop the
		// parent and shorten the whole path
		} else if(!strcmp(p, "..") && elements.size()) {
			elements.pop_back();
		} else {
			elements.push_back(p);
		}
	}

	for(vector<const char *>::const_iterator i = elements.begin(); i != elements.end(); ++i) {
		if(result.length() || isAbsolute) {
			result.push_back('/');
		}
		result.append(*i);
	}

	delete[] buf;
	return result;
}

string dirname(const string &s) {
	string clean = cleanPath(s);
	int pos = clean.find_last_of('/');

	/* This does the same thing, which one is more readable?
	return pos == string::npos ? string(".") : (pos ? clean.substr(0, pos) : string("/"));
	*/

	if(pos == string::npos) {
		return string(".");
	} else if(pos == 0) {
		return string("/");
	} else {
		return clean.substr(0, pos);
	}
}

string basename(const string &s) {
	string cleaned = cleanPath(s);
	int pos = cleaned.find_last_of('/');

	// cleanPath() promises that the last character wont be the slash, so "pos + 1" should be safe.
	return pos == string::npos ? cleaned : cleaned.substr(pos + 1);
}


string lastDir(const string &s){
	size_t i= s.find_last_of('/');
	size_t j= s.find_last_of('\\');
	size_t pos;

	if(i==string::npos){
		pos= j;
	}
	else if(j==string::npos){
		pos= i;
	}
	else{
		pos= i<j? j: i;
	}

	if (pos==string::npos){
		throw runtime_error(string(__FILE__)+" lastDir - i==string::npos");
	}

	return (s.substr(pos+1, s.length()));
}

string cutLastFile(const string &s){
	size_t i= s.find_last_of('/');
	size_t j= s.find_last_of('\\');
	size_t pos;

	if(i==string::npos){
		pos= j;
	}
	else if(j==string::npos){
		pos= i;
	}
	else{
		pos= i<j? j: i;
	}

	if (pos==string::npos){
		throw runtime_error(string(__FILE__)+"cutLastFile - i==string::npos");
	}

	return (s.substr(0, pos));
}

string cutLastExt(const string &s){
     size_t i= s.find_last_of('.');

	 if (i==string::npos){
          throw runtime_error(string(__FILE__)+"cutLastExt - i==string::npos");
	 }

     return (s.substr(0, i));
}

string ext(const string &s){
     size_t i;

     i=s.find_last_of('.')+1;

	 if (i==string::npos){
          throw runtime_error(string(__FILE__)+"cutLastExt - i==string::npos");
	 }

     return (s.substr(i, s.size()-i));
}

string replaceBy(const string &s, char c1, char c2){
	string rs= s;

	for(size_t i=0; i<s.size(); ++i){
		if (rs[i]==c1){
			rs[i]=c2;
		}
	}

	return rs;
}

// ==================== misc ====================

bool fileExists(const string &path) {
	if (FSFactory::getInstance()->usePhysFS) {
		return FSFactory::fileExists(path);
	} else {
		FILE* file= fopen(path.c_str(), "rb");
		if (file!=NULL) {
			fclose(file);
			return true;
		}
		return false;
	}
}

const string sharedLibVersionString= "v0.5";

}}//end namespace
