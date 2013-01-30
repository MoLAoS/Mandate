// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008      Jaagup Repän <jrepan@gmail.com>,
//				  2008      Daniel Santos <daniel.santos@pobox.com>
//				  2009-2010 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_BIOME_H_
#define _GLEST_GAME_BIOME_H_

#include <cassert>
#include <map>
#include <set>

#include "pch.h"
#include "vec.h"
#include "math_util.h"
#include "logger.h"
#include "game_constants.h"
#include "tileset.h"
#include "effect_type.h"
#include "exceptions.h"

using namespace Shared::Math;
using Shared::Graphics::Texture2D;
using namespace Glest::Util;
using namespace Glest::ProtoTypes;

namespace Glest { namespace Sim {

// =====================================================
// 	class Biome
//
///	Biome info for tiles
// =====================================================
class Biome {
private:
    string name;
    int tempMin;
    int tempMax;
    int moistureLevel;
    int altitudeMin;
    int altitudeMax;
    Tileset tileset;
    Emanations emanations;
public:
    bool load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft);

    string getName() const {return name;}
    int getTempMin() const {return tempMin;}
    int getTempMax() const {return tempMax;}
    int getMoistureLevel() const {return moistureLevel;}
    int getAltitudeMin() const {return altitudeMin;}
    int getAltitudeMax() const {return altitudeMax;}
    const Tileset *getTileset() const {return &tileset;}
    int getEmanationCount() const {return emanations.size();}
    Emanations getEmanations() const {return emanations;}
    EmanationType *getEmanation(int i) const {return emanations[i];}
};

}} //end namespace

#endif
