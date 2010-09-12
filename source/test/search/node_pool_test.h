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

#ifndef _TEST_NODE_POOL_H_
#define _TEST_NODE_POOL_H_

#include <cppunit/TestFixture.h>
#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>

#include "node_pool.h"
#include "vec.h"

using namespace Glest::Game::Search;
using Shared::Graphics::Vec2i;

namespace Test {

// =====================================================
//	class NodePoolTest
// =====================================================

class NodePoolTest : public CppUnit::TestFixture {
private:
	NodeStore *store;
	NodePool *pool;

public:
	NodePoolTest();
	~NodePoolTest();

	static CppUnit::Test *suite();
	void setUp();
	void tearDown();
	
	void testSetOpen();
	void testOpenListOrder();
	void testOpenListUpdate();
};

} // end namespace

#endif // _TEST_NODE_POOL_H_
