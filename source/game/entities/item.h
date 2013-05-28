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
class Item {
	friend class EntityFactory<Item>;
private:
    Statistics statistics;
	typedef vector<Modification*> Modifications;
public:
	int id;	/**< unique identifier  */
	Faction *faction;

    const Statistics *getStatistics() const {return &statistics;}
    CurrentStep currentSteps; /**< current timer step for resource creation */
    CurrentStep currentUnitSteps; /**< current timer step for unit creation */
    CurrentStep currentItemSteps; /**< current timer step for unit creation */
    CurrentStep currentProcessSteps; /**< current timer step for resource processes */

	const ItemType *type;			/**< the InitType of this item */

    Modifications modifications;

    void computeTotalUpgrade();
    //EffectsCreated effectsCreated; /**< All effects created by this item. */

public:
	struct CreateParams {
		int ident;
		const ItemType *type;
		Faction *faction;

		CreateParams(int ident, const ItemType *type, Faction *faction)
			: ident(ident), type(type), faction(faction) { }
	};

	struct LoadParams {
		const XmlNode *node;
		Faction *faction;

		LoadParams(const XmlNode *node, Faction *faction)
			: node(node), faction(faction) {}
	};
public:
    Item(CreateParams params);
	Item(LoadParams params);
	virtual ~Item();

public:
    const ItemType *getType() const {return type;}
	Faction *getFaction() const {return faction;}
    string getShortDesc() const;
    string getLongDesc();

    void init(int ident, const ItemType* type, Faction *faction);
};

// =====================================================
//  class ItemFactory
// =====================================================
typedef vector<Item> ItemStore;

class ItemFactory : public EntityFactory<Item> {
	friend class Glest::Sim::World;
private:
    //ItemStore items;
public:
	ItemFactory() {}
	~ItemFactory() {}
	Item* newItem(int ident, const ItemType* type, Faction *faction);
	Item* getItem(int id) { return EntityFactory<Item>::getInstance(id); }
};

}}// end namespace

#endif
