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


namespace Glest{ namespace Game{


// =====================================================
// 	class Metrics
// =====================================================

Metrics::Metrics(){
	Config &config= Config::getInstance();

	virtualW= 1000;
	virtualH= 750;

	screenW= config.getDisplayWidth();
	screenH= config.getDisplayHeight();

	minimapX= 10;
	minimapY= 750-128-30+16;
	minimapW= 128;
	minimapH= 128;

	displayX= 800;
	displayY= 250;
	displayW= 128;
	displayH= 480;
}


}}// end namespace
