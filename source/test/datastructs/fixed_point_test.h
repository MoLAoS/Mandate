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

#ifndef _TEST_FIXED_POINT_H_
#define _TEST_FIXED_POINT_H_

#include <cppunit/TestFixture.h>
#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>

#define FIXED_WARN_ON_OVERFLOW 1
#define FIXED_WARN_ON_PRECISION_LOSS 1
#define FIXED_WARN_ON_DIVIDE_BY_ZERO 1
#include "fixed.h"

using Shared::Math::fixed;

namespace Test {

// =====================================================
//	class FixedPointTest
// =====================================================

class FixedPointTest : public CppUnit::TestFixture {
private:
	fixed numbers[101]; // 0 - 100

public:
	FixedPointTest()	{}
	~FixedPointTest()	{}

	static CppUnit::Test *suite();
	void setUp();
	void tearDown()		{}
	
	void testAddSubtract();
	void testMultDiv();
	void testNeg();
	void testSqRt();
	void testPrecision();
};

}

#endif //_TEST_FIXED_POINT_H_