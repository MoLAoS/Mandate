// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _TEST_CIRCULAR_BUFFER_H_
#define _TEST_CIRCULAR_BUFFER_H_

#include <cppunit/TestFixture.h>
#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>

#include "socket.h"

using Shared::Platform::Socket;

namespace Test {

// =====================================================
//	class AnnotatedMapTest
// =====================================================

class CircularBufferTest : public CppUnit::TestFixture {
private:
	const char *alpha_data;
	size_t alpha_size;
	const char *num_data;
	size_t num_size;
	const char *alpha_num_data;
	size_t alpha_num_size;
	const char *hex_data;
	size_t hex_size;

public:
	CircularBufferTest();
	~CircularBufferTest();

	static CppUnit::Test *suite();
	void setUp();
	void tearDown();
	
	void testBasicOps();
	void testRollOver();
	void testPerfectFill();
	void testPeek();
};

} // end namespace Test

#endif // _TEST_CIRCULAR_BUFFER_H_
