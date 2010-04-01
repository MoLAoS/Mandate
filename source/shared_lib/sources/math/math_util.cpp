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
fixed fixed::sqRt() const {
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


}}

