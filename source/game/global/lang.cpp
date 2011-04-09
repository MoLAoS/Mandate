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

using Util::Logger;

// =====================================================
//  class Lang
// =====================================================

void Lang::setLocale(instring locale) {
	g_logger.logProgramEvent("Setting locale to '" + locale + "'");
	m_locale = locale;
	setlocale(LC_CTYPE, m_locale.c_str());
	
	m_mainFile = "gae/data/lang/" + m_locale + ".lng";
	m_mainStrings.clear();
	m_mainStrings.load(m_mainFile);

	string path = "/gae/data/defeat_messages/" + m_locale + ".txt";
	m_defeatStrings.clear();
	FileOps *f = g_fileFactory.getFileOps();
	char *buf = 0;
	try {
		f->openRead(path.c_str());
		int size = f->fileSize();
		buf = new char[size + 1];
		f->read(buf, size, 1);
		buf[size] = '\0';
		stringstream ss(buf);
		delete [] buf;
		buf = 0;
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
		delete [] buf;
		m_defeatStrings.clear();
	}
	delete f;
}

void Lang::loadScenarioStrings(instring scenarioDir, instring scenarioName) {
	m_scenarioFile = scenarioDir + "/" + scenarioName + "_" + m_locale + ".lng";
	m_scenarioStrings.clear();
	if (fileExists(m_scenarioFile)) { // try to load the current m_locale first
		m_scenarioStrings.load(m_scenarioFile);
	} else { // try english otherwise
		m_scenarioFile = scenarioDir + "/" + scenarioName + "/" + scenarioName + "_en.lng";
		if (fileExists(m_scenarioFile)) {
			m_scenarioStrings.load(m_scenarioFile);
		}
	}
}

void Lang::loadTechStrings(instring tech) {
	m_techFile = "techs/" + tech + "/lang/" + tech + "_" + m_locale + ".lng";
	m_techStrings.clear();
	if (fileExists(m_techFile)) { // try to load the current m_locale first
		m_techStrings.load(m_techFile);
	} else { // try english otherwise
		m_techFile = "techs/" + tech + "/lang/" + tech + "_en.lng";
		if (fileExists(m_techFile)) {
			m_techStrings.load(m_techFile);
		}
	}
}

void Lang::loadFactionStrings(instring tech, set<string> &factions) {
	foreach_const (set<string>, it, factions) {
		instring faction = *it;
		Properties &p = m_factionStringsMap[faction];
		string prePath = "techs/" + tech + "/factions/" + faction + "/lang/" + faction + "_";
		m_factionFiles[faction] = prePath + m_locale + ".lng";
		if (fileExists(m_factionFiles[faction])) {
			p.load(m_factionFiles[faction]);
		} else {
			m_factionFiles[faction] = prePath + "en.lng";
			if (fileExists(m_factionFiles[faction])) {
				p.load(m_factionFiles[faction]);
			}
		}
	}
}

typedef pair<string, string> LangError;

class LangErrors : private vector<LangError> {
public:
	LangErrors() {}

	void addLookUpMiss(instring key, instring file) {
		for (const_iterator it = begin(); it != end(); ++it) {
			if (it->first == key && it->second == file) {
				return;
			}
		}
		push_back(make_pair(key, file));
	}

	int getLookUpMissCount() const { return size(); }
	string getLookUpMiss(int i) {
		return "The key '" + (*this)[i].first + "' was looked up in " 
			+ (*this)[i].second + " and not found.";
	}
};
	
LangErrors f_langErrors;


string Lang::get(instring s) const {
	try {
		return m_mainStrings.getString(s);
	} catch (exception &) {
		f_langErrors.addLookUpMiss(s, m_mainFile);
		return "???" + s + "???";
	}
}

const string emptyString = "";

bool Lang::propertiesLookUp(const Properties &props, instring in_key, outstring out_res) const {
	out_res = props.getString(in_key, emptyString);
	return !out_res.empty();
}

string Lang::getScenarioString(instring s) const {
	try {
		return m_scenarioStrings.getString(s);
	} catch (exception &) {
		f_langErrors.addLookUpMiss(s, m_scenarioFile);
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

///@deprecated use lookUp()
string Lang::getTechString(instring s) const {
	try {
		return m_techStrings.getString(s);
	} catch (exception &) {
		f_langErrors.addLookUpMiss(s, m_techFile);
		return s;
	}
}

///@deprecated use lookUp()
string Lang::getFactionString(instring faction, instring s) const {
	FactionStrings::const_iterator it = m_factionStringsMap.find(faction);
	if (it == m_factionStringsMap.end()) {
		throw runtime_error("Invalid faction name passed to Lang::getFactionString() '" + faction + "'");
	}
	try {
		return it->second.getString(s);
	} catch (exception &) {
		map<string, string>::const_iterator it = m_factionFiles.find(faction);
		if (it != m_factionFiles.end()) {
			string file = it->second;
			f_langErrors.addLookUpMiss(s, file);
		}
		return s;
	}
}

bool Lang::cascadingLookUp(instring key, instring faction, outstring out_result) const {
	// 1. Validate faction string, and get faction PropertyMap
	FactionStrings::const_iterator it = m_factionStringsMap.find(faction);
	if (it == m_factionStringsMap.end()) {
		throw runtime_error("Invalid faction name passed to Lang::cascadingLookUp() '" + faction + "'");
	}
	// 2. try faction langfile,
	if (propertiesLookUp(it->second, key, out_result)) {
		return true;
	}
	// 3. try tech-tree langfile
	if (propertiesLookUp(m_techStrings, key, out_result)) {
		return true;
	}
	// 4. use global langfile
	if (propertiesLookUp(m_mainStrings, key, out_result)) {
		return true;
	}
	out_result = "???" + key + "???";
	return false;
}

bool Lang::replaceLookUp(instring in_key, instring in_faction, instring in_param, outstring out_res) const {
	if (cascadingLookUp(in_key, in_faction, out_res)) {
		string::size_type p = out_res.find("%s");
		if (p != string::npos) {
			out_res.replace(p, 2, in_param);
		}
		return true;
	}
	return false;
}

bool Lang::cascadingLookUp(instring in_key, instring in_faction, instring in_param, outstring out_res) const {
	string param;
	cascadingLookUp(in_param, in_faction, param);
	return replaceLookUp(in_key, in_faction, param, out_res);
}

bool Lang::lookUp(instring in_key, instring in_faction, instringList in_params, outstring out_res) const {
	string list;
	const int lastIndex = in_params.size() - 1;
	for (int i=0; i < lastIndex; ++i) {
		string res;
		g_lang.lookUp(in_params[i], in_faction, res);
		list += (i == lastIndex ? " & " : i ? ", " : "") + res;
	}
	return replaceLookUp(in_key, in_faction, list, out_res);
}

vector<string>& Lang::getLookUpErrors() {
	static vector<string> errors;
	errors.clear();
	for (int i=0; i < f_langErrors.getLookUpMissCount(); ++i) {
		errors.push_back(f_langErrors.getLookUpMiss(i));
	}
	return errors;
}

}}//end namespace
