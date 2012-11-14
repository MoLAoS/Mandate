// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_ITEMTYPE_H_
#define _GLEST_GAME_ITEMTYPE_H_

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <time.h>

#include "element_type.h"
#include "resource_type.h"
#include "game_constants.h"
#include "unit_type.h"
#include "abilities.h"
#include "vec.h"
#include "prototypes_enums.h"
#include "simulation_enums.h"

using std::map;
using std::vector;

using Shared::Graphics::Texture2D;
using Shared::Math::Vec3f;
using Glest::Sim::World;
using Glest::Gui::Clicks;

namespace Glest { namespace ProtoTypes {

class ItemType : public ProducibleType, public EnhancementType {

private:
	typedef vector<CreatedResource>     CreatedResources;
	typedef vector<Timer>               CreatedResourceTimers;

	typedef vector<CreatedUnit>         CreatedUnits;
	typedef vector<Timer>               CreatedUnitTimers;

	typedef vector<CreatedItem>         CreatedItems;
	typedef vector<Timer>               CreatedItemTimers;

	typedef vector<Process>             Processes;
	typedef vector<Timer>               ProcessTimers;

	typedef vector<DamageType>          Resistances;

	typedef vector<UnitsOwned> OwnedUnits;

    string name;
    const FactionType *m_factionType;

	const ArmourType *armourType;

public:
	string typeTag;
    string getTypeTag() const {return typeTag;}

	int qualityTier;
    int getQualityTier() const {return qualityTier;}

    Resistances resistances;

    OwnedUnits ownedUnits;

	const ArmourType *getArmourType() const	{return armourType;}

	CreatedResources createdResources;
	mutable CreatedResourceTimers createdResourceTimers;

	CreatedUnits createdUnits;
	mutable CreatedUnitTimers createdUnitTimers;

	CreatedItems createdItems;
	mutable CreatedItemTimers createdItemTimers;

	Processes processes;
	ProcessTimers processTimers;

    static ProducibleClass typeClass() { return ProducibleClass::ITEM; }

	void preLoad(const string &dir);
	bool load(const string &dir, const TechTree *techTree, const FactionType *factionType);
	const FactionType* getFactionType() const { return m_factionType; }

	ProducibleClass getClass() const override { return typeClass(); }
    string getUnitName() const {return name;}

    // resources created
	int getCreatedResourceCount() const					{return createdResources.size();}
	ResourceAmount getCreatedResource(int i, const Faction *f) const;
	int getCreate(const ResourceType *rt, const Faction *f) const;

	// resources created timers
	int getCreatedResourceTimerCount()                 {return createdResourceTimers.size();}
	Timer getCreatedResourceTimer(int i, const Faction *f) const;
	int getCreateTimer(const ResourceType *rt, const Faction *f) const;

    // processes
	int getProcessCount() const					{return processes.size();}
	Process getProcess(int i, const Faction *f) const;
	int getProcessing(const ResourceType *rt, const Faction *f) const;

	// process timers
	int getProcessTimerCount()                 {return processTimers.size();}
	Timer getProcessTimer(int i, const Faction *f) const;
	int getProcessingTimer(const ResourceType *rt, const Faction *f) const;

    // units created
	int getCreatedUnitCount() const					{return createdUnits.size();}
	CreatedUnit getCreatedUnit(int i, const Faction *f) const;
	int getCreateUnit(const UnitType *ut, const Faction *f) const;

	// units created timers
	int getUnitTimerCount()                 {return createdUnitTimers.size();}
	Timer getCreatedUnitTimer(int i, const Faction *f) const;
	int getCreateUnitTimer(const UnitType *ut, const Faction *f) const;

    // items created
	int getCreatedItemCount() const					{return createdItems.size();}
	CreatedItem getCreatedItem(int i, const Faction *f) const;
	int getCreateItem(const ItemType *it, const Faction *f) const;

	// items created timers
	int getItemTimerCount()                 {return createdItemTimers.size();}
	Timer getCreatedItemTimer(int i, const Faction *f) const;
	int getCreateItemTimer(const ItemType *it, const Faction *f) const;

};

}}// end namespace

#endif
