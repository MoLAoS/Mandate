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
#include "fixed_point_test.h"

#include "conversion.h"

using Shared::Util::Conversion::toStr;

#include "leak_dumper.h"

using Shared::Math::fixed;
using std::cout;
using std::endl;

namespace Test {

#define ADD_TEST(Class, Method) suiteOfTests->addTest( \
	new CppUnit::TestCaller<Class>(#Method, &Class::Method));

CppUnit::Test *FixedPointTest::suite() {
	CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite("FixedPointTest");
	ADD_TEST(FixedPointTest, testAddSubtract);
	ADD_TEST(FixedPointTest, testMultDiv);
	ADD_TEST(FixedPointTest, testNeg);
	ADD_TEST(FixedPointTest, testSqRt);
	ADD_TEST(FixedPointTest, testPrecision);

	return suiteOfTests;
}

void FixedPointTest::setUp() {
	for (int i=0; i <= 100; ++i) {
		numbers[i] = i;
	}
}

///@todo lots. This is a mess, needs organising and some proper test conditions for
/// known to not be precise results.

void FixedPointTest::testAddSubtract() {
	// basic addition/substraction
	fixed three = numbers[1] + numbers[2];	// basic fixed additions
	CPPUNIT_ASSERT(three == 3);
	CPPUNIT_ASSERT(three.intp() == 3);
	CPPUNIT_ASSERT(three.frac() == 0);

	fixed seven = numbers[4] + three;
	CPPUNIT_ASSERT(seven == numbers[7]);
	CPPUNIT_ASSERT(seven == 7);
	
	fixed fix13 = seven + numbers[6];
	CPPUNIT_ASSERT(fix13 == numbers[13]);
	CPPUNIT_ASSERT(fix13 == 13);

	fixed fix3 = numbers[1] + numbers[1];
	fix3 += numbers[1];				// increment-and-assign
	CPPUNIT_ASSERT(fix3 == 3);
	CPPUNIT_ASSERT(fix3.intp() == 3);
	CPPUNIT_ASSERT(fix3.frac() == 0);
	CPPUNIT_ASSERT(three == fix3);

	fixed fix27 = fix13 + 14;	// auto 'promote' int to fixed
	CPPUNIT_ASSERT(fix27 == 27);
	fixed fix31 = 18 + fix13;	// global add operator
	CPPUNIT_ASSERT(fix31 == numbers[31]);
	CPPUNIT_ASSERT(fix31 == 31);
	CPPUNIT_ASSERT(fix31 == fix27 + numbers[4]);

	fixed fix29 = fix31 - numbers[2];	// basic substractions
	CPPUNIT_ASSERT(fix29 == 29);


	fixed fix16 = fix29 - fix13;
	CPPUNIT_ASSERT(fix16 == 16);

	fixed fix2 = fix16 - numbers[7] - 7;
	CPPUNIT_ASSERT(fix2 == 2);

	///@todo test global subtract operator & decrement-and-assign operator, test big numbers
	/// to demonstate range limitions (ie, cause overflow)
}

void FixedPointTest::testMultDiv() {
	// multiplication/division
	fixed twoTimesFour = numbers[2] * numbers[4];	// basic multiplications
	CPPUNIT_ASSERT(twoTimesFour == 8);

	fixed threeTimesTwo = numbers[3] * numbers[2];
	CPPUNIT_ASSERT(threeTimesTwo == numbers[6]);
	CPPUNIT_ASSERT(threeTimesTwo == 6);

	fixed twoTimesThree = numbers[2] * numbers[3];
	CPPUNIT_ASSERT(twoTimesThree == threeTimesTwo);

	fixed half = numbers[1] / numbers[2];			// test some fractional ops
	CPPUNIT_ASSERT(half < numbers[1]);
	CPPUNIT_ASSERT(half > numbers[0]);
	CPPUNIT_ASSERT(half < 1);
	CPPUNIT_ASSERT(half > 0);

	fixed quarter1 = half * half;
	fixed quarter2 = numbers[1] / numbers[4];
	fixed threeQuarters = quarter1 * 3;
	fixed quarter3 = numbers[1] - threeQuarters;
	fixed quarter4 = 1024 / fixed(4096);

	// 4 == power of two, so these should all be identical (no chance of error creap)
	CPPUNIT_ASSERT(quarter1 == quarter2);
	CPPUNIT_ASSERT(quarter1 == quarter3);
	CPPUNIT_ASSERT(quarter1 == quarter4);

	fixed tmp = quarter1 * 3;
	CPPUNIT_ASSERT(tmp < 1);
	CPPUNIT_ASSERT(tmp > half);
	CPPUNIT_ASSERT(quarter1 * 4 == 1);

	// == 3 is not, there will be error
	fixed third = numbers[1] / 3;
	CPPUNIT_ASSERT(third > quarter1 && third < half);

	fixed sixth = numbers[1] / numbers[6];
	fixed oneSixthTimesTwo = sixth * 2;
	cout << "\none third = " << third << " [== " << toStr(third) << "]";
	cout << "\none sixth = " << sixth << " [== " << toStr(sixth) << "]";
	cout << "\ntwo sixths = " << oneSixthTimesTwo << " [== " << toStr(oneSixthTimesTwo) << "]";
	
	///@todo test within some error bound ... (determine error bound?)

	///@todo test multiply-and-assign, test divide-and-assign, 

	tmp = fixed(40) / 10;
	CPPUNIT_ASSERT(tmp == 4);
	tmp /= 4;
	CPPUNIT_ASSERT(tmp == 1);
}

void FixedPointTest::testNeg() {
	fixed neg_one = -numbers[1];	// operator-()
	fixed tmp = -1;			// operator=(int)
	fixed half = numbers[1] / numbers[2];
	CPPUNIT_ASSERT(neg_one == -1);
	CPPUNIT_ASSERT(neg_one == tmp);
	CPPUNIT_ASSERT(neg_one * neg_one == 1);
	CPPUNIT_ASSERT(neg_one * numbers[7] == -7);
	CPPUNIT_ASSERT(neg_one * half < 0);
	CPPUNIT_ASSERT(neg_one * half > -1);

	cout << "\n - 1/3 == " << -(fixed(1)/fixed(3));

	tmp = fixed(123) / 13;
	float tmpFloat = float(123) / 13;
	cout << "\nfixed fixedTmp = fixed(123) / 13 == " << tmp;
	cout << "\nfloat floatTmp = float(123) / 13 == " << tmpFloat;
	cout << "\nfixedTmp.toFloat() == " << tmp.toFloat();

	tmp = 123 / -fixed(13);
	tmpFloat = 123 / -float(13);
	cout << "\nfixed fixedTmp = 123 / -fixed(13) == " << tmp;
	cout << "\nfloat floatTmp = 123 / -float(13) == " << tmpFloat;
	cout << "\nfixedTmp.toFloat() == " << tmp.toFloat();

	tmp = fixed(-123) / 13;
	tmpFloat = float(-123) / 13;
	cout << "\nfixed fixedTmp = fixed(-123) / 13 == " << tmp;
	cout << "\nfloat floatTmp = float(-123) / 13 == " << tmpFloat;
	cout << "\nfixedTmp.toFloat() == " << tmp.toFloat();

	cout << endl << fixed(-13) << " + " << fixed(1) << " == " << fixed(-13) + fixed(1);
	cout << endl << fixed(-13) << " + " << half << " == " << fixed(-13) + half;
	cout << endl << fixed(-13) << " - " << half << " == " << fixed(-13) - half;
}

void FixedPointTest::testSqRt() {
	fixed rt4 = numbers[4].sqRt();
	cout << "\nsqrt(4) == " << rt4;

	fixed rt9 = fixed(9).sqRt();
	cout << "\nsqrt(9) == " << rt9 << " .toFloat() == " << rt9.toFloat();

	fixed fix8156 = 8156;
	fixed rt8156 = fix8156.sqRt();
	cout << "\n\nsqrt(8156) == " << rt8156;
	cout << "\nsquare(" << rt8156 << ") ==  " << (rt8156 * rt8156);
	//CPPUNIT_ASSERT( (rt8156 * rt8156) is 'close' to 8156 )

	Vec2i A(13,17), B(127,231); // dist == 242.47061677654882395517138458922
	fixed tmp = fixedDist(A, B);
	cout << "\nA:" << A << ", B:" << B;
	cout << "\nA.dist(B) == " << A.dist(B); // ref
	cout << "\nfixedDist(A, B) == " << tmp;
	//CPPUNIT_ASSERT( (fixedDist(A, B) is 'close' to 242.5 )
	cout << "\nfixedDist(A,B).toFloat() = " << tmp.toFloat();

	A = Vec2i(0,0); B = Vec2i(1,1);
	fixed res =  fixedDist(A, B);
	cout << "\nA = (0,0), B = " << B << ", dist = " << fixedDist(A, B);

	B = Vec2i(2,1);
	cout << "\nA = (0,0), B = " << B << ", dist = " << fixedDist(A, B);

	B = Vec2i(2,2);
	cout << "\nA = (0,0), B = " << B << ", dist = " << fixedDist(A, B);

	///@todo test stuff
}

void FixedPointTest::testPrecision() {
	fixed half = numbers[1] / numbers[2];
	fixed tmp = fixed(1) / 2;
	cout << "\n" << tmp << ".round() == " << tmp.round();
	tmp.raw() -= 1;
	cout << "\n" << tmp << ".round() == " << tmp.round();
	tmp.raw() += 2;
	cout << "\n" << tmp << ".round() == " << tmp.round();

	tmp = fixed(123) / -13;
	cout << "\n" << tmp << ".round() == " << tmp.round();

	cout << endl << tmp << " - " << half << " == " << tmp - half;

	tmp -= half;
	cout << "\n" << tmp << ".round() == " << tmp.round();

	cout << "\nfixed::max_int() == " << fixed::max_int();
	cout << "\nfixed::min_int() == " << fixed::min_int();

	fixed big = 523503;
	cout << "\n\nMultiplying very_big by very_big, should give nonsense result,";
	fixed wrong = big * big;
	cout << "\nbig = " << big << ", big * big == " << wrong;

	//
	// Divide by zero & division precision loss...
	//
	// NOTE: These 'tests' (demonstrations?) assume 12 bits to the right of the binary point
	//
	cout << "\n\nTest div by very small...";

	// just plain wrong,
	fixed very_small = 0;
	cout << "\n\nAttempting divide by zero, ";
	fixed div_res = 10 / very_small;
	cout << "\n10 / " << very_small << " == " << div_res;

	// a fractional part of 63/4096 (or less) should cause a divide by zero error (due to precision loss)
	very_small = fixed(63) / 4096;
	cout << "\n\nAttempting divide by very_small, (precision loss would cause div by 0)";
	div_res = 10 / very_small;
	cout << "\n10 / " << very_small << " == " << div_res;

	// but fractional part of 64/4096 should be OK.
	very_small = fixed(64) / 4096;
	cout << "\n\nAttempting divide by very_small, should be OK";
	div_res = 10 / very_small;
	cout << "\n10 / " << very_small << " == " << div_res;

	// ... fractional part of 1 + 63 / 4096 should be ok, but cuase loss of precision
	very_small = 1 + fixed(63) / 4096;
	cout << "\n\nAttempting divide by very_small, should be OK but lose precision";
	div_res = 10 / very_small;
	cout << "\n10 / " << very_small << " == " << div_res;

	cout << "\n\n";

	cout << "\nfixed(-27) / fixed(2) == " << fixed(-27) / fixed(2);
	cout << "\n(fixed(-27) / fixed(2)).intp() == " << (fixed(-27) / fixed(2)).intp();

	cout << "\nfixed(1) - half - half = " << fixed(1) - half - half;
	cout << "\nfixed(1) + half + half = " << fixed(1) + half + half;
	cout << "\nfixed(1) + half + -half = " << fixed(1) + half + -half;
}

} // end namespace Test
	
