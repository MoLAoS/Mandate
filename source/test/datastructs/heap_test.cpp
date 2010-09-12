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
#include "heap_test.h"

#include "conversion.h"
#include "random.h"

using Shared::Util::Random;
using Shared::Util::MinHeap;

#include "leak_dumper.h"

using std::cout;
using std::endl;

namespace Test {

#define ADD_TEST(Class, Method) suiteOfTests->addTest( \
	new CppUnit::TestCaller<Class>(#Method, &Class::Method));

CppUnit::Test *MinHeapTest::suite() {
	CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite("MinHeapTest");
	ADD_TEST(MinHeapTest, testHeapify);
	ADD_TEST(MinHeapTest, testPromote);

	return suiteOfTests;
}

struct DummyNode {
	int heap_ndx;
	void setHeapIndex(int ndx) { heap_ndx = ndx;  }
	int  getHeapIndex() const  { return heap_ndx; }
	
	int priority;
	bool operator<(const DummyNode &that) const {
		return this->priority < that.priority;
	}
};

void MinHeapTest::testHeapify() {
	std::set<int> priorities;
	std::map<int, int> elementMap;
	DummyNode nodes[1024];
	MinHeap<DummyNode> heap;

	cout << "\n\nAdding elements... \n\t";
	Random r;
	for (int i=0; i < 100; ) {
		int p = r.randRange(1, 4096);
		if (priorities.find(p) != priorities.end()) continue;
		priorities.insert(p);
		nodes[i].priority = p;
		cout << p << (i < 99 ? ", " : "\n\n");
		heap.insert(&nodes[i]);
		++i;
	}

	int lp = -1;
	cout << "Popping...\n\t";
	while (heap.size()) {
		DummyNode *ptr = heap.extract();
		int p = ptr->priority;
		CPPUNIT_ASSERT(lp < p);
		cout << p << (heap.size() ? ", " : "\n\n");
		CPPUNIT_ASSERT(priorities.find(p) != priorities.end());
		priorities.erase(p);
		lp = p;
	}
	CPPUNIT_ASSERT(priorities.empty());
}

void MinHeapTest::testPromote() {
	DummyNode nodes[1024];
	MinHeap<DummyNode> heap;

	cout << "\n\nAdding elements... \n\t";
	Random r;
	for (int i=0; i < 1000; ) {
		int p = r.randRange(1, 714024);
		nodes[i].priority = p;
		cout << p << (i < 999 ? ", " : "\n\n");
		heap.insert(&nodes[i]);
		++i;
	}

	int promotions = 0;
	for (int i=0; i < 1000; ++i) {
		if (r.randRange(0, 4) == 0 && nodes[i].priority > 2048) {
			nodes[i].priority = r.randRange(1, nodes[i].priority - 1);
			heap.promote(&nodes[i]);
			++promotions;
		}
	}

	cout << "Performed " << promotions << " promotions.\n\n";

	int lp = -1;
	cout << "Popping...\n\t";
	while (heap.size()) {
		DummyNode *ptr = heap.extract();
		int p = ptr->priority;
		CPPUNIT_ASSERT(lp <= p);
		cout << p << (heap.size() ? ", " : "\n\n");
		lp = p;
	}
}

} // end namespace Test
	
