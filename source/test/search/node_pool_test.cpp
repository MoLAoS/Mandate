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
#include "node_pool_test.h"

#include "leak_dumper.h"

namespace Test {

#define MAP_WIDTH  128
#define MAP_HEIGHT 128

NodePoolTest::NodePoolTest() {
}

NodePoolTest::~NodePoolTest() {
}

CppUnit::Test *NodePoolTest::suite() {
	CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite("NodePoolTest");
	suiteOfTests->addTest(new CppUnit::TestCaller<NodePoolTest>(
			"testSetOpen",
			&NodePoolTest::testSetOpen));
	suiteOfTests->addTest(new CppUnit::TestCaller<NodePoolTest>(
			"testOpenListOrder",
			&NodePoolTest::testOpenListOrder));
	suiteOfTests->addTest(new CppUnit::TestCaller<NodePoolTest>(
			"testOpenListUpdate",
			&NodePoolTest::testOpenListUpdate));
	return suiteOfTests;
}

void NodePoolTest::setUp() {
	store = new NodeStore(/*MAP_WIDTH,MAP_HEIGHT*/);
	pool = new NodePool();
	store->attachNodePool(pool);
}

void NodePoolTest::tearDown() {
	delete pool;
	delete store;
	pool = NULL;
	store = NULL;
}

void NodePoolTest::testSetOpen() {
	Vec2i pts [] = {
		Vec2i(0,0), Vec2i(7,7), Vec2i(13,19), Vec2i(MAP_WIDTH-1,MAP_HEIGHT-1), Vec2i(123,121), Vec2i(82,58)
	};
	for ( int y = 0; y < MAP_HEIGHT; ++y ) {
		for ( int x = 0; x < MAP_WIDTH; ++x ) {
			CPPUNIT_ASSERT( !store->isOpen(Vec2i(x,y)) );
			CPPUNIT_ASSERT( !store->isClosed(Vec2i(x,y)) );
		}
	}
	for ( int i=0; i < 6; ++i ) {
		store->setOpen(pts[i], Vec2i(0,0), 1.f, 1.f);
		CPPUNIT_ASSERT( store->isOpen(pts[i]) );
		CPPUNIT_ASSERT( !store->isClosed(pts[i]) );
		CPPUNIT_ASSERT( store->getCostTo(pts[i]) == 1.f );
		CPPUNIT_ASSERT( store->getHeuristicAt(pts[i]) == 1.f );
		CPPUNIT_ASSERT( store->getEstimateFor(pts[i]) == 2.f );
	}
}

void NodePoolTest::testOpenListOrder() {
	srand( 13 );
	Vec2i p;
	for ( int i=0; i < 500; ++i ) {
		do { // find unique position
			p.x = rand() % MAP_WIDTH;
			p.y = rand() % MAP_HEIGHT;
		} while ( store->isOpen(p) );
		float d = (float)(rand() % 64) + ((float)(rand() % 100)) / 100.f;
		float h = (float)(rand() % 64) + ((float)(rand() % 100)) / 100.f;
		store->setOpen(p, Vec2i(0,0), h, d);
	}
	float prevEstimate = -1.f;
	while ( (p = store->getBestCandidate()) != Vec2i(-1,-1) ) {
		float estimate = store->getEstimateFor(p);
		CPPUNIT_ASSERT( estimate >= prevEstimate );
		prevEstimate = store->getEstimateFor(p);
	}
}

void NodePoolTest::testOpenListUpdate() {
	Vec2i p, p0, p10, p20, p400, p500;
	
	// need a zero distance node to update 'from' ... set heuristic really high to keep it 
	// out of the way of the rest of the test...
	p0 = Vec2i(0,0);
	store->setOpen(p0,p0,99999.f, 0.f );

	float est10, est15, est16, est20, est99, est100, est400;
	for ( int i=1; i <= 500; ++i ) {
		do { // find unique position
			p.x = rand() % MAP_WIDTH;
			p.y = rand() % MAP_HEIGHT;
		} while ( store->isOpen(p) );
		float d = 150.f;
		float h = i / 10.f;
		store->setOpen(p, Vec2i(0,0), h, d);
		if ( i == 10 ) { p10 = p; est10 = store->getEstimateFor(p); }
		if ( i == 15 ) { est15 = store->getEstimateFor(p); }
		if ( i == 16 ) { est16 = store->getEstimateFor(p); }
		if ( i == 20 ) { p20 = p; est20 = store->getEstimateFor(p); }
		if ( i == 99 ) { est99 = store->getEstimateFor(p); }
		if ( i == 100 ) { est100 = store->getEstimateFor(p); }
		if ( i == 400 ) { p400 = p; est400 = store->getEstimateFor(p); }
		if ( i == 500 ) { p500 = p; }
	}

	int i = 1;
	for ( ; i < 10; ++i ) { store->getBestCandidate(); }
	CPPUNIT_ASSERT( store->getBestCandidate() == p10 );
	i++; // i == 10

	// update 20th node, such that it should become the 16th node afterwards...
	float nEst = (est15 + est16) / 2.f;
	float nDist = 150.f - (est20 - nEst);
	store->updateOpen(p20, p0, nDist);

	// should now be 16th
	for ( ; i < 16; ++i ) { store->getBestCandidate(); }
	CPPUNIT_ASSERT( store->getBestCandidate() == p20 );
	i++; // i == 16

	// update 400th node, such that it should become the 100th node afterwards...
	nEst = (est99 + est100) / 2.f;
	nDist = 150.f - (est400 - nEst);
	store->updateOpen(p400, p0, nDist);

	// should now be 100th
	for ( ; i < 100; ++i ) { store->getBestCandidate(); }
	CPPUNIT_ASSERT( store->getBestCandidate() == p400 );

	// update 500th node, to bring it all the way to the front
	store->updateOpen(p500, p0, 100.f);
	CPPUNIT_ASSERT( store->getBestCandidate() == p500 );
}

} // end namespace
