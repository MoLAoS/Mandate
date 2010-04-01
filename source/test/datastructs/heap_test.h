// ==============================================================
//	This file is part of the Glest Advanced Engine
//
//	Copyright (C) 2010 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _TEST_MIN_HEAP_H_
#define _TEST_MIN_HEAP_H_

#include <cppunit/TestFixture.h>
#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>

#include "heap.h"

using Shared::Util::MinHeap;

namespace Test {

// =====================================================
//	class MinHeapTest
// =====================================================

class MinHeapTest : public CppUnit::TestFixture {
public:
	MinHeapTest()	{}
	~MinHeapTest()	{}

	static CppUnit::Test *suite();
	void setUp()	{}
	void tearDown()	{}
	
	void testHeapify();
	void testPromote();

};

}

#endif //_TEST_MIN_HEAP_H_