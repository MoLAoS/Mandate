// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2010 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "random.h"
#include "timer.h"
#include "fixed.h"

using Shared::Math::fixed;
using Shared::Util::Random;
using Shared::Platform::Chrono;

const int NUM_TRIALS = 8192;

#define DO_TEST(sqRtFunc)													\
	{																		\
		Chrono chrono;														\
		chrono.start();														\
		for (int i=0; i < NUM_TRIALS; ++i) {								\
		fixed_roots[i] = fixed_numbers[i].sqRtFunc();						\
		}																	\
		chrono.stop();														\
		cout << "\nDone, " << NUM_TRIALS <<	" fixed point square roots ["	\
			<< #sqRtFunc << "].  Took " << chrono.getMicros() << "us";		\
	}


int main(int argc, char **argv) {
	Random rand; // default constructor seeds with 0

	fixed fixed_numbers[NUM_TRIALS];
	fixed fixed_roots[NUM_TRIALS];

	float float_numbers[NUM_TRIALS];
	float float_roots[NUM_TRIALS];

	for (int i=0; i < NUM_TRIALS; ++i) {
		fixed_numbers[i] = rand.randRange(4, 4000);
		fixed_numbers[i] += rand.randRange(0, 4095) / fixed(4096);
		
		float_numbers[i] = fixed_numbers[i].toFloat();
	}
	{	// using floats and sqrtf(), for reference
		Chrono chrono;
		chrono.start();
		for (int i=0; i < NUM_TRIALS; ++i) {
			float_roots[i] = sqrtf(float_numbers[i]);
		}
		chrono.stop();
		cout << "\nDone, " << NUM_TRIALS << " floating point square roots.  Took " << chrono.getMicros() << "us";
	}
	DO_TEST(sqRt);
	DO_TEST(sqRt_unrolled_once);
	DO_TEST(sqRt_unrolled_twice);
	DO_TEST(sqRt_unrolled_thrice);
	DO_TEST(sqRt_unrolled_completely);
	cout << "\n\n[Enter]";
	char boo[8];
	std::cin.getline(boo, 8);

}


