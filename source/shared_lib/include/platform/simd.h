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

#ifdef _MSC_VER
#	if defined _M_IX86 || defined _M_X64 || defined _M_IA64
#		include <emmintrin.h>
#	else
#		error Unsupported architecture
#	endif
#else
#	if defined __i386__ || defined __x86_64__ || defined __IA64__
#		include <emmintrin.h>
#	else
#		error Unsupported architecture
#	endif
#endif

#endif // _SHARED_UTIL_SIMD_H_
