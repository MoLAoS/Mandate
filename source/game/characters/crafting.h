// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_CRAFTING_H_
#define _GLEST_GAME_CRAFTING_H_

#include "resource_type.h"
#include "unit_stats_base.h"

namespace Glest{ namespace ProtoTypes {
// ===============================
// 	class Quality
// ===============================
class CraftStat {
private:
    string name;
    int value;
public:
    string getName() const {return name;}
    int getValue() const {return value;}
    void setValue(int i) {value = i;}
    void init(int val, string nam);
};

class CraftRes {
private:
    const ResourceType *resType;
    int value;
public:
    int getValue() const {return value;}
    const ResourceType *getType() const {return resType;}
    bool load(const XmlNode *node);
    void init(int newValue, const ResourceType *newResType);
};

class CraftItem {
private:
    const ItemType *itemType;
    int value;
public:
    int getValue() const {return value;}
    const ItemType *getType() {return itemType;}
    bool load(const XmlNode *node);
};

typedef vector<CraftStat> CraftStats;
typedef vector<CraftItem> CraftItems;
typedef vector<CraftRes> CraftResources;

}}//end namespace

#endif
