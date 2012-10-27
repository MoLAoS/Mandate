// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "trade_command.h"
#include "resource_type.h"
#include "util.h"
#include "sound_renderer.h"
#include "renderer.h"
#include "tech_tree.h"
#include "world.h"
#include "program.h"
#include "sim_interface.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Gui_Mandate {

// =====================================================
// 	class TradeCommands
// =====================================================

void TradeCommand::init(int i, const ResourceType* rt, Clicks cl) {
    tradeValue = i;
    resourceType = rt;
    clicks = cl;
    stringstream ss;
    ss << i;
    string newss = ss.str();
    m_tipKey = "Trade " + newss + " " + rt->getName();
    m_tipHeaderKey = rt->getName();
    name = rt->getName();
}

void TradeCommand::subDesc(const Faction *faction, TradeDescriptor *callback, const ResourceType *rt) const {
    Lang &lang = g_lang;
    callback->addElement("Trade: ");
    callback->addItem(rt, getResourceType()->getName());
}

void TradeCommand::describe(const Faction *faction, TradeDescriptor *callback, const ResourceType *rt) const {
	callback->setHeader("");
	callback->setTipText("");
	callback->addItem(rt, "");
	subDesc(faction, callback, rt);
}

}}//end namespace
