// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

// NO pre-compiled header!
// game_constants.h is in it, without GAME_CONSTANTS_DEF defined.

// The sole purpose of this file is to define EnumNames objects.

// any header which contains STRINGY_ENUM decs needs to be included here,

// This tells the macros in util.h to instantiate EnumNames objects, rather than just declare them extern
#define GAME_CONSTANTS_DEF

#include "lang_features.h"

#include "game_constants.h"
#include "simulation_enums.h"
#include "input_enums.h"

#include "menu_state_root.h"
#include "widget_style.h"
#include "widget_config.h"
#include "particle.h"
#include "cartographer.h"
#include "debug_stats.h"
#include "logger.h"

namespace Glest {

	void no_op() {

	}

}
