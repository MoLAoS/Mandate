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

using namespace Shared::Graphics;
namespace Shared { namespace Util {

// =====================================================
//	class Random
// =====================================================

class Random {
private:
	static const int m;
	static const int a;
	static const int b;

	int lastNumber;

public:
	Random() {
		lastNumber = 0;
	}

	Random(int seed) {
		lastNumber = 0;
		init(seed);
	}

	void init(int seed);

	int rand() {
		lastNumber = (a * lastNumber + b) % m;
		return lastNumber;
	}

	int randRange(int min, int max) {
		assert(min <= max);
		int diff = max - min;
		int res = min + static_cast<int>(static_cast<float>(diff + 1) * Random::rand() / m);
		assert(res >= min && res <= max);
		return res;
	}

	float randRange(float min, float max) {
		assert(min <= max);
		float rand01 = static_cast<float>(Random::rand()) / (m - 1);
		float res = min + (max - min) * rand01;
		assert(res >= min && res <= max);
		return res;
	}
	
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
