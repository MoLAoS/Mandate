// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include <cppunit/ui/text/TestRunner.h>

#include <iostream>

#include "reverse_rect_iter_test.h"
//#include "node_pool_test.h"
#include "influence_map_test.h"
#include "circular_buffer_test.h"
#include "fixed_point_test.h"
//#include "checksum_test.h"
#include "heap_test.h"
#include "line_test.h"

#include "leak_dumper.h"

using namespace Test;

int main(int argc, char **argv) {
	CppUnit::TextUi::TestRunner tester;

	tester.addTest(ReverseRectIteratorTest::suite());
	//tester.addTest(NodePoolTest::suite());
	tester.addTest(InfluenceMapTest::suite());
	tester.addTest(CircularBufferTest::suite());
	tester.addTest(FixedPointTest::suite());
//	tester.addTest(ChecksumTest::suite());
	tester.addTest(MinHeapTest::suite());
	tester.addTest(LineAlgorithmTest::suite());

	tester.run();

	char line[256];
	std::cout << "[Enter] to exit.";
	std::cin.getline(line,256);
	return 0;
}


