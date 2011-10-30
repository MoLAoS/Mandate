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

#include "pch.h"

#include <stack>
//#include <stdexcept>
#include <iostream>
#include "lua_script.h"
#include "conversion.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Shared { namespace Lua {

// =====================================================
//	class LuaScript
// =====================================================

LuaScript::LuaScript()
		: luaState(NULL)
		, argumentCount(0) {
}

LuaScript::~LuaScript() {
	close();
}

void LuaScript::close() {
	if (luaState) {
		lua_close(luaState);
		luaState = NULL;
	}
}

void LuaScript::startUp() {
	close();
	luaState = luaL_newstate();

// 	luaL_openlibs(luaState);
	//copied from luaL_openlibs, commented unneeded libs
	static const luaL_Reg lualibs[] = {
		{"", luaopen_base},
// 		{LUA_LOADLIBNAME, luaopen_package},
		{LUA_TABLIBNAME, luaopen_table},
// 		{LUA_IOLIBNAME, luaopen_io},
// 		{LUA_OSLIBNAME, luaopen_os},
		{LUA_STRLIBNAME, luaopen_string},
		{LUA_MATHLIBNAME, luaopen_math},
// 		{LUA_DBLIBNAME, luaopen_debug},
		{NULL, NULL}
	};
	const luaL_Reg *lib = lualibs;
	for (; lib->func; lib++) {
		lua_pushcfunction(luaState, lib->func);
		lua_pushstring(luaState, lib->name);
		lua_call(luaState, 1, 0);
	}

	if (luaState == NULL) {
		throw runtime_error("Can not allocate lua state");
	}
	argumentCount = -1;
}

bool LuaScript::loadCode(const string &code, const string &name) {
	int errorCode = luaL_loadbuffer(luaState, code.c_str(), code.size(), name.c_str());
	if (errorCode != 0) {
		//DEBUG
		/*FILE *fp = fopen("bad.lua", "w");
		if (fp) {
			fprintf(fp, "%s", code.c_str());
			fclose(fp);
		}*/
		lastError = "Error loading lua code: " + errorToString(errorCode) + "\n"
			+ luaL_checkstring(luaState, -1);
		return false;
	}

	//run code
	errorCode = lua_pcall(luaState, 0, 0, 0);
	if (errorCode != 0) {
		lastError = "Error initializing lua: " + errorToString(errorCode) + "\n"
			+ luaL_checkstring(luaState, -1);
		return false;
	}
	return true;
}

bool LuaScript::isDefined(const string &name) {
	bool defined = false;
	lua_getglobal(luaState, name.c_str());
	if (lua_isfunction(luaState, -1)) {
		defined = true;
	}
	lua_pop(luaState, 1);
	return defined;
}

std::stack<string> tableStack;

bool LuaScript::getGlobal(const char *name) {
	assert(tableStack.empty());
	lua_getglobal(luaState, name);
	if (lua_istable(luaState, -1)) {
		//cout << "getGlobal: " << name << endl;
		tableStack.push(name);
		return true;
	} else {
		//cout << "getGlobal: '" << name << "' not found." << endl;
		lua_pop(luaState, -1);
		return false;
	}	
}

bool LuaScript::getTable(const char *name) {
	lua_getfield(luaState, -1, name);
	if (lua_istable(luaState, -1)) {
		//cout << "getTable: " << name << endl;
		tableStack.push(name);
		return true;
	} else {
		//cout << "getTable: '" << name << "' not found." << endl;
		lua_pop(luaState, 1);
		return false;
	}	
}

void LuaScript::popTable() {
	if (tableStack.empty()) {
		assert(false);
	}
	//string s = tableStack.top();
	tableStack.pop();
	//cout << "Popping '" << s << "'" << endl;
	assert(lua_istable(luaState, -1));
	lua_pop(luaState, 1);
}

void LuaScript::popAll() {
	while (!tableStack.empty()) {
		popTable();
	}
}

bool LuaScript::getBoolField(const char *key, bool &out_res) {
	lua_getfield(luaState, -1, key);
	LuaArguments args(luaState);
	try {
		out_res = args.getBoolean(-1);
		lua_pop(luaState, 1);
		return true;
	} catch (LuaError &e) {
		lua_pop(luaState, 1);
		return false;
	}
}

bool LuaScript::getVec2iField(const char *key, Vec2i &out_res) {
	lua_getfield(luaState, -1, key);
	LuaArguments args(luaState);
	try {
		out_res = args.getVec2i(-1);
		lua_pop(luaState, 1);
		return true;
	} catch (LuaError &e) {
		lua_pop(luaState, 1);
		return false;
	}
}

bool LuaScript::getVec4iField(const char *key, Vec4i &out_res) {
	lua_getfield(luaState, -1, key);
	LuaArguments args(luaState);
	try {
		out_res = args.getVec4i(-1);
		lua_pop(luaState, 1);
		return true;
	} catch (LuaError &e) {
		lua_pop(luaState, 1);
		return false;
	}
}

bool LuaScript::getStringField(const char *key, string &out_res) {
	lua_getfield(luaState, -1, key);
	LuaArguments args(luaState);
	try {
		out_res = args.getString(-1);
		lua_pop(luaState, 1);
		return true;
	} catch (LuaError &e) {
		lua_pop(luaState, 1);
		return false;
	}
}

bool LuaScript::getStringSet(const char *key, StringSet &out_res) {
	lua_getfield(luaState, -1, key);
	LuaArguments args(luaState);
	try {
		out_res = args.getStringSet(-1);
		lua_pop(luaState, 1);
		return true;
	} catch (LuaError &e) {
		lua_pop(luaState, 1);
		return false;
	}
}

bool LuaScript::getBoolField(const char *key) {
	lua_getfield(luaState, -1, key);
	LuaArguments args(luaState);
	bool res;
	try {
		res = args.getBoolean(-1);
		lua_pop(luaState, 1);
		return res;
	} catch (LuaError &e) {
		lua_pop(luaState, 1);
		throw e;
	}
}

Vec2i LuaScript::getVec2iField(const char *key) {
	lua_getfield(luaState, -1, key);
	LuaArguments args(luaState);
	Vec2i res;
	try {
		res = args.getVec2i(-1);
		lua_pop(luaState, 1);
		return res;
	} catch (LuaError &e) {
		lua_pop(luaState, 1);
		throw e;
	}
}

Vec4i LuaScript::getVec4iField(const char *key) {
	lua_getfield(luaState, -1, key);
	LuaArguments args(luaState);
	Vec4i res;
	try {
		res = args.getVec4i(-1);
		lua_pop(luaState, 1);
		return res;
	} catch (LuaError &e) {
		lua_pop(luaState, 1);
		throw e;
	}
}

string LuaScript::getStringField(const char *key) {
	lua_getfield(luaState, -1, key);
	LuaArguments args(luaState);
	try {
		string res = args.getString(-1);
		lua_pop(luaState, 1);
		return res;
	} catch (LuaError &e) {
		lua_pop(luaState, 1);
		throw e;
	}
}

StringSet LuaScript::getStringSet(const char *key) {
	lua_getfield(luaState, -1, key);
	LuaArguments args(luaState);
	StringSet res;
	try {
		res = args.getStringSet(-1);
		lua_pop(luaState, 1);
		return res;
	} catch (LuaError &e) {
		lua_pop(luaState, 1);
		throw e;
	}
}

bool LuaScript::luaCallback(const string& functionName, int id, int userData) {
	lua_getglobal(luaState, functionName.c_str());
	lua_pushnumber(luaState, id);
	lua_pushnumber(luaState, userData);
	if (lua_pcall(luaState, 2, 0, 0)) {
		lastError = luaL_checkstring(luaState, -1);
		return false; // error
	}
	return true;
}

bool LuaScript::luaCall(const string& functionName) {
	lua_getglobal(luaState, functionName.c_str());
	argumentCount= 0;
	if (lua_pcall(luaState, argumentCount, 0, 0)) {
		lastError = luaL_checkstring(luaState, -1);
		return false; // error
	}
	return true;
}

bool LuaScript::luaDoLine(const string &str) {
	if (luaL_dostring(luaState, str.c_str())) {
		lastError = luaL_checkstring(luaState, -1);
		return false;
	}
	return true;
}

void LuaScript::registerFunction(LuaFunction luaFunction, const string &functionName) {
	lua_pushcfunction(luaState, luaFunction);
	lua_setglobal(luaState, functionName.c_str());
}

string LuaScript::errorToString(int errorCode) {
	string error = "LUA:: ";
	switch (errorCode) {
		case LUA_ERRSYNTAX:
			error += "Syntax error";
			break;
		case LUA_ERRRUN:
			error += "Runtime error";
			break;
		case LUA_ERRMEM:
			error += "Memory allocation error";
			break;
		case LUA_ERRERR:
			error += "Error while running the error handler";
			break;
		default:
			error += "Unknown error";
	}
	error += string(": ") + luaL_checkstring(luaState, -1);
	fprintf(stderr, "%s\n", error.c_str());
	return error;
}

bool LuaScript::checkType(LuaType type, int ndx) const {
	LuaArguments args(luaState);
	return args.checkType(type, ndx);
}


// =====================================================
//	class LuaArguments
// =====================================================

LuaArguments::LuaArguments(lua_State *luaState) {
	this->luaState= luaState;
	argCount = lua_gettop(luaState);
	returnCount= 0;
	returnInTable = false;
	returnInTableCount = 0;
	returnTableIndex = 0;
}

/* Better LUA error handling, if one of these fail then an attempt to call a C++ function
 * has been made with invalid arguments, throw a LuaError with a description of the problem,
 * the 'callback' that called this will construct a nice error message.
 */
bool LuaArguments::getBoolean(int ndx) const {
	if (!checkType(LuaType::BOOLEAN, ndx)) {
		string emsg = "Argument " + descArgPos(ndx) + " expected Boolean, got " + getType(ndx) + ".\n";
		throw LuaError(emsg);
	}
	return lua_toboolean(luaState, ndx);
}

int LuaArguments::getInt(int ndx) const {
	if (!checkType(LuaType::NUMBER, ndx)) {
		string emsg = "Argument " + descArgPos(ndx) + " expected Number, got " + getType(ndx) + ".\n";
		throw LuaError(emsg);
	}
	return luaL_checkint(luaState, ndx);
}

float LuaArguments::getFloat(int ndx) const {
	if (!checkType(LuaType::NUMBER, ndx)) {
		string emsg = "Argument " + descArgPos(ndx) + " expected Number, got " + getType(ndx) + ".\n";
		throw LuaError(emsg);
	}
	return float(luaL_checknumber(luaState, ndx));
}

string LuaArguments::getString(int ndx) const {
	if (!checkType(LuaType::STRING, ndx)) {
		string emsg = "Argument " + descArgPos(ndx) + " expected String, got " + getType(ndx) + ".\n";
		throw LuaError(emsg);
	}
	return luaL_checkstring(luaState, ndx);
}

Vec2i LuaArguments::getVec2i(int ndx) const {
	checkTable(ndx, 2u);
	Vec2i v;
	try {
		// push a nil key to the top of the stack since lua_next requires the key to be on top
		lua_pushnil(luaState);
		for (int i = 1; i <= 2; ++i) {
			lua_next(luaState, ndx-1); // table is now one index back, push next element on top
			if (!checkType(LuaType::NUMBER)) throw i;
			v.raw[i - 1] = lua_tointeger(luaState, -1); // store the value into the vector
			lua_pop(luaState, 1);	// and pop it off the stack
		}
		lua_pop(luaState, 1); // pop our nil key
	} catch (int element) {
		string badType = getType(-1);
		lua_pop(luaState, 2);
		throw LuaError("Argument " + descArgPos(ndx) + " element " + intToStr(element)
			+ " expected Number got " + badType + ".\n");
	}
	return v;
}

Vec3i LuaArguments::getVec3i(int ndx) const {
	checkTable(ndx, 3u);
	Vec3i v;
	try {
		lua_pushnil(luaState);
		for (int i = 1; i <= 3; ++i) {
			lua_next(luaState, ndx-1);
			if (!checkType(LuaType::NUMBER)) throw i;
			v.raw[i - 1] = lua_tointeger(luaState, -1);
			lua_pop(luaState, 1);
		}
		lua_pop(luaState, 1);
	} catch (int element) {
		string badType = getType(-1);
		lua_pop(luaState, 2);
		throw LuaError("Argument " + descArgPos(ndx) + " element " + intToStr(element)
			+ " expected Number got " + badType + ".\n");
	}
	return v;
}

Vec4i LuaArguments::getVec4i(int ndx) const {
	checkTable(ndx, 4);
	Vec4i v;
	try {
		lua_pushnil(luaState);
		for (int i = 1; i <= 4; ++i) {
			lua_next(luaState, ndx-1);
			if (!checkType(LuaType::NUMBER)) throw i;
			v.raw[i - 1] = lua_tointeger(luaState, -1);
			lua_pop(luaState, 1);
		}
		lua_pop(luaState, 1);
	} catch (int element) {
		string badType = getType(-1);
		lua_pop(luaState, 2);
		throw LuaError("Argument " + descArgPos(ndx) + " element " + intToStr(element)
			+ " expected Number got " + badType + ".\n");
	}
	return v;
}

StringSet LuaArguments::getStringSet(int ndx) const {
	size_t n = checkTable(ndx, 1, 4);
	StringSet res;
	try {
		lua_pushnil(luaState);
		for (int i = 1; i <= n; ++i) {
			lua_next(luaState, ndx - 1);
			if (!checkType(LuaType::STRING)) throw i;
			res[i - 1] = luaL_checkstring(luaState, -1);
			lua_pop(luaState, 1);
		}
		lua_pop(luaState, 1);
	} catch (int element) {
		string badType = getType(-1);
		lua_pop(luaState, 2);
		throw LuaError("Argument " + descArgPos(ndx) + " element " + intToStr(element)
			+ " expected String got " + badType + ".\n");
	}
	return res;
}

void LuaArguments::startReturnTable() {
	if (returnInTable) {
		throw runtime_error("error: attempt to return table within table to lua, not currently supported.");
	}
	returnInTable = true;
	lua_newtable(luaState);
	returnTableIndex = lua_gettop(luaState);
	++returnCount;
}

void LuaArguments::endReturnTable() {
	if (!returnInTable) {
		throw runtime_error("error: LuaArguments::endReturnTable() called without matching LuaArguments::startReturnTable().");
	}
	returnInTable = false;
	returnTableIndex = 0;
	returnInTableCount = 0;
}

void LuaArguments::returnInt(int value) {
	if (returnInTable) {
		++returnInTableCount;
		lua_pushinteger(luaState, value);
		lua_rawseti(luaState, -2, returnInTableCount);
	} else {
		++returnCount;
		lua_pushinteger(luaState, value);
	}
}

void LuaArguments::returnString(const string &value){
	if (returnInTable) {
		++returnInTableCount;
		lua_pushstring(luaState, value.c_str());
		lua_rawseti(luaState, -2, returnInTableCount);
	} else {
		++returnCount;
		lua_pushstring(luaState, value.c_str());
	}
}

void LuaArguments::returnVec2i(const Vec2i &value){
	if (returnInTable) {
		++returnInTableCount;
	} else {
		++returnCount;
	}
	lua_newtable(luaState);
	lua_pushnumber(luaState, value.x);
	lua_rawseti(luaState, -2, 1);
	lua_pushnumber(luaState, value.y);
	lua_rawseti(luaState, -2, 2);
	if (returnInTable) {
		lua_rawseti(luaState, -2, returnInTableCount);
	}
}

void LuaArguments::returnBool(bool value){
	if (returnInTable) {
		++returnInTableCount;
		lua_pushboolean(luaState, value);
		lua_rawseti(luaState, -2, returnInTableCount);
	} else {
		++returnCount;
		lua_pushboolean(luaState, value);
	}
}


const char* LuaArguments::getType(int ndx) const {
	if (lua_isnumber(luaState, ndx)) {
		return "Number";
	} else if (lua_isstring(luaState, ndx)) {
		return "String";
	} else if (lua_istable(luaState, ndx)) {
		return "Table";
	} else if (lua_isboolean(luaState, ndx)) {
		return "Boolean";
	} else if (lua_isnil(luaState, ndx)) {
		return "Nil";
	} else {
		return "Unknown";
	}
}

bool LuaArguments::checkType(LuaType type, int ndx) const {
	switch (type) {
		case LuaType::NIL:
			return lua_isnil(luaState, ndx);
		case LuaType::NUMBER:
			return lua_isnumber(luaState, ndx);
		case LuaType::STRING:
			return lua_isstring(luaState, ndx);
		case LuaType::BOOLEAN:
			return lua_isboolean(luaState, ndx);
		case LuaType::TABLE:
			return lua_istable(luaState, ndx);
		case LuaType::FUNCTION:
			return lua_isfunction(luaState, ndx);
		default:
			INVARIANT(false, "LuaArguments::checkType() passed bad arg.");
			return false;
	}
}

void LuaArguments::checkTable(int ndx, size_t size) const {
	if (!checkType(LuaType::TABLE, ndx)) {
		string emsg = "Argument " + descArgPos(ndx) + " expected Table, got " + getType(ndx) + ".\n";
		throw LuaError(emsg);
	}
	size_t tableSize = lua_objlen(luaState, ndx);
	if (tableSize != size) {
		string emsg = "Argument " + descArgPos(ndx) + " expected Table with " + intToStr(size) 
			+ " elements, got Table with " + intToStr(tableSize) + " elements.\n";
		throw LuaError(emsg);
	}
}

size_t LuaArguments::checkTable(int ndx, size_t minSize, size_t maxSize) const {
	if (!checkType(LuaType::TABLE, ndx)) {
		string emsg = "Argument " + descArgPos(ndx) + " expected Table, got " + getType(ndx) + ".\n";
		throw LuaError(emsg);
	}
	size_t tableSize = lua_objlen(luaState, ndx);
	if (tableSize < minSize || tableSize > maxSize) {
		string emsg = "Argument " + descArgPos(ndx) + " expected Table with " 
			+ intToStr(minSize) + " to " + intToStr(maxSize) + " elements, got Table with "
			+ intToStr(tableSize) + " elements.\n";
		throw LuaError(emsg);
	}
	return tableSize;
}

string LuaArguments::descArgPos(int ndx) const {
	INVARIANT(ndx, "LuaArguments::descArgPos() passed 0. Invalid stack index.");
	if (ndx > 0) {
		return intToStr(ndx);
	}
	return intToStr(argCount + 1 + ndx);
}

}}//end namespace
