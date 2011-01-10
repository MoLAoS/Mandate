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

#include "metrics.h"
#include "element_type.h"

#include "leak_dumper.h"


namespace Glest { namespace Global {


// =====================================================
// 	class Metrics
// =====================================================

Metrics::Metrics() {
	Config &config = Config::getInstance();
	screenW = config.getDisplayWidth();
	screenH = config.getDisplayHeight();
}

}} // end namespace

