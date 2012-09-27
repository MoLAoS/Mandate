// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
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
