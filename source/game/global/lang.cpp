// ==============================================================
// This file is part of Glest (www.glest.org)
//
// Copyright (C) 2001-2008 Martiño Figueroa
//
// You can redistribute this code and/or modify it under
// the terms of the GNU General Public License as published
// by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include <clocale>

#include "lang.h"
#include "logger.h"
#include "util.h"
#include "FSFactory.hpp"

#include "leak_dumper.h"

using std::exception;
using namespace Shared::Util;
using namespace Shared::PhysFS;

namespace Glest { namespace Global {

// =====================================================
//  class Lang
// =====================================================

void Lang::setLocale(const string &locale) {
	this->locale = locale;
	strings.clear();
	setlocale(LC_CTYPE, locale.c_str());
	string path = "gae/data/lang/" + locale + ".lng";
	strings.load(path);
	defeatStrings.clear();
	FileOps *f = g_fileFactory.getFileOps();
	path = "/gae/data/defeat_messages/" + locale + ".txt";
	try {
		f->openRead(path.c_str());
		int size = f->fileSize();
		char *buf = new char[size + 1];
		f->read(buf, size, 1);
		buf[size] = '\0';
		stringstream ss(buf);
		delete [] buf;
		char buffer[1024];
		while (!ss.eof()) {
			ss.getline(buffer, 1023);
			if (buffer[0] == ';' || buffer[0] == '\0' || buffer[0] == 10 || buffer[0] == 13) {
				continue;
			}
			string str(buffer);
			if (*(str.end() - 1) == 13) {
				str = str.substr(0, str.size() - 1);
			}
			if (!str.empty()) {
				defeatStrings.push_back(str);
			}
		}
	} catch (runtime_error &e) {
		defeatStrings.clear();
	}
	delete f; ///@todo since f is allocated in FSFactory it should be deleted there too.
}

string Lang::getDefeatedMessage() const {
	int seed = int(Chrono::getCurMicros());
	Random random(seed);
	if (defeatStrings.empty()) {
		return string("%s has been defeated.");
	} else {
		int ndx = random.randRange(0, defeatStrings.size() - 1);
		return defeatStrings[ndx];
	}
}

void Lang::loadScenarioStrings(const string &scenarioDir, const string &scenarioName) {
	string path = scenarioDir + "/" + scenarioName + "_" + locale + ".lng";

	scenarioStrings.clear();

	//try to load the current locale first
	if (fileExists(path)) {
		scenarioStrings.load(path);
	} else {
		//try english otherwise
		string path = scenarioDir + "/" + scenarioName + "/" + scenarioName + "_en.lng";
		if (fileExists(path)) {
			scenarioStrings.load(path);
		}
	}
}

string Lang::get(const string &s) const {
	try {
		return strings.getString(s);
	} catch (exception &) {
		return "???" + s + "???";
	}
}

string Lang::getScenarioString(const string &s) {
	try {
		return scenarioStrings.getString(s);
	} catch (exception &) {
		return "???" + s + "???";
	}
}

}}//end namespace
