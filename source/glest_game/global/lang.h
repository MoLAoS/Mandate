// ==============================================================
// This file is part of Glest (www.glest.org)
//
// Copyright (C) 2001-2008 Marti�o Figueroa
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
	string locale;
	Properties strings;
	Properties scenarioStrings;

private:
	Lang() {}

public:
	static Lang &getInstance() {
		static Lang lang;
		return lang;
	}
	void setLocale(const string &locale);
	void loadScenarioStrings(const string &scenarioDir, const string &scenarioName);
	string getScenarioString(const string &s);
	string get(const string &s) const;
};

}}//end namespace

#endif
