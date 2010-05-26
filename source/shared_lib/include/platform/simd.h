// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_SIMD_H_
#define _SHARED_UTIL_SIMD_H_

#include <types.h>

// simd headers and vector types
#if (defined __i386__ || defined __x86_64__ || defined(_MSC_VER) )
#	if defined(USE_SSE2_INTRINSICS)
#		include <emmintrin.h>	// SSE2
#	endif
#endif

namespace Shared { namespace Util {
}}//end namespace

#endif // _SHARED_UTIL_SIMD_H_
