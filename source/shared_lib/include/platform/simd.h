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

// Header file for explicit simd support

#include <types.h>

#if defined(USE_SSE2_INTRINSICS)
	// if we're using intrinsics, we have to align vectors
#	define ALIGN_12BYTE_VECTORS
#endif

#if defined(ALIGN_12BYTE_VECTORS)
#	define ALIGN_VECTORS
#endif

// stack alignment macros
#if defined(__GNUC__)
#	define ALIGN_ATTR(x) __attribute__((aligned(x)))
#	define ALIGN_DECL(x)
#elif defined(_MSC_VER) && (_MSC_VER >= 1300)
#	define ALIGN_ATTR(x) 
#	define ALIGN_DECL(x) __declspec(align(x))
#	ifndef __inline__
#		define __inline__ inline
#	endif
//#		define _mm_malloc(a, b) _aligned_malloc(a, b)
//#		define _mm_free(a) _aligned_free(a)
#else
#	pragma message("Don't konw how to align data on stack.")
#endif

// simd headers and vector types
#if (defined __i386__ || defined __x86_64__ || defined(_MSC_VER) )
#	if defined(USE_SSE2_INTRINSICS) || defined(ALIGN_VECTORS)
#		include <emmintrin.h>	// SSE2
#		if defined(_MSC_VER) && (_MSC_VER >= 1300)
			// I'm not certain that these are neccisary, will have to test it
//#			define _mm_malloc(a, b) _aligned_malloc(a, b)
//#			define _mm_free(a) _aligned_free(a)
#		endif
		typedef __m128 vFloat;
#	else
		typedef float vFloat;
#	endif
#elif defined __ppc__ || defined __ppc64__
#	include <altivec.h>
	typedef vector float vFloat;
#else
	// no simd support.
	typedef float vFloat;
	//typedef float vFloat __attribute__((__vector_size__(16)));
#	define ALIGN_ATTR(x)
#	define ALIGN_DECL(x)
#endif


#ifdef ALIGN_VECTORS
#	define ALIGN_VEC_ATTR ALIGN_ATTR(16)
#	define ALIGN_VEC_DECL ALIGN_DECL(16)
#else
#	define ALIGN_VEC_ATTR
#	define ALIGN_VEC_DECL
#endif

// Rather or not to align 12-byte vectors.
#ifdef ALIGN_12BYTE_VECTORS
#	define ALIGN_VEC12_ATTR ALIGN_ATTR(16)
#	define ALIGN_VEC12_DECL ALIGN_DECL(16)
#else
#	define ALIGN_VEC12_ATTR
#	define ALIGN_VEC12_DECL
#endif

//#define ALIGN16(len) ((len + 15) & ~15)
namespace Shared { namespace Util {



}}//end namespace

#endif // _SHARED_UTIL_SIMD_H_
