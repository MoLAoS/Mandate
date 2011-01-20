// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_RESOURCE_BAR_H_
#define _GLEST_GAME_RESOURCE_BAR_H_

#include <string>

#include "widgets.h"
#include "forward_decs.h"

using std::string;
using namespace Shared::Math;

namespace Glest { namespace Gui {

using namespace Widgets;
using namespace ProtoTypes;
using Entities::Faction;
using Glest::ProtoTypes::ResourceType;

// =====================================================
// 	class ResourceBar
//
///	A bar that displays the resources
// =====================================================

class ResourceBar : public Widget, public MouseWidget, public ImageWidget, public TextWidget {
private:
	const Faction *m_faction;
	vector<const ResourceType*> m_resourceTypes;
	vector<string> m_headerStrings;
	bool m_draggingWidget;
	Vec2i m_moveOffset;

public:
	ResourceBar(const Faction *faction, std::set<const ResourceType*> &types);
	~ResourceBar();

	virtual void update();
	virtual void render();
	virtual string desc() { return string("[ResourceBar: ") + descPosDim() + "]"; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);
};

}}//end namespace

#endif