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
#ifndef _LINE_ALGORITHM_TEST_HEADER_INCLUDED_
#define _LINE_ALGORITHM_TEST_HEADER_INCLUDED_

#include <cppunit/TestFixture.h>
#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>

#include "line.h"

namespace Test {

// =====================================================
//	class LineAlgorithmTest
// =====================================================

class LineAlgorithmTest : public CppUnit::TestFixture {
public:
	LineAlgorithmTest()		{}
	~LineAlgorithmTest()	{}

	static CppUnit::Test *suite();
	void setUp()	{}
	void tearDown()	{}
	
	void testNoMirror();
	void testHorizAndVert();
	void testSlope1();
	void testMirroredX();
	void testMirroredY();
	void testMirroredXY();
};

}


#endif // !def _LINE_ALGORITHM_TEST_HEADER_INCLUDED_