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

/**************************************************************************************************
 * Determine compiler and version
 */

/* Note the conflict between the outdated instructions for GCC_VERSION here:
 *     http://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
 * and intruduction of GCC_VERSION's definition in ansidecl.h starting in version 3.0 here:
 *     http://gcc.gnu.org/svn/gcc/branches/gcc-3_0-branch/include/ansidecl.h.
 *
 * None the less, having the patch level is preferrable (example, __deprecated being broken in
 * 3.3.0, but fixed in 3.3.1), so we're going to call ours GCC_FULL_VERSION and leave GCC_VERSION
 * alone. So for clarity's sake:
 *     GCC_VERSION      = Mmmm
 *     GCC_FULL_VERSION = Mmmpp
 * Where M = major, m = minor and p = patch level.
 *
 * Finally, Intel's compiler defines many of the same macros that gcc does just to tell you which
 * version is installed on the machine, nice huh?
 * http://software.intel.com/sites/products/documentation/hpc/compilerpro/en-us/cpp/lin/compiler_c/bldaps_cls/cppug_ccl/bldaps_macros_lin.htm
 */
#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#	define REALLY_GNUC 1
#	define GCC_FULL_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
# else /* just to be safe... */
#	ifdef REALLY_GNUC
#		undef REALLY_GNUC
#	endif
#	undef GCC_FULL_VERSION
#endif

/* ECC uses intrinsics for it's language extensions. */
#ifdef __INTEL_COMPILER
#	include <asm/intrinsics.h>
#endif


/**************************************************************************************************
 * Catch bad/old compilers
 */

#if REALLY_GNUC && GCC_FULL_VERSION < 30000
#	error Sorry, you're compiler is too old and we don't want to support it.
#endif

#if defined(_MSC_VER) && _MSC_VER < 1300 /* MSVC 2003 */
#	error This compiler has poor support for modern processors and tends to crash on some \
		  template structures and (depending upon your service pack) even produces bad code! \
		  Please upgrade to at least MSVC 2003 or remove this #error and proceed at your own risk.
#endif

/**************************************************************************************************
 * Check for language extensions and define macros
 */

/* NOTE: m$ cl does not have any branch hints.  See "Profile-Guided Optimizations" at
 * http://msdn.microsoft.com/en-us/library/e7k32f4k.aspx for more info. */

/* likely() and unlikely() branch hints */
#if (REALLY_GNUC && GCC_FULL_VERSION > 30000) || (defined(__IBMC__) && __IBMC__ >= 900) || \
		(defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 800)
#	define likely(x)				__builtin_expect(!!(x), 1)
#	define unlikely(x)				__builtin_expect(!!(x), 0)
#else
#	define likely(x)				(x)
#	define unlikely(x)				(x)
#endif

/* hot and cold reduces the need for likely() and unlikely() by effecting all calls to a function */
#if REALLY_GNUC && GCC_FULL_VERSION >= 40300 // TODO: add intel
#	define __hot					__attribute__((__hot__))
#	define __cold					__attribute__((__cold__))
#else
#	define __hot
#	define __cold
#endif

/* Use __must_check on any function who's return value should always be checked. */
#if REALLY_GNUC && GCC_FULL_VERSION >= 30400 // TODO: add intel
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
#if REALLY_GNUC && GCC_FULL_VERSION >= 30301
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
#if REALLY_GNUC
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
#	define __flatten				__attribute__((flatten))

	/* Screwed up prior to gcc 3.3 */
#	if GCC_FULL_VERSION < 30300
#		undef __used
#		define __used				__attribute__((__unused__))
#	endif
#endif

/* MS Visual Studio 2003 or higher compiler */
#ifdef _MSC_VER
#	define __always_inline			__forceinline
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


/**************************************************************************************************
 * Check for availability of C++11 features.
 *
 * NOTE: Checking for __cplusplus > 199711L alone (as defined at
 * http://www2.research.att.com/~bs/C++0xFAQ.html#0x) is not enough to ensure that a particular
 * feature is really supported, this just means that C++11 features are *enabled*.
 *
 * This is by no means complete.
 */

#if __cplusplus >= 199711L
#	define CXX11_ENABLED
#endif

#if defined(CXX11_ENABLED)

/* GCC
 * http://gcc.gnu.org/projects/cxx0x.html
 * http://wiki.apache.org/stdcxx/C%2B%2B0xCompilerSupport (appears to have alignas & alignof wrong for gcc)
 */
#if defined(REALLY_GNUC)
#	if GCC_FULL_VERSION > 30300
#		define CXX11_EXTERN_TEMPLATE
#		define CXX11_LONG_LONG
#	endif
#	if GCC_FULL_VERSION > 40300
#		define CXX11_C99_PREPROCESSOR
#		define CXX11_LONG_LONG
#		define CXX11_DECLTYPE_10
#		define CXX11_STATIC_ASSERTS
#		define CXX11_VARDIC_TEMPLATES_09
#		define CXX11_BUILTIN_TYPE_TRAITS
#		define CXX11_RVALUE_REFERENCES_10
#	endif
#	if GCC_FULL_VERSION > 40400
#		define CXX11_AUTO_09
#		define CXX11_AUTO_10
#		define CXX11_AUTO_FUNCTION_DECL
#		define CXX11_DEFAULT_DELETE_FUNC
#		define CXX11_STRONGLY_TYPED_ENUMS
#		define CXX11_INITIALIZER_LISTS
#		define CXX11_UNICODE_LITERALS
#		define CXX11_EXCEPTION_PTR
#		define CXX11_NAMESPACE_ASSOCIATION
#	endif
#	if GCC_FULL_VERSION > 40500
#		define CXX11_EXPLICIT_CONVERSION_OP
#		define CXX11_LAMBDAS_09
#		define CXX11_RAW_STRING_LITERALS
#		define CXX11_LOCAL_TYPES_AS_TEMPLATE_ARGS
#	endif
#	if GCC_FULL_VERSION > 40600
#		define CXX11_CONSTEXPR
#		define CXX11_RANGE_FOR
#		define CXX11_FORWARD_DECLARE_ENUMS
#		define CXX11_NULLPTR
#		define CXX11_UNRESTRICTED_UNIONS
#		define CXX11_NOEXCEPT
#	endif
#	if GCC_FULL_VERSION > 40700
#		define CXX11_OVERRIDE_AND_FINAL
#		define CXX11_IN_CLASS_MEMBER_INITIALIZERS
#		define CXX11_EXTENDED_FRIENDS
#	endif

/* MSVC++
 * http://blogs.msdn.com/b/vcblog/archive/2010/04/06/c-0x-core-language-features-in-vc10-the-table.aspx
 * http://msdn.microsoft.com/en-us/library/xey702bw%28v=vs.80%29.aspx
 */
#elif defined(_MSC_FULL_VER)
#	if _MSC_VER >= 1200 // MSVC v6.0
#		define CXX11_EXTERN_TEMPLATE
#		define CXX11_LONG_LONG
#	endif
#	if _MSC_VER >= 1300 // MSVC v7.0
#	endif
#	if _MSC_VER >= 1310 // MSVC v7.1 (2003)
#	endif
#	if _MSC_VER >= 1400 // MSVC v8.0 (2005)
#		define CXX11_BUILTIN_TYPE_TRAITS
#		define CXX11_OVERRIDE_AND_FINAL		// WARNING: not fully compliant with spec
#	endif
#	if _MSC_VER >= 1500 // MSVC v9.0 (2008)
#		define CXX11_EXTERN_TEMPLATE
#		define CXX11_LONG_LONG
#		define CXX11_RVALUE_REFERENCES_10
#		define CXX11_LOCAL_TYPES_AS_TEMPLATE_ARGS
#		define CXX11_C99_PREPROCESSOR		// WARNING: partial implementation
#	endif
#	if _MSC_VER >= 1600 // MSVC v10.0 (2010)
#		define CXX11_AUTO_09
#		define CXX11_AUTO_FUNCTION_DECL
#		define CXX11_STATIC_ASSERTS
#		define CXX11_DECLTYPE_10
#		define CXX11_LAMBDAS_10
#		define CXX11_LAMBDAS_11
#		define CXX11_NULLPTR
#		define CXX11_RVALUE_REFERENCES_20
#		define CXX11_EXCEPTION_PTR
#	endif
#	if _MSC_VER >= 1700 // MSVC v11.0 (not yet released)
#		define CXX11_DECLTYPE_11
#		define CXX11_RVALUE_REFERENCES_21
#	endif

/* Intel
 * http://wiki.apache.org/stdcxx/C%2B%2B0xCompilerSupport
 * http://software.intel.com/sites/products/documentation/studio/composer/en-us/2009/compiler_c/bldaps_cls/cppug_ccw/bldaps_macros_win.htm
 * https://secure.wikimedia.org/wikipedia/en/wiki/Intel_C%2B%2B_Compiler
 * http://software.intel.com/en-us/articles/c0x-features-supported-by-intel-c-compiler/
 */
#elif defined(__INTEL_COMPILER)
#	if __INTEL_COMPILER >= 900
#		define CXX11_EXTERN_TEMPLATE
#		define CXX11_LONG_LONG
#	endif
#	if __INTEL_COMPILER >= 1000
#		define CXX11_BUILTIN_TYPE_TRAITS
#	endif
#	if __INTEL_COMPILER >= 1100
#		define CXX11_AUTO_09
#		define CXX11_LAMBDAS_09
#		define CXX11_EXTENDED_FRIENDS
#		define CXX11_C99_PREPROCESSOR
#	endif
#	if __INTEL_COMPILER >= 1110	// (June 23, 2009)
#		define CXX11_RVALUE_REFERENCES_10
#	endif
#	if __INTEL_COMPILER >= 1200 // 12.0 "Intel C++ Composer XE 2011" (Nov 7, 2010)
#		define CXX11_LOCAL_TYPES_AS_TEMPLATE_ARGS
#		define CXX11_UNICODE_LITERALS // NOTE: compile with /Qoption,cpp,"--uliterals"
#		define CXX11_DECLTYPE_10
#		define CXX11_EXCEPTION_PTR
#	endif
#	if __INTEL_COMPILER >= 1210 // 12.1 "Intel C++ Composer XE 2011 SP1" (Nov 8, 2011)
#		define CXX11_NULLPTR // NOTE: compile with /Qoption,cpp,"--nullptr".
#		define CXX11_VARDIC_TEMPLATES_09
#		define CXX11_TEMPLATE_ALIAS
#		define CXX11_ATTRIBUTES
#		define CXX11_AUTO_FUNCTION_DECL
#		define CXX11_ALIAS_TEMPLATE
#	endif
#endif
#endif // defined(CXX11_ENABLED)

#if 0 // C++11 features not in any of our compilers
#	define CXX11_BUILTIN_TYPE_TRAITS
#	define CXX11_INHERITED_CONSTRUCTORS
#	define CXX11_THREAD_LOCAL_STORAGE
#	define CXX11_ALIGMENT // alignas and alignof
#	define CXX11_DELEGATING_CONSTRUCTORS
#endif



/**************************************************************************************************
 * Define macros to accomodate presence of absense of C++11 features.
 */

#ifdef CXX11_DEFAULT_DELETE_FUNC
#	define DEFAULT_FUNC				= default
#	define DELETE_FUNC				= delete
#else
#	define DEFAULT_FUNC
#	define DELETE_FUNC
#endif

#ifndef CXX11_OVERRIDE_AND_FINAL
#	define override
#	define final
#endif

#ifndef CXX11_NULLPTR
#	define nullptr NULL
#	define nullptr_t int
#endif

/**************************************************************************************************
 * Add function aliases as needed
 */

#if defined(_MSC_VER)
	// TODO: this needs a better check to make sure we don't already have it in some form
#	define __NEED_ROUND_FUNC
	// TODO: function inlines would be better (type safe, etc.)
#	define snprintf					_snprintf
#	define strtoull(np,ep,b)		_strtoui64(np,ep,b)
#	define strtof(np,ep)			((float)strtod(np,ep))
#	define strtold(np,ep)			((long double)strtod(np,ep))
#	define strcasecmp				stricmp
#	define log2(x)					(log(float(x))/log(2.f))
#endif

/* strtok in msvcrt uses TLS for thread safety and can therefore be aliased to strtok omitting the
 * POSIX-defined saveptr parameter of strtok_r.  The same is true for HP-UX's libc.*/
#if defined(_MSC_VER) || defined(_HPUX)
#	define strtok_r(a,b,c)			strtok(a,b)
// An inline is better, but we can't be sure strtok has been declared yet :(  Maybe these function
// alisas and inlines should get moved elsewhere, but the check for their need remain here?
//inline char *strtok_r(char *str, const char *delim, char **saveptr) {return strtok(str, delim);}
#endif

/**************************************************************************************************
 * Warn about stupid stuff
 */

#ifdef REALLY_GNUC
#	ifdef __GXX_RTTI
#		define __RTTI_ENABLED
//#		warning "Warning: you have RTTI enabled, which is not used and will only inflate your code"
#	endif
#	ifndef __EXCEPTIONS
#		warning "Warning: you have disabled exceptions and this program uses them."
#	endif
#elif defined(__INTEL_COMPILER)
#	ifdef __INTEL_RTTI__ && __INTEL_RTTI__ == 1
#		define __RTTI_ENABLED
//#		warning "Warning: you have RTTI enabled, which is not used and will only inflate your code"
#	endif
#	ifndef __EXCEPTIONS
#		warning "Warning: you have disabled exceptions and this program uses them."
#	endif
#elif defined(_MSC_FULL_VER)
#	ifdef _CPPRTTI
#		define __RTTI_ENABLED
//#		pragma message( "Warning: you have RTTI enabled, which is not used and will only inflate your code" )
#	endif
#	ifndef _CPPUNWIND
#		define __EXCEPTIONS_DISABLED
#		pragma message( "Warning: you have disabled exceptions and this program uses them." )
#	endif
#endif

#endif // _SHARED_LANGFEATURES_H_
