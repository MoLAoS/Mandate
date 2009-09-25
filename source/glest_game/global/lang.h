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

namespace Glest { namespace Game {

using Shared::Util::Properties;

// =====================================================
//  class Lang
//
/// String table
// =====================================================

class Lang {
private:
	string locale;		/**< Should have this format: language[_territory][.encoding][@script] */
#if 0
	string language;
	string territory;
	string encoding;
	script script;
#endif
	Properties strings;
	Properties scenarioStrings;

private:
	Lang() {}

public:
	static Lang &getInstance() {
		static Lang lang;
		return lang;
	}
	const string &getLocale() const			{return locale;}
#if 0
	const string &getLanguage() const		{return language;}
	const string &getTerritory() const		{return territory;}
	const string &getEncoding() const		{return encoding;}
	const string &getScript() const			{return script;}
#endif

	void setLocale(const string &locale);
	void loadScenarioStrings(const string &scenarioDir, const string &scenarioName);
	string getScenarioString(const string &s);
	string get(const string &s) const;
};

}}//end namespace

#endif
