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

#include "pch.h"
#include "leak_dumper.h"
#include "FSFactory.hpp"

#if _GAE_LEAK_DUMP_

using namespace Shared::PhysFS;

// =====================================================
//	class AllocRegistry
// =====================================================

struct AllocInfo {
	size_t line	: 30;
	size_t inUse : 1;
	size_t array : 1;	// 4
	const char *file;	// 8
	size_t bytes;		// 12
	void *ptr;			// 16
};

const size_t allocTableSize = 32768; // number of AllocInfo in each table
const size_t tableSize = sizeof(AllocInfo) * allocTableSize; // size of each table
const size_t tableCount = 16; // number of tables

AllocInfo* dataTables[tableCount]; // the info tables

size_t	s_allocCount = 0;		// total allocation count
size_t	s_allocBytes = 0;		// total bytes allocated
size_t	s_currentAlloc = 0;		// managed bytes currently allocated
size_t	s_unmanagedAlloc = 0;

bool	s_running = false;	// logging allocations

AllocRegistry *s_instance = 0;

AllocRegistry &AllocRegistry::getInstance(){
	if (!s_instance) {
		s_instance = (AllocRegistry*)malloc(sizeof(AllocRegistry));
		s_instance->init();
	}
	return *s_instance;
}

AllocRegistry::AllocRegistry() {}

void AllocRegistry::init() {
	// begin hokey pokey...
	// need FSFactory to init and register its shutdown before we do.
	// this will call new which will end up back here, s_instance now points to something
	// but this is ok, because s_running is false, so it wont blow anything up

	// delete old log (not really needed, but ensures FSFactory is constructed first)
	if (FSFactory::getInstance()->fileExists("leak_dump.log")) {
		FSFactory::getInstance()->removeFile("leak_dump.log");
	}
	for (int i=0; i < tableCount; ++i) {
		dataTables[i] = (AllocInfo*)malloc(tableSize);
		memset(dataTables[i], 0, tableSize);
	}
	s_running = true;
	// important: FSFactory::shutdown should be registered first, so as to be called last
	// we need FSFactory still running in our shutdown
	atexit(&AllocRegistry::shutdown);
}

void AllocRegistry::shutdown() {
	delete s_instance;
}

AllocRegistry::~AllocRegistry() {
	dump("leak_dump.log");
	s_running = false;
	for (int i=0; i < tableCount; ++i) {
		free(dataTables[i]);
	}
}

// just to make sure we do it the same...
size_t hashPointer(void *ptr) {
	return ((size_t(ptr) >> 4) % allocTableSize);
}

void AllocRegistry::allocate(void* ptr, const char* file, int line, size_t bytes, bool array) {
	if (!ptr || !s_running) return;
	size_t hash = hashPointer(ptr);
	bool stored = false;
	for (int i=0; i < tableCount; ++i) {
		if (!dataTables[i][hash].inUse) {
			dataTables[i][hash].inUse = 1;
			dataTables[i][hash].ptr = ptr;
			dataTables[i][hash].file = file;
			dataTables[i][hash].line = line;
			dataTables[i][hash].bytes = bytes;
			dataTables[i][hash].array = array ? 1 : 0;
			stored = true;
			break;
		}
	}
	++s_allocCount;
	if (!stored) {
		s_unmanagedAlloc += bytes;
	} else {
		s_currentAlloc += bytes;
	}
	s_allocBytes += bytes;
}

void AllocRegistry::deallocate(void* ptr, bool array) {
	if (!ptr || !s_running) return;
	size_t hash = hashPointer(ptr);
	for (int i=0; i < tableCount; ++i) {
		if (dataTables[i][hash].inUse && dataTables[i][hash].ptr == ptr) {
			s_currentAlloc -= dataTables[i][hash].bytes;
			memset(&dataTables[i][hash], 0, sizeof(AllocInfo));
			return;
		}
	}
}

void AllocRegistry::dump(const char *path) {
	s_running = false;
	int leakCount=0;
	size_t leakBytes=0;

	std::stringstream ss;
	ss << "Memory leak dump\n\n";

	for (int i=0; i < tableCount; ++i) {
		for (int j=0; j < allocTableSize; ++j) {
			if (dataTables[i][j].inUse) {
				AllocInfo &inf = dataTables[i][j];
				leakBytes += inf.bytes;
				ss << ++leakCount << ".  file: " << inf.file << ", line: " << inf.line
					<< ", bytes: " << inf.bytes << ", array: " << (inf.array ? "true" : "false") << "\n";
			}
		}
	}

	ss << "\nTotal leaks: " << leakCount << ", " << leakBytes << " bytes.\n";
	ss << "Manged memory not freed: " << s_currentAlloc << " bytes.\n";
	ss << "Unmanaged memory allocated: " << s_unmanagedAlloc << " bytes.\n";
	ss << "Total allocations: " << s_allocCount << ", " << s_allocBytes << " bytes.\n";

	FileOps *f = FSFactory::getInstance()->getFileOps();
	f->openWrite(path);
	f->write(ss.str().c_str(), ss.str().size(), 1);
	f->close();
	delete f;
}

#endif // SL_LEAK_DUMP
