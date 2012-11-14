// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
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
#include "modifications.h"
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

//typedef vector<Effect> EffectsCreated;

using namespace ProtoTypes;
class Item : public EnhancementType {
	friend class EntityFactory<Item>;

private:
	typedef vector<Modification*> Modifications;

public:
	int id;	/**< unique identifier  */
	Faction *faction;

    CurrentStep currentSteps; /**< current timer step for resource creation */
    CurrentStep currentUnitSteps; /**< current timer step for unit creation */
    CurrentStep currentItemSteps; /**< current timer step for unit creation */
    CurrentStep currentProcessSteps; /**< current timer step for resource processes */

	const ItemType *type;			/**< the UnitType of this item */

    Modifications modifications;

    //EffectsCreated effectsCreated; /**< All effects created by this item. */

    const ItemType *getType() const {return type;}
	Faction *getFaction() const {return faction;}
    string getShortDesc() const;
    string getLongDesc() const;

    void init(int ident, const ItemType* type, Faction *faction);
};

}}// end namespace

#endif
