// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "map_command.h"
#include "unit_type.h"
#include "util.h"
#include "sound_renderer.h"
#include "renderer.h"
#include "tileset.h"
#include "tech_tree.h"
#include "world.h"
#include "program.h"
#include "sim_interface.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Gui_Mandate {

// =====================================================
// 	class FactionBuild
// =====================================================

void MapBuild::init(MapObjectType* mot, Clicks cl) {
    mapObjectType = mot;
    clicks = cl;
    stringstream ss;
    m_tipKey = "Build " + mot->getName();
    m_tipHeaderKey = mot->getName();
    name = mot->getName();
}

void MapBuild::build(const Tileset *tileset, const Vec2i &pos) const {
    Map *map = g_world.getMap();
    Tile *tile = map->getTileFromCellPos(pos);
    Cell *cell = map->getCell(pos);
    Vec3f vert(pos.x, 0.0f + cell->getHeight(), pos.y);
    if (!tile->getObject()) {
        MapObject *builtObject = NULL;
        builtObject = g_world.newMapObject(mapObjectType, Vec2i(pos.x/2, pos.y/2), vert);
        tile->setObject(builtObject);
        for (int i = 0; i < g_world.getTechTree()->getResourceTypeCount(); ++i) {
            const ResourceType *rt = g_world.getTechTree()->getResourceType(i);
            if (rt->getClass() == ResourceClass::TILESET && rt->getTilesetObject() == mapObjectType->getClass()) {
                builtObject->setResource(rt, pos * cellScale);
            }
        }
    } else {
        g_console.addStdMessage("ObjectNoPlace");
    }
}

}}//end namespace
