// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_LUA_LUASCRIPT_H_
#define _SHARED_LUA_LUASCRIPT_H_

#include <stdexcept>
#include <string>
#include <lua.hpp>

#include "vec.h"
#include "conversion.h"

using std::string;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Vec4i;
using namespace Shared::Util;

namespace Shared { namespace Lua {

typedef lua_State LuaHandle;
typedef int(*LuaFunction)(LuaHandle*);

class LuaError : public runtime_error {
public:
	LuaError(const string &msg = string("Lua Error")) : runtime_error(msg) {}
	string desc() const {return what();}
};

// =====================================================
//	class LuaScript
// =====================================================

class LuaScript {
private:
	LuaHandle *luaState;
	int argumentCount;
	string lastError;

public:
	LuaScript();
	~LuaScript();

	void startUp();

	bool loadCode(const string &code, const string &name);

	bool isDefined( const string &functionName );
	bool luaCallback(const string& functionName, int id);
	bool luaCall(const string& functionName);

	string& getLastError() { return lastError; }

	void registerFunction(LuaFunction luaFunction, const string &functionName);

private:
	string errorToString(int errorCode);
	void close();
};

// =====================================================
//	class LuaArguments
// =====================================================

// class LuaStack

class LuaArguments {
private:
	lua_State *luaState;
	int returnCount;
	int args;

public:
	LuaArguments(lua_State *luaState);

	bool getBoolean(int ndx) const;
	int getInt(int argumentIndex) const;
	string getString(int argumentIndex) const;
	Vec2i getVec2i(int argumentIndex) const;
	Vec4i getVec4i(int argumentIndex) const;
	int getReturnCount() const					{return returnCount;}
	int getArgumentCount() const				{return args;}

	void returnInt(int value);
	void returnString(const string &value);
	void returnVec2i(const Vec2i &value);

	const char *getType ( int ndx ) const;
};

}}//end namespace

#endif
