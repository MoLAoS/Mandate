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
    string getName() {return name;}
    int getValue() {return value;}
    void setValue(int i) {value = i;}
    void init(int val, string nam);
};

typedef vector<CraftStat> CraftStats;

class CraftRes {
private:
    const ResourceType *resType;
    int value;
public:
    int getValue() {return value;}
    const ResourceType *getType() {return resType;}
    bool load(const XmlNode *node);
};

class CraftItem {
private:
    const ItemType *itemType;
    int value;
public:
    int getValue() {return value;}
    const ItemType *getType() {return itemType;}
    bool load(const XmlNode *node);
};

/*class Quality {
private:

public:

};*/

}}//end namespace

#endif
