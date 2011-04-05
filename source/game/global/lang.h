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

#ifndef _GLEST_GAME_LANG_H_
#define _GLEST_GAME_LANG_H_

#include "properties.h"
#include "util.h"
#include <set>

namespace Glest { namespace Global {

using namespace Shared::Util;
using std::set;

// =====================================================
//  class Lang
//
/// String table
// =====================================================

class Lang {
private:
	typedef map<string, Properties> FactionStrings;

private:
	string              m_locale; /**< Should have this format: language[_territory][.encoding][@script] */

	Properties          m_mainStrings;
	vector<string>      m_defeatStrings;
	Properties          m_techStrings;
	FactionStrings      m_factionStringsMap;
	Properties          m_scenarioStrings;

	string              m_mainFile;
	string              m_scenarioFile;
	string              m_techFile;
	map<string, string> m_factionFiles;

private:
	Lang() {}

public:
	static Lang &getInstance() {
		static Lang lang;
		return lang;
	}
	const string &getLocale() const			{return m_locale;}
	void setLocale(const string &locale);

	void loadScenarioStrings(const string &scenarioDir, const string &scenarioName);
	void loadTechStrings(const string &tech);
	void loadFactionStrings(const string &tech, set<string> &factions);

	string get(const string &s) const;
	string getTechString(const string &s);
	string getFactionString(const string &faction, const string &s);
	string getScenarioString(const string &s);
	string getDefeatedMessage() const;

	string getTranslatedFactionName(const string &factionName, const string &name) {
		string result = getFactionString(factionName, name);
		if (result == name) { // no custom name in faciton lang
			result = get(name); // try glob lang
			if (result.find("???") != string::npos) { // or just use name formatted
				result = formatString(name);
			}
		}
		return result;
	}

	string getTranslatedTechName(const string &name) {
		string result = getTechString(name);
		if (result == name) {
			result = get(name);
			if (result.find("???") != string::npos) {
				result = formatString(name);
			}
		}
		return result;
	}

	static vector<string>& getLookUpErrors();
};

}}//end namespace

#endif
