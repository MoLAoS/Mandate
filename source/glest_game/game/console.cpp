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

#include "pch.h"
#include "console.h"

#include "lang.h"
#include "config.h"
#include "program.h"
#include "game_constants.h"

#include "leak_dumper.h"

#if defined(WIN32) || defined(WIN64)
	#define snprintf _snprintf
#endif

namespace Glest{ namespace Game{

// =====================================================
// 	class Console
// =====================================================

Console::Console(){
	//config
	maxLines= Config::getInstance().getConsoleMaxLines();
	timeout= (float)Config::getInstance().getConsoleTimeout();

	timeElapsed= 0.0f;
}

void Console::addStdMessage(const string &s){
	addLine(Lang::getInstance().get(s));
}

void Console::addStdMessage(const string &s, const string &param1, const string &param2, const string &param3) {
	string msg = Lang::getInstance().get(s);
	size_t bufsize = msg.size() + param1.size()  + param2.size()  + param3.size() + 32;
	char *buf = new char[bufsize];
	snprintf(buf, bufsize - 1, msg.c_str(), param1.c_str(), param2.c_str(), param3.c_str());
	addLine(buf);
	delete[] buf;
}

void Console::addLine(string line){
	lines.insert(lines.begin(), StringTimePair(line, timeElapsed));
	if(lines.size()>maxLines){
		lines.pop_back();
	}
}

void Console::update(){
	timeElapsed+= 1.f / Config::getInstance().getWorldUpdateFPS();

	if(!lines.empty()){
		if(lines.back().second<timeElapsed-timeout){
			lines.pop_back();
		}
    }
}

bool Console::isEmpty(){
	return lines.empty();
}


}}//end namespace
