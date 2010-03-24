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
#include "reverse_rect_iter_test.h"

#include "leak_dumper.h"

using Shared::Math::Vec2i;

namespace Test {

ReverseRectIteratorTest::ReverseRectIteratorTest() {	
}

ReverseRectIteratorTest::~ReverseRectIteratorTest() {
}

CppUnit::Test *ReverseRectIteratorTest::suite() {
	CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite("ReverseRectIteratorTest");
	suiteOfTests->addTest(
		new CppUnit::TestCaller<ReverseRectIteratorTest>( "testBasicOperation",
									&ReverseRectIteratorTest::testBasicOperation)
	);
	suiteOfTests->addTest(
		new CppUnit::TestCaller<ReverseRectIteratorTest>( "testOneLiners",
									&ReverseRectIteratorTest::testOneLiners)
	);
	suiteOfTests->addTest(
		new CppUnit::TestCaller<ReverseRectIteratorTest>( "testParamOrder",
									&ReverseRectIteratorTest::testParamOrder)
	);
	suiteOfTests->addTest(
		new CppUnit::TestCaller<ReverseRectIteratorTest>( "testLoneCell",
									&ReverseRectIteratorTest::testLoneCell)
	);
	return suiteOfTests;
}

void ReverseRectIteratorTest::setUp() {
}

void ReverseRectIteratorTest::tearDown() {
}

void ReverseRectIteratorTest::testBasicOperation() {
	ReverseRectIterator rri(Vec2i(20,20), Vec2i(22,22));
	// sequence should be (22,22), (21,22), (20,22), (22,21), (21,21), (20,21), (22,20), (21,20), (20,20)
	Vec2i knownSequence[9] = {
		Vec2i(22,22), Vec2i (21,22), Vec2i(20,22),
		Vec2i(22,21), Vec2i (21,21), Vec2i(20,21),
		Vec2i(22,20), Vec2i (21,20), Vec2i(20,20)
	};
	int num = 0;
	for ( ; num < 9; ++num ) {
		CPPUNIT_ASSERT(rri.more());
		CPPUNIT_ASSERT(rri.next() == knownSequence[num]);
	}
	CPPUNIT_ASSERT(!rri.more());
}

void ReverseRectIteratorTest::testParamOrder() {
	ReverseRectIterator rri(Vec2i(20,20), Vec2i(22,22));
	// sequence should be (22,22), (21,22), (20,22), (22,21), (21,21), (20,21), (22,20), (21,20), (20,20)
	Vec2i knownSequence[9] = {
		Vec2i(22,22), Vec2i (21,22), Vec2i(20,22),
		Vec2i(22,21), Vec2i (21,21), Vec2i(20,21),
		Vec2i(22,20), Vec2i (21,20), Vec2i(20,20)
	};
	int num = 0;
	while ( rri.more() ) {
		CPPUNIT_ASSERT(rri.next() == knownSequence[num]);
		num++;
	}
	CPPUNIT_ASSERT(num == 9);
	rri = ReverseRectIterator(Vec2i(22,22), Vec2i(20,20));
	num = 0;
	while ( rri.more() ) {
		CPPUNIT_ASSERT(rri.next() == knownSequence[num]);
		num++;
	}
	CPPUNIT_ASSERT(num == 9);
	rri = ReverseRectIterator(Vec2i(20,22), Vec2i(22,20));
	num = 0;
	while ( rri.more() ) {
		CPPUNIT_ASSERT(rri.next() == knownSequence[num]);
		num++;
	}
	CPPUNIT_ASSERT(num == 9);
	rri = ReverseRectIterator(Vec2i(22,20), Vec2i(20,22));
	num = 0;
	while ( rri.more() ) {
		CPPUNIT_ASSERT(rri.next() == knownSequence[num]);
		num++;
	}
	CPPUNIT_ASSERT(num == 9);
}

void ReverseRectIteratorTest::testOneLiners() {
	ReverseRectIterator rri(Vec2i(97,63), Vec2i(103,63));
	// sequence should be (103,63), (102,63), (101,63), (100,63), (99,63), (98,63), (97,63)
	Vec2i horizontalSequence[7] = {
		Vec2i(103,63), Vec2i(102,63), Vec2i(101,63), Vec2i(100,63), 
		Vec2i(99,63), Vec2i(98,63), Vec2i(97,63)
	};
	int num = 0;
	for ( ; num < 7; ++num ) {
		CPPUNIT_ASSERT(rri.more());
		CPPUNIT_ASSERT(rri.next() == horizontalSequence[num]);
	}
	CPPUNIT_ASSERT(!rri.more());

	rri = ReverseRectIterator(Vec2i(125,197), Vec2i(125,193));
	// sequence should be (125,197), (125,196), (125,195), (125,194), (125,193)
	Vec2i verticalSequence[5] = {
		Vec2i(125,197), Vec2i(125,196), Vec2i(125,195), Vec2i(125,194), Vec2i(125,193)
	};
	for ( num = 0; num < 5; ++num ) {
		CPPUNIT_ASSERT(rri.more());
		CPPUNIT_ASSERT(rri.next() == verticalSequence[num]);
	}
	CPPUNIT_ASSERT(!rri.more());
}

void ReverseRectIteratorTest::testLoneCell() {
	Vec2i loneCell(194,47);
	ReverseRectIterator rri(loneCell, loneCell);
	// sequence should be (194,47)
	CPPUNIT_ASSERT(rri.more());
	CPPUNIT_ASSERT(rri.next() == loneCell);
	CPPUNIT_ASSERT(!rri.more());
}

} // end namespace
