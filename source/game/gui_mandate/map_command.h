// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_MAPCOMMAND_H_
#define _GLEST_GAME_MAPCOMMAND_H_

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <time.h>

#include "element_type.h"
#include "object_type.h"
#include "game_constants.h"
#include "vec.h"
#include "prototypes_enums.h"
#include "simulation_enums.h"

using std::map;
using std::vector;

using Shared::Graphics::Texture2D;
using Shared::Math::Vec3f;
using namespace Glest::ProtoTypes;
using Glest::Sim::World;
using Glest::Gui::Clicks;
using Glest::Sim::Tileset;

namespace Glest { namespace Gui_Mandate {

// =====================================================
// 	MapBuild
//
///	MapBuild for the faction displays
// =====================================================

class MapBuild {
private:
    MapObjectType *mapObjectType;
    string name;
    Vec2i pos;

protected:
    Clicks     clicks;
	string     m_tipKey;
	string     m_tipHeaderKey;
	string     emptyString;

public:
    const MapObjectType *getMapObjectType() {return mapObjectType;}
    virtual Clicks getClicks() const {return clicks;}
    Vec2i setPos(Vec2i newPos) {pos = newPos;}
    const string& getTipKey() const	{return m_tipKey;}
    const string getTipKey(const string &name) const {return emptyString;}
    const string getSubHeaderKey() const {return m_tipHeaderKey;}
    void init(MapObjectType* mot, Clicks cl);
    void build(const Tileset *tileset, const Vec2i &pos) const;
};

}}//end namespace

#endif
