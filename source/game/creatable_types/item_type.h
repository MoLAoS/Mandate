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

#include "forward_decs.h"
#include "creatable_type.h"
#include "vec.h"
#include "prototypes_enums.h"
#include "simulation_enums.h"

namespace Glest { namespace ProtoTypes {

class ItemType : public CreatableType {
private:
    typedef vector <string> CraftTypes;
	string typeTag;
    int qualityTier;
    CraftTypes craftTypes;
public:
    int getCraftTypeCount() const {return craftTypes.size();}
    string getCraftType(int i) const {return craftTypes[i];}
    string getTypeTag() const {return typeTag;}
    int getQualityTier() const {return qualityTier;}
    static ProducibleClass typeClass() { return ProducibleClass::ITEM; }
	ProducibleClass getClass() const override { return typeClass(); }
	void preLoad(const string &dir);
	bool load(const string &dir, const TechTree *techTree, const FactionType *factionType);
};

}}// end namespace

#endif
