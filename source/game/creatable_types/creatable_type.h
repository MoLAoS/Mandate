// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_CREATABLETYPE_H_
#define _GLEST_GAME_CREATABLETYPE_H_

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <time.h>

#include "forward_decs.h"
#include "element_type.h"
#include "actions.h"
#include "modifications.h"
#include "producibles.h"
#include "vec.h"

using std::map;
using std::vector;

using Shared::Graphics::Texture2D;
using Shared::Math::Vec3f;
using Glest::Sim::World;
using Glest::Gui::Clicks;

namespace Glest { namespace ProtoTypes {
// =====================================================
// 	class CreatableType
// =====================================================
class CreatableType : public ProducibleType {
private:
    Statistics statistics;
    typedef vector<string> ModifyNames;
    typedef vector<Modification> Modifications;
    typedef vector<EmanationType*> Emanations;
	typedef vector<UnitsOwned> OwnedUnits;
    ResourceProductionSystem resourceGeneration;
    ItemProductionSystem itemGeneration;
    UnitProductionSystem unitGeneration;
    ProcessProductionSystem processingSystem;
    Actions actions;
    Emanations emanations;
	OwnedUnits ownedUnits;
    Modifications modifications;
    ModifyNames modifyNames;
public:
	EffectTypes effectTypes;
private:
    const FactionType *m_factionType;
    int size;
    int height;
public:
    const Statistics *getStatistics() const {return &statistics;}
    const ResourceProductionSystem *getResourceProductionSystem() const {return &resourceGeneration;}
    const ItemProductionSystem *getItemProductionSystem() const {return &itemGeneration;}
    const UnitProductionSystem *getUnitProductionSystem() const {return &unitGeneration;}
    const ProcessProductionSystem *getProcessProductionSystem() const {return &processingSystem;}
    const Actions *getActions() const {return &actions;}
    Actions *getsActions() {return &actions;}
	const Emanations &getEmanations() const {return emanations;}
	OwnedUnits getOwnedUnits() const {return ownedUnits;}
	Modifications getModifications() const {return modifications;}
    const FactionType *getFactionType() const {return m_factionType;}
	int getSize() const {return size;}
	int getHeight() const {return height;}
	bool load(const XmlNode *creatableTypeNode, const string &dir, const TechTree *techTree, const FactionType *factionType, bool isItem);
};

}}// end namespace

#endif
