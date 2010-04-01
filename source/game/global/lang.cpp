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

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
//  class Lang
// =====================================================

void Lang::setLocale(const string &locale) {
	this->locale = locale;
	strings.clear();
	setlocale(LC_CTYPE, locale.c_str());
	string path = "gae/data/lang/" + locale + ".lng";
	strings.load(path);
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
