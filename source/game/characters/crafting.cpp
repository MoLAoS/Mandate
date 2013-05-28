// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "crafting.h"
#include "world.h"

namespace Glest { namespace ProtoTypes {
// ===============================
// 	class Quality
// ===============================
void CraftStat::init(int val, string nam) {
    value = val;
    name = nam;
}

/*void CraftRes::save(XmlNode *node) const {

}

void CraftItem::save(XmlNode *node) const {

}*/

bool CraftRes::load(const XmlNode *node) {
    bool loadOk = true;
    const XmlNode *resNode = node->getChild("resource");
    value = resNode->getAttribute("value")->getIntValue();
    string resName = resNode->getAttribute("type")->getRestrictedValue();
    resType = g_world.getTechTree()->getResourceType(resName);
    return loadOk;
}

bool CraftItem::load(const XmlNode *node) {
    bool loadOk = true;
    const XmlNode *itemNode = node->getChild("item");
    value = itemNode->getAttribute("value")->getIntValue();
    string itemName = itemNode->getAttribute("type")->getRestrictedValue();
	const FactionType *ft = 0;
	for (int i = 0; i < g_world.getTechTree()->getFactionTypeCount(); ++i) {
        ft = g_world.getTechTree()->getFactionType(i);
        for (int j = 0; j < ft->getItemTypeCount(); ++j) {
            if (itemName == ft->getItemType(i)->getName()) {
                break;
            }
        }
	}
    if (ft == 0) {
        loadOk = false;
    }
    itemType = ft->getItemType(itemName);
    return loadOk;
}

}}//end namespace
