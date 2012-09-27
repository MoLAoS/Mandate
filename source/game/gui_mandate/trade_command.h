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

#ifndef _GLEST_GAME_TRADECOMMAND_H_
#define _GLEST_GAME_TRADECOMMAND_H_

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <time.h>

#include "element_type.h"
#include "resource_type.h"
#include "game_constants.h"
#include "vec.h"
#include "prototypes_enums.h"
#include "simulation_enums.h"

using std::map;
using std::vector;

using Shared::Graphics::Texture2D;
using Shared::Math::Vec3f;
using namespace Glest::ProtoTypes;
using Glest::Sim::World;
using Glest::Gui::Clicks;

namespace Glest { namespace Gui_Mandate {

// =====================================================
// 	TradeDescriptor
// =====================================================

class TradeDescriptor {
public:
	virtual void setHeader(const string &header) = 0;
	virtual void setTipText(const string &mainText) = 0;
	virtual void addElement(const string &msg) = 0;
	virtual void addItem(const DisplayableType *dt, const string &msg) = 0;
	virtual void addReq(bool ok, const DisplayableType *dt, const string &msg) = 0;
};

// =====================================================
// 	TradeCommands
//
///	Trade commands for the faction displays
// =====================================================

class TradeCommand {
private:
    int tradeValue;
    const ResourceType *resourceType;
    string name;

protected:
    Clicks     clicks;
	string     m_tipKey;
	string     m_tipHeaderKey;
	string     emptyString;

public:
    const ResourceType *getResourceType() const {return resourceType;}
    int getTradeValue() const {return tradeValue;}
    virtual Clicks getClicks() const {return clicks;}
    void describe(const Faction *faction, TradeDescriptor *callback, const ResourceType *rt) const;
	virtual void subDesc(const Faction *faction, TradeDescriptor *callback, const ResourceType *rt) const;
    const string& getTipKey() const	{return m_tipKey;}
    const string getTipKey(const string &name) const {return emptyString;}
    const string getSubHeaderKey() const {return m_tipHeaderKey;}
    void init(int i, const ResourceType* rt, Clicks cl);
};

}}//end namespace

#endif
