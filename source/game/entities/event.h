// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_EVENT_H_
#define _GLEST_GAME_EVENT_H_

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <time.h>

#include "events.h"

#include "element_type.h"
#include "resource_type.h"
#include "game_constants.h"
#include "unit_type.h"
#include "item_type.h"
#include "logger.h"
#include "factory.h"
#include "type_factories.h"
#include "vec.h"
#include "prototypes_enums.h"
#include "simulation_enums.h"

using std::map;
using std::vector;

using Shared::Graphics::Texture2D;
using Shared::Math::Vec3f;
using Glest::Sim::World;
using Glest::Gui::Clicks;

namespace Glest { namespace Entities {
// =====================================================
//  class Event
// =====================================================
using namespace ProtoTypes;
class Event {
	friend class EntityFactory<Event>;
private:
public:
	int id;
	Faction *faction;
	EventType *type;
    EventTargetList targetList;

public:
	struct CreateParams {
		int ident;
		EventType *type;
		Faction *faction;
        EventTargetList targetList;

		CreateParams(int ident, EventType *type, Faction *faction, EventTargetList targetList)
			: ident(ident), type(type), faction(faction), targetList(targetList) { }
	};

	struct LoadParams {
		const XmlNode *node;
		Faction *faction;

		LoadParams(const XmlNode *node, Faction *faction)
			: node(node), faction(faction) {}
	};
public:
    Event(CreateParams params);
	Event(LoadParams params);
	virtual ~Event();

public:
    EventType *getType() {return type;}
	Faction *getFaction() {return faction;}
    int getTargetListCount() const {return targetList.size();}
    Unit *getTarget(int i) const {return targetList[i];}
    EventTargetList getTargetList() {return targetList;}

    void init(int ident, EventType* type, Faction *faction, EventTargetList targetList);
};

// =====================================================
//  class EventFactory
// =====================================================
class EventFactory : public EntityFactory<Event> {
	friend class Glest::Sim::World;
public:
	EventFactory() {}
	~EventFactory() {}
	Event* newEvent(int ident, EventType* type, Faction *faction, EventTargetList targetList);
	Event* getEvent(int id) { return EntityFactory<Event>::getInstance(id); }
};

}}// end namespace

#endif
