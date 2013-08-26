// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "event.h"

#include <cassert>

#include "unit.h"
#include "world.h"
#include "upgrade.h"
#include "map.h"
#include "command.h"
#include "object.h"
#include "config.h"
#include "skill_type.h"
#include "core_data.h"
#include "renderer.h"
#include "script_manager.h"
#include "cartographer.h"
#include "game.h"
#include "earthquake_type.h"
#include "sound_renderer.h"
#include "sim_interface.h"
#include "user_interface.h"
#include "route_planner.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Glest::Net;

namespace Glest { namespace Entities {
using namespace ProtoTypes;
// =====================================================
// 	class Event
// =====================================================
Event::Event(CreateParams params)
		: id(params.ident)
		, type(params.type)
		, faction(params.faction) {
}

Event::~Event() {
	if (!g_program.isTerminating() && World::isConstructed()) {
	}
}

void Event::init(int ident, EventType *newType, Faction *f) {
    type = newType;
    faction = f;
    id = ident;
}

// =====================================================
//  class EventFactory
// =====================================================
Event* EventFactory::newEvent(int ident, EventType* type, Faction *faction) {
	Event::CreateParams params(ident, type, faction);
	Event *event = new Event(params);
	return event;
}

}}//end namespace
