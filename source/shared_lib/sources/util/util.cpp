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
#include "util.h"

#include <ctime>
#include <cassert>
#include <list>
#include <stdexcept>
#include <cstring>
#include <cstdio>

#include "leak_dumper.h"
#include "platform_util.h"
#include "xml_parser.h"
#include "zlib.h"
#include "FSFactory.hpp"

// Hack
#if defined(WIN32) || defined(WIN64)
#	define strtok_r(a,b,c) strtok(a,b)
#else
#	include <glob.h>
#endif

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using Shared::Xml::XmlNode;

namespace Shared{ namespace Util{

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
			} else if (!(*ptr1 == '_' && (*ptr2 == ' ' || *ptr2 == '_'))) {
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

// =====================================================
//	class SimpleDataBuffer
// =====================================================

inline void SimpleDataBuffer::compact() {
	if(!size()) {
		start = end = 0;
	} else {
		// rewind buffer
		if(start) {
			memcpy(buf, &buf[start], size());
			end -= start;
			start = 0;
		}
	}
}

inline void SimpleDataBuffer::resizeBuffer(size_t newSize) {
	compact();
	if(!(buf = (char *)realloc(buf, bufsize = newSize))) {
		throw runtime_error("out of memory");
	}
	if(newSize < end) {
		end = newSize;
	}
}

void SimpleDataBuffer::expand(size_t minAdded) {
	if(room() < minAdded) {
		size_t newSize = minAdded > (size_t)(bufsize * 0.2f)
				? minAdded + bufsize
				: (size_t)(bufsize * 1.2f);
		resizeBuffer(newSize);
	}
}

void SimpleDataBuffer::compressUuencodIntoXml(XmlNode *dest, size_t chunkSize) {
	size_t uuencodedSize;
	int zstatus;
	char *linebuf;
	int chunks;
	uLongf compressedSize;
	char *tmpbuf;

	// allocate temp buffer for compressed data
	compressedSize = (uLongf)(size() * 1.001f + 12.f);
	tmpbuf = (char *)malloc(max((size_t)compressedSize, chunkSize + 1));

	if(!tmpbuf) {
		throw runtime_error("Out of Memory: failed to allocate " + intToStr(compressedSize) + " bytes of data.");
	}

	// compress data
	zstatus = ::compress2((Bytef *)tmpbuf, &compressedSize, (const Bytef *)data(), size(), Z_BEST_COMPRESSION);
	if(zstatus != Z_OK) {
		free(tmpbuf);
		throw runtime_error("error in zstream while compressing map data");
	}

	// shrink temp buffer
	tmpbuf = (char *)realloc(tmpbuf, max((size_t)compressedSize, chunkSize + 1));

	// write header info to xml node
	dest->addAttribute("size", (int)size());
	dest->addAttribute("compressed-size", (int)compressedSize);
	dest->addAttribute("compressed", true);
	dest->addAttribute("encoding", "base64");

	// uuencode tmpbuf back into this
	clear();
	ensureRoom(compressedSize * 4 / 3 + 5);
	uuencodedSize = room();
	Shared::Util::uuencode(buf, &uuencodedSize, tmpbuf, compressedSize);
	resize(uuencodedSize);

	// shrink temp buffer again, this time we'll use it as a line buffer
	tmpbuf = (char *)realloc(tmpbuf, chunkSize + 1);

	// finally, break it apart into chunkSize byte chunks and cram it into the xml file
	chunks = (uuencodedSize + chunkSize - 1) / chunkSize;
	for(int i = 0; i < chunks; i++) {
		strncpy(tmpbuf, &((char*)data())[i * chunkSize], chunkSize);
		tmpbuf[chunkSize] = 0;
		dest->addChild("data", tmpbuf);		// slightly inefficient, but oh well
	}
}

void SimpleDataBuffer::uudecodeUncompressFromXml(const XmlNode *src) {
	string s;
	char *tmpbuf;
	size_t expectedSize;
	uLongf actualSize;
	size_t compressedSize;
	size_t decodedSize;
	size_t encodedSize;
	char null = 0;

	expectedSize = src->getIntAttribute("size");
	compressedSize = src->getIntAttribute("compressed-size");
	encodedSize = compressedSize / 3 * 4 + (compressedSize % 3 ? compressedSize % 3 + 1 : 0) + 1;

	// retrieve encoded characters from xml document and store in buf
	clear();
	ensureRoom(max(expectedSize, encodedSize + 1));	// reduce buffer expansions
	for(int i = 0; i < src->getChildCount(); ++i) {
		s = src->getChild("data", i)->getStringAttribute("value");
		write(s.c_str(), s.size());
	}

	// add null terminator
	write(&null, 1);

	if(size() != encodedSize) {
		throw runtime_error("Error extracting uuencoded data from xml.  Expected "
				+ intToStr(encodedSize) + " characters of uuencoded data, but only found "
				+ intToStr(size()));
	}

	assert(strlen(buf) == size() - 1);

	// allocate tmpbuf to hold compressed (decoded) data
	tmpbuf = (char *)malloc(decodedSize = compressedSize);
	uudecode(tmpbuf, &decodedSize, buf);
	assert(decodedSize == compressedSize);

	clear();
	actualSize = room();
	int zstatus = ::uncompress((Bytef *)buf, &actualSize, (const Bytef *)tmpbuf, decodedSize);
	free(tmpbuf);

	if(zstatus != Z_OK) {
		throw runtime_error("error in zstream while decompressing map data in saved game");
	}

	if(actualSize != expectedSize) {
		throw runtime_error("Error extracting uuencoded data from xml.  Expected "
				+ intToStr(expectedSize) + " bytes of uncompressed data, but actual size was "
				+ intToStr(actualSize));
	}

	resize(expectedSize);
}

// =====================================================
//	Global Functions
// =====================================================

const char uu_base64[64] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};

void uuencode(char *dest, size_t *destSize, const void *src, size_t n) {
	char *p;
	const unsigned char *s;
	size_t remainder;
	size_t expectedSize;
	const unsigned char *lastTriplet;

	p = dest - 1;
	s = (unsigned char *)src;
	remainder = n % 3;
	expectedSize = n / 3 * 4 + (remainder ? remainder + 1 : 0) + 1; // how many bytes we expect to write
	assert(*destSize >= expectedSize);
	lastTriplet = &s[n - remainder - 1];

	for(; s < lastTriplet; s += 3) {
		*(++p) = uu_base64[ s[0] >> 2];
		*(++p) = uu_base64[(s[0] << 4 & 0x30) | s[1] >> 4];
		*(++p) = uu_base64[(s[1] << 2 & 0x3c) | s[2] >> 6];
		*(++p) = uu_base64[ s[2] & 0x3f];
	}

	if(remainder > 0) {
		*(++p) = uu_base64[s[0] >> 2];

		if(remainder == 1) {
			*(++p) = uu_base64[ s[0] << 4 & 0x30];
		} else {
			*(++p) = uu_base64[(s[0] << 4 & 0x30) | s[1] >> 4];
			*(++p) = uu_base64[ s[1] << 2 & 0x3c];
		}
	}
	*(++p) = 0;
	*destSize = ++p - dest;
	assert(*destSize == expectedSize);
}

inline unsigned char decodeChar(char c) {
	if(c >= 'a') {
		return c - 'a' + 26;
	} else if(c >= 'A') {
		return c - 'A';
	} else if(c >= '0') {
		return c - '0' + 52;
	} else if(c == '+') {
		return 62;
	} else if(c == '/') {
		return 63;
	} else {
		assert(false);
		throw runtime_error("invalid character passed to decodeChar.");
	}
}

void uudecode(void *dest, size_t *destSize, const char *src) {
	size_t n;
	unsigned char *p;
	size_t remainder;
	size_t expectedSize;
	const char *lastQuartet;

	n = strlen(src);
	p = (unsigned char *)dest - 1;
	--src;
	remainder = n % 4;
	assert(remainder != 1);		// a remainder of one is never valid
	expectedSize = (n / 4) * 3 + (remainder ? remainder - 1 : 0);
	lastQuartet = &src[n - remainder];

	// This is some slightly ugly recycling of variables, but it's to help the compiler keep
	// everything in registers: src, p, lastQuartet, a and b.
	while(src < lastQuartet) {
		unsigned char a, b;
		a = decodeChar(*(++src));
		b = decodeChar(*(++src));
		*(++p) = (a << 2 & 0xfc) | b >> 4;

		a = decodeChar(*(++src));
		*(++p) = (b << 4 & 0xf0) | a >> 2;

		b = decodeChar(*(++src));
		*(++p) = (a << 6 & 0xc0) | b;
	}

	if(remainder > 0) {
		unsigned char a, b;
		a = decodeChar(*(++src));
		b = decodeChar(*(++src));
		*(++p) = (a << 2 & 0xfc) | b >> 4;

		if(remainder == 3) {
			a = decodeChar(*(++src));
			*(++p) = (b << 4 & 0xf0) | a >> 2;
		}
	}
	*destSize = ++p - (unsigned char *)dest;
	assert(*destSize == expectedSize);
}


//finds all filenames like path and stores them in results
void findAll(const string &path, vector<string> &results, bool cutExtension){
	FSFactory *fsfac = FSFactory::getInstance();
	if(fsfac->physFS){
		results.clear();
		vector<string> strings = FSFactory::findAll(path, cutExtension);
		results.insert(results.begin(), strings.begin(), strings.end());
	}else{
	
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
	string result;
	vector<const char *> elements;
	char *buf = strcpy(new char[s.length() + 1], s.c_str());
	char *data;
	bool isAbsolute;

	// empty input gets empty output
	if(!s.length()) {
		return s;
	}

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

bool fileExists(const string &path){
	if(FSFactory::getInstance()->physFS){
		FSFactory::fileExists(path);
	}else{
		
	FILE* file= fopen(path.c_str(), "rb");
	if(file!=NULL){
		fclose(file);
		return true;
	}
	return false;
	
	}
}

}}//end namespace
