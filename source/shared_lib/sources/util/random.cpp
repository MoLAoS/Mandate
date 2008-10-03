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

#include "pch.h"
#include "random.h"

#include <cassert>
#include <time.h>

#include "leak_dumper.h"

namespace Shared { namespace Util {

// =====================================================
//	class Random
// =====================================================

const int Random::m= 714025;
const int Random::a= 1366;
const int Random::b= 150889;

void Random::init(int seed) {
	time_t curSeconds = 0;
	time(&curSeconds);
	lastNumber= (seed ^ curSeconds ^ clock()) % m;
}

}}//end namespace
