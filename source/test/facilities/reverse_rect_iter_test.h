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

#ifndef _TEST_REVERSE_RECT_ITERATOR_H_
#define _TEST_REVERSE_RECT_ITERATOR_H_

#include <cppunit/TestFixture.h>
#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>

#include "pos_iterator.h"

using Glest::Game::Util::ReverseRectIterator;

namespace Test {

// =====================================================
//	class AnnotatedMapTest
// =====================================================

class ReverseRectIteratorTest : public CppUnit::TestFixture {
private:

public:
	ReverseRectIteratorTest();
	~ReverseRectIteratorTest();

	static CppUnit::Test *suite();
	void setUp();
	void tearDown();
	
	void testBasicOperation();
	void testParamOrder();
	void testOneLiners();
	void testLoneCell();
};

} // end namespace

#endif // _TEST_REVERSE_RECT_ITERATOR_H_
