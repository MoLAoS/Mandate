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
#include "circular_buffer_test.h"

#include "leak_dumper.h"

using Shared::Platform::Socket;

namespace Test {

CircularBufferTest::CircularBufferTest() {	
}

CircularBufferTest::~CircularBufferTest() {
}

#define ADD_TEST(Class, Method) suiteOfTests->addTest( \
	new CppUnit::TestCaller<Class>(#Method, &Class::Method));

CppUnit::Test *CircularBufferTest::suite() {
	CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite("ReverseRectIteratorTest");
	ADD_TEST(CircularBufferTest, testBasicOps);
	ADD_TEST(CircularBufferTest, testRollOver);
	ADD_TEST(CircularBufferTest, testPerfectFill);
	ADD_TEST(CircularBufferTest, testPeek);
	return suiteOfTests;
}

void CircularBufferTest::setUp() {
	alpha_data = "abcdefghijklmnopqrstuvwxyz";
	alpha_size = strlen(alpha_data); // == 26

	num_data = "1234567890";
	num_size = strlen(num_data); // == 10

	alpha_num_data = "1234567890abcdefghijklmnopqrstuvwxyz";
	alpha_num_size = strlen(alpha_num_data); // == 36

	hex_data = "0123456789ABCDEF";
	hex_size = strlen(hex_data); // == 16
}

void CircularBufferTest::tearDown() {
}

void CircularBufferTest::testBasicOps() {

	static const int buf_size = 1024;

	Socket::CircularBuffer buffer(buf_size);

	bool limit;
	int max = buffer.getMaxWrite(limit);

	CPPUNIT_ASSERT(max == buf_size); // should be able to write 1024 bytes
	CPPUNIT_ASSERT(limit);			 // and if we do, we shouldn't be able to write any more

	memcpy(buffer.getWritePos(), alpha_num_data, alpha_num_size);
	buffer += alpha_num_size;

	max = buffer.getMaxWrite(limit);

	// max write should be size - 36
	CPPUNIT_ASSERT(max == buf_size - alpha_num_size);
	CPPUNIT_ASSERT(limit); // limit should still be true

	buffer -= alpha_num_size; // 'remove' the data 
	max = buffer.getMaxWrite(limit);
	CPPUNIT_ASSERT(max == buf_size - alpha_num_size);
	CPPUNIT_ASSERT(!limit);	// limit should now be false

	// free bytes should be buf_size
	int avail = buffer.getFreeBytes();
	CPPUNIT_ASSERT(avail == buf_size);
}

void CircularBufferTest::testRollOver() {
	static const int buf_size = 256;

	Socket::CircularBuffer buffer(buf_size);

	bool limit;
	int max = buffer.getMaxWrite(limit);

	CPPUNIT_ASSERT(max == buf_size);

	// write hex_data (buf_size / hex_size - 1) times 
	// leaving 16 bytes at the end of the buffer
	int times = buf_size / hex_size - 1;
	for (int i=0; i < times; ++i) {
		max = buffer.getMaxWrite(limit);
		CPPUNIT_ASSERT(max == buf_size - i * hex_size);
		memcpy(buffer.getWritePos(), hex_data, hex_size);
		buffer += hex_size;
	}
	max = buffer.getMaxWrite(limit);
	CPPUNIT_ASSERT(max == hex_size); // 16 bytes left
	CPPUNIT_ASSERT(limit);

	buffer -= hex_size; // make some room
	max = buffer.getMaxWrite(limit);
	CPPUNIT_ASSERT(max == hex_size); // max write == 16 bytes
	CPPUNIT_ASSERT(!limit); // but can write more after that
	int mem_free = buffer.getFreeBytes();
	CPPUNIT_ASSERT(mem_free == 2 * hex_size); // 32 bytes of free space

	const char *my_data = alpha_data;
	int to_write = alpha_size;

	// alpha_data will now need be written in two phases...

	memcpy(buffer.getWritePos(), my_data, max);
	buffer += max;
	my_data += max;
	to_write -= max;

	max = buffer.getMaxWrite(limit);
	CPPUNIT_ASSERT(max == hex_size);
	CPPUNIT_ASSERT(limit);
	CPPUNIT_ASSERT(max > to_write);

	memcpy(buffer.getWritePos(), my_data, to_write);
	buffer += to_write;

	max = buffer.getMaxWrite(limit);
	CPPUNIT_ASSERT(max == 2 * hex_size - alpha_size); // 6 bytes

	// we should have (times - 1) * hex_data and then 1 * alpha_data in the buffer now
	// with the alpha data 'wrapping around'

	size_t size = (times - 1) * hex_size + alpha_size;
	CPPUNIT_ASSERT(size == buffer.bytesAvailable());

	char *my_buffer = new char[size];
	CPPUNIT_ASSERT(buffer.readBytes(my_buffer, size));	// read should be ok
	CPPUNIT_ASSERT(buffer.bytesAvailable() == 0);		// no more bytes should be available
	CPPUNIT_ASSERT(buffer.getFreeBytes() == buf_size);	// and we should have buf_size free space

	int ndx = 0;
	for (int i=0; i < times - 1; ++i) {
		for (int j=0; j < hex_size; ++j) {
			CPPUNIT_ASSERT(my_buffer[ndx] == hex_data[ndx % hex_size]);
			++ndx;
		}
	}
	int offset = ndx;
	for (int i=0; i < alpha_size; ++i) {
		CPPUNIT_ASSERT(my_buffer[ndx] == alpha_data[ndx - offset]);
		++ndx;
	}
	delete my_buffer;
}

void CircularBufferTest::testPerfectFill() {
	static const int buf_size = 256;

	Socket::CircularBuffer buffer(buf_size);

	bool limit;
	int max = buffer.getMaxWrite(limit);

	CPPUNIT_ASSERT(max == buf_size);

	// write hex_data (buf_size / hex_size) times 
	// completely filling the buffer
	int times = buf_size / hex_size;
	for (int i=0; i < times; ++i) {
		max = buffer.getMaxWrite(limit);
		CPPUNIT_ASSERT(max == buf_size - i * hex_size);
		memcpy(buffer.getWritePos(), hex_data, hex_size);
		buffer += hex_size;
	}
	max = buffer.getMaxWrite(limit);
	CPPUNIT_ASSERT(max == 0); // no bytes left
	CPPUNIT_ASSERT(limit);

	size_t size = times * hex_size; // == 256
	CPPUNIT_ASSERT(size == buffer.bytesAvailable());

	char *my_buffer = new char[size];
	CPPUNIT_ASSERT(buffer.readBytes(my_buffer, size));	// read should be ok
	CPPUNIT_ASSERT(buffer.bytesAvailable() == 0);		// no more bytes should be available
	CPPUNIT_ASSERT(buffer.getFreeBytes() == buf_size);	// and we should have buf_size free space

	int ndx = 0;
	for (int i=0; i < times; ++i) {
		for (int j=0; j < hex_size; ++j) {
			CPPUNIT_ASSERT(my_buffer[ndx] == hex_data[ndx % hex_size]);
			++ndx;
		}
	}
	delete my_buffer;
}

void CircularBufferTest::testPeek() {
	static const int buf_size = 256;
	Socket::CircularBuffer buffer(buf_size);
	bool limit;
	int max = buffer.getMaxWrite(limit);
	CPPUNIT_ASSERT(max == buf_size);

	// write num_data (buf_size / num_size) times [==25]
	int times = buf_size / num_size;
	for (int i=0; i < times; ++i) {
		max = buffer.getMaxWrite(limit);
		CPPUNIT_ASSERT(max == buf_size - i * num_size);
		memcpy(buffer.getWritePos(), num_data, num_size);
		buffer += num_size;
	}
	max = buffer.getMaxWrite(limit);
	CPPUNIT_ASSERT(max == buf_size - times * num_size); // 6 bytes left
	CPPUNIT_ASSERT(limit);

	size_t size = times * num_size; // == 250
	CPPUNIT_ASSERT(size == buffer.bytesAvailable());

	char *my_buffer = new char[size];
	CPPUNIT_ASSERT(buffer.peekBytes(my_buffer, size));			// peek should be ok
	CPPUNIT_ASSERT(buffer.bytesAvailable() == size);			// 250 bytes should still be available
	CPPUNIT_ASSERT(buffer.getFreeBytes() == buf_size - size);	// and 6 bytes should still be free

	int ndx = 0;
	for (int i=0; i < times; ++i) {
		for (int j=0; j < num_size; ++j) {
			CPPUNIT_ASSERT(my_buffer[ndx] == num_data[ndx % num_size]);
			++ndx;
		}
	}
	delete my_buffer;
}


} // end namespace Test
