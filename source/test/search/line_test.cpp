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

#include "pch.h"
#include <cppunit/extensions/HelperMacros.h>
#include <algorithm>

#include "line_test.h"

using Shared::Util::line;

using std::cout;
using std::endl;
using std::vector;
using std::pair;
using std::make_pair;
using std::for_each;

namespace Test {

#define ADD_TEST(Class, Method) suiteOfTests->addTest( \
			new CppUnit::TestCaller<Class>(#Method, &Class::Method));

CppUnit::Test *LineAlgorithmTest::suite() {
	CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite("LineAlgorithmTest");

	ADD_TEST(LineAlgorithmTest, testNoMirror);
	ADD_TEST(LineAlgorithmTest, testHorizAndVert);
	ADD_TEST(LineAlgorithmTest, testSlope1);
	ADD_TEST(LineAlgorithmTest, testMirroredX);
	ADD_TEST(LineAlgorithmTest, testMirroredY);
	ADD_TEST(LineAlgorithmTest, testMirroredXY);

	return suiteOfTests;
}

typedef pair<int, int> CoOrd;

/** Our visit function object (plugs into the line algorithm) */
struct Visitor {
	vector<CoOrd> &results;
	Visitor(vector<CoOrd> &res) : results(res) {}

	void operator()(int x, int y) {
		results.push_back(make_pair(x, y));
	}
};

/** a function object to output a co-ord */
struct OutputCoOrd {
	void operator()(const CoOrd &c) {
		cout << "(" << c.first << ", " << c.second << ") ";
	}
};

/** function object to test a result sequence 
  * Construct with first CoOrd, then call operator() on remainder of sequence */
struct TestSequence {
	CoOrd last;
	TestSequence(CoOrd start) : last(start) {}

	void operator()(const CoOrd next) {
		int dx = std::abs(next.first - last.first);
		int dy = std::abs(next.second - last.second);
		CPPUNIT_ASSERT(dx < 2 && dy < 2 && (dx || dy));
		last = next;
	}
};

#undef min

/** perform a single line test */
void doTest(int x0, int y0, int x1, int y1) {
	int min_visits = std::min(std::abs(x0 - x1), std::abs(y0 - y1));
	cout << "\nTesting line from " << x0 << ", " << y0 << " to " << x1 << ", " << y1 << "...\n";

	vector<CoOrd> results;
	Visitor visitor(results);
	line(x0, y0, x1, y1, visitor);
	
	OutputCoOrd outputter;
	for_each(results.begin(), results.end(), outputter);

	TestSequence testSeq(*results.begin());
	for_each(results.begin() + 1, results.end(), testSeq);

	CPPUNIT_ASSERT(results.size() >= min_visits);
	CPPUNIT_ASSERT(results.front().first == x0 && results.front().second == y0);
	CPPUNIT_ASSERT(results.back().first == x1 && results.back().second == y1);
}

void LineAlgorithmTest::testNoMirror() {
	// y0 < y1 && x0 < x1
	// Horizontal and Vertical lines have special case code in the line algorithm
	// and will be tested with testHorizAndVert(), the two other code paths to test
	// are for dx < dy and dy < dx
	cout << "\n" << __FUNCTION__ << "()";
	int x0 = 10, y0 = 10, x1 = 15, y1 = 20;
	doTest(x0, y0, x1, y1);

	x1 = 20, y1 = 15;
	doTest(x0, y0, x1, y1);
}

void LineAlgorithmTest::testHorizAndVert() {
	cout << "\n" << __FUNCTION__ << "()";
	int x0 = 10, y0 = 10, x1 = 10, y1 = 20;
	doTest(x0, y0, x1, y1);

	x1 = 20, y1 = 10;
	doTest(x0, y0, x1, y1);
}

void LineAlgorithmTest::testSlope1() {
	cout << "\n" << __FUNCTION__ << "()";
	int x0 = 0, y0 = 0, x1 = 10, y1 = 10;
	doTest(x0, y0, x1, y1);
	doTest(x1, y1, x0, y0);

	x1 = y1 = -10;
	doTest(x0, y0, x1, y1);
	doTest(x1, y1, x0, y0);
}

void LineAlgorithmTest::testMirroredX() {
	cout << "\n" << __FUNCTION__ << "()";
	int x0 = 10, y0 = 10, x1 = 5, y1 = 20;
	doTest(x0, y0, x1, y1);

	x1 = 0, y1 = 15;
	doTest(x0, y0, x1, y1);
}

void LineAlgorithmTest::testMirroredY() {
	cout << "\n" << __FUNCTION__ << "()";
	int x0 = 10, y0 = 10, x1 = 15, y1 = 0;
	doTest(x0, y0, x1, y1);

	x1 = 20, y1 = 5;
	doTest(x0, y0, x1, y1);
}

void LineAlgorithmTest::testMirroredXY() {
	cout << "\n" << __FUNCTION__ << "()";
	int x0 = 10, y0 = 10, x1 = 5, y1 = 0;
	doTest(x0, y0, x1, y1);

	x1 = 0, y1 = 5;
	doTest(x0, y0, x1, y1);
}


} // end namespace Test