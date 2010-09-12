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
#include "leak_dumper.h"

#ifdef SL_LEAK_DUMP

AllocInfo::AllocInfo() :
		line(-1),
		file(""),
		bytes(-1),
		ptr(NULL),
		free(true),
		array(false) {
}

AllocInfo::AllocInfo(void* ptr, const char* file, int line, size_t bytes, bool array) :
		line(line),
		file(file),
		bytes(bytes),
		ptr(ptr),
		free(false),
		array(array) {
}

// =====================================================
//	class AllocRegistry
// =====================================================

// ===================== PRIVATE =======================

AllocRegistry::AllocRegistry() {
	allocCount= 0;
	allocBytes= 0;
	nonMonitoredCount= 0;
	nonMonitoredBytes= 0;
}

// ===================== PUBLIC ========================

AllocRegistry &AllocRegistry::getInstance(){
	static AllocRegistry allocRegistry;
	return allocRegistry;
}

AllocRegistry::~AllocRegistry(){
	dump("leak_dump.log");
}

void AllocRegistry::allocate(AllocInfo info){
	++allocCount;
	allocBytes+= info.bytes;
	unsigned hashCode= static_cast<unsigned>(reinterpret_cast<intptr_t>(info.ptr) % maxAllocs);

	for(unsigned i=hashCode; i<maxAllocs; ++i){
		if(allocs[i].free){
			allocs[i]= info;
			return;
		}
	}
	for(unsigned i=0; i<hashCode; ++i){
		if(allocs[i].free){
			allocs[i]= info;
			return;
		}
	}
	++nonMonitoredCount;
	nonMonitoredBytes+= info.bytes;
}

void AllocRegistry::deallocate(void* ptr, bool array){
	unsigned hashCode= static_cast<unsigned>(reinterpret_cast<intptr_t>(info.ptr) % maxAllocs);

	for(int i=hashCode; i<maxAllocs; ++i){
		if(!allocs[i].free && allocs[i].ptr==ptr && allocs[i].array==array){
			allocs[i].free= true;
			return;
		}
	}
	for(int i=0; i<hashCode; ++i){
		if(!allocs[i].free && allocs[i].ptr==ptr && allocs[i].array==array){
			allocs[i].free= true;
			return;
		}
	}
}

void AllocRegistry::reset(){
	for(int i=0; i<maxAllocs; ++i){
		allocs[i]= AllocInfo();
	}
}

void AllocRegistry::dump(const char *path){
	FILE *f= fopen(path, "wt");
	int leakCount=0;
	size_t leakBytes=0;

	fprintf(f, "Memory leak dump\n\n");

	for(int i=0; i<maxAllocs; ++i){
		if(!allocs[i].free){
			leakBytes+= allocs[i].bytes;
			fprintf(f, "%d.\tfile: %s, line: %d, bytes: %d, array: %d\n", ++leakCount, allocs[i].file, allocs[i].line, allocs[i].bytes, allocs[i].array);
		}
	}

	fprintf(f, "\nTotal leaks: %d, %d bytes\n", leakCount, leakBytes);
	fprintf(f, "Total allocations: %d, %d bytes\n", allocCount, allocBytes);
	fprintf(f, "Not monitored allocations: %d, %d bytes\n", nonMonitoredCount, nonMonitoredBytes);

	fclose(f);
}

#endif // SL_LEAK_DUMP
