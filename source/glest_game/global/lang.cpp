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

#include <stdexcept>

#include "lang.h"
#include "logger.h"
#include "util.h"

#include "leak_dumper.h"


using namespace std;

namespace Glest { namespace Game {

// =====================================================
//  class Lang
// =====================================================


string Lang::get(const string &s) const {
	try {
		return langStrings.getString(s);
	} catch (exception &) {
		return "???" + s + "???";
	}
}


}}//end namespace
