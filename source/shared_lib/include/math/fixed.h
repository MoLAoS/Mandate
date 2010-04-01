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

#ifndef _FIXED_POINT_TYPE_INCLUDED_
#define _FIXED_POINT_TYPE_INCLUDED_

#include "types.h"
#include "vec.h"
#include <iostream>
#include <stdexcept>
#include <cassert>

using namespace Shared::Platform;
using std::cout;
using std::endl;

#ifndef FIXED_THROW_ON_OVERFLOW
#	define FIXED_THROW_ON_OVERFLOW 0
#endif
#ifndef FIXED_THROW_ON_DIVIDE_BY_ZERO
#	define FIXED_THROW_ON_DIVIDE_BY_ZERO 0
#endif
#if FIXED_THROW_ON_OVERFLOW
#	define IF_THROW_ON_OVERFLOW(x) {x}
#else
#	define IF_THROW_ON_OVERFLOW(x)
#endif
#if FIXED_THROW_ON_DIVIDE_BY_ZERO
#	define IF_THROW_ON_DIVIDE_BY_ZERO(x) {x}
#else
#	define IF_THROW_ON_DIVIDE_BY_ZERO(x)
#endif
#ifndef FIXED_WARN_ON_OVERFLOW
#	define FIXED_WARN_ON_OVERFLOW 0
#endif
#ifndef FIXED_WARN_ON_PRECISION_LOSS
#	define FIXED_WARN_ON_PRECISION_LOSS 0
#endif
#ifndef FIXED_WARN_ON_DIVIDE_BY_ZERO
#	define FIXED_WARN_ON_DIVIDE_BY_ZERO 0
#endif
#if FIXED_WARN_ON_OVERFLOW
#	define IF_WARN_ON_OVERFLOW(x) {x}
#else
#	define IF_WARN_ON_OVERFLOW(x)
#endif
#if FIXED_WARN_ON_PRECISION_LOSS
#	define IF_WARN_ON_PRECISION_LOSS(x) {x}
#else
#	define IF_WARN_ON_PRECISION_LOSS(x)
#endif
#if FIXED_WARN_ON_DIVIDE_BY_ZERO
#	define IF_WARN_ON_DIVIDE_BY_ZERO(x) {x}
#else
#	define IF_WARN_ON_DIVIDE_BY_ZERO(x)
#endif

namespace Shared { namespace Math {

/** primitive & crude, but functional, fixed point class
 *
 * NOTE! This is not a general purpose fixed point type, it was designed to free glest of
 * _certain_ calculations being performed in floating point, for multi-platform network play.
 *
 * No method of converting a float to a fixed is provided, doing so would make it too easy
 * to make an error that would defeat the purpose of this excercise.
 * The reverse is ok, (the renderer needs some information that is now stored in fixed point)
 * and a toFloat() method is provided.
 *
 * All arithmetic is done in 32 bits, for multiply & divide each operand is _first_ scaled with a half shift...
 * Translation: Dealing with very small fractions or very big numbers is dangerous. Just say no!
 *
 * If needed, 'special' multiply & divide functions could be written to take advantage of any known
 * information about the operands, ie: multiplying a number known to be large by a number known to
 * be a small fraction we could perform a full shift on the 'big' (wiping out its fractional part)
 * and no shift on the small one, maintaining its precision.
 *
 * This has not been done, as it hasn't (yet?) been deemed necessary, and would be ugly.
 *
 */
class fixed {
private:
	static const int DATUM_BITS = 32;
	static const int F_BITS		= 12;
	static const int I_BITS		= DATUM_BITS - F_BITS;

	static const int FULL_SHIFT = F_BITS;
	static const int HALF_SHIFT = F_BITS / 2;

	static const int SCALE_FACTOR = (1 << F_BITS); // actualy the reciprical of the scaling factor
	static const int MAX_INTEGER = (1 << (I_BITS - 1)) - 1;

	static const int64 MAX_RAW = (MAX_INTEGER << F_BITS) | (SCALE_FACTOR - 1);
	static const int64 MIN_RAW = ~MAX_RAW;

public:
	static int64 max_frac()	{ return SCALE_FACTOR - 1;	}
	static int64 max_int()	{ return MAX_INTEGER;		}
	static int64 min_int()	{ return ~MAX_INTEGER;		}

private:
	union {
		struct {
			uint32	frac_part  : F_BITS;	// 12;
			int32	whole_part : I_BITS;	// 20;
		};
		int32 datum;
	};

public:
	fixed()	{ assert(F_BITS % 2 == 0); }
	fixed(const fixed &that) : datum(that.datum) { assert(F_BITS % 2 == 0); }
	fixed(const int val)	{ assert(F_BITS % 2 == 0); *this = val; }

	int		intp() const	{ return whole_part;	}
	int		frac() const	{ return frac_part;		}
	int32&	raw()			{ return datum;			}
	int32	raw_val() const { return datum;			}

	int	round() const {
		if (frac_part < SCALE_FACTOR / 2) return whole_part; // half rounds up, change to <= to round it down
		return whole_part + (whole_part >= 0 ? 1 : -1);
	}

	fixed abs() const { return datum >= 0 ? *this : -*this; }

	float toFloat() const {
		return whole_part + frac_part / float(SCALE_FACTOR);
	}

	fixed& operator=(const fixed &that) { datum = that.datum; return *this;	}

	fixed& operator=(const int &val)	{
		IF_WARN_ON_OVERFLOW(
			if (val > MAX_INTEGER)
				cout << "\nWARNING: fixed point assignment of number too large, important digits lost!";
		)
		IF_THROW_ON_OVERFLOW(
			if (val > MAX_INTEGER) throw std::runtime_error("Error: fixed point overflow.");
		)
		datum = val << FULL_SHIFT;
		return *this;
	}

#	define IF_ADDSUB_OVERFLOW(op)						\
			int64 bigRes = int64(datum) op that.datum;	\
			if (bigRes > MAX_RAW || bigRes < MIN_RAW)

	fixed& operator+=(const fixed &that) {
		IF_WARN_ON_OVERFLOW(
			IF_ADDSUB_OVERFLOW(+)
				cout << "\nWARNING: fixed point addition caused overflow, important digits lost!";
		)
		IF_THROW_ON_OVERFLOW(
			IF_ADDSUB_OVERFLOW(+)
				throw std::runtime_error("Error: fixed point overflow.");
		)
		datum += that.datum;
		return *this;
	}

	fixed& operator-=(const fixed &that) {
		IF_WARN_ON_OVERFLOW(
			IF_ADDSUB_OVERFLOW(-)
				cout << "\nWARNING: fixed point subtraction caused overflow, important digits lost!";
		)
		IF_THROW_ON_OVERFLOW(
			IF_ADDSUB_OVERFLOW(-)
				throw std::runtime_error("Error: fixed point overflow.");
		)
		datum -= that.datum;
		return *this;
	}

	/* positive and any top bits set == bad, negative and top bits not all set == bad */
#	define IF_MULT_OVERFLOW()																\
		int64 bigRes = (int64(datum) >> HALF_SHIFT) * (int64(that.datum) >> HALF_SHIFT);	\
		if ((bigRes >= 0 && bigRes & 0xFFFFFFFF00000000)									\
		|| (bigRes < 0 && (bigRes>>32) != -1))

	fixed& operator*=(const fixed &that) {
		IF_WARN_ON_PRECISION_LOSS(
			if (datum & ((1 << HALF_SHIFT) - 1) || that.datum & ((1 << HALF_SHIFT) - 1))
				cout << "\nWARNING: fixed point multiplication caused precision loss.";
		)
		IF_WARN_ON_OVERFLOW(
			IF_MULT_OVERFLOW()
				cout << "\nWARNING: fixed point multiplication caused overflow, important digits lost!";
		)
		IF_THROW_ON_OVERFLOW(
			IF_MULT_OVERFLOW()
				throw std::runtime_error("Error: fixed point overflow.");
		)
		datum = (datum >> HALF_SHIFT) * (that.datum >> HALF_SHIFT);
		return *this;
	}

	/* positive and any top bits set == bad, negative and top bits not all set == bad */
#	define IF_DIV_OVERFLOW()																\
		int64 bigRes = (int64(datum) << HALF_SHIFT) / (int64(that.datum) >> HALF_SHIFT);	\
		if ((bigRes >= 0 && bigRes & 0xFFFFFFFF00000000) 									\
		|| (bigRes < 0 && (bigRes >> 32) != -1))

	fixed& operator/=(const fixed &that) {
		IF_THROW_ON_DIVIDE_BY_ZERO(
			if (!(that.datum >> HALF_SHIFT))
				throw std::runtime_error("Error: fixed point divide by zero.");
		)
		IF_THROW_ON_OVERFLOW(
			IF_DIV_OVERFLOW()
				throw std::runtime_error("Error: fixed point overflow.");
		)
		IF_WARN_ON_DIVIDE_BY_ZERO(
			if (!that.datum || !(that.datum >> HALF_SHIFT)) {
				if (!that.datum)
					cout << "\nERROR: fixed point divide by zero.";
				else // (!(that.datum >> HALF_SHIFT))
					cout << "\nERROR: fixed point precision loss in division would cause divide by zero.";
				whole_part = max_int(); frac_part = max_frac();
				return *this;
			}
		)
		IF_WARN_ON_PRECISION_LOSS(
			if (that.datum & ((1 << HALF_SHIFT) - 1))
				cout << "\nWARNING: fixed point division caused precision loss.";
		)
		IF_WARN_ON_OVERFLOW(
			IF_DIV_OVERFLOW()
				cout << "\nWARNING: fixed point division caused overflow, important numbers lost!";
		)
		datum = (datum << HALF_SHIFT) / (that.datum >> HALF_SHIFT);
		return *this;
	}

	fixed operator+(const fixed &that) const { fixed res(*this); res += that; return res; }
	fixed operator-(const fixed &that) const { fixed res(*this); res -= that; return res; }
	fixed operator*(const fixed &that) const { fixed res(*this); res *= that; return res; }
	fixed operator/(const fixed &that) const { fixed res(*this); res /= that; return res; }

	fixed operator-() const { fixed res; res.datum = -datum; return res;		}

	bool operator!() const						{ return !datum;				}
	bool operator==(const fixed &that) const	{ return datum == that.datum;	}
	bool operator!=(const fixed &that) const	{ return datum != that.datum;	}
	bool operator<(const fixed &that) const		{ return datum < that.datum;	}
	bool operator>(const fixed &that) const		{ return datum > that.datum;	}
	bool operator<=(const fixed &that) const	{ return datum <= that.datum;	}
	bool operator>=(const fixed &that) const	{ return datum >= that.datum;	}

	fixed sqRt() const; // defined in math_util.cpp
	fixed sqRt_unrolled_once() const;
	fixed sqRt_unrolled_twice() const;
	fixed sqRt_unrolled_thrice() const;
	fixed sqRt_unrolled_completely() const;

}; // class fixed

inline fixed operator+(const int &lhs, const fixed &rhs) { fixed res(lhs); res += rhs; return res; }
inline fixed operator-(const int &lhs, const fixed &rhs) { fixed res(lhs); res -= rhs; return res; }
inline fixed operator*(const int &lhs, const fixed &rhs) { fixed res(lhs); res *= rhs; return res; }
inline fixed operator/(const int &lhs, const fixed &rhs) { fixed res(lhs); res /= rhs; return res; }

inline std::ostream& operator<<(std::ostream &lhs, const fixed &rhs) {
	fixed tmp = rhs.abs();
	lhs <<  (rhs.raw_val() < 0 ? "[ -" : "[ ");
	return lhs << tmp.intp() << " & " << tmp.frac() << "/" << (fixed::max_frac() + 1) << " ]";
}

/** Pythagoras, distance between to Vec2i positions as a fixed */
inline fixed fixedDist(const Vec2i &a, const Vec2i &b) {
	fixed dx = abs(a.x - b.x);
	fixed dy = abs(a.y - b.y);
	return (dx * dx + dy * dy).sqRt();
}

/** Very minimal vector of 2 fixeds, _DO_NOT_ use Shared::Math::Vec2<fixed> */
class fixedVec2 {
public:
	fixed x, y;

	fixedVec2() {}
	fixedVec2(fixed xy) : x(xy), y(xy) {}
	fixedVec2(fixed x, fixed y) : x(x), y(y) {}

	fixed dist(const fixedVec2 &that) {
		fixed dx = (x - that.x).abs();
		fixed dy = (y - that.y).abs();
		return (dx * dx + dy * dy).sqRt();
	}

	fixedVec2 operator+(const fixedVec2 &that) { return fixedVec2(x + that.x, y + that.y); }
	fixedVec2 operator/(const fixed &d)	{ return fixedVec2(x / d, y / d); }
};

}} // end namespace Shared::Math

#endif // !def _FIXED_POINT_TYPE_INCLUDED_
