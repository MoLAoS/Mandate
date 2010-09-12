// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "vec.h"
#include "fixed.h"

namespace Shared { namespace Math {

/// fixed point square root, adapted from code at c.snippets.org
/// @todo make faster? [test perfomance vs sqrtf(), if horrible, make faster.]
fixed fixed::sqRt_c() const {
	fixed root = 0;		/* accumulator      */
	uint32 r = 0;		/* remainder        */
	uint32 e = 0;		/* trial product    */
	uint32 x = datum;

#	define TOP2BITS(v) ((v & (3L << (DATUM_BITS - 2))) >> (DATUM_BITS - 2))
	for (int i = 0; i < DATUM_BITS / 2; ++i) {
		r = (r << 2) + TOP2BITS(x);
		x <<= 2;
		root.datum <<= 1;
		e = (root.datum << 1) + 1;
		if (r >= e) {
			r -= e;
			++root.datum;
		}
	}
	root.datum <<= HALF_SHIFT;
	return root;
}

#define LOOP_BODY()					\
		r = (r << 2) + TOP2BITS(x);	\
		x <<= 2;					\
		root.datum <<= 1;			\
		e = (root.datum << 1) + 1;	\
		if (r >= e) {				\
			r -= e;					\
			++root.datum;			\
		}

fixed fixed::sqRt_unrolled_once() const {
	fixed root = 0;		/* accumulator      */
	uint32 r = 0;		/* remainder        */
	uint32 e = 0;		/* trial product    */
	uint32 x = datum;

	// unrolled once, more seems to kill performance, code cache thrashing? more testing needed.
	for (int i = 0; i < DATUM_BITS / 4; ++i) {
		LOOP_BODY();
		LOOP_BODY();
	}
	root.datum <<= HALF_SHIFT;
	return root;
}

fixed fixed::sqRt_unrolled_twice() const {
	fixed root = 0;		/* accumulator      */
	uint32 r = 0;		/* remainder        */
	uint32 e = 0;		/* trial product    */
	uint32 x = datum;

	for (int i = 0; i < DATUM_BITS / 8; ++i) {
		LOOP_BODY();
		LOOP_BODY();
		LOOP_BODY();
		LOOP_BODY();
	}
	root.datum <<= HALF_SHIFT;
	return root;
}

fixed fixed::sqRt_unrolled_thrice() const {
	fixed root = 0;		/* accumulator      */
	uint32 r = 0;		/* remainder        */
	uint32 e = 0;		/* trial product    */
	uint32 x = datum;

	for (int i = 0; i < DATUM_BITS / 16; ++i) {
		LOOP_BODY();
		LOOP_BODY();
		LOOP_BODY();
		LOOP_BODY();
		LOOP_BODY();
		LOOP_BODY();
		LOOP_BODY();
		LOOP_BODY();
	}
	root.datum <<= HALF_SHIFT;
	return root;
}


fixed fixed::sqRt_unrolled_completely() const {
	fixed root = 0;		/* accumulator      */
	uint32 r = 0;		/* remainder        */
	uint32 e = 0;		/* trial product    */
	uint32 x = datum;

	LOOP_BODY();
	LOOP_BODY();
	LOOP_BODY();
	LOOP_BODY();
	LOOP_BODY();
	LOOP_BODY();
	LOOP_BODY();
	LOOP_BODY();

	LOOP_BODY();
	LOOP_BODY();
	LOOP_BODY();
	LOOP_BODY();
	LOOP_BODY();
	LOOP_BODY();
	LOOP_BODY();
	LOOP_BODY();

	root.datum <<= HALF_SHIFT;
	return root;
}


fixed fixed::sqRt_julery() const {
	int32 val = datum;
	uint32 temp, g=0, b = 0x8000, bshft = 15;
	do {
		if (val >= (temp = (((g << 1) + b)<<bshft--))) {
			g += b;
			val -= temp;
		}
	} while (b >>= 1);
	fixed res;
	res.datum = g << HALF_SHIFT;
	return res;
}

/* by Mark Crowne */
fixed fixed::sqRt_julery_crowne() const {
	uint32 temp, g = 0;
	int32 val = datum;

	if (val >= 0x40000000) {
		g = 0x8000; 
		val -= 0x40000000;
	}

#	define INNER_ISQRT(s)						\
	temp = (g << (s)) + (1 << ((s) * 2 - 2));	\
	if (val >= temp) {							\
		g += 1 << ((s)-1);						\
		val -= temp;							\
	}

	INNER_ISQRT (15)
	INNER_ISQRT (14)
	INNER_ISQRT (13)
	INNER_ISQRT (12)
	INNER_ISQRT (11)
	INNER_ISQRT (10)
	INNER_ISQRT ( 9)
	INNER_ISQRT ( 8)
	INNER_ISQRT ( 7)
	INNER_ISQRT ( 6)
	INNER_ISQRT ( 5)
	INNER_ISQRT ( 4)
	INNER_ISQRT ( 3)
	INNER_ISQRT ( 2)

#undef INNER_ISQRT

	temp = g+g+1;
	if (val >= temp) g++;
	fixed r;
	r.datum = g << HALF_SHIFT;
	return r;
}

}}

