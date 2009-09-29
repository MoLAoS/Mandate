// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "random.h"

#include <cassert>
#include <time.h>
#include <cstdlib>

#include "leak_dumper.h"

namespace Shared { namespace Util {

// =====================================================
//	class Random
// =====================================================

const int Random::m= 714025;
const int Random::a= 1366;
const int Random::b= 150889;

void Random::init(int seed) {
	seed = abs(seed);
#ifdef SEED_RANDOM_WITH_CLOCK
	assert(sizeof(time_t) == 32);
	time_t curSeconds = 0;
	time(&curSeconds);
	curSeconds = ((curSeconds & 0x000000ff) << 24)
			| ((curSeconds & 0x0000ff00) << 8)
			| ((curSeconds & 0x00ff0000) >> 8)
			| ((curSeconds & 0xff000000) >> 24);
	lastNumber= abs((int)((seed ^ curSeconds ^ clock()) % m));
#else
	lastNumber= seed % m;
#endif
}

}}//end namespace
