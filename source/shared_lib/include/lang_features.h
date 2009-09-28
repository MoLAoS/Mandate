// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================


/**
 * @file
 * Contains macros and other language-feature specific declarations, includes, etc.  The purpose
 * of this header is to encapsulate all code who's sole purpose is managing language features.
 *
 * NOTICE: Portions of this file have been copied from the Linux kernel tree
 * (include/linux/compiler*.h) and is copyright by their respective authors under the GPLv2 license.
 */


#ifndef _SHARED_LANGFEATURES_H_
#define _SHARED_LANGFEATURES_H_

/* Catch bad/old compilers */
#if 0 // oops, this is screwd up :(
#if defined(__GNUC__) && GCC_VERSION < 30000
#	error Sorry, you're compiler is too old and we don't want to support it.
#endif
#endif

#if defined(_MSC_VER) && _MSC_VER < 1300 /* MSVC 2003 */
#	error This compiler has poor support for modern processors and tends to crash on some \
		  template structures and (depending upon your service pack) even produces bad code! \
		  Please upgrade or remove this #error and proceed at your own risk.
#endif

/* use newer C++0x features of available */
#if __cplusplus > 199711L
#	define DELETE_FUNC				= delete
#else
#	define DELETE_FUNC
#endif


/* likely() and unlikely() branch hints */
#if (defined(__GNUC__) && __GNUC__ >= 3) || (defined(__IBMC__) && __IBMC__ >= 900)
#	define likely(x)				__builtin_expect(!!(x), 1)
#	define unlikely(x)				__builtin_expect(!!(x), 0)
#else /* m$ cl still doesn't have this, although they do have the profiler */
#	define likely(x)				(x)
#	define unlikely(x)				(x)
#endif

/* hot and cold reduces the need for likely() and unlikely() by effecting all calls to a function */
#if defined(__GNUC__) && GCC_VERSION >= 40300
#	define __hot					__attribute__((__hot__))
#	define __cold					__attribute__((__cold__))
#else
#	define __hot
#	define __cold
#endif

/* Use __must_check on any function who's return value should always be checked. */
#if defined(__GNUC__) && GCC_VERSION >= 30400
#	define __must_check				__attribute__((warn_unused_result))
#else
#	define __must_check
#endif

/* uninitialized_var is a trick to suppress uninitialized variable warning without generating
 * any code.
 */
#ifdef __GNUC__
#	ifdef __INTEL_COMPILER /* Intel compiler defines __GNUC__, but you have to do this differently */
#		define uninitialized_var(x) x
#	else
#		define uninitialized_var(x) x = x
#	endif
#else	/* none for msvc */
#	define uninitialized_var(x)
#endif

/* __deprecated and __deprecated_msg() mark functions, classes, etc. as deprecated */
#if defined(__GNUC__) && GCC_VERSION >= 30301
#	define __deprecated				__attribute__((deprecated))
#	define __deprecated_msg(x)		__attribute__((deprecated(x)))
#elif defined(_MSC_VER)
#	define __deprecated				__declspec(deprecated)
#	define __deprecated_msg(x)		__declspec(deprecated(x))
#else
#	define __deprecated
#	define __deprecated_msg(x)
#endif

/* Remaining attributes on GCC */
#if defined(__GNUC__)
#	ifndef __always_inline
#		define __always_inline		inline __attribute__((always_inline))
#	endif
#	define __noinline				__attribute__((noinline))
#	define __aligned_post(x)		__attribute__((aligned(x)))	/* put *after* declaration */
#	define __packed					__attribute__((packed))
#	define __pure					__attribute__((pure))
#	define __printf(a,b)			__attribute__((format(printf,a,b)))
#	define __maybe_unused			__attribute__((unused))
#	define __used					__attribute__((used))
#	define __novtable				/* automatic on GCC */
#	define __noreturn				__attribute__((noreturn))	/* put *before* declaration for ms compatibility */

	/* Screwed up prior to gcc 3.3 */
#	if GCC_VERSION < 30300
#		undef __used
#		define __used				__attribute__((__unused__))
#	endif
#endif

/* MS Visual Studio 2003 or higher compiler */
#ifdef _MSC_VER
#	define __always_inline			inline						/* none for MSVS */
#	define __noinline				__declspec(noinline)
#	define __aligned_pre(x)			__declspec(align(x))		/* put *before* declaration */
#	define __novtable				__declspec(novtable)
#	define __noreturn				__declspec(noreturn)		/* put *before* declaration */
#endif

#ifndef __aligned_pre
#	define __aligned_pre(x)
#endif
#ifndef __aligned_post
#	define __aligned_post(x)
#endif
#ifndef __packed
#	define __packed
#endif
#ifndef __pure
#	define __pure
#endif
#ifndef __printf
#	define __printf(a,b)
#endif
#ifndef __maybe_unused
#	define __maybe_unused
#endif
#ifndef __used
#	define __used
#endif

/* Some function aliases */
#if defined(WIN32) || defined(WIN64)
	extern float roundf(float f);
#	define snprintf					_snprintf
#	define strtoull(np,ep,b)		_strtoui64(np,ep,b)
#	define strtof(np,ep)			((float)strtod(np,ep))
#	define strtold(np,ep)			((long double)strtod(np,ep))
#endif


#endif // _SHARED_LANGFEATURES_H_
