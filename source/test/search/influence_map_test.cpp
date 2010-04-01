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

#include "influence_map_test.h"

#include "leak_dumper.h"

namespace Test {

InfluenceMapTest::InfluenceMapTest() {
}

InfluenceMapTest::~InfluenceMapTest() {
}

#define ADD_TEST(Class, Method) suiteOfTests->addTest( \
	new CppUnit::TestCaller<Class>(#Method, &Class::Method));

CppUnit::Test *InfluenceMapTest::suite() {
	CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite("InfluenceMapTest");
	ADD_TEST(InfluenceMapTest, testSquareTypeMap);
	ADD_TEST(InfluenceMapTest, testRectPatchMap);
	ADD_TEST(InfluenceMapTest, testRectFlowMap);
	ADD_TEST(InfluenceMapTest, testPatchMap1);
	ADD_TEST(InfluenceMapTest, testPatchMap2);
	ADD_TEST(InfluenceMapTest, testPatchMap3);
	ADD_TEST(InfluenceMapTest, testPatchMap4);
	ADD_TEST(InfluenceMapTest, testPatchMap5);
	ADD_TEST(InfluenceMapTest, testPatchMap6);
	return suiteOfTests;
}

void InfluenceMapTest::setUp() {
}

void InfluenceMapTest::tearDown() {
}

#define TEST_SET(p,pv,nv)						\
	CPPUNIT_ASSERT(iMap.getInfluence(p)==pv);	\
	CPPUNIT_ASSERT(iMap.setInfluence(p,nv));	\
	CPPUNIT_ASSERT(iMap.getInfluence(p)==nv);	

#define TEST_SET_CLAMPED(p,pv,sv,nv)			\
	CPPUNIT_ASSERT(iMap.getInfluence(p)==pv);	\
	CPPUNIT_ASSERT(iMap.setInfluence(p,sv));	\
	CPPUNIT_ASSERT(iMap.getInfluence(p)==nv);	

#define TEST_NOT_SET(p,pv,nv)					\
	CPPUNIT_ASSERT(iMap.getInfluence(p)==pv);	\
	CPPUNIT_ASSERT(!iMap.setInfluence(p,nv));	\
	CPPUNIT_ASSERT(iMap.getInfluence(p)==pv);	

#define TEST_SET_AND_RESET(p,pv,nv)				\
	CPPUNIT_ASSERT(iMap.getInfluence(p)==pv);	\
	CPPUNIT_ASSERT(iMap.setInfluence(p,nv));	\
	CPPUNIT_ASSERT(iMap.getInfluence(p)==nv);	\
	CPPUNIT_ASSERT(iMap.setInfluence(p,pv));	\
	CPPUNIT_ASSERT(iMap.getInfluence(p)==pv);

#define PairPF(x,y,v) pair<Point,float>(Point(x,y),v)

void InfluenceMapTest::testSquareTypeMap () {
	// Tests default values and set values around corner points of a square
	// TypeMap (with type == float), and diagonally through it, and miles out 
	// of it.
	Glest::Game::Search::Rectangle bounds(64,64, 64,64); // rect from points 64,64 -> 127,127 (inclusive)

	// init float map, set 'default' return val to 100.1
	TypeMap<float> iMap(bounds, 100.1f);
	// fill the iMap with some pretty numbers (0 in the middle, 100 at edges, interpolated inbetween)
	for ( int y = bounds.y; y < bounds.y + bounds.h; ++y ) {
		for ( int x = bounds.x; x < bounds.x + bounds.w; ++x ) {
			int d = max(abs(x - bounds.x - bounds.w / 2),abs(y - bounds.y - bounds.h / 2));
			CPPUNIT_ASSERT(iMap.setInfluence(Point(x,y), 100.f * ( d / ((bounds.w + bounds.h) / 4.f) )));
		}
	}
	// test bounds... [square map]
	// corner points, array for each corner, containing 4 points, the first three around
	// the corner but not on the influence map, the last just inside the influence map
	Point cornerPts[4][4] = {
		{ Point(63,63), Point(64,63), Point(63,64), Point(64,64) },			// NW
		{ Point(63,128), Point(64,128), Point(63,127), Point(64,127) },		// SW
		{ Point(128,63), Point(127,63), Point(128,64), Point(127,64) },		// NE
		{ Point(128,128), Point(127,128), Point(128,127), Point(127,127) }	// SE
	};
	// test around corner points...
	for ( int i = 0; i < 4; ++i ) {
		for ( int j = 0; j < 3; ++j ) {
			CPPUNIT_ASSERT(iMap.getInfluence(cornerPts[i][j]) == 100.1f); // default
		}
		CPPUNIT_ASSERT(iMap.getInfluence(cornerPts[i][3]) < 100.1f); // on map, less than 100.1
	}
	// test setting around and in corner points
	for ( int i = 0; i < 4; ++i ) {
		for ( int j = 0; j < 3; ++j ) {
			CPPUNIT_ASSERT(!iMap.setInfluence(cornerPts[i][j], 999.9f)); // return false
			CPPUNIT_ASSERT(iMap.getInfluence(cornerPts[i][j]) == 100.1f); // default, not 999.9f
		}
		CPPUNIT_ASSERT(iMap.setInfluence(cornerPts[i][3], 999.9f)); // return true
		CPPUNIT_ASSERT(iMap.getInfluence(cornerPts[i][3]) == 999.9f); // on map, 999.9f
	}

	// some nonsense co-ords, shoud all be default ( == 100.1f )
	CPPUNIT_ASSERT(iMap.getInfluence(Point(123456789,987654321)) == 100.1f);
	CPPUNIT_ASSERT(iMap.getInfluence(Point(-72,-93)) == 100.1f);
	CPPUNIT_ASSERT(iMap.getInfluence(Point(257,-56)) == 100.1f);
	CPPUNIT_ASSERT(iMap.getInfluence(Point(-47,68)) == 100.1f);

	// some known values from the initialisation above
	pair<Point,float> knownVals [] = {
		PairPF( 66, 66, 93.75f),	PairPF( 69, 69, 84.375f),
		PairPF( 72, 72, 75.f),		PairPF( 75, 75, 65.625f),
		PairPF( 78, 78, 56.25f),	PairPF( 87, 87, 28.125f),
		PairPF( 96, 96, 0.f),		PairPF( 97, 97, 3.125f),
		PairPF(100,100, 12.5f),		PairPF(103,103, 21.875f),
		PairPF(109,109, 40.625f),	PairPF(115,115, 59.375f),
		PairPF(121,121, 78.125f)
	};
	for ( int i = 0; i < 13; ++i ) {
		CPPUNIT_ASSERT(iMap.getInfluence(knownVals[i].first) == knownVals[i].second);
	}
}

void InfluenceMapTest::testRectPatchMap () {
	Glest::Game::Search::Rectangle pMapBounds(8,8,32,16);
	PatchMap<6> iMap(pMapBounds, 0);
	iMap.clearMap(1);

	// test bounds [rectanglular map, width > height]
	// corner points, array for each corner, containing 4 points, the first three around
	// the corner but not on the influence map, the last just inside the influence map
	Point cornerPts[4][4] = {
		{ Point(7,7), Point(8,7), Point(7,8), Point(8,8) },			// NW
		{ Point(7,24), Point(8,24), Point(7,23), Point(8,23) },		// SW
		{ Point(40,7), Point(39,7), Point(40,8), Point(39,8) },		// NE
		{ Point(40,24), Point(39,24), Point(40,23), Point(39,23) }	// SE
	};
	// test around corner points...
	for ( int i = 0; i < 4; ++i ) {
		for ( int j = 0; j < 3; ++j ) {
			CPPUNIT_ASSERT(iMap.getInfluence(cornerPts[i][j]) == 0); // default, 0
		}
		CPPUNIT_ASSERT(iMap.getInfluence(cornerPts[i][3]) == 1); // on map, == 1
	}
	iMap.clearMap(0);
	// test setting corner points
	for ( int i = 0; i < 4; ++i ) {
		for ( int j = 0; j < 3; ++j ) {
			TEST_NOT_SET(cornerPts[i][j],0,1); // not on map
		}
		TEST_SET(cornerPts[i][3],0,1); // on map
	}
	// some setInfluence() tests
	Point pts[] = {
		Point( 9,12),
		Point( 16,17),
		Point( 13,13)
	};
	TEST_SET(pts[0], 0, 13);
	TEST_SET(pts[1], 0, 61);
	TEST_SET_CLAMPED(pts[2], 0, 66, 63);
}

void InfluenceMapTest::testRectFlowMap () {
	Glest::Game::Search::Rectangle pMapBounds(8,8,32,48);
	Point noDir(0,0), north(0,-1), east(1,0), west(-1,0), northwest(-1,-1), southeast(1,1);
	FlowMap iMap(pMapBounds, noDir);
	iMap.clearMap(north);

	// test bounds [rectanglular map, height > width]
	// corner points, array for each corner, containing 4 points, the first three around
	// the corner but not on the influence map, the last just inside the influence map
	Point cornerPts[4][4] = {
		{ Point(7,7),	Point(8,7),	  Point(7,8),	Point(8,8)	 },	// NW
		{ Point(7,56),	Point(8,56),  Point(7,55),	Point(8,55)  },	// SW
		{ Point(40,7),	Point(39,7),  Point(40,8),	Point(39,8)  },	// NE
		{ Point(40,56), Point(39,56), Point(40,55), Point(39,55) }	// SE
	};
	// test around corner points...
	for ( int i = 0; i < 4; ++i ) {
		for ( int j = 0; j < 3; ++j ) {
			CPPUNIT_ASSERT(iMap.getInfluence(cornerPts[i][j]) == noDir); // default, noDir
		}
		CPPUNIT_ASSERT(iMap.getInfluence(cornerPts[i][3]) == north); // on map, == 1
	}
	iMap.clearMap(noDir);
	// test setting corner points
	for ( int i = 0; i < 4; ++i ) {
		for ( int j = 0; j < 3; ++j ) {
			TEST_NOT_SET(cornerPts[i][j],noDir,north); // not on map
		}
		TEST_SET(cornerPts[i][3], noDir, north); // on map
	}
	iMap.clearMap(east);
	Point pts[] = { Point( 9,12), Point( 16,17), Point( 13,13) };
	TEST_SET(pts[0],east,west);
	TEST_SET(pts[1],east,northwest);
	TEST_SET(pts[2],east,southeast);
}

void InfluenceMapTest::testPatchMap1() {
	Glest::Game::Search::Rectangle bounds(8,8,72,4);
	PatchMap<1> iMap(bounds, 0);
	iMap.zeroMap();

	// section 0, test each bit position (8,8 -> 39,8)
	for ( int x = 8; x < 40; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,1);
	}
	// section 1, test each bit position (40,8 -> 71,8)
	for ( int x = 40; x < 72; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,1);
	}
	// section 2, padded section, test from (72,8 -> 79,8)
	for ( int x = 72; x < 80; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,1);
	}
	// test beyond row 0, ensure padded area is not used
	for ( int x = 80; x < 84; ++x ) {
		Point p(x,8);
		TEST_NOT_SET(p,0,1);
	}	
	// test section 1 of row 2
	for ( int x = 40; x < 72; ++x ) {
		Point p(x,9);
		TEST_SET_AND_RESET(p,0,1);
	}

	bounds = Glest::Game::Search::Rectangle(107,107,7,7);
	iMap = PatchMap<1>(bounds, 0);
	iMap.zeroMap();

	for (int x=107; x < 114; ++x) {
		Point p(x,107);	
		TEST_SET_AND_RESET(p, 0, 1);
	}
	Point p(106, 107);
	TEST_NOT_SET(p, 0, 1);
	p = Point(114, 107);
	TEST_NOT_SET(p, 0, 1);

}

void InfluenceMapTest::testPatchMap2() {
	Glest::Game::Search::Rectangle bounds(8,8,40,4);
	PatchMap<2> iMap(bounds, 0);
	iMap.zeroMap();

	// section 0, test each bit position (8,8 -> 23,8)
	for ( int x = 8; x < 24; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,1);
	}
	// section 1, test each bit position (24,8 -> 39,8)
	for ( int x = 24; x < 40; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,2);
	}
	// section 2, padded section, test from (40,8 -> 47,8)
	for ( int x = 40; x < 48; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,3);
	}
	// test beyond row 0, ensure padded area is not used
	for ( int x = 48; x < 50; ++x ) {
		Point p(x,8);
		TEST_NOT_SET(p,0,2);
	}	
	// test section 1 of row 2
	for ( int x = 24; x < 40; ++x ) {
		Point p(x,9);
		TEST_SET_AND_RESET(p,0,1);
	}
}

void InfluenceMapTest::testPatchMap3() {
	Glest::Game::Search::Rectangle bounds(8,8,25,4);
	PatchMap<3> iMap(bounds, 0);
	iMap.zeroMap();

	// section 0, test each bit position (8,8 -> 17,8)
	for ( int x = 8; x < 18; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,1);
	}
	// section 1, test each bit position (18,8 -> 27,8)
	for ( int x = 18; x < 28; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,2);
	}
	// section 2, padded section, test from (28,8 -> 32,8)
	for ( int x = 28; x < 33; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,3);
	}
	// test beyond row 0, ensure padded area is not used
	for ( int x = 33; x < 36; ++x ) {
		Point p(x,8);
		TEST_NOT_SET(p,0,2);
	}	
	// test section 1 of row 2
	for ( int x = 18; x < 28; ++x ) {
		Point p(x,9);
		TEST_SET_AND_RESET(p,0,1);
	}
}
void InfluenceMapTest::testPatchMap4() {
	Glest::Game::Search::Rectangle bounds(8,8,20,4);
	PatchMap<4> iMap(bounds, 0);
	iMap.zeroMap();

	// section 0, test each bit position (8,8 -> 15,8)
	for ( int x = 8; x < 16; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,1);
	}
	// section 1, test each bit position (16,8 -> 23,8)
	for ( int x = 16; x < 24; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,2);
	}
	// section 2, padded section, test from (24,8 -> 27,8)
	for ( int x = 24; x < 28; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,3);
	}
	// test beyond row 0, ensure padded area is not used
	for ( int x = 28; x < 31; ++x ) {
		Point p(x,8);
		TEST_NOT_SET(p,0,2);
	}	
	// test section 1 of row 2
	for ( int x = 16; x < 24; ++x ) {
		Point p(x,9);
		TEST_SET_AND_RESET(p,0,1);
	}
}
void InfluenceMapTest::testPatchMap5() {
	Glest::Game::Search::Rectangle bounds(8,8,15,4);
	PatchMap<5> iMap(bounds, 0);
	iMap.zeroMap();

	// section 0, test each bit position (8,8 -> 13,8)
	for ( int x = 8; x < 14; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,1);
	}
	// section 1, test each bit position (14,8 -> 19,8)
	for ( int x = 14; x < 20; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,2);
	}
	// section 2, padded section, test from (20,8 -> 22,8)
	for ( int x = 20; x < 23; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,3);
	}
	// test beyond row 0, ensure padded area is not used
	for ( int x = 23; x < 26; ++x ) {
		Point p(x,8);
		TEST_NOT_SET(p,0,2);
	}	
	// test section 1 of row 2
	for ( int x = 14; x < 20; ++x ) {
		Point p(x,9);
		TEST_SET_AND_RESET(p,0,1);
	}
}
void InfluenceMapTest::testPatchMap6() {
	Glest::Game::Search::Rectangle bounds(8,8,12,4);
	PatchMap<6> iMap(bounds, 0);
	iMap.zeroMap();

	// section 0, test each bit position (8,8 -> 12,8)
	for ( int x = 8; x < 13; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,1);
	}
	// section 1, test each bit position (13,8 -> 17,8)
	for ( int x = 13; x < 18; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,2);
	}
	// section 2, padded section, test from (18,8 -> 19,8)
	for ( int x = 18; x < 20; ++x ) {
		Point p(x,8);
		TEST_SET_AND_RESET(p,0,3);
	}
	// test beyond row 0, ensure padded area is not used
	for ( int x = 20; x < 23; ++x ) {
		Point p(x,8);
		TEST_NOT_SET(p,0,2);
	}	
	// test section 1 of row 2
	for ( int x = 13; x < 18; ++x ) {
		Point p(x,9);
		TEST_SET_AND_RESET(p,0,1);
	}
}


} // namespace Test