// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_PRODUCIBLES_H_
#define _GLEST_GAME_PRODUCIBLES_H_

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <time.h>

#include "forward_decs.h"
#include "xml_parser.h"

using std::map;
using std::vector;
using namespace Shared::Xml;
using namespace Glest::Entities;

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class ResourceProductionSystem
// =====================================================
class ResourceProductionSystem {
private:
	typedef vector<ResourceAmount>  StoredResources;
	typedef vector<ResourceAmount>  StarterResources;
	typedef vector<CreatedResource> CreatedResources;
	typedef vector<Timer>           CreatedResourceTimers;
	StoredResources storedResources;
    StarterResources starterResources;
	CreatedResources createdResources;
	mutable CreatedResourceTimers createdResourceTimers;
public:
	StoredResources getStoredResources() const {return storedResources;};
    StarterResources getStarterResources() const {return starterResources;};
	CreatedResources getCreatedResources() const {return createdResources;};
	// resources stored
	int getStoredResourceCount() const					{return storedResources.size();}
	ResourceAmount getStoredResource(int i, const Faction *f) const;
	int getStore(const ResourceType *rt, const Faction *f) const;
    // resources created
	int getCreatedResourceCount() const					{return createdResources.size();}
	ResourceAmount getCreatedResource(int i, const Faction *f) const;
	int getCreate(const ResourceType *rt, const Faction *f) const;
	// resources created timers
	int getCreatedResourceTimerCount()                 {return createdResourceTimers.size();}
	Timer getCreatedResourceTimer(int i, const Faction *f) const;
	// load resource system
	bool load(const XmlNode *resourceProductionNode, const string &dir, const TechTree *techTree, const FactionType *factionType);
};
// =====================================================
// 	class ItemProductionSystem
// =====================================================
class ItemProductionSystem {
private:
	typedef vector<CreatedItem> CreatedItems;
	typedef vector<Timer>       CreatedItemTimers;
	CreatedItems createdItems;
	mutable CreatedItemTimers createdItemTimers;
public:
    // items created
    CreatedItems getCreatedItems() const {return createdItems;}
	int getCreatedItemCount() const					{return createdItems.size();}
	CreatedItem getCreatedItem(int i, const Faction *f) const;
	int getCreateItem(const ItemType *it, const Faction *f) const;
	// items created timers
	int getItemTimerCount()                 {return createdItemTimers.size();}
	Timer getCreatedItemTimer(int i, const Faction *f) const;
	// load resource system
	bool load(const XmlNode *itemProductionNode, const string &dir, const TechTree *techTree, const FactionType *factionType);
};
// =====================================================
// 	class ProcessProductionSystem
// =====================================================
class ProcessProductionSystem {
private:
	typedef vector<Process> Processes;
	typedef vector<Timer>   ProcessTimers;
	Processes processes;
	ProcessTimers processTimers;
public:
    // processes
	int getProcessCount() const					{return processes.size();}
	Processes getProcesses() const {return processes;}
	Process getProcess(int i, const Faction *f) const;
	int getProcessing(const ResourceType *rt, const Faction *f) const;
	// process timers
	int getProcessTimerCount()                 {return processTimers.size();}
	Timer getProcessTimer(int i, const Faction *f) const;
	// load resource system
	bool load(const XmlNode *processProductionNode, const string &dir, const TechTree *techTree, const FactionType *factionType);
};
// =====================================================
// 	class UnitProductionSystem
// =====================================================
class UnitProductionSystem {
private:
	typedef vector<CreatedUnit> CreatedUnits;
	typedef vector<Timer>       CreatedUnitTimers;
	CreatedUnits createdUnits;
	mutable CreatedUnitTimers createdUnitTimers;
public:
    // units created
    CreatedUnits getCreatedUnits() const {return createdUnits;}
	int getCreatedUnitCount() const					{return createdUnits.size();}
	CreatedUnit getCreatedUnit(int i, const Faction *f) const;
	int getCreateUnit(const UnitType *ut, const Faction *f) const;
	// units created timers
	int getUnitTimerCount()                 {return createdUnitTimers.size();}
	Timer getCreatedUnitTimer(int i, const Faction *f) const;
	// load resource system
	bool load(const XmlNode *unitProductionNode, const string &dir, const TechTree *techTree, const FactionType *factionType);
};

}}// end namespace

#endif
