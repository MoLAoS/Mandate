// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008      Jaagup Repän <jrepan@gmail.com>,
//				  2008      Daniel Santos <daniel.santos@pobox.com>
//				  2009-2010 James McCulloch <silnarm@gmail.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "biome.h"

#include <cassert>

#include "map.h"
#include "world.h"
#include "tileset.h"
#include "logger.h"
#include "tech_tree.h"

#include "leak_dumper.h"
#include "fixed.h"
#include "sim_interface.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Math;

namespace Glest { namespace Sim {
using Main::Program;
using Search::Cartographer;
using Gui::Selection;
// =====================================================
// 	class Biome
// =====================================================
bool Biome::load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft) {
    bool loadOk = true;
    name = baseNode->getAttribute("name")->getRestrictedValue();
    tempMin = baseNode->getAttribute("min-temp")->getIntValue();
    tempMax = baseNode->getAttribute("max-temp")->getIntValue();
    moistureLevel = baseNode->getAttribute("moisture")->getIntValue();
    altitudeMin = baseNode->getAttribute("min-alt")->getIntValue();
    altitudeMax = baseNode->getAttribute("max-alt")->getIntValue();
    tileset.load(dir, g_world.getTechTree());
    const XmlNode *emanationsNode = baseNode->getChild("emanations", 0, false);
    if (emanationsNode) {
        emanations.resize(emanationsNode->getChildCount());
        for (int i = 0; i < emanationsNode->getChildCount(); ++i) {
            const XmlNode *emanationNode = emanationsNode->getChild("emanation", i);
            if (!emanations[i]->load(emanationNode, dir, tt, ft)) {
                loadOk = false;
            }
        }
    }
    return loadOk;
}

}}//end namespace


