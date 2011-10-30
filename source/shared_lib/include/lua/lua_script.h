// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
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
#include "util.h"

using std::string;
using Shared::Math::Vec2i;
using Shared::Math::Vec3i;
using Shared::Math::Vec4i;
using namespace Shared::Util;

namespace Shared { namespace Lua {

WRAPPED_ENUM( LuaType, NIL, NUMBER, STRING, BOOLEAN, TABLE, FUNCTION );

typedef lua_State LuaHandle;
typedef int(*LuaFunction)(LuaHandle*);

class LuaError : public runtime_error {
public:
	LuaError(const string &msg = string("Lua Error")) : runtime_error(msg) {}
	string desc() const {return what();}
};

class StringSet {
	string values[4];

public:
	StringSet(){}

	string& operator[](unsigned i) {
		assert(i < 4);
		return values[i];
	}
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
	void close();
	void atPanic(lua_CFunction func) {
		lua_atpanic(luaState, func);
	}

	bool loadCode(const string &code, const string &name);

	bool getGlobal(const char *tableName);
	bool getTable(const char *tableName);
	void popTable();
	void popAll();

	bool getBoolField(const char *key, bool &out_res);
	bool getVec2iField(const char *key, Vec2i &out_res);
	bool getVec4iField(const char *key, Vec4i &out_res);
	bool getStringField(const char *key, string &out_res);
	bool getStringSet(const char *key, StringSet &out_res);

	bool getBoolField(const char *key);
	Vec2i getVec2iField(const char *key);
	Vec4i getVec4iField(const char *key);
	string getStringField(const char *key);
	StringSet getStringSet(const char *key);

	bool isDefined(const string &functionName);
	bool luaCallback(const string& functionName, int id, int userData);
	bool luaCall(const string& functionName);

	bool luaDoLine(const string &str);

	string& getLastError() { return lastError; }

	void registerFunction(LuaFunction luaFunction, const string &functionName);
	bool checkType(LuaType type, int ndx = -1) const;

private:
	string errorToString(int errorCode);
};

// =====================================================
//	class LuaArguments
// =====================================================

// class LuaStack // better name?
class LuaArguments {
private:
	lua_State *luaState;
	int returnCount;
	int argCount;

	bool returnInTable;
	int returnInTableCount;
	int returnTableIndex;

public:
	LuaArguments(lua_State *luaState);

	bool getBoolean(int ndx) const;
	int getInt(int argumentIndex) const;
	float getFloat(int argumentIndex) const;
	string getString(int argumentIndex) const;
	Vec2i getVec2i(int argumentIndex) const;
	Vec3i getVec3i(int argumentIndex) const;
	Vec4i getVec4i(int argumentIndex) const;
	StringSet getStringSet(int ndx) const;
	int getReturnCount() const					{return returnCount;}
	int getArgumentCount() const				{return argCount;}

	void startReturnTable();
	void endReturnTable();

	void returnInt(int value);
	void returnString(const string &value);
	void returnVec2i(const Vec2i &value);
	void returnBool(bool val);

	const char* getType(int ndx) const;
	// check type of item on top of stack
	bool checkType(LuaType type, int ndx = -1) const;

private:
	// check for the presence of a table with the given size on the stack at index ndx
	void checkTable(int ndx, size_t size) const;
	size_t checkTable(int ndx, size_t minSize, size_t maxSize) const;

	string descArgPos(int ndx) const;
};

}}//end namespace

#endif
