// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "checksum.h"

#include <cassert>
#include <stdexcept>

#include "util.h"

#include "leak_dumper.h"


using namespace std;

namespace Shared{ namespace Util{

// =====================================================
//	class Checksum
// =====================================================

Checksum::Checksum(){
	sum= 0;
	r= 55665;
	c1= 52845;
	c2= 22719;
}

void Checksum::addByte(int8 value){
	int32 cipher= (value ^ (r >> 8));

	r= (cipher + r) * c1 + c2;
	sum+= cipher;
}

void Checksum::addString(const string &value){
	for(int i= 0; i<value.size(); ++i){
		addByte(value[i]);
	}
}

void Checksum::addFile(const string &path, bool text){

	FILE* file = fopen(path.c_str(), "rb");

	if(!file) {
		throw runtime_error("Can not open file: " + path);
	}

	addString(basename(path));

	while(!feof(file)) {
		int8 byte= 0;

		fread(&byte, 1, 1, file);
		if(text && !isprint(byte)) {
			continue;
		}
		addByte(byte);
	}

	fclose(file);
}

}}//end namespace
