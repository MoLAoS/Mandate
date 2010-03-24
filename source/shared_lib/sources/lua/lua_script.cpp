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

//#include <stdexcept>
#include <iostream>
#include "lua_script.h"
#include "conversion.h"
#include "leak_dumper.h"

using namespace std;
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
	luaL_openlibs(luaState);
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
		lastError = "Error loading lua code: " + errorToString(errorCode);
		return false;
	}

	//run code
	errorCode = lua_pcall(luaState, 0, 0, 0) != 0;
	if (errorCode != 0) {
		lastError = "Error initializing lua: " + errorToString(errorCode);
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

bool LuaScript::luaCallback(const string& functionName, int id, int userData) {
	lua_getglobal(luaState, functionName.c_str());
	lua_pushnumber(luaState, id);
	lua_pushnumber(luaState, userData);
	if ( lua_pcall(luaState, 2, 0, 0) ) {
		lastError = luaL_checkstring(luaState, 1);
		return false; // error
	}
	return true;
}

bool LuaScript::luaCall(const string& functionName) {
	lua_getglobal(luaState, functionName.c_str());
	argumentCount= 0;
	if ( lua_pcall(luaState, argumentCount, 0, 0) ) {
		lastError = luaL_checkstring(luaState, 1);
		return false; // error
	}
	return true;
}

bool LuaScript::luaDoLine(const string &str) {
	if ( luaL_dostring(luaState, str.c_str()) ) {
		lastError = luaL_checkstring(luaState, 1);
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
	cerr << error << endl;
	return error;
}

// =====================================================
//	class LuaArguments
// =====================================================

LuaArguments::LuaArguments(lua_State *luaState){
	this->luaState= luaState;
	args = lua_gettop(luaState);
	returnCount= 0;
}

/* Better LUA error handling, if one of these fail then an attempt to call a C++ function
 * has been made with invalid arguments, throw a LuaError with a description of the problem,
 * the 'callback' that called this will construct a nice error message.
 */
bool LuaArguments::getBoolean ( int ndx ) const {
	if ( !lua_isboolean ( luaState, ndx ) ) {
		string emsg = "Argument " + intToStr(-ndx) + " expected Boolean, got " + getType(ndx) + ".\n";
		throw LuaError(emsg);
	}
	return lua_toboolean(luaState, ndx);
}

int LuaArguments::getInt(int argumentIndex) const{
	if(!lua_isnumber(luaState, argumentIndex)){
		string emsg = "Argument " + intToStr(-argumentIndex) + " expected Number, got " + getType(argumentIndex) + ".\n";
		throw LuaError(emsg);
	}
	return luaL_checkint(luaState, argumentIndex);
}

string LuaArguments::getString(int argumentIndex) const{
	if(!lua_isstring(luaState, argumentIndex)){
		string emsg = "Argument " + intToStr(-argumentIndex) + " expected String, got " + getType(argumentIndex) + ".\n";
		throw LuaError(emsg);
	}
	return luaL_checkstring(luaState, argumentIndex);
}

Vec2i LuaArguments::getVec2i(int argumentIndex) const{
	Vec2i v;
	
	if ( ! lua_istable(luaState, argumentIndex ) ) {
		string emsg = "Argument " + intToStr(-argumentIndex) + " expected Table, got " + getType(argumentIndex) + ".\n";
		throw LuaError(emsg);
	}
	if ( luaL_getn(luaState, argumentIndex) != 2 ) {
		string emsg = "Argument " + intToStr(-argumentIndex) + " expected Table with two elements, got Table with " 
			+ intToStr(luaL_getn(luaState, argumentIndex)) + " elements.\n";
		throw LuaError(emsg);
	}
	lua_rawgeti(luaState, argumentIndex, 1);
	v.x= luaL_checkint(luaState, argumentIndex);
	lua_pop(luaState, 1);

	lua_rawgeti(luaState, argumentIndex, 2);
	v.y= luaL_checkint(luaState, argumentIndex);
	lua_pop(luaState, 1);

	return v;
}

Vec4i LuaArguments::getVec4i(int ndx) const {
	Vec4i v;
	if ( ! lua_istable(luaState, ndx) ) {
		string emsg = "Argument " + intToStr(ndx) + " expected Table, got " + getType(ndx) + ".\n";
		throw LuaError ( emsg );
	}
	if ( luaL_getn(luaState, ndx) != 4 ) {
		string emsg = "Argument " + intToStr(ndx) + " expected Table with four elements, got Table with " 
			+ intToStr(luaL_getn(luaState, ndx)) + " elements.\n";
		throw LuaError(emsg);
	}

	lua_rawgeti(luaState, ndx, 1);
	v.x= luaL_checkint(luaState, ndx);
	lua_pop(luaState, 1);

	lua_rawgeti(luaState, ndx, 2);
	v.y= luaL_checkint(luaState, ndx);
	lua_pop(luaState, 1);

	lua_rawgeti(luaState, ndx, 3);
	v.z= luaL_checkint(luaState, ndx);
	lua_pop(luaState, 1);

	lua_rawgeti(luaState, ndx, 4);
	v.w= luaL_checkint(luaState, ndx);
	lua_pop(luaState, 1);

	return v;
}

void LuaArguments::returnInt(int value){
	++returnCount;
	lua_pushinteger(luaState, value);
}

void LuaArguments::returnString(const string &value){
	++returnCount;
	lua_pushstring(luaState, value.c_str());
}

void LuaArguments::returnVec2i(const Vec2i &value){
	++returnCount;

	lua_newtable(luaState);

	lua_pushnumber(luaState, value.x);
	lua_rawseti(luaState, -2, 1);

	lua_pushnumber(luaState, value.y);
	lua_rawseti(luaState, -2, 2);
}

void LuaArguments::returnBool(bool value){
	++returnCount;
	lua_pushboolean(luaState, value);
}

const char* LuaArguments::getType(int ndx) const {
	if ( lua_isnumber(luaState, ndx) ) {
		return "Number";
	}
	else if ( lua_isstring(luaState, ndx) ) {
		return "String";
	}
	else if ( lua_istable(luaState, ndx) ) {
		return "Table";
	}
	else if (lua_isboolean(luaState, ndx) ) {
		return "Boolean";
	}
	else if ( lua_isnil(luaState, ndx) ) {
		return "Nil";
	}
	else {
		return "Unknown";
	}
}

}}//end namespace
