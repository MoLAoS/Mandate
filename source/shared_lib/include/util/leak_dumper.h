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

#ifndef _LEAKDUMPER_H_
#define _LEAKDUMPER_H_

#include <map>

using std::map;

// _GAE_LEAK_DUMP_ controls if leak dumping is enabled or not

#ifndef _GAE_LEAK_DUMP_
#	define _GAE_LEAK_DUMP_ 0
#endif

#if _GAE_LEAK_DUMP_

#include <cstdlib>
#include <cstdio>

//including this header in any file of a project will cause all
//leaks to be dumped into leak_dump.txt, but only allocations that
//ocurred in a file where this header is included will have
//file and line number

// =====================================================
// class AllocRegistry
// =====================================================

class AllocRegistry {
private:
	AllocRegistry();
	void dump(const char *path);
	void init();
	static void shutdown();

public:
	~AllocRegistry();
	static AllocRegistry &getInstance();

	void allocate(void* ptr, const char* file, int line, size_t bytes, bool array);
	void deallocate(void* ptr, bool array);
};

//if an allocation ocurrs in a file where "leaks_dumper.h" is not included
//this operator new is called and file and line will be unknown
inline void * operator new(size_t bytes) {
	void *ptr = malloc(bytes);
	AllocRegistry::getInstance().allocate(ptr, "unknown", 0, bytes, false);
	return ptr;
}

inline void operator delete(void *ptr) {
	AllocRegistry::getInstance().deallocate(ptr, false);
	free(ptr);
}

inline void * operator new[](size_t bytes) {
	void *ptr = malloc(bytes);
	AllocRegistry::getInstance().allocate(ptr, "unknown", 0, bytes, true);
	return ptr;
}

inline void operator delete [](void *ptr) {
	AllocRegistry::getInstance().deallocate(ptr, true);
	free(ptr);
}

//if an allocation ocurrs in a file where "leaks_dumper.h" is included
//this operator new is called and file and line will be known
inline void * operator new(size_t bytes, char* file, int line) {
	void *ptr = malloc(bytes);
	AllocRegistry::getInstance().allocate(ptr, file, line, bytes, false);
	return ptr;
}

inline void operator delete(void *ptr, char* file, int line) {
	AllocRegistry::getInstance().deallocate(ptr, false);
	free(ptr);
}

inline void * operator new[](size_t bytes, char* file, int line) {
	void *ptr = malloc(bytes);
	AllocRegistry::getInstance().allocate(ptr, file, line, bytes, true);
	return ptr;
}

inline void operator delete [](void *ptr, char* file, int line) {
	AllocRegistry::getInstance().deallocate(ptr, true);
	free(ptr);
}

#ifdef USE_SSE2_INTRINSICS
//TODO Add _mm_malloc and _mm_free for mesh data  
#endif

#define new new(__FILE__, __LINE__)

#endif // SL_LEAK_DUMP

#endif // _LEAKDUMPER_H_
