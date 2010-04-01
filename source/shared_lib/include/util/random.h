// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_RANDOM_H_
#define _SHARED_UTIL_RANDOM_H_

#include <cassert>
#include "vec.h"
#include "fixed.h"

using namespace Shared::Math;

namespace Shared { namespace Util {

// =====================================================
//	class Random
// =====================================================

class Random {
private:
	static const int m = 714025;
	static const int a = 1366;
	static const int b = 150889;

	int lastNumber;

public:
	Random() {
		lastNumber = 0;
	}

	Random(int seed) {
		init(seed);
	}

	void init(int seed) {
		lastNumber = abs(seed) % m;
	}

	// defined in util.cpp
	int rand();
	int randRange(int min, int max);
	float randRange(float min, float max);
	//fixed randRange(fixed min, fixed max);

	template<typename T> Vec2<T> randRange(const Vec2<T> &min, const Vec2<T> &max) {
		return Vec2<T>(
				randRange(min.x, max.x),
				randRange(min.y, max.y));
	}
	
	template<typename T> Vec3<T> randRange(const Vec3<T> &min, const Vec3<T> &max) {
		return Vec3<T>(
				randRange(min.x, max.x),
				randRange(min.y, max.y),
				randRange(min.z, max.z));
	}
	
	template<typename T> Vec4<T> randRange(const Vec4<T> &min, const Vec4<T> &max) {
		return Vec4<T>(
				randRange(min.x, max.x),
				randRange(min.y, max.y),
				randRange(min.z, max.z),
				randRange(min.w, max.w));
	}
};

}}//end namespace

#endif
