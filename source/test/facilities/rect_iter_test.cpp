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

#include "pch.h"
#include <cppunit/extensions/HelperMacros.h>
#include "rect_iter_test.h"

#include "leak_dumper.h"

using Shared::Math::Vec2i;

#define ADD_TEST(Class, Method) suiteOfTests->addTest( \
	new CppUnit::TestCaller<Class>(#Method, &Class::Method));

namespace Test {

RectIteratorTest::RectIteratorTest() {	
}

RectIteratorTest::~RectIteratorTest() {
}

CppUnit::Test *RectIteratorTest::suite() {
	CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite("RectIteratorTest");

	ADD_TEST(RectIteratorTest, testBasicOperation)
	ADD_TEST(RectIteratorTest, testOneLiners)
	ADD_TEST(RectIteratorTest, testParamOrder)
	ADD_TEST(RectIteratorTest, testLoneCell)

	return suiteOfTests;
}

void RectIteratorTest::setUp() {
}

void RectIteratorTest::tearDown() {
}

void RectIteratorTest::testBasicOperation() {
	RectIterator ri(Vec2i(20,20), Vec2i(22,22));

	Vec2i knownSequence[9] = {
		Vec2i(20,20), Vec2i (21,20), Vec2i(22,20),
		Vec2i(20,21), Vec2i (21,21), Vec2i(22,21),
		Vec2i(20,22), Vec2i (21,22), Vec2i(22,22)
	};
	int num = 0;
	for ( ; num < 9; ++num ) {
		CPPUNIT_ASSERT(ri.more());
		CPPUNIT_ASSERT(ri.next() == knownSequence[num]);
	}
	CPPUNIT_ASSERT(!ri.more());
}

void RectIteratorTest::testParamOrder() {
	RectIterator ri(Vec2i(20,20), Vec2i(22,22));

	Vec2i knownSequence[9] = {
		Vec2i(20,20), Vec2i (21,20), Vec2i(22,20),
		Vec2i(20,21), Vec2i (21,21), Vec2i(22,21),
		Vec2i(20,22), Vec2i (21,22), Vec2i(22,22)
	};
	int num = 0;
	while (ri.more()) {
		CPPUNIT_ASSERT(ri.next() == knownSequence[num]);
		num++;
		CPPUNIT_ASSERT(num <= 9);
	}
	CPPUNIT_ASSERT(num == 9);
	ri = RectIterator(Vec2i(22,22), Vec2i(20,20));
	num = 0;
	while (ri.more()) {
		CPPUNIT_ASSERT(ri.next() == knownSequence[num]);
		num++;
		CPPUNIT_ASSERT(num <= 9);
	}
	CPPUNIT_ASSERT(num == 9);
	ri = RectIterator(Vec2i(20,22), Vec2i(22,20));
	num = 0;
	while (ri.more()) {
		CPPUNIT_ASSERT(ri.next() == knownSequence[num]);
		num++;
		CPPUNIT_ASSERT(num <= 9);
	}
	CPPUNIT_ASSERT(num == 9);
	ri = RectIterator(Vec2i(22,20), Vec2i(20,22));
	num = 0;
	while (ri.more()) {
		CPPUNIT_ASSERT(ri.next() == knownSequence[num]);
		num++;
		CPPUNIT_ASSERT(num <= 9);
	}
	CPPUNIT_ASSERT(num == 9);
}

void RectIteratorTest::testOneLiners() {
	RectIterator ri(Vec2i(97,63), Vec2i(103,63));

	Vec2i horizontalSequence[7] = {
		Vec2i(97,63), Vec2i(98,63), Vec2i(99,63), Vec2i(100,63), Vec2i(101,63), Vec2i(102,63), Vec2i(103,63)
	};
	int num = 0;
	for ( ; num < 7; ++num ) {
		CPPUNIT_ASSERT(ri.more());
		CPPUNIT_ASSERT(ri.next() == horizontalSequence[num]);
	}
	CPPUNIT_ASSERT(!ri.more());

	ri = RectIterator(Vec2i(125,197), Vec2i(125,193));
	Vec2i verticalSequence[5] = {
		Vec2i(125,193), Vec2i(125,194), Vec2i(125,195), Vec2i(125,196), Vec2i(125,197)
	};
	for (num = 0; num < 5; ++num) {
		CPPUNIT_ASSERT(ri.more());
		CPPUNIT_ASSERT(ri.next() == verticalSequence[num]);
	}
	CPPUNIT_ASSERT(!ri.more());
}

void RectIteratorTest::testLoneCell() {
	Vec2i loneCell(194,47);
	RectIterator ri(loneCell, loneCell);
	// sequence should be (194,47)
	CPPUNIT_ASSERT(ri.more());
	CPPUNIT_ASSERT(ri.next() == loneCell);
	CPPUNIT_ASSERT(!ri.more());
}

} // end namespace
