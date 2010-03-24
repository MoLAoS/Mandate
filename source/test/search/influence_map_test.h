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

#ifndef _TEST_INFLUENCE_MAP_H_
#define _TEST_INFLUENCE_MAP_H_

#include <cppunit/TestFixture.h>
#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>

#include "influence_map.h"
#include "vec.h"

using namespace Glest::Game::Search;
using Shared::Math::Vec2i;

namespace Test {

// =====================================================
//	class NodePoolTest
// =====================================================

class InfluenceMapTest : public CppUnit::TestFixture {
private:

public:
	InfluenceMapTest();
	~InfluenceMapTest();

	static CppUnit::Test *suite();
	void setUp();
	void tearDown();

	void testSquareTypeMap ();
	void testRectPatchMap ();
	void testRectFlowMap ();
	void testPatchMap1();
	void testPatchMap2();
	void testPatchMap3();
	void testPatchMap4();
	void testPatchMap5();
	void testPatchMap6();
};

} // end namespace

#endif // _TEST_INFLUENCE_MAP_H_
