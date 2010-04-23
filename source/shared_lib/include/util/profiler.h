// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2009-2010 James McCulloch
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_PROFILER_H_
#define _SHARED_UTIL_PROFILER_H_

//#define SL_PROFILE
//#define SL_TRACE

// SL_PROFILE controls if profile is enabled or not
// SL_TRACE controls function tracing

// The Profiler and Section classes have been 'hidden away', just put _PROFILE_FUNCTION(); at the 
// beginning of any function you want timed.

#include <string>
using std::string;

namespace Shared { namespace Util {

#ifndef SL_PROFILE
#	define _PROFILE_FUNCTION() {}
#	define _PROFILE_SCOPE(name) {}
	namespace Profile {
		inline void profileEnd() {}
	}

#else // SL_PROFILE

	namespace Profile {
		void profileEnd();
		void sectionBegin(const string &name);
		void sectionEnd(const string &name);
	}

	/** Helper, created on stack at start of functions to profile */
	class ProfileSection {
	private:
		const string name;

	public:
		ProfileSection(const char *name) : name(name) {
			Profile::sectionBegin(name);
		}
		~ProfileSection() {
			Profile::sectionEnd(name);
		}
	};

#	define _PROFILE_FUNCTION() Shared::Util::ProfileSection _func_profile(__FUNCTION__)
#	define _PROFILE_SCOPE(name) Shared::Util::ProfileSection _func_profile(name)

#endif //SL_PROFILE

#ifdef SL_TRACE
	namespace Trace {
		class FunctionTrace {
		private:
			const char *name;
			int callDepth;

		public:
			FunctionTrace(const char *name);
			~FunctionTrace();
		};
	}
#	define _TRACE_FUNCTION()  Shared::Util::Trace::FunctionTrace _trace_function(__FUNCTION__)
#else
#	define _TRACE_FUNCTION()
#endif

}}//end namespace Shared::Util


#endif 
