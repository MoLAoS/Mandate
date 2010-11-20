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
	m_locale = locale;
	
	m_mainStrings.clear();
	setlocale(LC_CTYPE, m_locale.c_str());
	string path = "gae/data/lang/" + m_locale + ".lng";
	m_mainStrings.load(path);

	m_defeatStrings.clear();
	FileOps *f = g_fileFactory.getFileOps();
	path = "/gae/data/defeat_messages/" + m_locale + ".txt";
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
				m_defeatStrings.push_back(str);
			}
		}
	} catch (runtime_error &e) {
		m_defeatStrings.clear();
	}
	delete f; ///@todo since f is allocated in FSFactory it should be deleted there too.
}

void Lang::loadScenarioStrings(const string &scenarioDir, const string &scenarioName) {
	string path = scenarioDir + "/" + scenarioName + "_" + m_locale + ".lng";
	m_scenarioStrings.clear();
	if (fileExists(path)) { // try to load the current m_locale first
		m_scenarioStrings.load(path);
	} else { // try english otherwise		
		string path = scenarioDir + "/" + scenarioName + "/" + scenarioName + "_en.lng";
		if (fileExists(path)) {
			m_scenarioStrings.load(path);
	}
	}
}

void Lang::loadTechStrings(const string &tech) {
	string path = "techs/" + tech + "/" + tech + "_" + m_locale + ".lng";
	m_techStrings.clear();
	if (fileExists(path)) { // try to load the current m_locale first
		m_techStrings.load(path);
	} else { // try english otherwise
		path = "techs/" + tech + "/" + tech + "_en.lng";
		if (fileExists(path)) {
			m_techStrings.load(path);
		}
	}
}

void Lang::loadFactionStrings(const string &tech, set<string> &factions) {
	foreach_const (set<string>, it, factions) {
		const string &faction = *it;
		Properties &p = m_factionStringsMap[faction];
		string prePath = "techs/" + tech + "/factions/" + faction + "/" + faction + "_";
		string path = prePath + m_locale + ".lng";
	if (fileExists(path)) {
			p.load(path);
	} else {
			path = prePath + "en.lng";
		if (fileExists(path)) {
				p.load(path);
		}
	}
	}
}

string Lang::get(const string &s) const {
	try {
		return m_mainStrings.getString(s);
	} catch (exception &) {
		return "???" + s + "???";
	}
}

string Lang::getScenarioString(const string &s) {
	try {
		return m_scenarioStrings.getString(s);
	} catch (exception &) {
		return "???" + s + "???";
	}
}

string Lang::getDefeatedMessage() const {
	int seed = int(Chrono::getCurMicros());
	Random random(seed);
	if (m_defeatStrings.empty()) {
		return string("%s has been defeated.");
	} else {
		int ndx = random.randRange(0, m_defeatStrings.size() - 1);
		return m_defeatStrings[ndx];
	}
}

string Lang::getTechString(const string &s) {
	try {
		return m_techStrings.getString(s);
	} catch (exception &) {
		return s;
	}
}

string Lang::getFactionString(const string &faction, const string &s) {
	try {
		return m_factionStringsMap[faction].getString(s);
	} catch (exception &) {
		return s;
	}
}

}}//end namespace
