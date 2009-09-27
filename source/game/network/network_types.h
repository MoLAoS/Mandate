// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_NETWORKTYPES_H_
#define _GLEST_GAME_NETWORKTYPES_H_

#include <string>
#include <limits.h>

#include "types.h"
#include "vec.h"
#include "command.h"
#include "socket.h"

using std::string;
using Shared::Platform::int8;
using Shared::Platform::int16;
using Shared::Platform::int32;
using Shared::Graphics::Vec2i;

namespace Glest { namespace Game {

class NetworkException : public runtime_error {
public:
	NetworkException(const string &msg) : runtime_error(msg) {}
	NetworkException(const char *msg) : runtime_error(msg) {}
};
	
// =====================================================
//	class NetworkString
// =====================================================

template<int S> class NetworkString : public NetworkWriteable {
private:
	uint16 size;	//size excluding null terminator, max of S - 1
	char buffer[S];
	string s;

public:
	NetworkString() /*: s() */{
		assert(S && S < USHRT_MAX);
		size = 0;
		*buffer = 0;
	}
	NetworkString(const string &str) {
		assert(S && S < USHRT_MAX);
		(*this) = str;
	}
	string getString() const			{return buffer;}
	void operator=(const string &str) {
		/*
		s = str;
		if(s.size() > S) {
			s.resize(S);
		}
		*/
		size = (uint16)(str.size() < S ? str.size() : S - 1);
		strncpy(buffer, str.c_str(), size);
		buffer[size] = 0;
	}

	size_t getNetSize() const {
		//return s.size() + sizeof(uint16);
		return sizeof(size) + size;
	}

	size_t getMaxNetSize() const {
		return sizeof(size) + sizeof(buffer);
	}

	void write(NetworkDataBuffer &buf) const {
		buf.write(size);
		buf.write(buffer, size);
	}

	void read(NetworkDataBuffer &buf) {
		buf.read(size);
		assert(size < S);
		buf.read(buffer, size);
		buffer[size] = 0;
	}
};

}}//end namespace

#endif
