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

#ifndef _GLEST_GAME_ITEM_H_
#define _GLEST_GAME_ITEM_H_

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <time.h>

#include "element_type.h"
#include "resource_type.h"
#include "game_constants.h"
#include "unit_type.h"
#include "item_type.h"
#include "logger.h"
#include "factory.h"
#include "type_factories.h"
#include "unit.h"
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
using namespace ProtoTypes;
class Item : public EnhancementType {
	friend class EntityFactory<Item>;

private:
	int id;	/**< unique identifier  */
	Faction *faction;

public:
    CurrentStep currentSteps; /**< current timer step for resource creation */
    CurrentStep currentUnitSteps; /**< current timer step for unit creation */
    CurrentStep currentProcessSteps; /**< current timer step for resource processes */

	ItemType *type;			/**< the UnitType of this item */

    Effects effectsCreated; /**< All effects created by this item. */

	Faction *getFaction() const {return faction;}
    string getShortDesc() const;
    string getLongDesc() const;
};

}}// end namespace

#endif
